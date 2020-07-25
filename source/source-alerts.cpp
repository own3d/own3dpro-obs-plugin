#include "source-alerts.hpp"

// Need to wrap around browser source.
// Dropdown to select alert type.
//

#define STR "Source.Alerts"
#define STR_SIZE STR ".Size"

#define KEY_SIZE "Size"

own3d::source::alert_factory::alert_factory()
{
	_info.id           = "own3d-alert";
	_info.type         = OBS_SOURCE_TYPE_INPUT;
	_info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_DO_NOT_DUPLICATE;

	set_resolution_enabled(true);
	/*	set_activity_tracking_enabled(true);
	set_visibility_tracking_enabled(true);*/
	//set_have_child_sources(true);
	set_have_active_child_sources(true);
	finish_setup();

	register_proxy("own3d_alert");
}

own3d::source::alert_factory::~alert_factory() {}

const char* own3d::source::alert_factory::get_name()
{
	return D_TRANSLATE(STR);
}

void own3d::source::alert_factory::get_defaults2(obs_data_t* data)
{
	obs_data_set_default_string(data, KEY_SIZE, "384x512");
}

obs_properties_t* own3d::source::alert_factory::get_properties2(own3d::source::alert_instance*)
{
	auto* prs = obs_properties_create();

	{
		obs_properties_add_text(prs, KEY_SIZE, D_TRANSLATE(STR_SIZE), OBS_TEXT_DEFAULT);
	}

	return prs;
}

own3d::source::alert_instance::alert_instance(obs_data_t* data, obs_source_t* self)
	: obs::source_instance(data, self), _size(), _url()
{
	_browser = std::shared_ptr<obs_source_t>(
		obs_source_create_private("browser_source", obs_source_get_name(_self), nullptr), own3d::source_deleter);
	if (!_browser)
		throw std::runtime_error("Failed to create browser source.");

	if (data)
		load(data);
}

own3d::source::alert_instance::~alert_instance() {}

void own3d::source::alert_instance::load(obs_data_t* settings)
{
	update(settings);
}

void own3d::source::alert_instance::migrate(obs_data_t*, std::uint64_t) {}

bool own3d::source::alert_instance::parse_size(std::string_view text)
{
	bool res = false;

	if (auto pos = text.find("x"); pos != std::string_view::npos) {
		uint32_t w    = _size.first;
		uint32_t h    = _size.second;
		char*    eptr = nullptr;

		errno = 0;
		if (w = strtol(text.data(), &eptr, 10); (eptr == text.data()) || (errno == ERANGE)) {
			return false;
		}

		errno = 0;
		if (h = strtol(text.data() + pos + 1, &eptr, 10); (eptr == text.data() + pos + 1) || (errno == ERANGE)) {
			return false;
		}

		res          = res || (w != _size.first) || (h != _size.second);
		_size.first  = w;
		_size.second = h;
	} else {
		uint32_t s    = _size.first;
		char*    eptr = nullptr;

		errno = 0;
		if (s = strtol(text.data(), &eptr, 10); (eptr == text.data()) || (errno == ERANGE)) {
			return false;
		}

		res          = res || (s != _size.first) || (s != _size.second);
		_size.first  = s;
		_size.second = s;
	}

	return res;
}

bool own3d::source::alert_instance::parse_alert_type()
{
	std::vector<char> buffer(2048);
	std::string       format = own3d::get_api_endpoint("obs/browser-source/%s/alerts");
	buffer.resize(snprintf(buffer.data(), buffer.size(), format.c_str(), own3d::get_unique_identifier().data()));

	std::string url = std::string(buffer.data(), buffer.data() + buffer.size());
	if (_url.compare(url) == 0) {
		return false;
	}

	_url = url;
	return true;
}

void own3d::source::alert_instance::update(obs_data_t* settings)
{
	bool refresh = false;
	refresh      = refresh || parse_size(obs_data_get_string(settings, KEY_SIZE));
	refresh      = refresh || parse_alert_type();

	if (refresh) { // Update browser source
		auto bdata = std::shared_ptr<obs_data_t>(obs_source_get_settings(_browser.get()), own3d::data_deleter);

		obs_data_set_int(bdata.get(), "width", _size.first);
		obs_data_set_int(bdata.get(), "height", _size.second);
		obs_data_set_bool(bdata.get(), "fps_custom", false);
		obs_data_set_bool(bdata.get(), "reroute_audio", true);
		obs_data_set_bool(bdata.get(), "restart_when_active", false);
		obs_data_set_bool(bdata.get(), "shutdown", true);
		obs_data_set_string(bdata.get(), "url", _url.c_str());

		obs_source_update(_browser.get(), bdata.get());
	}
}

void own3d::source::alert_instance::save(obs_data_t*) {}

std::uint32_t own3d::source::alert_instance::width()
{
	return _size.first;
}

std::uint32_t own3d::source::alert_instance::height()
{
	return _size.second;
}

void own3d::source::alert_instance::video_tick(float_t) {}

void own3d::source::alert_instance::video_render(gs_effect_t*)
{
	obs_source_video_render(_browser.get());
}

void own3d::source::alert_instance::enum_active_sources(obs_source_enum_proc_t enum_callback, void* param)
{
	if (_browser)
		enum_callback(_self, _browser.get(), param);
}
