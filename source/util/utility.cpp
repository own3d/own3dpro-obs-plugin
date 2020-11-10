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

#include "utility.hpp"

QString own3d::util::hex_to_string(QString text)
{
	/* static thread_local */ std::vector<char> buffer;
	buffer.resize(text.length() / 2);

	for (size_t n = 0; n < buffer.size(); n++) {
		QStringRef part{&text, static_cast<int>(n * 2), 2};
		bool       status = false;
		uint       dec    = part.toUInt(&status, 16);
		if (status) {
			buffer[n] = static_cast<char>(dec);
		}
	}

	return QString::fromUtf8(buffer.data(), static_cast<int>(buffer.size()));
}
