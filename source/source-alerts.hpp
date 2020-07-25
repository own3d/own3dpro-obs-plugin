#pragma once
#include <memory>
#include <string>
#include <utility>

#include <obs.h>
#include "obs/obs-source-factory.hpp"
#include "plugin.hpp"

namespace own3d::source {
	class alert_instance;

	class alert_factory : public obs::source_factory<own3d::source::alert_factory, own3d::source::alert_instance> {
		public:
		alert_factory();
		virtual ~alert_factory();

		const char* get_name() override;

		void get_defaults2(obs_data_t* data) override;

		obs_properties_t* get_properties2(own3d::source::alert_instance* data) override;
	};

	class alert_instance : public obs::source_instance {
		std::shared_ptr<obs_source_t>           _browser;
		std::pair<std::uint32_t, std::uint32_t> _size;
		std::string                             _url;

		public:
		alert_instance(obs_data_t*, obs_source_t*);
		virtual ~alert_instance();

		void load(obs_data_t* settings) override;

		void migrate(obs_data_t* settings, std::uint64_t version) override;

		bool parse_size(std::string_view text);

		bool parse_alert_type();

		void update(obs_data_t* settings) override;

		void save(obs_data_t* settings) override;

		std::uint32_t width() override;

		std::uint32_t height() override;

		void video_tick(float_t seconds) override;

		void video_render(gs_effect_t* effect) override;

		void enum_active_sources(obs_source_enum_proc_t enum_callback, void* param) override;
	};
} // namespace own3d::source
