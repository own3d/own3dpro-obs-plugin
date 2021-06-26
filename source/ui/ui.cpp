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

#include "ui.hpp"
#include <QMainWindow>
#include <QMenuBar>
#include <QTranslator>
#include "plugin.hpp"

#include <obs-frontend-api.h>

static constexpr std::string_view I18N_MENU                 = "Menu";
static constexpr std::string_view I18N_THEMEBROWSER_MENU    = "Menu.ThemeBrowser";
static constexpr std::string_view I18N_MENU_CHECKFORUPDATES = "Menu.CheckForUpdates";

static constexpr std::string_view CFG_PRIVACYPOLICY = "privacypolicy";

inline void qt_init_resource()
{
	Q_INIT_RESOURCE(own3d);
}

inline void qt_cleanup_resource()
{
	Q_CLEANUP_RESOURCE(own3d);
}

class own3d_translator : public QTranslator {
	public:
	own3d_translator(QObject* parent = nullptr) {}
	~own3d_translator() {}

	virtual QString translate(const char* context, const char* sourceText, const char* disambiguation = nullptr,
							  int n = -1) const override
	{
		static constexpr std::string_view prefix = "OWN3D::";

		if (disambiguation) {
			std::string_view view{disambiguation};
			if (view.substr(0, prefix.length()) == prefix) {
				return QString::fromUtf8(D_TRANSLATE(view.substr(prefix.length()).data()));
			}
		} else if (sourceText) {
			std::string_view view{sourceText};
			if (view.substr(0, prefix.length()) == prefix) {
				return QString::fromUtf8(D_TRANSLATE(view.substr(prefix.length()).data()));
			}
		}
		return QString();
	}
};

own3d::ui::ui::~ui()
{
	obs_frontend_remove_event_callback(obs_event_handler, this);
	qt_cleanup_resource();
}

own3d::ui::ui::ui()
	: _translator(), _gdpr(), _privacypolicy(false), _menu(), _menu_action(), _theme_action(), _update_action(),
	  _theme_browser(), _download(), _eventlist_dock(), _eventlist_dock_action()
{
	qt_init_resource();
	obs_frontend_add_event_callback(obs_event_handler, this);

	{ // Load Configuration
		auto cfg = own3d::configuration::instance();

		// GDPR Flag
		obs_data_set_default_bool(cfg->get().get(), CFG_PRIVACYPOLICY.data(), false);
		_privacypolicy = obs_data_get_bool(cfg->get().get(), CFG_PRIVACYPOLICY.data());
	}
}

void own3d::ui::ui::obs_event_handler(obs_frontend_event event, void* private_data)
{
	own3d::ui::ui* ui = reinterpret_cast<own3d::ui::ui*>(private_data);
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		ui->load();
	} else if (event == OBS_FRONTEND_EVENT_EXIT) {
		ui->unload();
	}
}

void own3d::ui::ui::load()
{
	// Add translator.
	_translator = static_cast<QTranslator*>(new own3d_translator(this));
	QCoreApplication::installTranslator(_translator);
	{ // GDPR
		_gdpr = QSharedPointer<own3d::ui::gdpr>::create(reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window()));
		_gdpr->connect(_gdpr.get(), &own3d::ui::gdpr::accepted, this, &own3d::ui::ui::on_gdpr_accept);
		_gdpr->connect(_gdpr.get(), &own3d::ui::gdpr::declined, this, &own3d::ui::ui::on_gdpr_decline);
	}

	{ // Updater
		_updater =
			QSharedPointer<own3d::ui::updater>::create(reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window()));
	}

	{ // OWN3D Menu
		QMainWindow* main_widget = reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window());

		_menu = new QMenu(main_widget);

		// Add Theme/Design Browser/Store
		_theme_action = _menu->addAction(QString::fromUtf8(D_TRANSLATE(I18N_THEMEBROWSER_MENU.data())));
		connect(_theme_action, &QAction::triggered, this, &own3d::ui::ui::menu_theme_triggered);

		// Add Updater
		_update_action = _menu->addAction(D_TRANSLATE(I18N_MENU_CHECKFORUPDATES.data()));
		connect(_update_action, &QAction::triggered, this, &own3d::ui::ui::menu_update_triggered);

		{ // Add an actual Menu entry.
			_menu_action = new QAction(main_widget);
			_menu_action->setMenuRole(QAction::NoRole);
			_menu_action->setMenu(_menu);
			_menu_action->setText(QString::fromUtf8(D_TRANSLATE(I18N_MENU.data())));

			// Insert the new menu right before the Help menu.
			QList<QMenu*> obs_menus =
				main_widget->menuBar()->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly);
			if (QMenu* help_menu = obs_menus.at(1); help_menu) {
				main_widget->menuBar()->insertAction(help_menu->menuAction(), _menu_action);
			} else {
				main_widget->menuBar()->addAction(_menu_action);
			}
		}
	}

	{ // Create Theme Browser.
		_theme_browser = new own3d::ui::browser();
		connect(_theme_browser, &own3d::ui::browser::selected, this, &own3d::ui::ui::own3d_theme_selected);
	}

	{ // Event List Dock
		_eventlist_dock        = new own3d::ui::dock::eventlist();
		_eventlist_dock_action = _eventlist_dock->add_obs_dock();
	}

	// Verify that the user has accepted the privacy policy.
	if (!_privacypolicy) {
		_gdpr->show();
	} else {
		// If the user already agreed to the privacy policy, immediately check for updates.
		_updater->check();
	}
}

void own3d::ui::ui::unload()
{
	{ // Event List Dock
		_eventlist_dock->deleteLater();
		_eventlist_dock        = nullptr;
		_eventlist_dock_action = nullptr;
	}

	if (_theme_browser) { // Theme Browser
		_theme_browser->deleteLater();
		disconnect(_theme_browser, &own3d::ui::browser::selected, this, &own3d::ui::ui::own3d_theme_selected);
		_theme_browser = nullptr;
	}

	if (_menu) { // OWN3D Menu
		_update_action->deleteLater();
		_theme_action->deleteLater();
		_menu_action->deleteLater();
		_menu->deleteLater();
	}

	if (_updater) { // Updater
		_updater->deleteLater();
		_updater.reset();
	}

	if (_gdpr) { // GDPR
		_gdpr->deleteLater();
		_gdpr.reset();
	}

	// Remove translator.
	QCoreApplication::removeTranslator(_translator);
}

void own3d::ui::ui::on_gdpr_accept()
{
	_gdpr->close();
	_gdpr.reset();
	_privacypolicy = true;

	auto cfg = own3d::configuration::instance();
	obs_data_set_bool(cfg->get().get(), CFG_PRIVACYPOLICY.data(), _privacypolicy);
	cfg->save();

	// Check for new updates.
	_updater->check();
}

void own3d::ui::ui::on_gdpr_decline()
{
	_gdpr->close();
	_gdpr.reset();
	_privacypolicy = false;

	auto cfg = own3d::configuration::instance();
	obs_data_set_bool(cfg->get().get(), CFG_PRIVACYPOLICY.data(), _privacypolicy);
	cfg->save();

	static_cast<QMainWindow*>(obs_frontend_get_main_window())->close();
}

void own3d::ui::ui::menu_theme_triggered(bool)
{
	_theme_browser->show();
}

void own3d::ui::ui::menu_update_triggered(bool)
{
	if (_updater)
		_updater->check();
}

std::shared_ptr<own3d::ui::ui> own3d::ui::ui::_instance = nullptr;

void own3d::ui::ui::initialize()
{
	if (!own3d::ui::ui::_instance)
		own3d::ui::ui::_instance = std::make_shared<own3d::ui::ui>();
}

void own3d::ui::ui::finalize()
{
	own3d::ui::ui::_instance.reset();
}

std::shared_ptr<own3d::ui::ui> own3d::ui::ui::instance()
{
	return own3d::ui::ui::_instance;
}

void own3d::ui::ui::own3d_theme_selected(const QUrl& download_url, const QString& name, const QString& hash)
{ // We have a theme to download!
	// Hide the browser.
	_theme_browser->hide();

	// Recreate the download dialog.
	// FIXME! Don't recreate it, instead reset it.
	_download = new own3d::ui::installer(download_url, name, hash);
}
