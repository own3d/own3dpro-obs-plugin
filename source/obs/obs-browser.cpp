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

#include "obs-browser.hpp"
#include <util/platform.h>

static void* library = nullptr;

QCef* obs::browser::instance()
{
	static QCef* (*qcef_create)(void) = nullptr;
	static QCef* cef                  = nullptr;

	if (!library) {
#ifdef _WIN32
		library = os_dlopen("obs-browser");
#else
		library = os_dlopen("../obs-plugins/obs-browser");
#endif
		if (!library) {
			return nullptr;
		}
	}

	if (!cef) {
		if (!qcef_create) {
			qcef_create = reinterpret_cast<decltype(qcef_create)>(os_dlsym(library, "obs_browser_create_qcef"));
			if (!qcef_create) {
				return nullptr;
			}
		}

		cef = qcef_create();
		if (!cef) {
			return nullptr;
		}
	}

	return cef;
}

int obs::browser::version()
{
	int (*qcef_version)(void) = nullptr;

	if (!library) {
#ifdef _WIN32
		library = os_dlopen("obs-browser");
#else
		library = os_dlopen("../obs-plugins/obs-browser");
#endif
		if (!library) {
			return 0;
		}
	}

	if (!qcef_version) {
		qcef_version = reinterpret_cast<decltype(qcef_version)>(os_dlsym(library, "obs_browser_qcef_version_export"));
		if (!qcef_version) {
			return 0;
		}
	}

	return qcef_version();
}
