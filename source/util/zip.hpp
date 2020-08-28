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
#include <cinttypes>
#include <filesystem>
#include <functional>

#include <zip.h>

namespace own3d {
	namespace util {
		class zip {
			zip_t*                _archive;
			std::filesystem::path _file_path;
			std::filesystem::path _out_path;

			public:
			~zip();
			zip(std::filesystem::path path, std::filesystem::path output_path);

			uint64_t get_file_count();

			void extract_file(uint64_t idx, std::function<void(uint64_t, uint64_t)> callback);
		};
	} // namespace util
} // namespace own3d
