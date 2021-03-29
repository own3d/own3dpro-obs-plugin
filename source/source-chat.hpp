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

#pragma once
#include <memory>
#include <string>
#include <utility>

#include <obs.h>
#include "obs/obs-source-factory.hpp"
#include "plugin.hpp"

namespace own3d::source {
	class chat_instance;

	class LIB_LOCAL chat_factory : public obs::source_factory<own3d::source::chat_factory, own3d::source::chat_instance> {
		public:
		chat_factory();
		virtual ~chat_factory();

		const char* get_name() override;

		void get_defaults2(obs_data_t* data) override;

		obs_properties_t* get_properties2(own3d::source::chat_instance* data) override;
	};

	class LIB_LOCAL chat_instance : public obs::source_instance {
		std::shared_ptr<obs_source_t>           _browser;
		std::pair<std::uint32_t, std::uint32_t> _size;
		std::string                             _url;
		bool                                    _initialized;

		public:
		chat_instance(obs_data_t*, obs_source_t*);
		virtual ~chat_instance();

		void apply_settings(obs_data_t* data);

		void load(obs_data_t* data) override;

		void migrate(obs_data_t* data, std::uint64_t version) override;

		bool parse_size_text(std::string_view text, uint32_t& size, bool w_or_h);

		bool parse_size(std::string_view text);

		bool parse_chat(uint32_t color, std::string_view font);

		bool parse_settings(obs_data_t* data);

		void update(obs_data_t* data) override;

		void save(obs_data_t* data) override;

		std::uint32_t width() override;

		std::uint32_t height() override;

		void video_tick(float_t seconds) override;

		void video_render(gs_effect_t* effect) override;

		void enum_active_sources(obs_source_enum_proc_t enum_callback, void* param) override;
	};
} // namespace own3d::source
