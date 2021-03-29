#pragma once
#include <QWidget>

#include <util/platform.h>
#include <util/util.hpp>
#include "plugin.hpp"

LIB_LOCAL struct QCefCookieManager {
	virtual ~QCefCookieManager() {}

	virtual bool DeleteCookies(const std::string& url, const std::string& name)                        = 0;
	virtual bool SetStoragePath(const std::string& storage_path, bool persist_session_cookies = false) = 0;
	virtual bool FlushStore()                                                                          = 0;

	typedef std::function<void(bool)> cookie_exists_cb;

	virtual void CheckForCookie(const std::string& site, const std::string& cookie, cookie_exists_cb callback) = 0;
};

LIB_LOCAL class QCefWidget : public QWidget {
	Q_OBJECT

	protected:
	inline QCefWidget(QWidget* parent) : QWidget(parent) {}

	public:
	virtual void setURL(const std::string& url)              = 0;
	virtual void setStartupScript(const std::string& script) = 0;
	virtual void allowAllPopups(bool allow)                  = 0;
	virtual void closeBrowser()                              = 0;
	virtual void reloadPage()                                = 0;

	signals:
	void titleChanged(const QString& title);
	void urlChanged(const QString& url);
};

LIB_LOCAL struct QCef {
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

static inline LIB_LOCAL QCef* obs_browser_create_qcef(void)
{
#ifdef _WIN32
	void* lib = os_dlopen("obs-browser");
#else
	void* lib = os_dlopen("../obs-plugins/obs-browser");
#endif
	QCef* (*create_qcef)(void) = nullptr;

	if (!lib) {
		return nullptr;
	}

	create_qcef = (decltype(create_qcef))os_dlsym(lib, "obs_browser_create_qcef");
	if (!create_qcef)
		return nullptr;

	return create_qcef();
}

static inline LIB_LOCAL int obs_browser_qcef_version(void)
{
#ifdef _WIN32
	void* lib = os_dlopen("obs-browser");
#else
	void* lib = os_dlopen("../obs-plugins/obs-browser");
#endif
	int (*qcef_version)(void) = nullptr;

	if (!lib) {
		return 0;
	}

	qcef_version = (decltype(qcef_version))os_dlsym(lib, "obs_browser_qcef_version_export");
	if (!qcef_version)
		return 0;

	return qcef_version();
}
