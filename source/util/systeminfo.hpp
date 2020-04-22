#pragma once
#include <string>
#include <vector>

namespace own3d {
	namespace util {
		namespace systeminfo {
			const std::string           os_name();
			const std::vector<uint64_t> os_version();

			const std::string username();

			const std::string cpu_manufacturer();
			const std::string cpu_name();

			const uint64_t total_physical_memory();
			const uint64_t available_physical_memory();
			const uint64_t total_virtual_memory();
			const uint64_t available_virtual_memory();
		} // namespace systeminfo
	}     // namespace util
} // namespace own3d
