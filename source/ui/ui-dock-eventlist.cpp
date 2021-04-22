// Integration of the OWN3D service into OBS Studio
// Copyright (C) 2021 own3d media GmbH <support@own3d.tv>
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

#include "ui-dock-eventlist.hpp"
#include <QMainWindow>
#include <obs-frontend-api.h>
#include <string_view>
#include "plugin.hpp"

static constexpr std::string_view I18N_EVENTLIST = "Dock.EventList";

static constexpr std::string_view CFG_EVENTLIST          = "dock.eventlist";
static constexpr std::string_view CFG_EVENTLIST_FIRSTRUN = "dock.eventlist.firstrun";

own3d::ui::dock::eventlist::eventlist() : QDockWidget(reinterpret_cast<QWidget*>(obs_frontend_get_main_window()))
{
	std::string token = std::string(own3d::get_unique_identifier());
	std::string url   = own3d::get_web_endpoint("popout/0/event-list?machine-token=" + token);

	_browser = obs::browser::instance()->create_widget(this, url);

	_layout = new QHBoxLayout(this);
	_layout->setContentsMargins(0, 0, 0, 0);
	_layout->setSpacing(0);
	_layout->addWidget(_browser);

	this->setWidget(_browser);
	this->setLayout(_layout);
	this->setMinimumSize(400, 500);
	this->setMaximumSize(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
	this->setWindowTitle(QString::fromUtf8(D_TRANSLATE(I18N_EVENTLIST.data())));
	this->setObjectName("own3d::eventlist");

	// Dock functionality
	this->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable
					  | QDockWidget::DockWidgetFloatable);
	this->setAllowedAreas(Qt::AllDockWidgetAreas);

	{ // Check if this is the first time the user runs this plug-in.
		auto cfg  = own3d::configuration::instance();
		auto data = cfg->get();

		obs_data_set_default_bool(data.get(), CFG_EVENTLIST_FIRSTRUN.data(), true);
	}
}

own3d::ui::dock::eventlist::~eventlist() {}

QAction* own3d::ui::dock::eventlist::add_obs_dock()
{
	QAction* action = reinterpret_cast<QAction*>(obs_frontend_add_dock(this));
	action->setObjectName("own3d::eventlist::action");
	action->setText(this->windowTitle());

	{ // Restore Dock Status
		auto mw = reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window());
		mw->restoreDockWidget(this);
	}

	{ // Is this the first time the user runs the plugin?
		auto cfg  = own3d::configuration::instance();
		auto data = cfg->get();

		if (obs_data_get_bool(data.get(), CFG_EVENTLIST_FIRSTRUN.data())) {
			this->hide();
			obs_data_set_bool(data.get(), CFG_EVENTLIST_FIRSTRUN.data(), false);
			cfg->save();
		}
	}

	return action;
}

void own3d::ui::dock::eventlist::closeEvent(QCloseEvent* event)
{
	this->hide();
}
