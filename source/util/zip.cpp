#include "zip.hpp"
#include <fstream>
#include "plugin.hpp"

own3d::util::zip::~zip()
{
	zip_close(_archive);
}

own3d::util::zip::zip(std::filesystem::path path, std::filesystem::path output_path)
	: _file_path(path), _out_path(output_path)
{
	int32_t error = 0;
	_archive      = zip_open(_file_path.string().c_str(), ZIP_CHECKCONS | ZIP_RDONLY, &error);
	if (error != 0) {
		DLOG_ERROR("Unzipping file '%s' failed with error code %ld.", _file_path.string().c_str(), error);
		throw std::runtime_error("Failed to read zip file.");
	}
}

uint64_t own3d::util::zip::get_file_count()
{
	return zip_get_num_entries(_archive, ZIP_FL_UNCHANGED);
}

void own3d::util::zip::extract_file(uint64_t idx, std::function<void(uint64_t, uint64_t)> callback)
{
	std::shared_ptr<zip_file_t> file = std::shared_ptr<zip_file_t>(zip_fopen_index(_archive, idx, ZIP_FL_UNCHANGED),
																   [](zip_file_t* v) { zip_fclose(v); });
	if (!file) {
		DLOG_ERROR("Failed to extract file index %lld from archive '%s'.", idx, _file_path.string().c_str());
		throw std::runtime_error("Failed to extract file from archive.");
	}

	// Retrieve file info.
	struct zip_stat stat;
	zip_stat_index(_archive, idx, ZIP_FL_UNCHANGED, &stat);

	// Update callback
	callback(stat.size, 0);

	// Build output file path.
	std::filesystem::path filepath = _out_path;
	filepath.append(stat.name);
	if (filepath.has_parent_path()) {
		std::filesystem::create_directories(filepath.parent_path());
	}

	// Write file
	if (stat.size > 0) {
		std::ofstream stream{filepath, std::ios::binary | std::ios::trunc | std::ios::out};
		if (stream.bad()) {
			DLOG_ERROR("Failed to extract file '%s' from archive '%s'.", stat.name, _file_path.string().c_str());
			throw std::runtime_error("Failed to extract file.");
		}
		std::vector<char> buffer;
		buffer.resize(16384);
		for (size_t n = 0; n < stat.size; n += buffer.size()) {
			callback(stat.size, n);
			uint64_t bytes = zip_fread(file.get(), buffer.data(), buffer.size());
			stream.write(buffer.data(), bytes);
		}
		stream.close();
	}
}
