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

#pragma once
#include <QDialog>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251 4365 4371 4619 4946)
#endif
#include "ui_gdpr.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace own3d::ui {
	class gdpr : public QDialog, protected Ui::GDPR {
		Q_OBJECT;

		public:
		gdpr();
		virtual ~gdpr();

		void closeEvent(QCloseEvent* event);
		void showEvent(QShowEvent* event);

		protected slots:
		void on_agreement1_checked(int checked);
		void on_decline();
		void on_accept();

		signals:
		void declined(void);
		void accepted(void);
	};
} // namespace own3d::ui
