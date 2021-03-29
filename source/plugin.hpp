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
#include <cinttypes>
#include <filesystem>
#include <string>
#include <vector>
#include "version.hpp"

#include <obs-module.h>
#include <util/platform.h>
#undef strtoll

#ifdef WIN32
#define OWN3D_USER_AGENT "Own3d.tv OBS Plugin/" OWN3DTV_VERSION_STRING " (Windows)"
#elif APPLE
#define OWN3D_USER_AGENT "Own3d.tv OBS Plugin/" OWN3DTV_VERSION_STRING " (MacOS)"
#else
#define OWN3D_USER_AGENT "Own3d.tv OBS Plugin/" OWN3DTV_VERSION_STRING " (Unix)"
#endif

// Common Global defines
/// Log Functionality
#define DLOG_PREFIX "[Own3d.TV]"
#define DLOG_(level, ...) blog(level, DLOG_PREFIX " " __VA_ARGS__)
#define DLOG_ERROR(...) DLOG_(LOG_ERROR, __VA_ARGS__)
#define DLOG_WARNING(...) DLOG_(LOG_WARNING, __VA_ARGS__)
#define DLOG_INFO(...) DLOG_(LOG_INFO, __VA_ARGS__)
#define DLOG_DEBUG(...) DLOG_(LOG_DEBUG, __VA_ARGS__)
/// Currrent function name (as const char*)
#ifdef _MSC_VER
// Microsoft Visual Studio
#define __FUNCTION_SIG__ __FUNCSIG__
#define __FUNCTION_NAME__ __func__
#elif defined(__GNUC__) || defined(__MINGW32__)
// GCC and MinGW
#define __FUNCTION_SIG__ __PRETTY_FUNCTION__
#define __FUNCTION_NAME__ __func__
#else
// Any other compiler
#define __FUNCTION_SIG__ __func__
#define __FUNCTION_NAME__ __func__
#endif
// Import, Export, Local
#if defined _MSC_VER || defined __CYGWIN__
#define LIB_IMPORT __declspec(dllimport)
#define LIB_EXPORT __declspec(dllexport)
#define LIB_LOCAL
#else
#define LIB_IMPORT __attribute__((visibility("defulat")))
#define LIB_EXPORT __attribute__((visibility("defulat")))
#define LIB_LOCAL __attribute__((visibility("hidden")))
#endif

namespace own3d {
	LIB_LOCAL inline void source_deleter(obs_source_t* v)
	{
		obs_source_release(v);
	}

	LIB_LOCAL inline void sceneitem_deleter(obs_sceneitem_t* v)
	{
		obs_sceneitem_remove(v);
	}

	LIB_LOCAL inline void data_deleter(obs_data_t* v)
	{
		obs_data_release(v);
	}

	LIB_LOCAL inline void data_item_deleter(obs_data_item_t* v)
	{
		obs_data_item_release(&v);
	}

	LIB_LOCAL inline void data_array_deleter(obs_data_array_t* v)
	{
		obs_data_array_release(v);
	}

	class LIB_LOCAL configuration {
		std::shared_ptr<obs_data_t> _data;
		std::filesystem::path       _data_path;
		std::filesystem::path       _data_path_bk;

		public:
		~configuration();

		configuration();

		std::shared_ptr<obs_data_t> get();

		void save();

		// Singleton
		private:
		static std::shared_ptr<own3d::configuration> _instance;

		public:
		static void initialize();
		static void finalize();

		static std::shared_ptr<own3d::configuration> instance();
	};

	LIB_LOCAL bool is_sandbox();

	LIB_LOCAL std::string get_api_endpoint(std::string_view const args = "");

	LIB_LOCAL std::string get_web_endpoint(std::string_view const args = "");

	LIB_LOCAL bool testing_enabled();

	LIB_LOCAL std::string_view testing_archive_name();

	LIB_LOCAL std::string_view testing_archive_path();

	LIB_LOCAL std::string_view get_unique_identifier();

	LIB_LOCAL void reset_unique_identifier();
} // namespace own3d

#define D_TRANSLATE(x) obs_module_text(x)
