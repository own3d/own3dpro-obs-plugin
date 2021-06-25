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

#pragma once
#include <mutex>
#include <string_view>
#include <thread>

#include "ui_updater.h"

namespace own3d::ui {
	struct version_info {
		uint16_t major = 0;
		uint16_t minor = 0;
		uint16_t patch = 0;
		uint16_t tweak = 0;
		char     type  = 0;
		uint16_t index = 0;

		version_info(std::string_view version);
		version_info();

		/** Check if this version is newer than the other version.
		 * @param other The other version to check against.
		 * @return true if this is newer, otherwise false.
		*/
		bool is_newer(version_info& other);
	};

	class updater : public QDialog, protected Ui::Updater {
		Q_OBJECT

		std::mutex  _lock;
		bool        _is_checking;
		std::thread _task;

		public:
		updater(QWidget* parent);
		~updater();

		void check();

		private:
		void check_main();

		signals:
		; // Needed by some linters.

		void error(QString message);
		void update_available(own3d::ui::version_info info);
		void check_completed();

		private slots:
		void on_update_available(own3d::ui::version_info info);
		void on_check_complete();

		void on_update_decline();
		void on_update_accept();
	};
} // namespace own3d::ui

Q_DECLARE_METATYPE(own3d::ui::version_info);
