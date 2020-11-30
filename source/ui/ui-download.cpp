// Integration of the OWN3D service into OBS Studio
// Copyright (C) 2020 own3d media GmbH <support@own3d.tv>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "ui-download.hpp"
#include <chrono>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <thread>
#include "json/json.hpp"
#include "plugin.hpp"

constexpr std::string_view I18N_TITLE          = "ThemeInstaller.Title";
constexpr std::string_view I18N_STATE_WAITING  = "ThemeInstaller.State.Waiting";
constexpr std::string_view I18N_STATE_DOWNLOAD = "ThemeInstaller.State.Download";
constexpr std::string_view I18N_STATE_EXTRACT  = "ThemeInstaller.State.Extract";
constexpr std::string_view I18N_STATE_INSTALL  = "ThemeInstaller.State.Install";

own3d::ui::installer_thread::~installer_thread() {}

own3d::ui::installer_thread::installer_thread(std::string url, std::string name, std::filesystem::path path,
											  std::filesystem::path out_path, QObject* parent)
	: QThread(parent), _url(url), _name(name), _path(path), _out_path(out_path)
{}

void own3d::ui::installer_thread::run_download()
{
	std::ofstream             stream;
	util::curl                curl;
	own3d::ui::download_state state;

	if (own3d::testing_enabled()) {
		state = download_state::DONE;
		emit download_status(uint64_t(0), uint64_t(0), static_cast<int64_t>(state));
		return;
	}

	{ // Emit signal.
		state = download_state::UNKNOWN;
		emit download_status(uint64_t(0), uint64_t(0), static_cast<int64_t>(state));
	}

	{ // Set up output file.
		stream = std::ofstream(_path, std::ios::binary | std::ios::trunc | std::ios::out);
		if (stream.bad()) {
			state = download_state::FAILED;
			emit download_status(uint64_t(0), uint64_t(0), static_cast<int64_t>(state));
			throw std::runtime_error("");
		}
	}

	{ // Begin curl work.
		curl.set_option(CURLOPT_HTTPGET, true);
		curl.set_option(CURLOPT_URL, _url);
		curl.set_write_callback([this, &stream](void* buf, size_t n, size_t c) {
			stream.write(reinterpret_cast<char*>(buf), n * c);
			return n * c;
		});
		curl.set_xferinfo_callback([this, &state](uint64_t total, uint64_t now, uint64_t, uint64_t) {
			if (state == download_state::UNKNOWN) {
				state = download_state::CONNECTING;
			} else if ((state == download_state::CONNECTING) && ((total != 0) || (now != 0))) {
				state = download_state::DOWNLOADING;
			}
			emit download_status(static_cast<uint64_t>(now), static_cast<uint64_t>(total), static_cast<int64_t>(state));
			return int32_t(0);
		});
		CURLcode res = curl.perform();
		if (res != CURLE_OK) {
			state = download_state::FAILED;
			emit download_status(uint64_t(0), uint64_t(0), static_cast<int64_t>(state));
			throw std::runtime_error("");
		}
	}

	// Manually close the stream.
	stream.close();

	{ // Emit that we are done.
		state = download_state::DONE;
		emit download_status(uint64_t(0), uint64_t(0), static_cast<int64_t>(state));
	}
}

void own3d::ui::installer_thread::run_extract()
{
	util::zip archive{_path, _out_path};
	for (uint64_t idx = 0, edx = archive.get_file_count(); idx < edx; idx++) {
#ifdef _DEBUG
		DLOG_DEBUG("Extracting file %llu of %llu from Theme '%s'...", (idx + 1), edx, _name.c_str());
#endif
		archive.extract_file(idx, [this, &idx, &edx](uint64_t t, uint64_t n) {
#ifdef _DEBUG
			DLOG_DEBUG("  %llu of %llu bytes extracted...", n, t);
#endif
			emit extract_status(idx, edx, n, t);
		});
	}
}

static void replace_tokens(obs_data_t* data, std::string base_directory_path)
{
	constexpr std::string_view TOKEN_PATH = "<REPLACE|ME>";
	constexpr std::string_view TOKEN_UUID = "<machine-token>";

	std::string_view machine_token = own3d::get_unique_identifier();
	for (obs_data_item_t* item = obs_data_first(data); item != nullptr; obs_data_item_next(&item)) {
		switch (obs_data_item_gettype(item)) {
		case obs_data_type::OBS_DATA_STRING: {
			std::string string;

			if (const char* cstr = obs_data_item_get_string(item); cstr == nullptr) {
				break;
			} else {
				string = cstr;
			}

			for (std::string::size_type pos = string.find(TOKEN_PATH); pos != std::string::npos;
				 pos                        = string.find(TOKEN_PATH)) {
				string = string.replace(pos, TOKEN_PATH.length(), base_directory_path);
			}
			for (std::string::size_type pos = string.find(TOKEN_UUID); pos != std::string::npos;
				 pos                        = string.find(TOKEN_UUID)) {
				string = string.replace(pos, TOKEN_UUID.length(), machine_token);
			}

			obs_data_item_set_string(&item, string.c_str());
			break;
		}
		case obs_data_type::OBS_DATA_OBJECT: {
			obs_data_t* chld = obs_data_item_get_obj(item);
			replace_tokens(chld, base_directory_path);
		}
		}
	}
}

static void adjust_collection_entry(std::shared_ptr<obs_data_t> data, std::string base_path)
{
	auto settings = std::shared_ptr<obs_data_t>(obs_data_get_obj(data.get(), "settings"), own3d::data_deleter);
	if (!settings)
		return;

	replace_tokens(settings.get(), base_path);
}

static void adjust_collection_entries(std::shared_ptr<obs_data_array_t> data, std::string base_path)
{
	for (size_t idx = 0, edx = obs_data_array_count(data.get()); idx < edx; idx++) {
		auto obj = std::shared_ptr<obs_data_t>(obs_data_array_item(data.get(), idx), own3d::data_deleter);
		adjust_collection_entry(obj, base_path);

		auto filters =
			std::shared_ptr<obs_data_array_t>(obs_data_get_array(obj.get(), "filters"), own3d::data_array_deleter);
		if (filters)
			adjust_collection_entries(filters, base_path);
	}
}

static void adjust_collection(std::shared_ptr<obs_data_t> data, std::string name, std::string base_path)
{
	// Need to adjust:
	// data.transitions
	// data.sources
	// data.sources[...].filters
	// data.groups[...].filters

	auto groups =
		std::shared_ptr<obs_data_array_t>(obs_data_get_array(data.get(), "groups"), own3d::data_array_deleter);
	auto sources =
		std::shared_ptr<obs_data_array_t>(obs_data_get_array(data.get(), "sources"), own3d::data_array_deleter);
	auto transitions =
		std::shared_ptr<obs_data_array_t>(obs_data_get_array(data.get(), "transitions"), own3d::data_array_deleter);

	if (groups)
		adjust_collection_entries(groups, base_path);
	if (sources)
		adjust_collection_entries(sources, base_path);
	if (transitions)
		adjust_collection_entries(transitions, base_path);

	// Update name.
	obs_data_set_string(data.get(), "name", name.c_str());
}

static std::string make_filename(std::string name)
{
	size_t       base_len = name.length();
	size_t       len      = os_utf8_to_wcs(name.data(), base_len, nullptr, 0);
	std::wstring wfile;

	if (!len)
		return name;

	wfile.resize(len);
	os_utf8_to_wcs(name.data(), base_len, &wfile[0], len + 1);

	for (size_t i = wfile.size(); i > 0; i--) {
		size_t im1 = i - 1;

		if (iswspace(wfile[im1])) {
			wfile[im1] = '_';
		} else if (wfile[im1] != '_' && !iswalnum(wfile[im1])) {
			wfile.erase(im1, 1);
		}
	}

	if (wfile.size() == 0)
		wfile = L"characters_only";

	len = os_wcs_to_utf8(wfile.c_str(), wfile.size(), nullptr, 0);
	if (!len)
		return name;

	name.resize(len);
	os_wcs_to_utf8(wfile.c_str(), wfile.size(), name.data(), len + 1);

	return name;
}

void own3d::ui::installer_thread::run_install()
{
	std::shared_ptr<obs_data_t> data;
	std::string                 name = _name;
	std::string                 file_name;
	std::filesystem::path       file_path;
	std::filesystem::path       collection_path = std::filesystem::absolute(
        std::filesystem::path(_out_path).append("..").append("..").append("..").append("..").append("basic").append(
            "scenes"));

	emit install_status(false);

	// While there is no direct way to trigger a refresh of the scene collections,
	// we can actually do this by switching scene collection. So to make things work:
	// 1. Get the name of the current scene collection.
	// 2. Create a new scene collection.
	// 3. Switch back to the previous collection.
	// 4. Swap out the JSON of the scene collection.
	// 5. Switch back to the new collection.
	// 6. ???
	// 7. Profit

	// Step 1: Generate a new unique collection name.
	{
		std::set<std::string> names;
		{
			char** names_raw = obs_frontend_get_scene_collections();
			for (char** ptr = names_raw; *ptr != nullptr; ptr++) {
				names.emplace(*ptr);
			}
			bfree(names_raw);
		}

		if (names.find(name) != names.end()) {
			std::stringstream sstr;

			// Name already exists, make it unique.
			for (size_t idx = 1; true; idx++) {
				sstr.str(std::string());
				sstr << name;
				sstr << " (" << idx << ")";
				std::string test = sstr.str();
				if (names.find(test) == names.end()) {
					// We found a new unique name, so use it.
					name = test;
					break;
				}
			}
		}
	}

	// Step 2: Generate a unique file name (unique collection name does not guarantee this).
	{
		file_name = make_filename(name);
		file_path = std::filesystem::path(collection_path).append(file_name).concat(".json");

		if (std::filesystem::exists(file_path)) {
			std::stringstream sstr;

			// Name already exists, make it unique.
			for (size_t idx = 1; true; idx++) {
				sstr.str(std::string());
				sstr << idx << ".json";
				file_path = std::filesystem::path(collection_path).append(file_name).concat(sstr.str());
				if (!std::filesystem::exists(file_path)) {
					break;
				}
			}
		}
	}

	// Step 3: Load the extracted JSON file and apply fixes.
	{
		auto data_path = _out_path;
		data_path      = data_path.append("data.json");

		if (!std::filesystem::exists(data_path)) {
			throw std::runtime_error("Unable to install, missing data.json file.");
		}

		data.reset(obs_data_create_from_json_file(data_path.u8string().c_str()), own3d::data_deleter);
		if (!data) {
			throw std::runtime_error("Failed to install theme, data.json may be corrupted.");
		}

		adjust_collection(data, name, std::filesystem::absolute(_out_path).u8string());
	}

	// Step 3: Store current scene collection, and create new temporary one.
	std::string cur = obs_frontend_get_current_scene_collection();
	obs_frontend_add_scene_collection(name.c_str());
	emit switch_collection(QString::fromStdString(cur));
	for (; strcmp(obs_frontend_get_current_scene_collection(), cur.c_str()) != 0;) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Step 4: Save JSON as new file.
	if (!obs_data_save_json_safe(data.get(), file_path.u8string().c_str(), ".tmp", ".bk")) {
		throw std::runtime_error("Failed to install theme, unable to write output file.");
	}

	// Step 5: Switch to the new scene collection.
	emit switch_collection(QString::fromStdString(name));
	for (; strcmp(obs_frontend_get_current_scene_collection(), name.c_str()) != 0;) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	emit install_status(true);
	obs_frontend_save();
}

void own3d::ui::installer_thread::run()
try {
	run_download();
	run_extract();
	run_install();
} catch (std::exception const& ex) {
	DLOG_ERROR("Installation of Theme '%s' failed due to error: %s", _name.c_str(), ex.what());
	emit error();
} catch (...) {
	emit error();
}

own3d::ui::installer::~installer() {}

own3d::ui::installer::installer(const QUrl& url, const QString& name, const QString& hash)
	: QDialog(reinterpret_cast<QWidget*>(obs_frontend_get_main_window())), Ui::ThemeDownload(), _download_url(url),
	  _theme_name(name), _download_hash(hash)
{
	// Check if we are in test mode or now.
	if (!own3d::testing_enabled()) {
		// Generate a download filename.
		_theme_archive_path = std::filesystem::temp_directory_path();
		_theme_archive_path.append("own3d");
		std::filesystem::create_directories(_theme_archive_path);
		_theme_archive_path.append(name.toStdString());
		if (hash.length() > 0)
			_theme_archive_path.concat(hash.toStdString().substr(0, 16));
		_theme_archive_path.concat(".pack");
	} else {
		_theme_archive_path = own3d::testing_archive_path();
		_theme_name         = QString::fromUtf8(own3d::testing_archive_name().data());
	}

	{ // Generate a theme extraction path
		{
			char* buf   = obs_module_config_path("themes");
			_theme_path = std::filesystem::u8path(buf);
			bfree(buf);
		}
		_theme_path.append(_theme_name.toStdString());
		std::filesystem::create_directories(_theme_path);
	}

	{ // Update UI elements.
		setupUi(this);
		setModal(true);
		buttonBox->setVisible(false);
		show();
		{
			std::vector<char> buffer(2048);
			snprintf(buffer.data(), buffer.size(), D_TRANSLATE(I18N_TITLE.data()), _theme_name.toStdString().c_str());
			setWindowTitle(QString::fromStdString(std::string{buffer.data()}));
		}
		themeName->setText(_theme_name);
	}

	// Spawn a worker thread and begin work.
	_worker = new installer_thread(url.toString().toStdString(), _theme_name.toStdString(), _theme_archive_path,
								   _theme_path, this);
	connect(_worker, &own3d::ui::installer_thread::download_status, this, &own3d::ui::installer::handle_download_status,
			Qt::QueuedConnection);
	connect(_worker, &own3d::ui::installer_thread::extract_status, this, &own3d::ui::installer::handle_extract_status,
			Qt::QueuedConnection);
	connect(_worker, &own3d::ui::installer_thread::install_status, this, &own3d::ui::installer::handle_install_status,
			Qt::QueuedConnection);
	connect(_worker, &own3d::ui::installer_thread::switch_collection, this,
			&own3d::ui::installer::handle_switch_collection, Qt::QueuedConnection);
	_worker->start();
}

void own3d::ui::installer::update_progress(double_t percent, bool is_download, bool is_extract, bool is_install)
{
	if (is_download) {
		status->setText(QString::fromStdString(std::string(D_TRANSLATE(I18N_STATE_DOWNLOAD.data()))));
		percent /= 3.;
	} else if (is_extract) {
		status->setText(QString::fromStdString(std::string(D_TRANSLATE(I18N_STATE_EXTRACT.data()))));
		percent /= 3.;
		percent += (1. / 3.);
	} else if (is_install) {
		status->setText(QString::fromStdString(std::string(D_TRANSLATE(I18N_STATE_INSTALL.data()))));
		percent /= 3.;
		percent += (2. / 3.);
	} else {
		status->setText(QString::fromStdString(std::string(D_TRANSLATE(I18N_STATE_WAITING.data()))));
		percent = NAN;
	}

	if (isnan(percent)) {
		progressBar->setRange(0, 0);
		progressBar->setValue(0);
		progressBar->setTextVisible(false);
	} else {
		uint32_t val = static_cast<uint32_t>(percent * static_cast<double_t>((std::numeric_limits<uint32_t>::max)()));
		progressBar->setRange((std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)());
		progressBar->setValue(static_cast<int32_t>(static_cast<int64_t>((std::numeric_limits<int32_t>::min)()) + val));
		progressBar->setTextVisible(true);
	}
}

void own3d::ui::installer::handle_download_status(uint64_t now_bytes, uint64_t total_bytes, int64_t state)
{
	switch (static_cast<own3d::ui::download_state>(state)) {
	case download_state::UNKNOWN:
	case download_state::CONNECTING:
		DLOG_DEBUG("Starting download of Theme '%s'...", _theme_name.toStdString().c_str());
		update_progress(0, false, false, false);
		break;
	case download_state::DOWNLOADING:
		DLOG_DEBUG("Download of Theme '%s' at %llu/%llu...", _theme_name.toStdString().c_str(), now_bytes, total_bytes);
		update_progress(static_cast<double_t>(now_bytes) / static_cast<double_t>(total_bytes), true, false, false);
		break;
	case download_state::DONE:
		DLOG_DEBUG("Download of Theme '%s' complete.", _theme_name.toStdString().c_str());
		update_progress(1.0, true, false, false);
		break;
	case download_state::FAILED:
		DLOG_DEBUG("Download of Theme '%s' failed.", _theme_name.toStdString().c_str());
		update_progress(NAN, true, false, false);
		emit error();
		break;
	}
}

void own3d::ui::installer::handle_extract_status(uint64_t now_files, uint64_t total_files, uint64_t now_bytes,
												 uint64_t total_bytes)
{
	double_t progress  = static_cast<double_t>(now_bytes) / static_cast<double_t>(total_bytes);
	double_t progress2 = (static_cast<double_t>(now_files) + progress) / static_cast<double_t>(total_files);
	update_progress(progress2, false, true, false);
}

void own3d::ui::installer::handle_install_status(bool complete)
{
	update_progress(complete ? 1.0 : 0.0, false, false, true);
}

void own3d::ui::installer::handle_switch_collection(QString new_collection)
{
	obs_frontend_set_current_scene_collection(new_collection.toUtf8().data());
}
