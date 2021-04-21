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
#include <QString>
#include <QWidget>
#include <functional>
#include <string>
#include <util/util.hpp>

// -------------------------------------------------------------------------------- //
#define FN_EXTERN extern
#ifdef _MSC_VER
#define FN_IMPORT __declspec(dllimport)
#define FN_HIDE
#else
#define FN_IMPORT
#define FN_HIDE __attribute__((visibility("hidden")))
#endif

struct FN_HIDE QCefCookieManager {
	virtual ~QCefCookieManager() {}

	virtual bool DeleteCookies(const std::string& url, const std::string& name)                        = 0;
	virtual bool SetStoragePath(const std::string& storage_path, bool persist_session_cookies = false) = 0;
	virtual bool FlushStore()                                                                          = 0;

	typedef std::function<void(bool)> cookie_exists_cb;

	virtual void CheckForCookie(const std::string& site, const std::string& cookie, cookie_exists_cb callback) = 0;
};

class FN_HIDE QCefWidget : public QWidget {
	Q_OBJECT

	protected:
	inline QCefWidget(QWidget* parent) : QWidget(parent) {}

	public:
	virtual void setURL(const std::string& url)              = 0;
	virtual void setStartupScript(const std::string& script) = 0;
	virtual void allowAllPopups(bool allow)                  = 0;
	virtual void closeBrowser()                              = 0;

	signals:
	void titleChanged(const QString& title);
	void urlChanged(const QString& url);
};

struct FN_HIDE QCef {
	virtual ~QCef() {}

	virtual bool init_browser(void)          = 0;
	virtual bool initialized(void)           = 0;
	virtual bool wait_for_browser_init(void) = 0;

	virtual QCefWidget* create_widget(QWidget* parent, const std::string& url,
									  QCefCookieManager* cookie_manager = nullptr) = 0;

	virtual QCefCookieManager* create_cookie_manager(const std::string& storage_path,
													 bool               persist_session_cookies = false) = 0;

	virtual BPtr<char> get_cookie_path(const std::string& storage_path) = 0;

	virtual void add_popup_whitelist_url(const std::string& url, QObject* obj) = 0;
	virtual void add_force_popup_url(const std::string& url, QObject* obj)     = 0;
};

// -------------------------------------------------------------------------------- //

namespace obs {
	namespace browser {
		FN_HIDE QCef* instance();

		FN_HIDE int version();
	} // namespace browser
} // namespace obs
