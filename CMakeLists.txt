# Integration of the OWN3D service into OBS Studio
# Copyright (C) 2020 own3d media GmbH <support@own3d.tv>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# CMake Setup
cmake_minimum_required(VERSION 3.12.0)

################################################################################
# Configure Type
################################################################################

# Detect if we are building by ourselves or as part of something else.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
	set(GROUPED OFF)
	set(PREFIX "")
else()
	set(GROUPED ON)
	set(PREFIX "OWN3D_")
endif()
set(LOGPREFIX "OWN3D:")

################################################################################
# Versioning
################################################################################

set(VERSION_MAJOR 1)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_TWEAK 0)
set(VERSION_SUFFIX "")
set(VERSION_COMMIT "00000000")

# Check if we are in a git repository.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git")
	# Try and figure out where git is.
	find_program(GIT git
		PATHS
			/bin
			/sbin
			/usr/bin
			/usr/local/bin
	)

	if(GIT)
		set(GIT_RESULT)
		set(GIT_OUTPUT)
		set(GIT_ERROR)

		execute_process(
			COMMAND "${GIT}" describe --tags --long --match "[0-9]*.[0-9]*.[0-9]*" --abbrev=8 HEAD
			WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
			RESULT_VARIABLE GIT_RESULT
			OUTPUT_VARIABLE GIT_OUTPUT
			ERROR_VARIABLE GIT_ERROR
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)

		if(GIT_RESULT EQUAL 0)
			string(REPLACE "-" "." GIT_OUTPUT "${GIT_OUTPUT}")
			string(REPLACE "." ";" GIT_OUTPUT "${GIT_OUTPUT}")

			# Parse Version
			list(GET GIT_OUTPUT 0 VERSION_MAJOR)
			list(GET GIT_OUTPUT 1 VERSION_MINOR)
			list(GET GIT_OUTPUT 2 VERSION_PATCH)
			list(GET GIT_OUTPUT 3 VERSION_BUILD)
			list(GET GIT_OUTPUT 4 VERSION_COMMIT)

			# Patch needs additional parsing.
			# This may be a [0-9]*[a-z]*[0-9]+ string.
			string(REGEX MATCHALL "^([0-9]+)([a-z]+[0-9]+)?" T_MATCHES "${VERSION_PATCH}")
			set(VERSION_PATCH "${CMAKE_MATCH_1}")
			if(CMAKE_MATCH_2)
				set(VERSION_SUFFIX "${CMAKE_MATCH_2}")
			endif()
		else()
			message(WARNING "${LOGPREFIX} Failed to detect version, using default instead.")
		endif()
	endif()
endif()

# Allow manual overrides of the detected version.
set(${PREFIX}VERSION "" CACHE STRING "Override OWN3D.Pro version with this string. Format: Major.Minor.Patch[Suffix][-Tweak[-Commit8c]]")
if(${PREFIX}VERSION)
	string(REPLACE "-" "." T_VERSION "${${PREFIX}VERSION}")
	string(REPLACE "." ";" T_VERSION "${${PREFIX}VERSION}")

	list(LENGTH T_VERSION T_VERSIONLEN)
	list(GET T_VERSION 0 VERSION_MAJOR)
	list(GET T_VERSION 1 VERSION_MINOR)
	list(GET T_VERSION 2 VERSION_PATCH)
	if (T_VERSIONLEN GREATER_EQUAL 3)
		list(GET T_VERSION 3 VERSION_BUILD)
	else()
		set(VERSION_BUILD 0)
	endif()
	if (T_VERSIONLEN GREATER_EQUAL 4)
		list(GET T_VERSION 4 VERSION_COMMIT)
	else()
#		set(VERSION_COMMIT "00000000")
	endif()

	# Patch needs additional parsing.
	# This may be a [0-9]*[a-z]*[0-9]+ string.
	string(REGEX MATCHALL "([0-9]+)([a-z0-9]+)" T_MATCHES "${VERSION_PATCH}")
	set(VERSION_PATCH "${CMAKE_MATCH_1}")
	if(CMAKE_MATCH_2)
		set(VERSION_SUFFIX "${CMAKE_MATCH_2}")
	else()
		set(VERSION_SUFFIX "")
	endif()
endif()

# Generate Version String
if(VERSION_COMMIT)
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}-${VERSION_COMMIT}")
else()
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}")
endif()

# Log the detected version.
message(STATUS "${LOGPREFIX} Version ${VERSION_STRING}")

################################################################################
# Project
################################################################################
project(
	own3d
	VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}
	HOMEPAGE_URL "https://own3d.pro/"
)

# Full Project Name
set(PROJECT_FULL_NAME "OWN3D for OBS Studio")

# Description
set(PROJECT_DESCRIPTION "")

# Authors (TODO: Generate this from AUTHORS)
set(PROJECT_AUTHORS "")

# Copyright Years (TODO: Update every year)
set(PROJECT_COPYRIGHT_YEARS "2020")

# Versioning
set(PROJECT_VERSION_STRING ${VERSION_STRING})

################################################################################
# Modules
################################################################################

# Search Path
set(CMAKE_MODULE_PATH 
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/clang" 
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" 
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

# Include
include("util")

################################################################################
# Platform Setup
################################################################################

# Operating System
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(D_PLATFORM_OS "windows")
	set(D_PLATFORM_WINDOWS 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(D_PLATFORM_OS "linux")
	set(D_PLATFORM_LINUX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(D_PLATFORM_OS "macos")
	set(D_PLATFORM_MAC 1)
else()
	set(D_PLATFORM_OS "unknown")
	set(D_PLATFORM_UNKNOWN 1)
	message(WARNING "${LOGPREFIX} The operating system '${CMAKE_SYSTEM_NAME}' is unknown to to this script, continue at your own risk.")
endif()

# Pointer Size (bits)
math(EXPR D_PLATFORM_BITS_PTR "8*${CMAKE_SIZEOF_VOID_P}")

set(ARCH_INSTR_32 "i386;i686;x86;arm;ARM")
set(ARCH_INSTR_64 "x86_64;AMD64;IA64;arm64;ARM64")
set(ARCH_INSTR_X86 "i386;i686;x86;x86_64;AMD64")
set(ARCH_INSTR_ARM "arm;ARM;arm64;ARM64")
set(ARCH_INSTR_ITANIUM "IA64")

# Bitness
list(FIND ARCH_INSTR_32 "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	set(D_PLATFORM_BITS 32)
endif()

list(FIND ARCH_INSTR_64 "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	set(D_PLATFORM_BITS 64)
endif()

# Basic Instruction Set
list(FIND ARCH_INSTR_X86 "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	set(D_PLATFORM_ARCH "x86")
	set(D_PLATFORM_ARCH_X86 1)
endif()

list(FIND ARCH_INSTR_ARM "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	set(D_PLATFORM_ARCH "ARM")
	set(D_PLATFORM_ARCH_ARM 1)
endif()

list(FIND ARCH_INSTR_ITANIUM "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	set(D_PLATFORM_ARCH "Itanium")
	set(D_PLATFORM_ARCH_ITANIUM 1)
endif()

################################################################################
# Detect if we are building with OBS Studio (different from Grouped builds)
################################################################################

set(STANDALONE ON)
if(GROUPED AND (TARGET libobs))
	set(STANDALONE OFF)
endif()
if(STANDALONE)
	message(STATUS "${LOGPREFIX} This is a standalone build, please make sure you've followed the instructions.")
	include("DownloadProject")
	set(${PREFIX}OBS_NATIVE OFF)
else()
	message(STATUS "${LOGPREFIX} This is a combined build.")
	set(${PREFIX}OBS_NATIVE ON)
	if (NOT TARGET obs-frontend-api)
		message(FATAL_ERROR "${LOGPREFIX} This project does not support non-UI builds.")
		return()
	endif()
endif()

################################################################################
# Options
################################################################################

set(${PREFIX}ENABLE_CLANG OFF CACHE BOOL "Enable Clang integration for supported compilers.")

################################################################################
# Clang
################################################################################

if(${PREFIX}ENABLE_CLANG AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/clang/Clang.cmake"))
	include("Clang")
	set(HAVE_CLANG ON)
endif()

################################################################################
# Standalone Build: OBS Studio
################################################################################

if(STANDALONE)
	# Options
	set(${PREFIX}DOWNLOAD_OBS_URL "" CACHE STRING "(Optional) URL of prebuilt libOBS archive to download.")
	set(${PREFIX}DOWNLOAD_OBS_HASH "" CACHE STRING "(Optional) The hash for the libOBS archive.")
	mark_as_advanced(
		${PREFIX}DOWNLOAD_OBS_URL
		${PREFIX}DOWNLOAD_OBS_HASH
	)

	# Allow overriding what version we build against.
	if(${PREFIX}DOWNLOAD_OBS_URL)
		set(_DOWNLOAD_OBS_URL ${PREFIX}DOWNLOAD_OBS_URL)
		set(_DOWNLOAD_OBS_HASH ${PREFIX}DOWNLOAD_OBS_HASH)
	else()
		set(_DOWNLOAD_OBS_VERSION "26.1.0-ci")
		if (D_PLATFORM_WINDOWS)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBS_URL "https://github.com/Xaymar/obs-studio/releases/download/${_DOWNLOAD_OBS_VERSION}/obs-studio-x64-0.0.0.0-windows-${D_PLATFORM_BITS}.7z")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBS_HASH "SHA256=EA9EE4CCEC0A19605C0768F1F4C21391A919CF0886F7A7CC94D63A95402CFD9B")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		elseif(D_PLATFORM_LINUX)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBS_URL "https://github.com/Xaymar/obs-studio/releases/download/${_DOWNLOAD_OBS_VERSION}/obs-studio-x64-0.0.0.0-ubuntu-${D_PLATFORM_BITS}.7z")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBS_HASH "SHA256=2B2793CDD0C3F5940924665022E31A76C10E34174073BF7B221325C11E764DF5")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		elseif(D_PLATFORM_MAC)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBS_URL "https://github.com/Xaymar/obs-studio/releases/download/${_DOWNLOAD_OBS_VERSION}/obs-studio-x64-0.0.0.0-macos-${D_PLATFORM_BITS}.7z")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBS_HASH "SHA256=34ABD3E574C62DA4E026E5DE271B93AB5151927E564F77264A59456F6ABCBDD0")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		else()
			message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
			return()
		endif()
	endif()
	
	# Download libOBS
	download_project(
		PROJ libobs
		URL "${_DOWNLOAD_OBS_URL}"
		URL_HASH "${_DOWNLOAD_OBS_HASH}"
		DOWNLOAD_NO_PROGRESS OFF
		UPDATE_DISCONNECTED OFF
	)

	include("${libobs_SOURCE_DIR}/cmake/LibObs/LibObsConfig.cmake")
	include("${libobs_SOURCE_DIR}/cmake/obs-frontend-api/obs-frontend-apiConfig.cmake")
endif()

################################################################################
# Standalone Build: OBS Studio Dependencies
################################################################################

if(STANDALONE AND NOT D_PLATFORM_LINUX)
	# Options
	set(${PREFIX}DOWNLOAD_OBSDEPS_URL "" CACHE STRING "(Optional) URL of prebuilt libOBS archive to download.")
	set(${PREFIX}DOWNLOAD_OBSDEPS_HASH "" CACHE STRING "(Optional) The hash for the libOBS archive.")
	mark_as_advanced(
		${PREFIX}DOWNLOAD_OBSDEPS_URL
		${PREFIX}DOWNLOAD_OBSDEPS_HASH
	)
	
	# Allow overriding what version we build against.
	if(${PREFIX}DOWNLOAD_OBSDEPS_URL)
		set(_DOWNLOAD_OBSDEPS_URL ${PREFIX}DOWNLOAD_OBSDEPS_URL)
		set(_DOWNLOAD_OBSDEPS_HASH ${PREFIX}DOWNLOAD_OBSDEPS_HASH)
	else()
		if (D_PLATFORM_WINDOWS)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBSDEPS_URL "https://github.com/Xaymar/obs-studio/releases/download/26.1.0/dependencies_windows.7z")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBSDEPS_HASH "SHA256=F5F8F5A53FDB26FF8F4D24811CA0FEA891B3A0C8477515B886129669A81D37C2")
				elseif (D_PLATFORM_BITS EQUAL 32)
					set(_DOWNLOAD_OBSDEPS_HASH "SHA256=F5F8F5A53FDB26FF8F4D24811CA0FEA891B3A0C8477515B886129669A81D37C2")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		elseif(D_PLATFORM_MAC)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBSDEPS_URL "https://github.com/obsproject/obs-deps/releases/download/2020-12-22/macos-deps-2020-12-22.tar.gz")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBSDEPS_HASH "SHA256=B316DD7802E0DEAE2BD43F52D4C8BD52EF893D203B90148C2AB851A00AA733FF")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		else()
			message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
			return()
		endif()
	endif()
	
	# Download libOBS
	download_project(
		PROJ obsdeps
		URL "${_DOWNLOAD_OBSDEPS_URL}"
		URL_HASH "${_DOWNLOAD_OBSDEPS_HASH}"
		DOWNLOAD_NO_PROGRESS OFF
		UPDATE_DISCONNECTED OFF
	)

	if (D_PLATFORM_WINDOWS)
		set(_OBSDEPS_PATH "${obsdeps_SOURCE_DIR}/win${D_PLATFORM_BITS}")
	elseif(D_PLATFORM_MAC)
		set(_OBSDEPS_PATH "${obsdeps_SOURCE_DIR}/obsdeps")
	endif()
endif()

################################################################################
# Standalone Build: Qt v5.x
################################################################################

if(STANDALONE AND NOT D_PLATFORM_LINUX)
	set(${PREFIX}DOWNLOAD_QT OFF CACHE BOOL "Download Qt?")

	if(${PREFIX}DOWNLOAD_QT)
		set(${PREFIX}DOWNLOAD_QT_URL "" CACHE STRING "")
		set(${PREFIX}DOWNLOAD_QT_HASH "" CACHE STRING "")
		mark_as_advanced(
			${PREFIX}DOWNLOAD_QT_URL
			${PREFIX}DOWNLOAD_QT_HASH
		)
	
		# Allow overriding what version we build against.
		if(${PREFIX}DOWNLOAD_QT_URL)
			set(_DOWNLOAD_QT_URL ${PREFIX}DOWNLOAD_QT_URL)
			set(_DOWNLOAD_QT_HASH ${PREFIX}DOWNLOAD_QT_HASH)
		else()
			if (D_PLATFORM_WINDOWS)
				if (D_PLATFORM_ARCH_X86)
					if (D_PLATFORM_BITS EQUAL 64)
						set(_DOWNLOAD_QT_URL "https://github.com/Xaymar/obs-studio/releases/download/26.1.0/qt_5.15.2_windows_64.7z")
						set(_DOWNLOAD_QT_HASH "SHA256=109B9C21EF165B0C46DFAA9AD23124F2070ED4D74207C4AFB308183CB8D43BDD")
					elseif (D_PLATFORM_BITS EQUAL 32)
						set(_DOWNLOAD_QT_URL "https://github.com/Xaymar/obs-studio/releases/download/26.1.0/qt_5.15.2_windows_32.7z")
						set(_DOWNLOAD_QT_HASH "SHA256=372E4FBF2A15DD4FDA955A07334D8B8AC6802990148C9CB4E766C90205F8F570")
					else()
						message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
						return()
					endif()
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			elseif(D_PLATFORM_MAC)
				if (D_PLATFORM_ARCH_X86)
					set(_DOWNLOAD_QT_URL "https://github.com/obsproject/obs-deps/releases/download/2020-12-22/macos-qt-5.15.2-2020-12-22.tar.gz")
					if (D_PLATFORM_BITS EQUAL 64)
						set(_DOWNLOAD_QT_HASH "SHA256=5BA0BC722F54391FE27508D6DB57EC63BDA22DAD890F5960145C731C1BB837ED")
					else()
						message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
						return()
					endif()
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		endif()
		
		download_project(
			PROJ qt
			URL "${_DOWNLOAD_QT_URL}"
			URL_HASH "${_DOWNLOAD_QT_HASH}"
			DOWNLOAD_NO_PROGRESS OFF
			UPDATE_DISCONNECTED OFF
		)
		
		set(Qt5_DIR "${qt_SOURCE_DIR}" CACHE STRING "Path to Qt5")
		if (D_PLATFORM_WINDOWS)
			if (D_PLATFORM_ARCH_X86)
				if (D_PLATFORM_BITS EQUAL 64)
					CacheSet(Qt5_DIR "${qt_SOURCE_DIR}/lib/cmake/Qt5/")
				elseif (D_PLATFORM_BITS EQUAL 32)
					CacheSet(Qt5_DIR "${qt_SOURCE_DIR}/lib/cmake/Qt5/")
				endif()
			endif()
		elseif(D_PLATFORM_MAC)
			CacheSet(Qt5_DIR "${qt_SOURCE_DIR}/lib/cmake/Qt5/")
		endif()
	endif()
endif()

################################################################################
# CMake / Compiler Dependencies
################################################################################

# Configure Version Header
configure_file(
	"cmake/version.hpp.in"
	"${PROJECT_BINARY_DIR}/source/version.hpp"
)
configure_file(
	"cmake/module.cpp.in"
	"${PROJECT_BINARY_DIR}/source/module.cpp"
)

# Packaging
if("${CMAKE_PACKAGE_SUFFIX_OVERRIDE}" STREQUAL "")
	set(_PACKAGE_SUFFIX_OVERRIDE "${VERSION_STRING}")
else()
	set(_PACKAGE_SUFFIX_OVERRIDE "${CMAKE_PACKAGE_SUFFIX_OVERRIDE}")
endif()
set(_PACKAGE_FULL_NAME "${CMAKE_PACKAGE_PREFIX}/${CMAKE_PACKAGE_NAME}-${_PACKAGE_SUFFIX_OVERRIDE}")

# Windows
if(WIN32)
	## Installer (InnoSetup)
	get_filename_component(ISS_FILES_DIR "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_FILES_DIR}" ISS_FILES_DIR)
	get_filename_component(ISS_PACKAGE_DIR "${CMAKE_PACKAGE_PREFIX}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_PACKAGE_DIR}" ISS_PACKAGE_DIR)
	get_filename_component(ISS_SOURCE_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_SOURCE_DIR}" ISS_SOURCE_DIR)
	configure_file(
		"cmake/installer.iss.in"
		"${PROJECT_BINARY_DIR}/installer.iss"
	)

	# Windows Specific Resource Definition
	set(PROJECT_PRODUCT_NAME "${PROJECT_FULL_NAME}")
	set(PROJECT_COMPANY_NAME "${PROJECT_AUTHORS}")
	set(PROJECT_COPYRIGHT "${PROJECT_AUTHORS} © ${PROJECT_COPYRIGHT_YEARS}")
	set(PROJECT_LEGAL_TRADEMARKS_1 "")
	set(PROJECT_LEGAL_TRADEMARKS_2 "")
	
	configure_file(
		"cmake/version.rc.in"
		"${PROJECT_BINARY_DIR}/cmake/version.rc"
		@ONLY
	)	
endif()

# QT5
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

# CURL
find_package(CURL QUIET)
if(NOT CURL_FOUND)
	if(STANDALONE)
		set(CURL_LIBRARIES "${_OBSDEPS_PATH}/bin/libcurl.lib")
		set(CURL_INCLUDE_DIRS "${_OBSDEPS_PATH}/include")
	else()
		# Already defined by OBS
		set(CURL_LIBRARIES "${CURL_LIB}")
		set(CURL_INCLUDE_DIRS "${CURL_INCLUDE_DIR}")
	endif()
	CacheSet(CURL_LIBRARY_DEBUG ${CURL_LIBRARIES})
	CacheSet(CURL_LIBRARY_RELEASE ${CURL_LIBRARIES})
	CacheSet(CURL_INCLUDE_DIR ${CURL_INCLUDE_DIRS})
	set(CURL_FOUND ON)
endif()

# zlib
set(ZLIB_LIBRARY "" CACHE FILEPATH "Path to zlib/zlib.lib")
set(ZLIB_INCLUDE "" CACHE PATH "Path to zlib/include")

# libzip
set(LIBZIP_LIBRARY "" CACHE FILEPATH "Path to libzip library.")
set(LIBZIP_INCLUDE "" CACHE PATH "Path zo libzip's includes.")

################################################################################
# Code
################################################################################
set(PROJECT_DATA
	"${PROJECT_SOURCE_DIR}/data/locale/en-US.ini"
)
set(PROJECT_TEMPLATES
	"cmake/version.hpp.in"
	"cmake/module.cpp.in"
)
set(PROJECT_PRIVATE_GENERATED
	"${PROJECT_BINARY_DIR}/source/module.cpp"
	"${PROJECT_BINARY_DIR}/source/version.hpp"
)
set(PROJECT_PRIVATE_SOURCE
	"source/plugin.hpp"
	"source/plugin.cpp"
	"source/source-alerts.hpp"
	"source/source-alerts.cpp"
	"source/source-chat.hpp"
	"source/source-chat.cpp"
	"source/source-labels.hpp"
	"source/source-labels.cpp"
	"source/obs/obs-browser.hpp"
	"source/obs/obs-browser.cpp"
	"source/obs/obs-source-factory.hpp"
	"source/ui/ui.hpp"
	"source/ui/ui.cpp"
	"source/ui/ui-browser.hpp"
	"source/ui/ui-browser.cpp"
	"source/ui/ui-dock-eventlist.hpp"
	"source/ui/ui-dock-eventlist.cpp"
	"source/ui/ui-download.hpp"
	"source/ui/ui-download.cpp"
	"source/util/utility.hpp"
	"source/util/utility.cpp"
	"source/util/curl.hpp"
	"source/util/curl.cpp"
	"source/util/systeminfo.hpp"
	"source/util/systeminfo.cpp"
	"source/util/zip.hpp"
	"source/util/zip.cpp"
)
set(PROJECT_UI
	"ui/own3d.qrc"
	"ui/theme-download.ui"
)
set(PROJECT_PRIVATE
	${PROJECT_DATA}
	${PROJECT_TEMPLATES}
	${PROJECT_PRIVATE_GENERATED}
	${PROJECT_PRIVATE_SOURCE}
	${PROJECT_UI}
)

set(PROJECT_LIBRARIES
	${CURL_LIBRARIES}
	${LIBZIP_LIBRARY}
	${ZLIB_LIBRARY}
)
set(PROJECT_LIBRARIES_DELAYED )
set(PROJECT_INCLUDE_DIRS
	${CURL_INCLUDE_DIRS}
	${libobs_INCLUDE_DIRS}
	${LIBZIP_INCLUDE}
	${ZLIB_INCLUDE}
)
set(PROJECT_DEFINES )

if(D_PLATFORM_WINDOWS)
	list(APPEND PROJECT_LIBRARIES
		 Advapi32
	)
	list(APPEND PROJECT_DEFINEDS
		_CRT_SECURE_NO_WARNINGS
		_ENABLE_EXTENDED_ALIGNED_STORAGE
		# windows.h
		WIN32_LEAN_AND_MEAN
		NOGPICAPMASKS
		NOVIRTUALKEYCODES
		#NOWINMESSAGES
		NOWINSTYLES
		NOSYSMETRICS
		NOMENUS
		NOICONS
		NOKEYSTATES
		NOSYSCOMMANDS
		NORASTEROPS
		NOSHOWWINDOW
		NOATOM
		NOCLIPBOARD
		NOCOLOR
		#NOCTLMGR
		NODRAWTEXT
		#NOGDI
		NOKERNEL
		#NOUSER
		#NONLS
		NOMB
		NOMEMMGR
		NOMETAFILE
		NOMINMAX
		#NOMSG
		NOOPENFILE
		NOSCROLL
		NOSERVICE
		NOSOUND
		#NOTEXTMETRIC
		NOWH
		NOWINOFFSETS
		NOCOMM
		NOKANJI
		#NOHELP
		NOPROFILER
		NODEFERWINDOWPOS
		NOMCX
		NOIME
		NOMDI
		NOINOUT
	)
endif()

source_group(TREE "${PROJECT_SOURCE_DIR}/data" PREFIX "Data Files" FILES ${PROJECT_DATA})
source_group(TREE "${PROJECT_BINARY_DIR}/source" PREFIX "Generated Files" FILES ${PROJECT_PRIVATE_GENERATED})
source_group(TREE "${PROJECT_SOURCE_DIR}/cmake" PREFIX "Template Files" FILES ${PROJECT_TEMPLATES})
source_group(TREE "${PROJECT_SOURCE_DIR}/source" PREFIX "Source Files" FILES ${PROJECT_PRIVATE_SOURCE})

################################################################################
# Target
################################################################################

add_library(${PROJECT_NAME} MODULE
	${PROJECT_PUBLIC}
	${PROJECT_PRIVATE}
)

set_target_properties(${PROJECT_NAME}
	PROPERTIES
		PREFIX ""
		IMPORT_PREFIX ""
)

# Include Directories
target_include_directories(${PROJECT_NAME}
	PUBLIC
	PRIVATE
		"${PROJECT_BINARY_DIR}/source"
		"${PROJECT_SOURCE_DIR}/source"
		${PROJECT_INCLUDE_DIRS}
)

# Link Libraries
target_link_libraries(${PROJECT_NAME}
	${PROJECT_LIBRARIES}
)
if(D_PLATFORM_WINDOWS)
	target_link_libraries(${PROJECT_NAME}
		Delayimp.lib
	)
	foreach(DELAYLOAD ${PROJECT_LIBRARIES_DELAYED})
		get_target_property(_lf ${PROJECT_NAME} LINK_FLAGS)
		if (NOT _lf)
			set(_lf "")
		endif()
		set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${_lf} /DELAYLOAD:${DELAYLOAD}")
	endforeach()
endif()

# Definitions
target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		${PROJECT_DEFINITIONS}
)

# C++ Standard and Extensions
set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

# File Version
if(NOT D_PLATFORM_MAC)
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
		VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
		SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
	)
endif()

# UI
target_link_libraries(${PROJECT_NAME}
	obs-frontend-api
	Qt5::Core
	Qt5::Widgets
)
set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	AUTOUIC TRUE
	AUTOUIC_SEARCH_PATHS "${PROJECT_SOURCE_DIR};${PROJECT_SOURCE_DIR}/ui"
	AUTOMOC TRUE
	AUTORCC TRUE
	AUTOGEN_BUILD_DIR "${PROJECT_BINARY_DIR}/source"
	AUTOGEN_SOURCE_GROUP "Qt Files"
)

################################################################################
# C/C++ Compiler Adjustments
################################################################################
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
	message(STATUS "${LOGPREFIX} Applying custom flags for MSVC style build.")

	# MSVC/ClangCL
	# - Dynamically link Microsoft C/C++ Redistributable.
	# - Enable /W3 and disable useless warnings.
	# - Enable C++ exceptions with SEH exceptions.
	# - Enable multi-processor compiling.

	# Build with dynamic MSVC linkage.
    target_compile_options(${PROJECT_NAME} PRIVATE 
        $<$<CONFIG:>:/MD>
        $<$<CONFIG:Debug>:/MDd>
        $<$<CONFIG:Release>:/MD>
        $<$<CONFIG:RelWithDebInfo>:/MD>
        $<$<CONFIG:MinSizeRel>:/MD>
    )

	# Enable most useful warnings.
	set(DISABLED_WARNINGS 
		"/wd4061" "/wd4100" "/wd4180" "/wd4201" "/wd4464" "/wd4505" "/wd4514"
		"/wd4571" "/wd4623" "/wd4625" "/wd4626" "/wd4668" "/wd4710" "/wd4774"
		"/wd4820" "/wd5026" "/wd5027" "/wd5039" "/wd5045" "/wd26812"
	)
    target_compile_options(${PROJECT_NAME} PRIVATE "/W3")
	foreach(WARN ${DISABLED_WARNINGS})
		target_compile_options(${PROJECT_NAME} PRIVATE "${WARN}")
	endforeach()

	# C++ Exceptions & SEH
	target_compile_options(${PROJECT_NAME} PRIVATE "/EHa")

	# Multiprocessor compiling
	target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	message(STATUS "${LOGPREFIX} Applying custom flags for GNU style build.")
	
	# Clang/AppleClang/GNU
	# - Don't export by default. (Temporarily disabled)
	# - Enable all and extra warnings.
	
	target_compile_options(${PROJECT_NAME} PRIVATE 
		"-Wall"
		"-Wextra"
	)
	# add_compile_options("-fvisibility=hidden")
endif()

################################################################################
# Extra Tools
################################################################################

# Clang
if(${PREFIX}ENABLE_CLANG AND HAVE_CLANG)
	generate_compile_commands_json(
		TARGETS ${PROJECT_NAME}
	)
	clang_tidy(
		TARGETS ${PROJECT_NAME}
		VERSION 9.0.0
	)
	clang_format(
		TARGETS ${PROJECT_NAME}
		DEPENDENCY
		VERSION 9.0.0
	)
endif()

# Apple otool
if(D_PLATFORM_MAC)
	# OBS
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND install_name_tool ARGS -change "@rpath/libobs.0.dylib" "@executable_path/../Frameworks/libobs.0.dylib" $<TARGET_FILE:${PROJECT_NAME}>
	)
	message(STATUS "${LOGPREFIX} Added post-build step for adjusting libobs linking path.")

	# OBS Front-End API
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND install_name_tool ARGS -change "@rpath/libobs-frontend-api.dylib" "@executable_path/../Frameworks/libobs-frontend-api.dylib" $<TARGET_FILE:${PROJECT_NAME}>
	)
	message(STATUS "${LOGPREFIX} Added post-build step for adjusting libobs-frontend-api linking path.")

	# Qt5
	string(LENGTH "${Qt5_DIR}" T_PATH_LEN)
	math(EXPR T_PATH_LEN "${T_PATH_LEN} - 10")
	string(SUBSTRING "${Qt5_DIR}" 0 ${T_PATH_LEN} T_PATH)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	# - QtCore
		COMMAND install_name_tool ARGS -change "${T_PATH}/QtCore.framework/Versions/5/QtCore" "@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore" $<TARGET_FILE:${PROJECT_NAME}>
	# - QtGui
		COMMAND install_name_tool ARGS -change "${T_PATH}/QtGui.framework/Versions/5/QtGui" "@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui" $<TARGET_FILE:${PROJECT_NAME}>
	# - QtWidgets
		COMMAND install_name_tool ARGS -change "${T_PATH}/QtWidgets.framework/Versions/5/QtWidgets" "@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets" $<TARGET_FILE:${PROJECT_NAME}>
	)
	message(STATUS "${LOGPREFIX} Added post-build step for adjusting Qt5 linking path (Found: ${Qt5_DIR} resolved to ${T_PATH}).")
endif()

################################################################################
# Installation
################################################################################

if(${PREFIX}OBS_NATIVE)
	install_obs_plugin_with_data(${PROJECT_NAME} data)
else()
	if(D_PLATFORM_WINDOWS)
		install(
			TARGETS ${PROJECT_NAME}
			RUNTIME DESTINATION "obs-plugins/${D_PLATFORM_BITS}bit/" COMPONENT OWN3D
			LIBRARY DESTINATION "obs-plugins/${D_PLATFORM_BITS}bit/" COMPONENT OWN3D
		)
		install(
			DIRECTORY "data/"
			DESTINATION "data/obs-plugins/${PROJECT_NAME}/"
		)
		if(MSVC)
			install(
				FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
				DESTINATION "obs-plugins/${D_PLATFORM_BITS}bit/"
				OPTIONAL
			)
		endif()
		
		add_custom_target(
			PACKAGE_7Z
			${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.7z" --format=7zip --
				"${CMAKE_INSTALL_PREFIX}/obs-plugins"
				"${CMAKE_INSTALL_PREFIX}/data"
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
		)
		add_custom_target(
			PACKAGE_ZIP
			${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.zip" --format=zip --
				"${CMAKE_INSTALL_PREFIX}/obs-plugins"
				"${CMAKE_INSTALL_PREFIX}/data"
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
		)
	elseif(D_PLATFORM_MAC)
		install(
			TARGETS ${PROJECT_NAME}
			RUNTIME DESTINATION "${PROJECT_NAME}/bin/" COMPONENT OWN3D
			LIBRARY DESTINATION "${PROJECT_NAME}/bin/" COMPONENT OWN3D
			PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
		)
		install(
			DIRECTORY "data/"
			DESTINATION "${PROJECT_NAME}/data/"
			COMPONENT OWN3D
			FILE_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			DIRECTORY_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
		)

		add_custom_target(
			PACKAGE_7Z
			${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.7z" --format=7zip --
				"${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}"
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
		)
		add_custom_target(
			PACKAGE_ZIP
			${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.zip" --format=zip --
				"${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}"
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
		)
	elseif(D_PLATFORM_LINUX)
		if(STRUCTURE_PACKAGEMANAGER)
			install(
				TARGETS ${PROJECT_NAME}
				RUNTIME DESTINATION "lib/obs-plugins/" COMPONENT OWN3D
				LIBRARY DESTINATION "lib/obs-plugins/" COMPONENT OWN3D
				PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
			install(
				DIRECTORY "data/"
				DESTINATION "share/obs/obs-plugins/${PROJECT_NAME}"
				COMPONENT OWN3D
				FILE_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
				DIRECTORY_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
		else()
			install(
				TARGETS ${PROJECT_NAME}
				RUNTIME DESTINATION "plugins/${PROJECT_NAME}/bin/${D_PLATFORM_BITS}bit/" COMPONENT OWN3D
				LIBRARY DESTINATION "plugins/${PROJECT_NAME}/bin/${D_PLATFORM_BITS}bit/" COMPONENT OWN3D
				PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
			install(
				DIRECTORY "data/"
				DESTINATION "plugins/${PROJECT_NAME}/data/"
				COMPONENT OWN3D
				FILE_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
				DIRECTORY_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
		
			add_custom_target(
				PACKAGE_7Z
				${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.7z" --format=7zip --
					"${CMAKE_INSTALL_PREFIX}/plugins"
				WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
			)
			add_custom_target(
				PACKAGE_ZIP
				${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.zip" --format=zip --
					"${CMAKE_INSTALL_PREFIX}/plugins"
				WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
			)
		endif()
	endif()
endif()
