#pragma once
#include <map>
#include <memory>
#include <obs.h>
#include "obs-source-factory.hpp"
#include "plugin.hpp"

/* Loads OBS exported scene collections.
 * 
 * The OBS JSON Export is a little weird, and seems to rely on out of order create
 * and insertion. In order to completely load the file, you first have to create
 * all sources, scenes and groups, and then you can actually insert them into the
 * correct scenes and groups.
 * 
 * An alternative way to deal with this would be to load things one source at a
 * time, and resolve any references to sources immediately. This however does not
 * work with third-party solutions which may have a source name in their settings.
 * In fact, it is not clear if those will work at all, since most of the sources
 * may have to be privately created to work at all.
 *
 * In the end, we will mimic the OBS internal behavior as close as possible. Our
 * first task is to create all sources, including scenes and groups, but exclude
 * the items of those scenes and groups for safety reasons. Once that is done we
 * actually create the contents of the scene in one go. After that, all that's
 * left to do is call obs_source_load on all sources, and we're done.
 */

namespace own3d {
	class active_child_helper {
		obs_source_t*                 _parent;
		std::shared_ptr<obs_source_t> _child;

		public:
		active_child_helper(obs_source_t* parent, std::shared_ptr<obs_source_t> child) : _parent(parent), _child(child)
		{
			obs_source_add_active_child(_parent, _child.get());
		}
		~active_child_helper()
		{
			obs_source_remove_active_child(_parent, _child.get());
		}
	};

	class json_source : public obs::source_instance {
		std::shared_ptr<obs_data_t> _data;
		std::string                 _data_file;

		std::map<std::string, std::shared_ptr<obs_source_t>> _sources;
		std::map<std::string, std::shared_ptr<obs_source_t>> _scenes;
		std::shared_ptr<obs_source_t>                        _scene;
		std::shared_ptr<active_child_helper>                 _scene_child;
		std::string                                          _scene_name;

		std::shared_ptr<gs_texrender_t> _rt;

		public:
		json_source(obs_data_t* settings, obs_source_t* _child);
		virtual ~json_source();

		virtual uint32_t get_width() override;

		virtual uint32_t get_height() override;

		virtual void update(obs_data_t* settings) override;

		virtual void save(obs_data_t* settings) override;

		virtual void load(obs_data_t* settings) override;

		virtual void activate() override;

		virtual void deactivate() override;

		virtual void show() override;

		virtual void hide() override;

		virtual void video_tick(float seconds) override;

		virtual void video_render(gs_effect_t* effect) override;

		virtual void enum_active_sources(obs_source_enum_proc_t enum_callback, void* param) override;

		virtual void enum_all_sources(obs_source_enum_proc_t enum_callback, void* param) override;

		void get_properties(obs_properties_t* props);

		bool modified_file(obs_properties_t* props, obs_property_t* pr, obs_data_t* settings);
	};

	class json_source_factory : public obs::source_factory<own3d::json_source_factory, own3d::json_source> {
		public:
		json_source_factory();

		virtual const char* get_name() override;

		virtual void get_defaults2(obs_data_t* data) override;

		virtual obs_properties_t* get_properties2(own3d::json_source* data) override;

		public:
		static void register_in_obs();
	};

} // namespace own3d
