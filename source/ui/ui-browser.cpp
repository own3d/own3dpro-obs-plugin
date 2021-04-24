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

#include "ui-browser.hpp"
#include <obs-frontend-api.h>
#include <sstream>
#include <string>
#include "plugin.hpp"
#include "util/utility.hpp"

constexpr std::string_view own3d_url            = "https://obs.own3d.tv/";
constexpr std::string_view own3d_url_path_theme = "obs?machine_token=";

constexpr std::string_view I18N_THEME_BROWSER_TITLE = "ThemeBrowser.Title";

own3d::ui::browser::~browser() {}

own3d::ui::browser::browser() : QDialog(reinterpret_cast<QWidget*>(obs_frontend_get_main_window()))
{
	// Styling and layout.
	setMinimumSize(1300, 730);
	{
		_layout = new QVBoxLayout();
		_layout->setContentsMargins(0, 0, 0, 0);
		_layout->setSpacing(0);
		setLayout(_layout);
	}

	// Title
	setWindowTitle(QString::fromUtf8(D_TRANSLATE(I18N_THEME_BROWSER_TITLE.data())));
	setWindowFlag(Qt::WindowContextHelpButtonHint, false);

	// Create Browser Widget
	_cef = obs::browser::instance();
	if (!_cef) {
#ifdef WIN32
		throw std::runtime_error("Failed to load obs-browser.dll.");
#else
		throw std::runtime_error("Failed to load obs-browser.so.");
	}

	// Create Widget
	_cef_widget = _cef->create_widget(this, generate_url().toString().toStdString());
	_layout->addWidget(_cef_widget);

	// Connect signals.
	connect(_cef_widget, &QCefWidget::urlChanged, this, &own3d::ui::browser::url_changed);
	connect(_cef_widget, &QCefWidget::titleChanged, this, &own3d::ui::browser::title_changed);
}

void own3d::ui::browser::show()
{
	if (own3d::testing_enabled()) {
		emit selected(QUrl(QString::fromUtf8(own3d::testing_archive_path().data())),
					  QString::fromUtf8(own3d::testing_archive_name().data()), "");
		return;
	}

	_cef_widget->setURL(generate_url().toString().toStdString());
	static_cast<QWidget*>(this)->show();
}

void own3d::ui::browser::closeEvent(QCloseEvent*)
{
	emit cancelled();
}

QUrl own3d::ui::browser::generate_url()
{
	QUrl url;
	url.setUrl(QString::fromStdString(own3d::get_web_endpoint("obs")));
	QUrlQuery urlq;
	urlq.addQueryItem("machine_token", QString::fromUtf8(own3d::get_unique_identifier().data()));
	urlq.addQueryItem("version", OWN3DTV_VERSION_STRING);
	url.setQuery(urlq);
	return url;
}

void own3d::ui::browser::url_changed(const QString& p_url)
{
	// URL checks:
	// - Download: about:blank?theme-url=DATA&theme-hash=DATA&theme-name=DATA
	// - Cancel: about:blank?cancel=true

	// Check if the URL is actually valid.
	QUrl url{p_url};
	if (!url.isValid())
		return;

	QString     path  = url.path();
	std::string debug = path.toStdString();
	if (path.contains(QString{"/obs/token-invalid"}, Qt::CaseInsensitive)) {
		own3d::reset_unique_identifier();
		show();
	} else if (path.contains(QString{"/obs/download"}, Qt::CaseInsensitive)) {
		// Check if this is just canceling the download.
		QUrlQuery urlq{url.query()};
		if (urlq.hasQueryItem("cancel")) {
			emit cancelled();
			return;
		}

		if (urlq.hasQueryItem("theme-url")) {
			QString download = util::hex_to_string(urlq.queryItemValue("theme-url"));
			QString name     = urlq.queryItemValue("theme-name");
			QString hash     = "";

#ifdef _DEBUG
			DLOG_DEBUG("Starting downloading theme '%s' from '%s' with hash '%s'.", name.toStdString().c_str(),
					   download.toStdString().c_str(), hash.toStdString().c_str());
#endif

			emit selected(QUrl(download), name, hash);
		}
	}
}

void own3d::ui::browser::title_changed(const QString& title)
{
	std::stringstream sstr;
	sstr << std::string(D_TRANSLATE(I18N_THEME_BROWSER_TITLE.data()));
	if (title.length() > 0) {
		sstr << " - ";
		sstr << title.toStdString();
	}
	setWindowTitle(QString::fromStdString(sstr.str()));
}
