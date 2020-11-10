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

#include "source-alerts.hpp"
#include <algorithm>

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
	obs_data_set_default_string(data, KEY_SIZE, "50%");
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
	: obs::source_instance(data, self), _browser(), _size(), _url(), _initialized(false)
{
	// Set reasonable defaults.
	_size.first  = 128;
	_size.second = 128;
	_url         = "https://own3d.tv/";

	// If there are settings already, parse them.
	if (data)
		parse_settings(data);

	// Create a custom obs_data_t for the browser source to use.
	auto bdata = std::shared_ptr<obs_data_t>(obs_data_create(), [](obs_data_t* v) { obs_data_release(v); });
	apply_settings(bdata.get());

	// Create browser with existing settings.
	_browser = std::shared_ptr<obs_source_t>(
		obs_source_create_private("browser_source", obs_source_get_name(_self), bdata.get()), own3d::source_deleter);
	if (!_browser)
		throw std::runtime_error("Failed to create browser source.");

	// Trigger a load event.
	obs_source_load(_browser.get());
}

own3d::source::alert_instance::~alert_instance() {}

void own3d::source::alert_instance::apply_settings(obs_data_t* data)
{
	obs_data_set_int(data, "width", std::max<int64_t>(16, _size.first));
	obs_data_set_int(data, "height", std::max<int64_t>(16, _size.second));
	obs_data_set_bool(data, "fps_custom", false);
	obs_data_set_bool(data, "reroute_audio", false);
	obs_data_set_bool(data, "restart_when_active", false);
	obs_data_set_bool(data, "shutdown", true);
	obs_data_set_string(data, "url", _url.c_str());
}

void own3d::source::alert_instance::load(obs_data_t* data)
{
	update(data);
}

void own3d::source::alert_instance::migrate(obs_data_t*, std::uint64_t) {}

bool own3d::source::alert_instance::parse_size_text(std::string_view text, uint32_t& size, bool w_or_h)
{
	char* eptr       = nullptr;
	bool  is_percent = text.find("%") != std::string_view::npos;
	errno            = 0;
	if (is_percent) {
		double_t prc = 0;
		if (prc = strtod(text.data(), &eptr); (eptr == text.data()) || (errno == ERANGE)) {
			return false;
		}

		if (obs_video_info ovi = {0}; obs_get_video_info(&ovi)) {
			if (w_or_h) {
				size = static_cast<uint32_t>((prc / 100.0) * ovi.base_height);
			} else {
				size = static_cast<uint32_t>((prc / 100.0) * ovi.base_width);
			}
		} else {
			return false;
		}
	} else {
		if (size = strtol(text.data(), &eptr, 10); (eptr == text.data()) || (errno == ERANGE)) {
			return false;
		}
	}
	return true;
}

bool own3d::source::alert_instance::parse_size(std::string_view text)
{
	bool     res = false;
	uint32_t w   = _size.first;
	uint32_t h   = _size.second;
	if (auto pos = text.find("x"); pos != std::string_view::npos) {
		if (!parse_size_text(text.substr(0, pos), w, false))
			return false;
		if (!parse_size_text(text.substr(pos + 1), h, true))
			return false;
	} else {
		if (!parse_size_text(text, w, false))
			return false;
		if (!parse_size_text(text, h, true))
			return false;
	}
	res          = res || (w != _size.first) || (h != _size.second);
	_size.first  = w;
	_size.second = h;

	return res;
}

bool own3d::source::alert_instance::parse_alert_type()
{
	std::vector<char> buffer(2048);
	std::string       format = own3d::get_api_endpoint("obs/browser-source/%s/components/alerts");
	buffer.resize(snprintf(buffer.data(), buffer.size(), format.c_str(), own3d::get_unique_identifier().data()));

	std::string url = std::string(buffer.data(), buffer.data() + buffer.size());
	if (_url.compare(url) == 0) {
		return false;
	}

	_url = url;
	return true;
}

bool own3d::source::alert_instance::parse_settings(obs_data_t* data)
{
	bool refresh = !_initialized;
	refresh      = parse_size(obs_data_get_string(data, KEY_SIZE)) || refresh;
	refresh      = parse_alert_type() || refresh;

	return refresh;
}

void own3d::source::alert_instance::update(obs_data_t* data)
{
	if (!parse_settings(data))
		return;

	std::shared_ptr<obs_data_t> bdata(obs_source_get_settings(_browser.get()), own3d::data_deleter);
	apply_settings(bdata.get());
	obs_source_update(_browser.get(), bdata.get());

	_initialized = true;
}

void own3d::source::alert_instance::save(obs_data_t*) {}

std::uint32_t own3d::source::alert_instance::width()
{
	return std::max<uint32_t>(1, _size.first);
}

std::uint32_t own3d::source::alert_instance::height()
{
	return std::max<uint32_t>(1, _size.second);
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
