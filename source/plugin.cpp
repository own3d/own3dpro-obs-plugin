#include "plugin.hpp"
#include <stdexcept>
#include <thread>
#include "json/json.hpp"
#include "source-alerts.hpp"
#include "source-labels.hpp"
#include "ui/ui.hpp"
#include "util/curl.hpp"
#include "util/systeminfo.hpp"

constexpr std::string_view CFG_UNIQUE_ID = "UniqueId";

MODULE_EXPORT bool obs_module_load(void)
try {
	// Initialize Configuration
	own3d::configuration::initialize();

	// Retrieve unique Machine Id.
	own3d::get_unique_identifier();

	// Initialize UI
	own3d::ui::ui::initialize();

	// Sources
	{
		static auto labels = std::make_shared<own3d::source::label_factory>();
		static auto alerts = std::make_shared<own3d::source::alert_factory>();
	}

	return true;
} catch (const std::exception& ex) {
	DLOG_ERROR("Failed to load plugin due to error: %s", ex.what());
	return false;
} catch (...) {
	DLOG_ERROR("Failed to load plugin.");
	return false;
}

MODULE_EXPORT void obs_module_unload(void)
try {
	// Finalize Configuration
	own3d::configuration::finalize();
} catch (...) {
	DLOG_ERROR("Failed to unload plugin.");
}

#ifdef _WIN32 // Windows Only
#include <Windows.h>

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID)
{
	return TRUE;
}
#endif

own3d::configuration::~configuration()
{
	save();
}

own3d::configuration::configuration()
{
	{
		const char* path = obs_module_config_path("config.json");
		if (path) {
			_data_path = path;
		} else {
			throw std::runtime_error("Plugin has no configuration directory, libobs broke.");
		}
		_data_path_bk = _data_path;
		_data_path_bk.concat(".bk");
	}

	if ((!std::filesystem::is_directory(_data_path.parent_path()))
		&& (!std::filesystem::create_directories(_data_path.parent_path()))) {
		_data = std::shared_ptr<obs_data_t>(obs_data_create(), ::own3d::data_deleter);
	} else {
		if (std::filesystem::exists(_data_path)) {
			_data = std::shared_ptr<obs_data_t>(obs_data_create_from_json_file_safe(_data_path.string().c_str(), ".bk"),
												::own3d::data_deleter);
		} else {
			_data = std::shared_ptr<obs_data_t>(obs_data_create(), ::own3d::data_deleter);
		}
	}
}

std::shared_ptr<obs_data_t> own3d::configuration::get()
{
	obs_data_addref(_data.get());
	return std::shared_ptr<obs_data_t>(_data.get(), ::own3d::data_deleter);
}

void own3d::configuration::save()
{
	obs_data_save_json_safe(_data.get(), _data_path.string().c_str(), ".tmp", ".bk");
}

std::shared_ptr<own3d::configuration> own3d::configuration::_instance = nullptr;

void own3d::configuration::initialize()
{
	if (!own3d::configuration::_instance)
		own3d::configuration::_instance = std::make_shared<own3d::configuration>();
}

void own3d::configuration::finalize()
{
	own3d::configuration::_instance = nullptr;
}

std::shared_ptr<own3d::configuration> own3d::configuration::instance()
{
	return own3d::configuration::_instance;
}

std::string dont_rn0rnrn_me(std::string str)
{
	// CURL gives us a string with "\r\n0\r\n\r\n" at the end, which we don't want.
	// So instead we cut off the string at the first \r we detect from it.

	for (size_t n = 0; n < str.length(); n++) {
		if (str[n] == '\r')
			return str.substr(0, n);
	}
	return str;
}

size_t uuid_write_cb(void* buffer, size_t size, size_t count, std::string* uuid, own3d::util::curl* curl)
{
	int32_t http_code = 0;
	curl->get_info(CURLINFO_RESPONSE_CODE, http_code);
	switch (http_code) {
	case 200:
		*uuid = std::string(reinterpret_cast<const char*>(buffer), reinterpret_cast<const char*>(buffer) + count);
		break;
	default:
		*uuid = "";
		break;
	}
	return size * count;
}

bool own3d::is_sandbox()
{
	constexpr std::string_view KEY = "sandbox";
	auto                       cfg = own3d::configuration::instance()->get();
	return obs_data_get_bool(cfg.get(), KEY.data());
}

std::string own3d::get_api_endpoint(std::string_view const args)
{
	constexpr std::string_view URL_ENDPOINT         = "api/v1/";
	constexpr std::string_view URL_ENDPOINT_SANDBOX = "api/v1/";

	std::vector<char> buffer(65535);
	if (is_sandbox()) {
		buffer.resize(snprintf(buffer.data(), buffer.capacity(), "%s%s", URL_ENDPOINT_SANDBOX.data(), args.data()));
	} else {
		buffer.resize(snprintf(buffer.data(), buffer.capacity(), "%s%s", URL_ENDPOINT.data(), args.data()));
	}
	std::string text = std::string(buffer.data(), buffer.data() + buffer.size());
	return get_web_endpoint(text);
}

std::string own3d::get_web_endpoint(std::string_view const args /*= ""*/)
{
	constexpr std::string_view URL_ENDPOINT         = "https://obs.own3d.tv/";
	constexpr std::string_view URL_ENDPOINT_SANDBOX = "https://sandbox.obs.own3d.tv/";

	std::vector<char> buffer(65535);
	if (is_sandbox()) {
		buffer.resize(snprintf(buffer.data(), buffer.capacity(), "%s%s", URL_ENDPOINT_SANDBOX.data(), args.data()));
	} else {
		buffer.resize(snprintf(buffer.data(), buffer.capacity(), "%s%s", URL_ENDPOINT.data(), args.data()));
	}
	return std::string(buffer.data(), buffer.data() + buffer.size());
}

bool own3d::testing_enabled()
{
	// To protect against prying eyes which we don't want to find this, this is a
	// SHA-256 hash of the word "testing". This will not prevent in-depth reverse
	// engineering.
	constexpr std::string_view KEY = "CF80CD8AED482D5D1527D7DC72FCEFF84E6326592848447D2DC0B0E87DFC9A90";

	auto cfg = own3d::configuration::instance()->get();
	return obs_data_get_bool(cfg.get(), KEY.data());
}

std::string_view own3d::testing_archive_name()
{
	// To protect against prying eyes which we don't want to find this, this is a
	// SHA-256 hash of the word "testing_archive_name". This will not prevent
	// in-depth reverse engineering.
	constexpr std::string_view KEY = "F90FA561985819E266033858A2DFC1F1CE65B1C4A583059B42DA14A9000D7F81";

	auto cfg = own3d::configuration::instance()->get();
	return obs_data_get_string(cfg.get(), KEY.data());
}

std::string_view own3d::testing_archive_path()
{
	// To protect against prying eyes which we don't want to find this, this is a
	// SHA-256 hash of the word "testing_archive_path". This will not prevent
	// in-depth reverse engineering.
	constexpr std::string_view KEY = "1FE54EE4FF368EC47DA55091044C69E3188169069B99434780108E8E626A8510";

	auto cfg = own3d::configuration::instance()->get();
	return obs_data_get_string(cfg.get(), KEY.data());
}

static std::string request_unique_identifier()
{
	auto j_ = nlohmann::json::object();
	{
		auto j_versions = nlohmann::json::object();
		{
			auto j_version    = nlohmann::json::array();
			j_version[0]      = obs_get_version_string();
			j_version[1]      = (obs_get_version() >> 24) & 0xFF;
			j_version[2]      = (obs_get_version() >> 16) & 0xFF;
			j_version[3]      = obs_get_version() & 0xFFFF;
			j_versions["obs"] = j_version;
		}
		{
			auto j_version       = nlohmann::json::array();
			j_version[0]         = OWN3DTV_VERSION_STRING;
			j_version[1]         = OWN3DTV_VERSION_MAJOR;
			j_version[2]         = OWN3DTV_VERSION_MINOR;
			j_version[3]         = OWN3DTV_VERSION_PATCH;
			j_version[4]         = OWN3DTV_COMMIT;
			j_versions["plugin"] = j_version;
		}
		j_["versions"] = j_versions;
	}
	{
		auto j_system = nlohmann::json::object();
		{ // CPU Information
			auto j_cpu      = nlohmann::json::array();
			j_cpu[0]        = own3d::util::systeminfo::cpu_manufacturer();
			j_cpu[1]        = own3d::util::systeminfo::cpu_name();
			j_system["cpu"] = j_cpu;
		}
		{ // Memory Information
			auto j_mem         = nlohmann::json::array();
			j_mem[0]           = own3d::util::systeminfo::total_physical_memory();
			j_mem[1]           = own3d::util::systeminfo::total_virtual_memory();
			j_system["memory"] = j_mem;
		}
		{ // User Information
			j_system["user"] = own3d::util::systeminfo::username();
		}
		{ // OS Information
			auto j_os = nlohmann::json::array();
			j_os[0]   = own3d::util::systeminfo::os_name();
			auto osv  = own3d::util::systeminfo::os_version();
			for (auto v : osv) {
				j_os.push_back(v);
			}
			j_system["os"] = j_os;
		}
		j_["system"] = j_system;
	}

	CURLcode    res = CURLE_AGAIN;
	std::string id;
	for (size_t n = 0; (n < 10) && (res != CURLE_OK); n++) {
		own3d::util::curl rq;
		std::string       args = j_.dump();
		rq.set_option(CURLOPT_USERAGENT, OWN3D_USER_AGENT);
		rq.set_option(CURLOPT_URL, own3d::get_api_endpoint("machine-tokens/issue"));
		rq.set_option(CURLOPT_POSTFIELDS, args.c_str());
		rq.set_option(CURLOPT_POST, true);
		rq.set_header("Content-Type", "application/json");

		rq.set_write_callback(
			std::bind(uuid_write_cb, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, &id, &rq));

		if (res = rq.perform(); res != CURLE_OK) {
			DLOG_WARNING("Attempt %llu at retrieving a unique machine id failed.", n);
		} else {
			if (id.length() != 0) {
				DLOG_WARNING("Attempt %llu at retrieving a unique machine id returned empty id.", n);
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	if (res != CURLE_OK) {
		DLOG_ERROR("Failed to acquire unique machine id, error code %lld. Functionality disabled.", res);
#ifndef _DEBUG
		throw std::runtime_error("Failed to acquire unique machine id.");
#endif
	}

	if (id.size() == 0) {
		DLOG_ERROR("API returned an empty machine id. Functionality disabled.");
#ifndef _DEBUG
		throw std::runtime_error("Failed to acquire unique machine id.");
#endif
	}

	return id;
}

std::string_view own3d::get_unique_identifier()
{
	auto             cfg = own3d::configuration::instance()->get();
	std::string_view id  = obs_data_get_string(cfg.get(), CFG_UNIQUE_ID.data());

	if (id.length() == 0) { // Id is invalid, request a new one.
		std::string idv = request_unique_identifier();
		if (idv.length() > 0) {
			obs_data_set_string(cfg.get(), CFG_UNIQUE_ID.data(), idv.c_str());
			id = obs_data_get_string(cfg.get(), CFG_UNIQUE_ID.data());
			own3d::configuration::instance()->save();
			DLOG_INFO("Acquired unique machine token.");
		} else {
			DLOG_ERROR("Failed to acquire machine token.");
		}
	}

	return id;
}

void own3d::reset_unique_identifier()
{
	obs_data_unset_user_value(own3d::configuration::instance()->get().get(), CFG_UNIQUE_ID.data());
	own3d::configuration::instance()->save();
}
