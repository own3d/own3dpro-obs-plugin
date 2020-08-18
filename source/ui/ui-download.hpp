#pragma once
#include <QThread>
#include <QUrl>
#include <filesystem>
#include <fstream>
#include <obs-frontend-api.h>
#include "ui_theme-download.h"
#include "util/curl.hpp"
#include "util/zip.hpp"

namespace own3d::ui {
	enum class download_state : int64_t {
		UNKNOWN,
		CONNECTING,
		DOWNLOADING,
		DONE,
		FAILED,
	};

	class installer_thread : public QThread {
		Q_OBJECT;

		std::string           _url;
		std::string           _name;
		std::filesystem::path _path;
		std::filesystem::path _out_path;

		public:
		~installer_thread();
		installer_thread(std::string url, std::string name, std::filesystem::path path, std::filesystem::path out_path,
						 QObject* parent = nullptr);

		private:
		void run_download();

		void run_extract();

		void run_install();

		public:
		virtual void run() override;

		signals:
		; // Needed by some linters.

		void download_status(uint64_t now_bytes, uint64_t total_bytes, int64_t state);

		void extract_status(uint64_t now_files, uint64_t total_files, uint64_t now_bytes, uint64_t total_bytes);

		void install_status(bool complete);

		void error();

		void switch_collection(QString new_collection);
	};

	class installer : public QDialog, public Ui::ThemeDownload {
		Q_OBJECT;

		installer_thread* _worker;

		QUrl                  _download_url;
		QString               _download_hash;
		QString               _theme_name;
		std::filesystem::path _theme_archive_path;
		std::filesystem::path _theme_path;

		util::zip* _extractor;

		public:
		~installer();
		installer(const QUrl& url, const QString& name, const QString& hash);

		private:
		void update_progress(double_t percent, bool is_download, bool is_extract, bool is_install);

		private slots:
		; // Needed by some linters.

		void handle_download_status(uint64_t now_bytes, uint64_t total_bytes, int64_t state);

		void handle_extract_status(uint64_t now_files, uint64_t total_files, uint64_t now_bytes, uint64_t total_bytes);

		void handle_install_status(bool complete);

		void handle_switch_collection(QString new_collection);

		signals:
		; // Needed by some linters.
		void error();
	};
} // namespace own3d::ui
