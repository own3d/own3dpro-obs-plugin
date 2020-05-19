#include "ui.hpp"
#include "plugin.hpp"

static constexpr std::string_view I18N_THEMEBROWSER_MENU = "Menu.ThemeBrowser";

own3d::ui::ui::~ui()
{
	obs_frontend_remove_event_callback(obs_event_handler, this);
}

own3d::ui::ui::ui() : _theme_browser()
{
	obs_frontend_add_event_callback(obs_event_handler, this);
}

void own3d::ui::ui::obs_event_handler(obs_frontend_event event, void* private_data)
{
	own3d::ui::ui* ui = reinterpret_cast<own3d::ui::ui*>(private_data);
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		ui->load();
	}
}

void own3d::ui::ui::load()
{
	initialize_obs();
	initialize_browser();
}

void own3d::ui::ui::initialize_obs()
{
	// Attach to OBS's Tools menu.
	_action =
		reinterpret_cast<QAction*>(obs_frontend_add_tools_menu_qaction(D_TRANSLATE(I18N_THEMEBROWSER_MENU.data())));
	connect(_action, &QAction::triggered, this, &own3d::ui::ui::own3d_action_triggered);
}

void own3d::ui::ui::initialize_browser()
{
	// Create Theme Browser.
	_theme_browser = new own3d::ui::browser();
	connect(_theme_browser, &own3d::ui::browser::selected, this, &own3d::ui::ui::own3d_theme_selected);
}

void own3d::ui::ui::own3d_action_triggered(bool)
{
	_theme_browser->show();
}

std::shared_ptr<own3d::ui::ui> own3d::ui::ui::_instance = nullptr;

void own3d::ui::ui::initialize()
{
	if (!own3d::ui::ui::_instance)
		own3d::ui::ui::_instance = std::make_shared<own3d::ui::ui>();
}

void own3d::ui::ui::finalize()
{
	own3d::ui::ui::_instance.reset();
}

std::shared_ptr<own3d::ui::ui> own3d::ui::ui::instance()
{
	return own3d::ui::ui::_instance;
}

void own3d::ui::ui::own3d_theme_selected(const QUrl& download_url, const QString& name, const QString& hash)
{ // We have a theme to download!
	// Hide the browser.
	_theme_browser->hide();

	// Recreate the download dialog.
	// FIXME! Don't recreate it, instead reset it.
	_download = new own3d::ui::installer(download_url, name, hash);
}
