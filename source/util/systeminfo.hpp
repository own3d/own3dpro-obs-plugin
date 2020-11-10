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

#pragma once
#include <string>
#include <vector>

namespace own3d {
	namespace util {
		namespace systeminfo {
			const std::string           os_name();
			const std::vector<uint64_t> os_version();

			const std::string username();

			const std::string cpu_manufacturer();
			const std::string cpu_name();

			const uint64_t total_physical_memory();
			const uint64_t available_physical_memory();
			const uint64_t total_virtual_memory();
			const uint64_t available_virtual_memory();
		} // namespace systeminfo
	}     // namespace util
} // namespace own3d
