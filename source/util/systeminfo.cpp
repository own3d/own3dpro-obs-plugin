#include "systeminfo.hpp"
#include <map>
#include <vector>

#ifdef WIN32
#include <Windows.h>
#define SECURITY_WIN32
#include <Security.h>
#endif

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <cpuid.h>
#endif

const std::string own3d::util::systeminfo::os_name()
{
#ifdef WIN32
	return "Windows";
#else
	return "Unix";
#endif
}

const std::vector<uint64_t> own3d::util::systeminfo::os_version()
{
	std::vector<uint64_t> result;

#ifdef WIN32
	OSVERSIONINFOEXW vi;
	vi.dwOSVersionInfoSize = sizeof(vi);
	if (GetVersionExW(reinterpret_cast<LPOSVERSIONINFOW>(&vi))) {
		result.push_back(static_cast<uint64_t>(vi.dwMajorVersion));
		result.push_back(static_cast<uint64_t>(vi.dwMinorVersion));
		result.push_back(static_cast<uint64_t>(vi.dwBuildNumber));
		result.push_back(static_cast<uint64_t>(vi.dwPlatformId));
		result.push_back(static_cast<uint64_t>(vi.wServicePackMajor));
		result.push_back(static_cast<uint64_t>(vi.wServicePackMinor));
		result.push_back(static_cast<uint64_t>(vi.wSuiteMask));
		result.push_back(static_cast<uint64_t>(vi.wProductType));
		result.push_back(static_cast<uint64_t>(vi.wReserved));
	}
#endif
	return result;
}

const std::string own3d::util::systeminfo::username()
{
#ifdef WIN32
	std::vector<WCHAR> buf(256);
	ULONG              asd = static_cast<ULONG>(buf.size());
	if (GetUserNameW(buf.data(), &asd)) {
		std::vector<CHAR> buf2(static_cast<size_t>(asd) * 4u);
		int               len = WideCharToMultiByte(CP_UTF8, 0, buf.data(), static_cast<int>(buf.size()), buf2.data(),
                                      static_cast<int>(buf2.size()), 0, 0);
		return std::string(buf2.data(), buf2.data() + len);
	}
#endif
	return std::string("Unknown");
}

static int cpuid_max()
{
	union {
		int info[4];
	} data = {static_cast<int>(0x80000000), 0, 0, 0};
#ifdef _MSC_VER
	__cpuidex(data.info, data.info[0], 0);
#else
	__get_cpuid(data.info[0], &data.info[0], &data.info[1], &data.info[2], &data.info[4]);
#endif
	return data.info[0];
}

const std::string own3d::util::systeminfo::cpu_manufacturer()
{
	union {
		int info[4];
		struct {
			uint32_t   _eax;
			const char str1[4];
			const char str3[4];
			const char str2[4];
		};
	} data = {0, 0, 0, 0};
#ifdef _MSC_VER
	__cpuidex(data.info, data.info[0], 0);
#else
	__get_cpuid(data.info[0], &data.info[0], &data.info[1], &data.info[2], &data.info[4]);
#endif
	std::vector<char> ids(4 * 4);
	snprintf(ids.data(), ids.size(), "%4.4s%4.4s%4.4s", data.str1, data.str2, data.str3);
	std::string id(ids.data());
	if ((id == "AuthenticAMD") || (id == "AMDisbetter!")) {
		return "AMD";
	} else if (id == "GenuineIntel") {
		return "Intel";
	} else if (id == "bhyve bhyve") {
		return "byhve";
	} else if (id == " KVMKVMKVM ") {
		return "KVM";
	} else if (id == "TCGTCGTCGTCG") {
		return "QEMU";
	} else if (id == "Microsoft Hv") {
		return "Microsoft Hyper-V";
	} else if (id == " lrpepyh vr") {
		return "Parallels";
	} else if (id == "VMwareVMware") {
		return "VMware";
	} else if (id == "XenVMMXenVMM") {
		return "Xen HVM";
	} else {
		return id;
	}
}

const std::string own3d::util::systeminfo::cpu_name()
{
	if (cpuid_max() < 0x80000004) {
		// Technically could use cpuid with eax=1 to get a basic idea, but that's just annoying.
		return std::string("Unknown");
	}

	union res {
		int        info[4] = {0, 0, 0, 0};
		const char str[16];
	};

	res data1 = {static_cast<int>(0x80000002), 0, 0, 0};
	res data2 = {static_cast<int>(0x80000003), 0, 0, 0};
	res data3 = {static_cast<int>(0x80000004), 0, 0, 0};

#ifdef _MSC_VER
	__cpuidex(data1.info, data1.info[0], 0);
	__cpuidex(data2.info, data2.info[0], 0);
	__cpuidex(data3.info, data3.info[0], 0);
#else
	__get_cpuid(data1.info[0], &data1.info[0], &data1.info[1], &data1.info[2], &data1.info[4]);
	__get_cpuid(data2.info[0], &data2.info[0], &data2.info[1], &data2.info[2], &data2.info[4]);
	__get_cpuid(data3.info[0], &data3.info[0], &data3.info[1], &data3.info[2], &data3.info[4]);
#endif

	std::vector<char> name;
	name.resize(49);
	snprintf(name.data(), name.size(), "%.16s%.16s%.16s", data1.str, data2.str, data3.str);

	return std::string(name.data());
}

const uint64_t own3d::util::systeminfo::total_physical_memory()
{
#ifdef WIN32
	MEMORYSTATUSEX mse;
	mse.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&mse);
	return static_cast<uint64_t>(mse.ullTotalPhys);
#else
	return size_t();
#endif
}

const uint64_t own3d::util::systeminfo::available_physical_memory()
{
#ifdef WIN32
	MEMORYSTATUSEX mse;
	mse.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&mse);
	return static_cast<uint64_t>(mse.ullAvailVirtual);
#else
	return size_t();
#endif
}

const uint64_t own3d::util::systeminfo::total_virtual_memory()
{
#ifdef WIN32
	MEMORYSTATUSEX mse;
	mse.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&mse);
	return static_cast<uint64_t>(mse.ullTotalVirtual);
#else
	return size_t();
#endif
}

const uint64_t own3d::util::systeminfo::available_virtual_memory()
{
#ifdef WIN32
	MEMORYSTATUSEX mse;
	mse.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&mse);
	return static_cast<uint64_t>(mse.ullAvailVirtual);
#else
	return size_t();
#endif
}
