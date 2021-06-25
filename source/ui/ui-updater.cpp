// Copyright (c) 2020 Michael Fabian Dirks <info@xaymar.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "ui-updater.hpp"
#include "plugin.hpp"
#include "util/curl.hpp"
#include "version.hpp"

#include <QDesktopServices>
#include <QMainWindow>
#include <QUrl>
#include <nlohmann/json.hpp>
#include <sstream>

#include <obs-frontend-api.h>

static constexpr std::string_view UPDATE_URL = "https://own3d.pro/download-plugin";

own3d::ui::version_info::version_info(std::string_view version)
{
	// version can be:
	// A.B
	// A.BeF
	// A.B.C
	// A.B.CeF
	// A.B.C.D
	// A.B.C.DeF

	// Strip the commit information if it is appended.
	if (size_t dash_at = version.find_first_of('-'); dash_at != std::string_view::npos) {
		version = version.substr(0, dash_at);
	}

	size_t dot1 = version.find_first_of('.');
	size_t dot2 = version.find_first_of('.', dot1 + 1);
	size_t dot3 = version.find_first_of('.', dot2 + 1);
	char*  rem  = nullptr;

	if (dot1 == std::string_view::npos) {
		throw std::invalid_argument("Malformed version.");
	}

	major = strtoul(version.substr(0, dot1).data(), &rem, 10);
	if (dot2 == std::string_view::npos) {
		minor = strtoul(version.substr(dot1 + 1).data(), &rem, 10);
	} else {
		minor = strtoul(version.substr(dot1 + 1, dot2).data(), &rem, 10);
		if (dot3 == std::string_view::npos) {
			patch = strtoul(version.substr(dot2 + 1).data(), &rem, 10);
		} else {
			patch = strtoul(version.substr(dot2 + 1, dot3).data(), &rem, 10);
			if (dot3 == std::string_view::npos) {
				tweak = strtoul(version.substr(dot3 + 1).data(), &rem, 10);
			} else {
				tweak = strtoul(version.substr(dot3 + 1, dot3).data(), &rem, 10);
			}
		}
	}

	// If there is remaining data, it's likely to be the 'eF' part.
	if (size_t len = (rem - version.data()); len < version.length()) {
		size_t remlen = version.length() - len;

		if (remlen < 2) {
			throw std::invalid_argument("Malformed version.");
		}

		type  = *rem;
		index = strtoul(rem + 1, nullptr, 10);
	}
}

own3d::ui::version_info::version_info() : version_info(OWN3D_VERSION_STRING) {}

bool own3d::ui::version_info::is_newer(version_info& other)
{
	// 1. Compare Major version:
	//     A. Ours is greater: Remote is older.
	//     B. Theirs is greater: Remote is newer.
	//     C. Continue the check.
	if (major < other.major)
		return false;
	if (major > other.major)
		return true;

	// 2. Compare Minor version:
	//     A. Ours is greater: Remote is older.
	//     B. Theirs is greater: Remote is newer.
	//     C. Continue the check.
	if (minor < other.minor)
		return false;
	if (minor > other.minor)
		return true;

	// 3. Compare Patch version:
	//     A. Ours is greater: Remote is older.
	//     B. Theirs is greater: Remote is newer.
	//     C. Continue the check.
	if (patch < other.patch)
		return false;
	if (patch > other.patch)
		return true;

	// 4. Compare Tweak version:
	//     A. Ours is greater: Remote is older.
	//     B. Theirs is greater: Remote is newer.
	//     C. Continue the check.
	if (tweak < other.tweak)
		return false;
	if (tweak > other.tweak)
		return true;

	// 5. Compare Type:
	//     A. Ours empty: Remote is older.
	//     B. Theirs empty: Remote is newer.
	//     C. Continue the check.
	// A automatically implies that ours is not empty for B. A&B combined imply that both are not empty for step 5.
	if (type == 0)
		return false;
	if (other.type == 0)
		return true;

	// 6. Compare Type value:
	//     A. Ours is greater: Remote is older.
	//     B. Theirs is greater: Remote is newer.
	//     C. Continue the check.
	if (type < other.type)
		return false;
	if (type > other.type)
		return true;

	// 7. Compare Index:
	//    A. Ours is greater or equal: Remote is older or identical.
	//    B. Remote is newer
	if (index < other.index)
		return false;

	return true;
}

own3d::ui::updater::updater(QWidget* parent) : QDialog(parent), Ui::Updater(), _lock(), _is_checking(false), _task()
{
	// Set up UI elements.
	setupUi(this);

	// Don't delete this object when closed.
	setAttribute(Qt::WA_DeleteOnClose, false);

	// Connect internal logic.
	connect(this, &own3d::ui::updater::update_available, this, &own3d::ui::updater::on_update_available,
			Qt::QueuedConnection);
	connect(this, &own3d::ui::updater::check_completed, this, &own3d::ui::updater::on_check_complete);
	connect(cancel, &QPushButton::clicked, this, &own3d::ui::updater::on_update_decline);
	connect(ok, &QPushButton::clicked, this, &own3d::ui::updater::on_update_accept);
}

own3d::ui::updater::~updater()
{
	// Forcefully rejoin with the thread if any is active.
	if (_task.joinable())
		_task.join();
}

void own3d::ui::updater::check()
{
	std::unique_lock<std::mutex> lock(_lock);
	if (_is_checking) {
		return;
	}

	// Forcefully rejoin with the thread if any is active.
	if (_task.joinable())
		_task.join();

	// Spawn a new task to check for updates.
	_is_checking = true;
	_task        = std::thread{std::bind(&own3d::ui::updater::check_main, this)};
}

void own3d::ui::updater::check_main()
{
	try {
		own3d::util::curl curl;
		std::stringstream stream;
		version_info      current;

		// Request update information from the remote.
		curl.set_option(CURLOPT_HTTPGET, true);
		curl.set_option(CURLOPT_POST, false);
		curl.set_option(CURLOPT_URL, own3d::get_api_endpoint("obs/releases"));
		curl.set_option(CURLOPT_TIMEOUT, 10);
		curl.set_write_callback([this, &stream](void* buf, size_t n, size_t c) {
			stream.write(reinterpret_cast<char*>(buf), n * c);
			return n * c;
		});
		CURLcode res = curl.perform();

		// Parse returned information.
		if (res == CURLE_OK) {
			long response_code;
			curl.get_info(CURLINFO_RESPONSE_CODE, response_code);

			if (response_code != 200) {
				throw std::runtime_error(
					QString::fromUtf8("Server responded with code %1d.").arg(response_code).toStdString());
				return;
			}

			nlohmann::json data;
			try {
				data = nlohmann::json::parse(stream.str());
			} catch (std::exception const& ex) {
				throw ex;
			}

			if (!data.is_array()) {
				throw std::runtime_error("JSON response is malformed.");
			}

			// Iterate over all array entries.
			for (auto entry : data) {
				auto prerelease = entry.find("prerelease");
				if (prerelease == entry.end()) {
					throw std::runtime_error("JSON response is malformed.");
				}

				auto tag_name = entry.find("tag_name");
				if (tag_name == entry.end()) {
					throw std::runtime_error("JSON response is malformed.");
				}

				// Ignore Testing channel.
				if (prerelease->get<bool>() == true) {
					continue;
				}

				// Compare the update version.
				version_info update(tag_name->get<std::string>());
				if (update.is_newer(current)) {
					emit update_available(update);
				}
			}

		} else {
			std::string str = QString::fromUtf8("Failed to query server for updates: %1s")
								  .arg(QString::fromUtf8(curl_easy_strerror(res)))
								  .toStdString();
			throw std::runtime_error(str);
		}

		emit check_completed();
	} catch (std::exception const& ex) {
		emit error(QString::fromUtf8(ex.what()));
		emit check_completed();
	} catch (...) {
		emit error(QString::fromUtf8("An unknown error occurred."));
		emit check_completed();
	}
}

void own3d::ui::updater::on_update_available(own3d::ui::version_info info)
{
	QString currentversion =
		QString::fromUtf8("%1.%2.%3").arg(OWN3D_VERSION_MAJOR).arg(OWN3D_VERSION_MINOR).arg(OWN3D_VERSION_PATCH);
	currentVersion->setText(currentversion);
	QString latestversion = QString::fromUtf8("%1.%2.%3").arg(info.major).arg(info.minor).arg(info.patch);
	latestVersion->setText(latestversion);

	setModal(true);
	show();
}

void own3d::ui::updater::on_check_complete()
{
	std::unique_lock<std::mutex> lock(_lock);
	_is_checking = false;
}

void own3d::ui::updater::on_update_decline()
{
	// Close this window, and remove it from modality.
	close();
	setModal(false);
}

void own3d::ui::updater::on_update_accept()
{
	// Close this window, and remove it from modality.
	close();
	setModal(false);

	// Open the update URL.
	QDesktopServices::openUrl(QUrl(QString::fromUtf8(UPDATE_URL.data())));

	// Close OBS Studio automatically.
	reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window())->close();
}
