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
#include <QBoxLayout>
#include <QDialog>
#include <QFrame>
#include <QUrl>
#include <QUrlQuery>
#include "obs/obs-browser.hpp"

namespace own3d::ui {
	class browser : public QDialog {
		Q_OBJECT;

		private:
		QVBoxLayout* _layout;
		QCef*        _cef;
		QCefWidget*  _cef_widget;

		public:
		~browser();
		browser();

		void show();

		protected:
		void closeEvent(QCloseEvent*) override;

		QUrl generate_url();

		private Q_SLOTS:
		; // Needed by some linters.
		void url_changed(const QString& url);
		void title_changed(const QString& title);

		signals:
		; // Needed by some linters.
		void selected(const QUrl& download_url, const QString& name, const QString& hash);
		void cancelled();
	};
} // namespace own3d::ui
