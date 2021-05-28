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
#include "plugin.hpp"

static constexpr std::string_view I18N_THEMEBROWSER_MENU = "Menu.ThemeBrowser";

own3d::ui::ui::~ui()
{
	obs_frontend_remove_event_callback(obs_event_handler, this);
}

own3d::ui::ui::ui() : _action(), _theme_browser(), _download(), _eventlist_dock(), _eventlist_dock_action()
{
	obs_frontend_add_event_callback(obs_event_handler, this);
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
	{ // OWN3D Menu
		_action =
			reinterpret_cast<QAction*>(obs_frontend_add_tools_menu_qaction(D_TRANSLATE(I18N_THEMEBROWSER_MENU.data())));
		connect(_action, &QAction::triggered, this, &own3d::ui::ui::own3d_action_triggered);
	}

	{ // Create Theme Browser.
		_theme_browser = new own3d::ui::browser();
		connect(_theme_browser, &own3d::ui::browser::selected, this, &own3d::ui::ui::own3d_theme_selected);
	}

	{ // Event List Dock
		_eventlist_dock        = QSharedPointer<own3d::ui::dock::eventlist>::create();
		_eventlist_dock_action = _eventlist_dock->add_obs_dock();
	}

	{ // Event List Dock
		_chat_dock        = QSharedPointer<own3d::ui::dock::chat>::create();
		_chat_dock_action = _chat_dock->add_obs_dock();
	}
}

void own3d::ui::ui::unload()
{
	{ // Chat Dock
		_chat_dock->deleteLater();
		_chat_dock        = nullptr;
		_chat_dock_action = nullptr;
	}

	{ // Event List Dock
		_eventlist_dock->deleteLater();
		_eventlist_dock        = nullptr;
		_eventlist_dock_action = nullptr;
	}

	{ // Theme Browser
		_theme_browser->deleteLater();
		disconnect(_theme_browser, &own3d::ui::browser::selected, this, &own3d::ui::ui::own3d_theme_selected);
		_theme_browser = nullptr;
	}

	{ // OWN3D Menu
		_action = nullptr;
	}
}

void own3d::ui::ui::own3d_action_triggered(bool)
{
	_theme_browser->show();
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
