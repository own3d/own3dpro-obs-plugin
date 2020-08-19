#include "source-chat.hpp"
#include <algorithm>
#include <string_view>

#define STR "Source.Chat"
#define STR_SIZE STR ".Size"
#define STR_COLOR STR ".Color"
#define STR_FONT STR ".Font"

#define KEY_SIZE "Size"
#define KEY_COLOR "Color"
#define KEY_FONT "Font"

static constexpr std::string_view fonts[] = {
	"Alfa Slab One", "Anton",          "Arbutus",          "Audiowide",    "Azonix",         "Bangers",    "Bebas Neue",
	"Black Ops One", "Brick Sans",     "Built Titling",    "Bungee",       "Chewy",          "Cinzel",     "Evogria",
	"Fredoka One",   "Gotham Ultra",   "Lobster",          "Luckiest Guy", "Nemesis Grant",  "Open Sans",  "Oswald",
	"Pacifico",      "Patua One",      "Permanent Marker", "Pirata One",   "Press Start 2P", "Qualy",      "Righteous",
	"Roboto",        "Sancreek",       "Shojumaru",        "Sigmar One",   "Slackey",        "Squada One", "Teko",
	"Ubuntu",        "UnifrakturCook", "Vampiro One",      "Wallpoet",
};

own3d::source::chat_factory::chat_factory()
{
	_info.id           = "own3d-chat";
	_info.type         = OBS_SOURCE_TYPE_INPUT;
	_info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_DO_NOT_DUPLICATE;

	set_resolution_enabled(true);
	/*	set_activity_tracking_enabled(true);
	set_visibility_tracking_enabled(true);*/
	//set_have_child_sources(true);
	set_have_active_child_sources(true);
	finish_setup();

	register_proxy("own3d_chat");
}

own3d::source::chat_factory::~chat_factory() {}

const char* own3d::source::chat_factory::get_name()
{
	return D_TRANSLATE(STR);
}

void own3d::source::chat_factory::get_defaults2(obs_data_t* data)
{
	obs_data_set_default_string(data, KEY_SIZE, "20%x100%");
	obs_data_set_default_int(data, KEY_COLOR, 0xFFFFFFFF);
	obs_data_set_default_string(data, KEY_FONT, fonts[0].data());
}

obs_properties_t* own3d::source::chat_factory::get_properties2(own3d::source::chat_instance*)
{
	auto* prs = obs_properties_create();

	{
		obs_properties_add_text(prs, KEY_SIZE, D_TRANSLATE(STR_SIZE), OBS_TEXT_DEFAULT);
	}

	{
		obs_properties_add_color(prs, KEY_COLOR, D_TRANSLATE(STR_COLOR));
	}

	{
		auto* p =
			obs_properties_add_list(prs, KEY_FONT, D_TRANSLATE(STR_FONT), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
		for (auto f : fonts) {
			obs_property_list_add_string(p, f.data(), f.data());
		}
	}

	return prs;
}

own3d::source::chat_instance::chat_instance(obs_data_t* data, obs_source_t* self)
	: obs::source_instance(data, self), _size(), _url(), _initialized(false)
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

own3d::source::chat_instance::~chat_instance() {}

void own3d::source::chat_instance::apply_settings(obs_data_t* data)
{
	obs_data_set_int(data, "width", std::max<int64_t>(16, _size.first));
	obs_data_set_int(data, "height", std::max<int64_t>(16, _size.second));
	obs_data_set_bool(data, "fps_custom", false);
	obs_data_set_bool(data, "reroute_audio", true);
	obs_data_set_bool(data, "restart_when_active", false);
	obs_data_set_bool(data, "shutdown", true);
	obs_data_set_string(data, "url", _url.c_str());
}

void own3d::source::chat_instance::load(obs_data_t* data)
{
	update(data);
}

void own3d::source::chat_instance::migrate(obs_data_t*, std::uint64_t) {}

bool own3d::source::chat_instance::parse_size_text(std::string_view text, uint32_t& size, bool w_or_h)
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

bool own3d::source::chat_instance::parse_size(std::string_view text)
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

bool own3d::source::chat_instance::parse_chat(uint32_t color, std::string_view font)
{
	std::vector<char> buffer(2048);
	std::string format = own3d::get_api_endpoint("obs/browser-source/%s/components/chat?font-family=%s&color=%08X");
	buffer.resize(snprintf(buffer.data(), buffer.size(), format.c_str(), own3d::get_unique_identifier().data(),
						   font.data(), color));

	std::string url = std::string(buffer.data(), buffer.data() + buffer.size());
	if (_url.compare(url) == 0) {
		return false;
	}

	_url = url;
	return true;
}

bool own3d::source::chat_instance::parse_settings(obs_data_t* data)
{
	bool refresh = !_initialized;
	refresh      = parse_size(obs_data_get_string(data, KEY_SIZE)) || refresh;
	refresh = parse_chat(static_cast<uint32_t>(obs_data_get_int(data, KEY_COLOR)), obs_data_get_string(data, KEY_FONT))
			  || refresh;

	return refresh;
}

void own3d::source::chat_instance::update(obs_data_t* data)
{
	if (!parse_settings(data))
		return;

	std::shared_ptr<obs_data_t> bdata(obs_source_get_settings(_browser.get()), own3d::data_deleter);
	apply_settings(bdata.get());
	obs_source_update(_browser.get(), bdata.get());

	_initialized = true;
}

void own3d::source::chat_instance::save(obs_data_t*) {}

std::uint32_t own3d::source::chat_instance::width()
{
	return std::max<uint32_t>(1, _size.first);
}

std::uint32_t own3d::source::chat_instance::height()
{
	return std::max<uint32_t>(1, _size.second);
}

void own3d::source::chat_instance::video_tick(float_t) {}

void own3d::source::chat_instance::video_render(gs_effect_t*)
{
	obs_source_video_render(_browser.get());
}

void own3d::source::chat_instance::enum_active_sources(obs_source_enum_proc_t enum_callback, void* param)
{
	if (_browser)
		enum_callback(_self, _browser.get(), param);
}
