#pragma once
#include <QAction>
#include <memory>
#include <obs-frontend-api.h>
#include "ui-browser.hpp"
#include "ui-download.hpp"

namespace own3d::ui {
	class ui : public QObject {
		Q_OBJECT;

		private:
		QAction*            _action;
		own3d::ui::browser* _theme_browser;

		own3d::ui::installer* _download;

		public:
		~ui();
		ui();

		private:
		static void obs_event_handler(obs_frontend_event event, void* private_data);

		void load();

		void initialize_obs();

		void initialize_browser();

		private slots:
		; // Needed by some linters.
		void own3d_action_triggered(bool);

		void own3d_theme_selected(const QUrl& download_url, const QString& name, const QString& hash);

		private /* Singleton */:
		static std::shared_ptr<own3d::ui::ui> _instance;

		public /* Singleton */:
		static void initialize();

		static void finalize();

		static std::shared_ptr<own3d::ui::ui> instance();
	};
} // namespace own3d::ui
