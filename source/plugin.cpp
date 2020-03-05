#include "plugin.hpp"
#include <stdexcept>
#include "json-source.hpp"

MODULE_EXPORT bool obs_module_load(void)
try {
	own3d::json_source_factory::register_in_obs();
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
} catch (...) {
	DLOG_ERROR("Failed to unload plugin.");
}

#ifdef _WIN32 // Windows Only
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID)
{
	return TRUE;
}
#endif
