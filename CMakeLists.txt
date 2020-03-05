# Experimental new Sources, Filters and Transitions for OBS Studio
# Copyright (C) 2017 - 2018 Michael Fabian Dirks
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# CMake Setup
cmake_minimum_required(VERSION 3.8.0)

# Automatic Versioning
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(VERSION_TWEAK 0)
set(VERSION_SUFFIX "")
set(PROJECT_COMMIT "N/A")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git")
	set(GIT_RESULT "")
	set(GIT_OUTPUT "")
	execute_process(
		COMMAND git rev-list --count --topo-order ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}${VERSION_SUFFIX}..HEAD
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		RESULT_VARIABLE GIT_RESULT
		OUTPUT_VARIABLE GIT_OUTPUT
		OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE ERROR_QUIET
	)
	if(GIT_RESULT EQUAL 0)
		set(VERSION_TWEAK ${GIT_OUTPUT})
	endif()
	execute_process(
		COMMAND git rev-parse --short=8 HEAD
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		RESULT_VARIABLE GIT_RESULT
		OUTPUT_VARIABLE GIT_OUTPUT
		OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE ERROR_QUIET
	)
	if(GIT_RESULT EQUAL 0)
		set(PROJECT_COMMIT ${GIT_OUTPUT})
	endif()
endif()

# Define Project
project(
	own3d-tv
	VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}
)
set(PROJECT_FULL_NAME "Own3d.TV for OBS Studio")
set(PROJECT_DESCRIPTION "")
set(PROJECT_AUTHORS "")
set(PROJECT_COPYRIGHT_YEARS "2020")

################################################################################
# Setup / Bootstrap
################################################################################

# Search Path
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/clang" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# CMake Modules
include("util")
if (EXISTS "${CMAKE_SOURCE_DIR}/cmake/Clang/Clang.cmake")
	include("Clang")
	set(HAVE_CLANG ON)
endif()

# Detect Build Type
if("${CMAKE_SOURCE_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
	set(PropertyPrefix "")
else()
	set(PropertyPrefix "${PROJECT_NAME}_")
endif()

# Detect Architecture
math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
if("${BITS}" STREQUAL "32")
	set(ARCH "x86")
	message(STATUS "StreamFX no longer supports 32-bit builds, you are on your own if you continue.")
else()
	set(ARCH "x64")
endif()

# All Warnings, Extra Warnings, Pedantic
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	# using Clang
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-missing-braces -Wmissing-field-initializers -Wno-c++98-compat-pedantic -Wold-style-cast -Wno-documentation -Wno-documentation-unknown-command -Wno-covered-switch-default -Wno-switch-enum")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-missing-braces -Wmissing-field-initializers -Wno-c++98-compat-pedantic -Wold-style-cast -Wno-documentation -Wno-documentation-unknown-command -Wno-covered-switch-default -Wno-switch-enum")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# GCC: -fpermissive is required as GCC does not allow the same template to be in different namespaces.
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wpedantic -fpermissive -Wno-long-long -Wno-missing-braces -Wmissing-field-initializers")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -fpermissive -Wno-long-long -Wno-missing-braces -Wmissing-field-initializers")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
	# using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(MSVC_EXTRA_FLAGS "/wd4061 /wd4100 /wd4180 /wd4201 /wd4464 /wd4505 /wd4514 /wd4571 /wd4623 /wd4625 /wd4626 /wd4710 /wd4774 /wd4820 /wd5026 /wd5027 /wd5045")
	# Force to always compile with W4
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/Wall" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
	endif()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSVC_EXTRA_FLAGS}")
	
	if(CMAKE_C_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/Wall" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Wall")
	endif()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MSVC_EXTRA_FLAGS}")

	if("${PropertyPrefix}" STREQUAL "")
		# Speed Optimized Configuration
		set(_SPEED_COMPILER_FLAGS "/O2 /Oi /Ot /Oy /GF /GS- /Qpar /arch:AVX /GR /GL")
		set(_SPEED_LINKER_FLAGS "/LTCG:incremental /OPT:ICF=4 /INCREMENTAL /OPT:REF")
		set(CMAKE_CXX_FLAGS_SPEED "${CMAKE_CXX_FLAGS_RELWTIHDEBINFO} ${_SPEED_COMPILER_FLAGS}")
		set(CMAKE_C_FLAGS_SPEED "${CMAKE_C_FLAGS_RELWTIHDEBINFO} ${_SPEED_COMPILER_FLAGS}")
		set(CMAKE_EXE_LINKER_FLAGS_SPEED "${CMAKE_EXE_LINKER_FLAGS_RELWTIHDEBINFO} ${_SPEED_LINKER_FLAGS}")
		set(CMAKE_SHARED_LINKER_FLAGS_SPEED "${CMAKE_EXE_LINKER_FLAGS_RELWTIHDEBINFO} ${_SPEED_LINKER_FLAGS}")
		set(CMAKE_MODULE_LINKER_FLAGS_SPEED "${CMAKE_EXE_LINKER_FLAGS_RELWTIHDEBINFO} ${_SPEED_LINKER_FLAGS}")
		mark_as_advanced(
			CMAKE_CXX_FLAGS_SPEED
			CMAKE_C_FLAGS_SPEED
			CMAKE_EXE_LINKER_FLAGS_SPEED
			CMAKE_SHARED_LINKER_FLAGS_SPEED
			CMAKE_MODULE_LINKER_FLAGS_SPEED
		)

		if(CMAKE_CONFIGURATION_TYPES)
			list(APPEND CMAKE_CONFIGURATION_TYPES Speed)
			list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
			CacheSet(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}")
		endif()
	endif()
endif()

# C++ Standard and Extensions
## Use C++17 and no non-standard extensions.
set(_CXX_STANDARD 17)
set(_CXX_EXTENSIONS OFF)

################################################################################
# Options
################################################################################
set(${PropertyPrefix}OBS_NATIVE FALSE CACHE BOOL "Use native obs-studio build" FORCE)
set(${PropertyPrefix}OBS_REFERENCE FALSE CACHE BOOL "Use referenced obs-studio build" FORCE)
set(${PropertyPrefix}OBS_PACKAGE FALSE CACHE BOOL "Use packaged obs-studio build" FORCE)
set(${PropertyPrefix}OBS_DOWNLOAD FALSE CACHE BOOL "Use downloaded obs-studio build")
mark_as_advanced(FORCE OBS_NATIVE OBS_PACKAGE OBS_REFERENCE OBS_DOWNLOAD)

# Solve OBS_NATIVE, OBS_REFERENCE, OBS_PACKAGE, OBS_DOWNLOAD
if(TARGET libobs)
	message(STATUS "${PROJECT_NAME}: Using native obs-studio.")
	CacheSet(${PropertyPrefix}OBS_NATIVE TRUE)
else()
	message(STATUS "${PROJECT_NAME}: Using packaged or remote obs-studio.")
	CacheSet(${PropertyPrefix}OBS_NATIVE FALSE)

	set(CMAKE_PACKAGE_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Path for generated archives.")
	set(CMAKE_PACKAGE_NAME "${PROJECT_NAME}" CACHE STRING "Name for the generated archives.")
	set(CMAKE_PACKAGE_SUFFIX_OVERRIDE "" CACHE STRING "Override for the suffix.")

	if(${PropertyPrefix}OBS_DOWNLOAD)
		set(${PropertyPrefix}OBS_DOWNLOAD_VERSION "24.0.3" CACHE STRING "OBS Studio Version to download")
		set(${PropertyPrefix}OBS_DEPENDENCIES_VERSION "24.0.0" CACHE STRING "OBS Studio Version to download")
		set(${PropertyPrefix}OBS_DOWNLOAD_URL "https://github.com/Xaymar/obs-studio/releases/download/${OBS_DOWNLOAD_VERSION}-ci/obs-studio-${ARCH}-0.0.0.0-vs2017.7z")
		set(${PropertyPrefix}OBS_DEPENDENCIES_URL "https://cdn.xaymar.com/obs/dependencies_${OBS_DEPENDENCIES_VERSION}.zip")
	else()
		set(${PropertyPrefix}OBS_STUDIO_DIR "" CACHE PATH "OBS Studio Source/Package Directory")
		set(${PropertyPrefix}OBS_DEPENDENCIES_DIR "" CACHE PATH "OBS Studio Dependencies Directory")
		if(EXISTS "${OBS_STUDIO_DIR}/cmake/LibObs/LibObsConfig.cmake")
			message(STATUS "${PROJECT_NAME}: Using packaged obs-studio.")
			CacheSet(${PropertyPrefix}OBS_PACKAGE TRUE)
		elseif(EXISTS "${OBS_STUDIO_DIR}/libobs/obs-module.h")
			message(STATUS "${PROJECT_NAME}: Using referenced obs-studio.")
			CacheSet(${PropertyPrefix}OBS_REFERENCE TRUE)
		else()
			message(FATAL_ERROR "${PROJECT_NAME}: No OBS Studio detected. If you wish to continue, either check ${PropertyPrefix}OBS_DOWNLOAD or fix your configuration")
			return()
		endif()
	endif()
endif()

################################################################################
# CMake / Compiler Dependencies
################################################################################

# Configure Version Header
configure_file(
	"${PROJECT_SOURCE_DIR}/cmake/version.hpp.in"
	"${PROJECT_BINARY_DIR}/source/version.hpp"
)
configure_file(
	"${PROJECT_SOURCE_DIR}/cmake/module.cpp.in"
	"${PROJECT_BINARY_DIR}/source/module.cpp"
)

# Packaging
if("${CMAKE_PACKAGE_SUFFIX_OVERRIDE}" STREQUAL "")
	set(_PACKAGE_SUFFIX_OVERRIDE "${PROJECT_VERSION}${VERSION_SUFFIX}-${PROJECT_COMMIT}")
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
		"${PROJECT_SOURCE_DIR}/cmake/installer.iss.in"
		"${PROJECT_BINARY_DIR}/installer.iss"
	)

	# Windows Specific Resource Definition
	set(PROJECT_PRODUCT_NAME "${PROJECT_FULL_NAME}")
	set(PROJECT_COMPANY_NAME "${PROJECT_AUTHORS}")
	set(PROJECT_COPYRIGHT "${PROJECT_AUTHORS} © ${PROJECT_COPYRIGHT_YEARS}")
	set(PROJECT_LEGAL_TRADEMARKS_1 "")
	set(PROJECT_LEGAL_TRADEMARKS_2 "")
	
	configure_file(
		"${PROJECT_SOURCE_DIR}/cmake/version.rc.in"
		"${PROJECT_BINARY_DIR}/cmake/version.rc"
		@ONLY
	)	
endif()

# Download OBS Studio and OBS Dependencies
if(${PropertyPrefix}OBS_DOWNLOAD)
	include("DownloadProject")
	
	download_project(
		PROJ libobs
		URL ${OBS_DOWNLOAD_URL}
		UPDATE_DISCONNECTED 1
	)
	
	download_project(
		PROJ obsdeps
		URL ${OBS_DEPENDENCIES_URL}
		UPDATE_DISCONNECTED 1
	)
endif()

# Load OBS Studio & Dependencies
if(${PropertyPrefix}OBS_PACKAGE)
	include("${OBS_STUDIO_DIR}/cmake/LibObs/LibObsConfig.cmake")
elseif(${PropertyPrefix}OBS_REFERENCE)
	set(obsPath "${OBS_STUDIO_DIR}")
	include("${OBS_STUDIO_DIR}/cmake/external/FindLibobs.cmake")
elseif(${PropertyPrefix}OBS_DOWNLOAD)
	include("${libobs_SOURCE_DIR}/cmake/LibObs/LibObsConfig.cmake")
endif()

################################################################################
# Code
################################################################################
set(PROJECT_DATA_LOCALE
	"${PROJECT_SOURCE_DIR}/data/locale/en-US.ini"
)
set(PROJECT_DATA_EFFECTS
)
set(PROJECT_DATA_SHADERS
)
set(PROJECT_DATA
	${PROJECT_DATA_LOCALE}
	${PROJECT_DATA_EFFECTS}
	${PROJECT_DATA_SHADERS}
)

set(PROJECT_LIBRARIES
)

set(PROJECT_TEMPLATES
	"${PROJECT_SOURCE_DIR}/cmake/version.hpp.in"
	"${PROJECT_SOURCE_DIR}/cmake/module.cpp.in"
	"${PROJECT_SOURCE_DIR}/cmake/DownloadProject.cmake"
	"${PROJECT_SOURCE_DIR}/cmake/DownloadProject.CmakeLists.cmake.in"
	"${PROJECT_SOURCE_DIR}/cmake/util.cmake"
)
if(WIN32)
	list(APPEND PROJECT_TEMPLATES
		"${PROJECT_SOURCE_DIR}/cmake/installer.iss.in"
		"${PROJECT_SOURCE_DIR}/cmake/version.rc.in"
	)
endif()

set(PROJECT_PUBLIC
)
set(PROJECT_PRIVATE_GENERATED
	"${PROJECT_BINARY_DIR}/source/module.cpp"
	"${PROJECT_BINARY_DIR}/source/version.hpp"
)
set(PROJECT_PRIVATE_SOURCE
	"${PROJECT_SOURCE_DIR}/source/plugin.hpp"
	"${PROJECT_SOURCE_DIR}/source/plugin.cpp"
	"${PROJECT_SOURCE_DIR}/source/json-source.hpp"
	"${PROJECT_SOURCE_DIR}/source/json-source.cpp"
	"${PROJECT_SOURCE_DIR}/source/obs-source-factory.hpp"
	"${PROJECT_SOURCE_DIR}/source/obs-source-factory.cpp"
)

set(PROJECT_PRIVATE
	${PROJECT_DATA}
	${PROJECT_PRIVATE_GENERATED}
	${PROJECT_PRIVATE_SOURCE}
	${PROJECT_TEMPLATES}
)

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

# Clang-Format
if(HAVE_CLANG)
	clang_format(
		TARGETS ${PROJECT_NAME}
		DEPENDENCY
		VERSION 9.0.0
	)
endif()

# Include Directories
target_include_directories(${PROJECT_NAME}
	PUBLIC
	PRIVATE
		"${PROJECT_BINARY_DIR}/source"
		"${PROJECT_SOURCE_DIR}/source"
)

# OBS Studio
if(${PropertyPrefix}OBS_NATIVE)
	target_link_libraries(${PROJECT_NAME}
		libobs
	)
elseif(${PropertyPrefix}OBS_REFERENCE)
	target_include_directories(${PROJECT_NAME}
		PRIVATE
			"${OBS_STUDIO_DIR}/libobs"
	)
	target_link_libraries(${PROJECT_NAME}
		"${LIBOBS_LIB}"
	)
elseif(${PropertyPrefix}OBS_PACKAGE)
	target_include_directories(${PROJECT_NAME}
		PRIVATE
			"${OBS_STUDIO_DIR}/include"
	)
	target_link_libraries(${PROJECT_NAME}
		libobs
	)
elseif(${PropertyPrefix}OBS_DOWNLOAD)
	target_link_libraries(${PROJECT_NAME}
		libobs
	)
endif()

# Link Libraries
target_link_libraries(${PROJECT_NAME}
	${PROJECT_LIBRARIES}
)

# Definitions
if (WIN32)
	target_compile_definitions(${PROJECT_NAME}
		PRIVATE
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
			NOCTLMGR
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
			NOHELP
			NOPROFILER
			NODEFERWINDOWPOS
			NOMCX
			NOIME
			NOMDI
			NOINOUT
	)
endif()

# C++ Standard and Extensions
set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD ${_CXX_STANDARD}
		CXX_EXTENSIONS ${_CXX_EXTENSIONS}
)

# File Version
if(WIN32)
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
		VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
		SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
	)
else()
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
		VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
		SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
	)
endif()

################################################################################
# Installation
################################################################################

if(${PropertyPrefix}OBS_NATIVE)
	install_obs_plugin_with_data(${PROJECT_NAME} data)
else()
	install(
		TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION "./obs-plugins/${BITS}bit/" COMPONENT Runtime
		LIBRARY DESTINATION "./obs-plugins/${BITS}bit/" COMPONENT Runtime
	)
	if(MSVC)
		install(
			FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
			DESTINATION "./obs-plugins/${BITS}bit/"
			OPTIONAL
		)
	endif()
	
	install(
		DIRECTORY "${PROJECT_SOURCE_DIR}/data/"
		DESTINATION "./data/obs-plugins/${PROJECT_NAME}/"
	)
	
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
endif()
