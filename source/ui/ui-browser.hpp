#pragma once
#include <QBoxLayout>
#include <QDialog>
#include <QFrame>
#include <QUrl>
#include <QUrlQuery>
#include "obs/obs-browser-panel.hpp"

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
