#pragma once
#include <cinttypes>
#include <filesystem>
#include <functional>

#include <zip.h>

namespace own3d {
	namespace util {
		class zip {
			zip_t*                _archive;
			std::filesystem::path _file_path;
			std::filesystem::path _out_path;

			public:
			~zip();
			zip(std::filesystem::path path, std::filesystem::path output_path);

			uint64_t get_file_count();

			void extract_file(uint64_t idx, std::function<void(uint64_t, uint64_t)> callback);
		};
	} // namespace util
} // namespace own3d
