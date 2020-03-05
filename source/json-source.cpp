#include "json-source.hpp"
#include <filesystem>
#include <list>

#include <graphics/matrix4.h>
#include <util/platform.h>

#define KEY_FILE "File"
#define KEY_SCENE "Scene"

void source_deleter(obs_source_t* v)
{
	obs_source_release(v);
}
void data_deleter(obs_data_t* v)
{
	obs_data_release(v);
}
void data_item_deleter(obs_data_item_t* v)
{
	obs_data_item_release(&v);
}
void data_array_deleter(obs_data_array_t* v)
{
	obs_data_array_release(v);
}
void sceneitem_deleter(obs_sceneitem_t* v)
{
	obs_sceneitem_remove(v);
	// obs_sceneitem_release does not deal with any of the other code.
	// Consistency isn't on a high list in OBS...
}

own3d::json_source::json_source(obs_data_t* settings, obs_source_t* _child)
	: obs::source_instance(settings, _child), _data(), _data_file(), _sources(), _scenes(), _scene()
{
	obs_enter_graphics();
	_rt = {gs_texrender_create(GS_RGBA, GS_ZS_NONE), [](gs_texrender_t* v) {
			   obs_enter_graphics();
			   gs_texrender_destroy(v);
			   obs_leave_graphics();
		   }};
	obs_leave_graphics();
}

own3d::json_source::~json_source()
{
	_sources.clear();
	_scenes.clear();
	_scene.reset();
}

uint32_t own3d::json_source::get_width()
{
	if (!_scene)
		return 0;

	return obs_source_get_width(_scene.get());
}

uint32_t own3d::json_source::get_height()
{
	if (!_scene)
		return 0;

	return obs_source_get_height(_scene.get());
}

struct rebuild_data {
	std::string                                                          base_file;
	std::map<std::shared_ptr<obs_source_t>, std::shared_ptr<obs_data_t>> source_to_data;
	std::map<std::string, std::shared_ptr<obs_source_t>>                 name_to_source;
	std::map<std::string, std::shared_ptr<obs_source_t>>                 name_to_scene;
	std::list<std::shared_ptr<obs_source_t>> scenes; // Contains all scene objects, not just scenes.
	std::list<std::shared_ptr<obs_source_t>> everything;
};

void assign_generic_source_info(std::shared_ptr<obs_source_t> _child, std::shared_ptr<obs_data_t> data)
{
	// Interlacing
	obs_source_set_deinterlace_mode(
		_child.get(), static_cast<obs_deinterlace_mode>(obs_data_get_int(data.get(), "deinterlace_mode")));
	obs_source_set_deinterlace_field_order(_child.get(), static_cast<obs_deinterlace_field_order>(
															 obs_data_get_int(data.get(), "deinterlace_field_order")));

	// Audio
	obs_source_set_balance_value(_child.get(), obs_data_get_double(data.get(), "balance"));
	obs_source_set_audio_mixers(_child.get(), obs_data_get_int(data.get(), "mixers"));
	obs_source_set_volume(_child.get(), obs_data_get_double(data.get(), "volume"));
	obs_source_set_muted(_child.get(), obs_data_get_bool(data.get(), "muted"));
	obs_source_set_sync_offset(_child.get(), obs_data_get_int(data.get(), "sync"));
	obs_source_set_monitoring_type(_child.get(),
								   static_cast<obs_monitoring_type>(obs_data_get_int(data.get(), "monitoring-type")));

	// Hotkeys
	obs_source_enable_push_to_mute(_child.get(), obs_data_get_bool(data.get(), "push-to-mute"));
	obs_source_set_push_to_mute_delay(_child.get(), obs_data_get_int(data.get(), "push-to-mute-delay"));
	obs_source_enable_push_to_talk(_child.get(), obs_data_get_bool(data.get(), "push-to-talk"));
	obs_source_set_push_to_talk_delay(_child.get(), obs_data_get_int(data.get(), "push-to-talk-delay"));

	// Other
	obs_source_set_enabled(_child.get(), obs_data_get_bool(data.get(), "enabled"));
	obs_source_set_flags(_child.get(), obs_data_get_int(data.get(), "flags"));
}

void convert_relative_to_absolute_paths(obs_data_t* data, std::string base_dir)
{
	static std::string keyword{"<REPLACE|ME>"};
	for (obs_data_item_t* item = obs_data_first(data); item != nullptr; obs_data_item_next(&item)) {
		switch (obs_data_item_gettype(item)) {
		case obs_data_type::OBS_DATA_STRING: {
			const char* cstr = obs_data_item_get_string(item);
			if (cstr == nullptr)
				break;
			std::string string = cstr;
			std::string prefix = string.substr(0, keyword.length());
			if (prefix == keyword) {
				string = base_dir + string.substr(keyword.length());
			}
			obs_data_item_set_string(&item, string.c_str());
			break;
		}
		case obs_data_type::OBS_DATA_OBJECT: {
			obs_data_t* chld = obs_data_item_get_obj(item);
			convert_relative_to_absolute_paths(chld, base_dir);
			//obs_data_release(chld);
		}
		}
	}
}

void parse_source_list(std::shared_ptr<obs_data_array_t> sources, rebuild_data& data)
{
	for (size_t idx = 0, edx = obs_data_array_count(sources.get()); idx < edx; idx++) {
		std::shared_ptr<obs_data_t> item{obs_data_array_item(sources.get(), idx), data_deleter};

		// Skip any source that has no name or an invalid name.
		const char* name = obs_data_get_string(item.get(), "name");
		if ((!name) || (strlen(name) <= 0))
			continue;

		// Grab the id and apply some corrections.
		std::string id       = obs_data_get_string(item.get(), "id");
		bool        is_group = false;
		if (id == "group") { // ToDo: Should this actually be done?! This breaks setups.
			// A 'group' is just a 'scene', so let's fix that.
			id       = "scene";
			is_group = true;
		}

		// If it has a name, let's try to create it.
		std::shared_ptr<obs_data_t>   settings{obs_data_get_obj(item.get(), "settings"), data_deleter};
		std::shared_ptr<obs_source_t> _child;
		if (id == "scene") {
			std::shared_ptr<obs_data_t> temp{obs_data_create(), data_deleter};

			obs_data_set_bool(temp.get(), "custom_size", obs_data_get_bool(settings.get(), "custom_size"));
			obs_data_set_int(temp.get(), "cx", obs_data_get_int(settings.get(), "cx"));
			obs_data_set_int(temp.get(), "cy", obs_data_get_int(settings.get(), "cy"));
			obs_data_set_int(temp.get(), "id_counter", obs_data_get_int(settings.get(), "id_counter"));
			obs_data_set_array(temp.get(), "items", obs_data_array_create());

			// Scenes and Groups are created without settings and fixed up later.
			_child = {obs_source_create_private(id.c_str(), name, temp.get()), source_deleter};
		} else {
			convert_relative_to_absolute_paths(settings.get(), data.base_file);
			_child = {obs_source_create_private(id.c_str(), name, settings.get()), source_deleter};
		}
		if (!_child) {
			// If creation of the source failed, skip the source.
			continue;
		}
		DLOG_INFO("Created source '%s' with name '%s'.", id.c_str(), name);

		// Assign generic data.
		assign_generic_source_info(_child, item);

		// And then insert the source into the data structures.
		data.everything.push_back(_child);
		data.source_to_data.emplace(_child, settings);
		data.name_to_source.emplace(name, _child);
		if ((id == "scene") && !is_group) {
			data.name_to_scene.emplace(name, _child);
		}
		if (id == "scene") {
			data.scenes.push_back(_child);
		}

		// But wait, there's more! Filters.
		std::shared_ptr<obs_data_array_t> filters{obs_data_get_array(item.get(), "filters"), data_array_deleter};
		for (size_t idx = 0, edx = obs_data_array_count(filters.get()); idx < edx; idx++) {
			// Filters are basically a list of sources that belong to a certain source and are unique to that source.
			// Their data is identical to any other source.
			std::shared_ptr<obs_data_t> item{obs_data_array_item(filters.get(), idx), data_deleter};

			// Skip any source that has no name or an invalid name.
			const char* name = obs_data_get_string(item.get(), "name");
			if ((!name) || (strlen(name) <= 0))
				continue;

			// If it has a name, let's try to create it.
			std::string                   id = obs_data_get_string(item.get(), "id");
			std::shared_ptr<obs_data_t>   settings{obs_data_get_obj(item.get(), "settings"), data_deleter};
			std::shared_ptr<obs_source_t> filter{obs_source_create_private(id.c_str(), name, settings.get()),
												 source_deleter};
			if (!filter) {
				// If creation of the source failed, skip the source.
				continue;
			}

			// Assign generic data.
			assign_generic_source_info(filter, item);

			// Attach to parent source.
			obs_source_filter_add(_child.get(), filter.get());
			DLOG_INFO("    Added filter '%s' with name '%s'.", id.c_str(), name);

			// Insert into data structures.
			data.everything.push_back(filter);
		}
	}
}

void parse_scene_contents(rebuild_data& data)
{
	vec2               pos;
	vec2               scale;
	vec2               bounds;
	obs_sceneitem_crop crop;

	for (auto scene : data.scenes) {
		auto kv = data.source_to_data.find(scene);
		if (kv == data.source_to_data.end())
			continue;

		// Now that we are done building all sources, we need to add things to
		// scenes and groups. We can't just assign the settings data as it
		// relies on names which we can't rely on as they might collide with
		// the users data.

		// But the first thing we have to do is call '_load' on them due to the
		// custom_size setting not being exposed to the outside. This seems to
		// be an intentional limitation of OBS Studio so that one creates groups
		// instead of scenes.
		obs_source_load(kv->first.get());

		std::shared_ptr<obs_data_array_t> items{obs_data_get_array(kv->second.get(), "items"), data_array_deleter};
		if (!items) {
			DLOG_INFO("Building scene '%s' failed due to missing items array.", obs_source_get_name(scene.get()));
			continue;
		}

		DLOG_INFO("Building scene '%s' with %lld items ...", obs_source_get_name(scene.get()),
				  obs_data_array_count(items.get()));
		for (size_t idx = 0, edx = obs_data_array_count(items.get()); idx < edx; idx++) {
			std::shared_ptr<obs_data_t> obj{obs_data_array_item(items.get(), idx), data_deleter};
			if (!obj) {
				DLOG_INFO("    <%lld> Entry corrupted.", idx);
				continue;
			}

			const char* name = obs_data_get_string(obj.get(), "name");
			if (!name) {
				DLOG_INFO("    <%lld> Entry missing name.", idx);
				continue;
			}

			auto _child = data.name_to_source.find(name);
			if (_child == data.name_to_source.end()) {
				DLOG_INFO("    <%lld> Failed to find source '%s'.", idx, name);
				continue;
			}

			obs_sceneitem_t* item = obs_scene_add(obs_scene_from_source(scene.get()), _child->second.get());
			if (!item) {
				DLOG_INFO("    <%lld> Failed to add source '%s'.", idx, name);
				continue;
			}

			obs_sceneitem_set_alignment(item, obs_data_get_int(obj.get(), "align"));
			obs_data_get_vec2(obj.get(), "bounds", &bounds);
			obs_sceneitem_set_bounds(item, &bounds);
			obs_sceneitem_set_bounds_alignment(item, obs_data_get_int(obj.get(), "bounds_align"));
			obs_sceneitem_set_bounds_type(item,
										  static_cast<obs_bounds_type>(obs_data_get_int(obj.get(), "bounds_type")));
			crop.left   = obs_data_get_int(obj.get(), "crop_left");
			crop.right  = obs_data_get_int(obj.get(), "crop_right");
			crop.top    = obs_data_get_int(obj.get(), "crop_top");
			crop.bottom = obs_data_get_int(obj.get(), "crop_bottom");
			obs_sceneitem_set_crop(item, &crop);
			//obs_sceneitem_set_locked(item, obs_data_get_bool(obj.get(), "locked"));
			obs_data_get_vec2(obj.get(), "pos", &pos);
			obs_sceneitem_set_pos(item, &pos);
			obs_sceneitem_set_rot(item, obs_data_get_double(obj.get(), "rot"));
			obs_data_get_vec2(obj.get(), "scale", &scale);
			obs_sceneitem_set_scale(item, &scale);
			obs_sceneitem_set_scale_filter(item,
										   static_cast<obs_scale_type>(obs_data_get_int(obj.get(), "scale_filter")));
			obs_sceneitem_set_visible(item, obs_data_get_bool(obj.get(), "visible"));
			obs_sceneitem_force_update_transform(item);

			DLOG_INFO("    <%lld> Added source '%s'.", idx, name);
		}
	}
}

void own3d::json_source::update(obs_data_t* settings)
{
	// Try to update file.
	if (const char* file = obs_data_get_string(settings, KEY_FILE); _data_file != file)
		try {
			std::shared_ptr<obs_data_t> data{obs_data_create_from_json_file(file),
											 [](obs_data_t* v) { obs_data_release(v); }};

			// Loading of the files:
			// - All sources are private.
			//   - Can't rely on names for loading as private sources don't have any.
			// - Should build name to source map.
			// - Should build list of all sources.

			rebuild_data rd;

			// Create base directory name from file name.
			std::filesystem::path abs_path      = std::filesystem::absolute(std::filesystem::path{std::string{file}});
			std::string           abs_path_name = abs_path.parent_path().string();
			rd.base_file                        = abs_path_name;

			// Get Sources and create everything.
			std::shared_ptr<obs_data_array_t> data_sources{obs_data_get_array(data.get(), "sources"),
														   data_array_deleter};
			parse_source_list(data_sources, rd);

			// Get Groups and create everything. (These are effectively a list of scenes inside scenes.)
			std::shared_ptr<obs_data_array_t> data_groups{obs_data_get_array(data.get(), "groups"), data_array_deleter};
			parse_source_list(data_groups, rd);

			// We should now have a full list of all sources, including their
			// filters and data. The next step from here is to build the scenes
			// and groups, as we can't rely on name indexing for these.
			parse_scene_contents(rd);

			// Call load on all sources so they know they've been loaded from
			// disk and restored.
			for (auto& _child : rd.everything) {
				if (obs_source_get_id(_child.get()) != std::string{"scene"})
					obs_source_load(_child.get());
			}

			// Update internal tables.
			_data_file = file;
			_data.swap(data);
			_sources.swap(rd.name_to_source);
			_scenes.swap(rd.name_to_scene);
			_scene.reset();
			_scene_name.clear();
			_scene_child.reset();
		} catch (...) {
		}

	if (const char* name = obs_data_get_string(settings, KEY_SCENE); _scene_name != name) {
		_scene_child.reset();
		auto scene = _scenes.find(name);
		if (scene != _scenes.end()) {
			_scene       = scene->second;
			_scene_name  = name;
			_scene_child = std::make_shared<active_child_helper>(_self, _scene);
		}
	}
}

void own3d::json_source::save(obs_data_t* settings)
{
	obs_data_set_string(settings, KEY_FILE, _data_file.c_str());
	obs_data_set_string(settings, KEY_SCENE, _scene_name.c_str());
}

void own3d::json_source::load(obs_data_t* settings)
{
	update(settings);
}

void own3d::json_source::activate() {}

void own3d::json_source::deactivate() {}

void own3d::json_source::show() {}

void own3d::json_source::hide() {}

void own3d::json_source::video_tick(float seconds) {}

void own3d::json_source::video_render(gs_effect_t* effect)
{
	const float color[4] = {1.0, 1.0, 1.0, 1.0};
	if (!_scene)
		return;

	gs_debug_marker_begin_format(color, "Render JSON");
	gs_texrender_reset(_rt.get());
	if (!gs_texrender_begin(_rt.get(), get_width(), get_height())) {
		return;
	}
	vec4 clearc = {0};
	gs_clear(GS_CLEAR_COLOR, &clearc, 0, 0);
	gs_ortho(0, get_width(), 0, get_height(), 0, 1);
	obs_source_video_render(_scene.get());
	gs_texrender_end(_rt.get());
	gs_debug_marker_end();

	gs_debug_marker_begin_format(color, "Draw JSON");
	while (gs_effect_loop(obs_get_base_effect(OBS_EFFECT_DEFAULT), "Draw")) {
		obs_source_draw(gs_texrender_get_texture(_rt.get()), 0, 0, get_width(), get_height(), false);
	}
	gs_debug_marker_end();
}

void own3d::json_source::enum_active_sources(obs_source_enum_proc_t enum_callback, void* param)
{
	if (_scene) {
		enum_callback(_self, _scene.get(), param);
	}
}

void own3d::json_source::enum_all_sources(obs_source_enum_proc_t enum_callback, void* param)
{
	if (_scene) {
		enum_callback(_self, _scene.get(), param);
	}
}

void own3d::json_source::get_properties(obs_properties_t* props)
{
	{
		auto p = obs_properties_add_path(props, KEY_FILE, KEY_FILE, OBS_PATH_FILE, "JSON (*.json)", _data_file.c_str());
		obs_property_set_modified_callback2(
			p,
			[](void* ptr, obs_properties_t* props, obs_property_t* pr, obs_data_t* settings) noexcept {
				return reinterpret_cast<own3d::json_source*>(ptr)->modified_file(props, pr, settings);
			},
			this);
	}
	{
		auto p = obs_properties_add_list(props, KEY_SCENE, KEY_SCENE, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	}
}

bool own3d::json_source::modified_file(obs_properties_t* props, obs_property_t* pr, obs_data_t* settings)
{
	// The 'modified_callback' for properties requires you to call update yourself,
	// as the next update call after it will be too late. So in order to not dup-
	// licate code, it is best to call update as early as possible and always up-
	// date no matter the reason or if things actually updated. This kind of breaks
	// user input catastrophically as OBS re-creates everything and does not re-use
	// any already existing elements.
	//
	// In essence, you will see this call structure in OBS 25.x and earlier (and
	// possible later versions too):
	// - modified_callback
	//   - if returned true: update
	// - update
	// - when dialog is closed:
	//   - update
	//   - save

	// Manually calling it here.
	update(settings);

	// Rebuild list.
	{
		auto p = obs_properties_get(props, KEY_SCENE);
		obs_property_list_clear(p);
		for (auto& kv : _scenes) {
			obs_property_list_add_string(p, kv.first.c_str(), kv.first.c_str());
		}
	}

	return true;
}

own3d::json_source_factory::json_source_factory()
{
	_info.id           = "own3dtv-json-source";
	_info.type         = OBS_SOURCE_TYPE_INPUT;
	_info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;

	set_have_active_child_sources(true);
	set_have_child_sources(true);
	this->finish_setup();
}

const char* own3d::json_source_factory::get_name()
{
	return "Own3d.TV JSON Source";
}

void own3d::json_source_factory::get_defaults2(obs_data_t* data) {}

obs_properties_t* own3d::json_source_factory::get_properties2(own3d::json_source* data)
{
	obs_properties_t* props = obs_properties_create();
	obs_properties_set_param(props, data, [](void*) noexcept {});

	if (data) {
		data->get_properties(props);
	}

	return props;
}

void own3d::json_source_factory::register_in_obs()
{
	static std::shared_ptr<own3d::json_source_factory> inst;
	inst = std::make_shared<own3d::json_source_factory>();
}
