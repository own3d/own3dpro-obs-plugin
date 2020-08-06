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
cmake_minimum_required(VERSION 3.12.0)

# Automatic Versioning
set(VERSION_MAJOR 1)
set(VERSION_MINOR 3)
set(VERSION_PATCH 0)
set(VERSION_TWEAK 0)
set(VERSION_SUFFIX "")
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
if(PROJECT_COMMIT)
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}-${PROJECT_COMMIT}")
else()
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}")
endif()

# Define Project
project(
	own3d
	VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}
)
set(PROJECT_FULL_NAME "OWN3D for OBS Studio")
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
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Clang/Clang.cmake")
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
	if (TARGET obs-frontend-api)
		set(HAVE_OBS_FRONTEND TRUE)
	endif()
else()
	message(STATUS "${PROJECT_NAME}: Using packaged or remote obs-studio.")
	CacheSet(${PropertyPrefix}OBS_NATIVE FALSE)

	set(CMAKE_PACKAGE_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Path for generated archives.")
	set(CMAKE_PACKAGE_NAME "${PROJECT_NAME}" CACHE STRING "Name for the generated archives.")
	set(CMAKE_PACKAGE_SUFFIX_OVERRIDE "" CACHE STRING "Override for the suffix.")

	if(UNIX)
		set(UNIX_LOCAL_STRUCTURE TRUE CACHE BOOL "Package for a local linux install.")
	endif()

	if(NOT ${PropertyPrefix}OBS_DOWNLOAD)
		set(${PropertyPrefix}OBS_STUDIO_DIR "" CACHE PATH "OBS Studio Source/Package Directory")
		set(${PropertyPrefix}OBS_DEPENDENCIES_DIR "" CACHE PATH "OBS Studio Dependencies Directory")
		if(EXISTS "${OBS_STUDIO_DIR}/cmake/LibObs/LibObsConfig.cmake")
			message(STATUS "${PROJECT_NAME}: Using packaged obs-studio.")
			CacheSet(${PropertyPrefix}OBS_PACKAGE TRUE)
		elseif(EXISTS "${OBS_STUDIO_DIR}/libobs/obs-module.h")
			message(STATUS "${PROJECT_NAME}: Using referenced obs-studio.")
			CacheSet(${PropertyPrefix}OBS_REFERENCE TRUE)
		else()
			message(FATAL_ERROR "${PROJECT_NAME}: No OBS Studio detected. Automatically setting ${PropertyPrefix}OBS_DOWNLOAD...")
			CacheSet(${PropertyPrefix}OBS_DOWNLOAD TRUE)
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

# Download OBS Studio and OBS Dependencies
if(${PropertyPrefix}OBS_DOWNLOAD)
	include("DownloadProject")
	
	set(OBS_DOWNLOAD_VERSION "25.0.3-fe-ci")
	if(WIN32)
		# Windows
		set(OBS_DEPENDENCIES_VERSION "25.0.0")
		set(OBS_QT_VERSION "5.10.1")

		download_project(
			PROJ libobs
			URL https://github.com/Xaymar/obs-studio/releases/download/${OBS_DOWNLOAD_VERSION}/obs-studio-${ARCH}-0.0.0.0-vs2019.7z
			UPDATE_DISCONNECTED 1
		)

		download_project(
			PROJ obsdeps
			URL https://cdn.xaymar.com/obs/dependencies_${OBS_DEPENDENCIES_VERSION}.7z
			UPDATE_DISCONNECTED 1
		)
	elseif(UNIX)
		# Unix, Linux
		download_project(
			PROJ libobs
			URL https://github.com/Xaymar/obs-studio/releases/download/${OBS_DOWNLOAD_VERSION}/obs-studio-${ARCH}-0.0.0.0-ubuntu1804.7z
			UPDATE_DISCONNECTED 1
		)

		# Dependencies must be installed like normal OBS Studio
		message("Linux builds require your install the necessary development packages, take a look at the obs-studio build guide for them.")
	else()
		# Other OS
		message(FATAL "Unable to download libOBS as this Operating System is not yet supported.")
	endif()
endif()

# Load OBS Studio & Dependencies
if(${PropertyPrefix}OBS_PACKAGE)
	include("${OBS_STUDIO_DIR}/cmake/LibObs/LibObsConfig.cmake")
	if (EXISTS "${OBS_STUDIO_DIR}/cmake/obs-frontend-api/obs-frontend-apiConfig.cmake")
		include("${OBS_STUDIO_DIR}/cmake/obs-frontend-api/obs-frontend-apiConfig.cmake")
		set(HAVE_OBS_FRONTEND TRUE)
	endif()
elseif(${PropertyPrefix}OBS_REFERENCE)
	set(obsPath "${OBS_STUDIO_DIR}")
	include("${OBS_STUDIO_DIR}/cmake/external/FindLibobs.cmake")
elseif(${PropertyPrefix}OBS_DOWNLOAD)
	set(_INCLUDE_PREFIX "")
	if(WIN32)
		set(_INCLUDE_PREFIX "${libobs_SOURCE_DIR}/cmake")
	elseif(UNIX)
		set(_INCLUDE_PREFIX "${libobs_SOURCE_DIR}/usr/local/lib/cmake")
	else()
	endif()
	
	include("${_INCLUDE_PREFIX}/LibObs/LibObsConfig.cmake")
	if (EXISTS "${_INCLUDE_PREFIX}/obs-frontend-api/obs-frontend-apiConfig.cmake")
		include("${_INCLUDE_PREFIX}/obs-frontend-api/obs-frontend-apiConfig.cmake")
		
		if(WIN32)
			download_project(
				PROJ qt
				URL https://cdn.xaymar.com/obs/qt_${OBS_QT_VERSION}.7z
				UPDATE_DISCONNECTED 1
			)

			set(Qt5_DIR "${qt_SOURCE_DIR}" CACHE STRING "Path to Qt5")
			if("${BITS}" STREQUAL "32")
				CacheSet(Qt5_DIR "${qt_SOURCE_DIR}/msvc2017/lib/cmake/Qt5/")
			else()
				CacheSet(Qt5_DIR "${qt_SOURCE_DIR}/msvc2017_64/lib/cmake/Qt5/")
			endif()
		endif()
		set(HAVE_OBS_FRONTEND TRUE)
	endif()
	
	set(libobs_INCLUDE_DIRS "${libobs_SOURCE_DIR}/include")
endif()

# QT5
if(HAVE_OBS_FRONTEND)
	find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
endif()

# CURL
find_package(CURL QUIET)
if(NOT CURL_FOUND)
	if(${PropertyPrefix}OBS_NATIVE)
		# Already defined by OBS
		set(CURL_LIBRARIES "${CURL_LIB}")
		set(CURL_INCLUDE_DIRS "${CURL_INCLUDE_DIR}")
	elseif(${PropertyPrefix}OBS_REFERENCE)
		set(CURL_LIBRARIES "${OBS_DEPENDENCIES_DIR}/bin/libcurl.lib")
		set(CURL_INCLUDE_DIRS "${OBS_DEPENDENCIES_DIR}/include")
	elseif(${PropertyPrefix}OBS_PACKAGE)
		set(CURL_LIBRARIES "${OBS_DEPENDENCIES_DIR}/bin/libcurl.lib")
		set(CURL_INCLUDE_DIRS "${OBS_DEPENDENCIES_DIR}/include")	
	elseif(${PropertyPrefix}OBS_DOWNLOAD)
		set(CURL_LIBRARIES "${obsdeps_SOURCE_DIR}/win${BITS}/bin/libcurl.lib")
		set(CURL_INCLUDE_DIRS "${obsdeps_SOURCE_DIR}/win${BITS}/include")
	endif()
	CacheSet(CURL_LIBRARY_DEBUG ${CURL_LIBRARIES})
	CacheSet(CURL_LIBRARY_RELEASE ${CURL_LIBRARIES})
	CacheSet(CURL_INCLUDE_DIR ${CURL_INCLUDE_DIRS})
	set(CURL_FOUND ON)
endif()

# ZLIB
find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
	if(${PropertyPrefix}OBS_NATIVE)
		# Already defined by OBS
		set(ZLIB_LIBRARIES "${ZLIB_LIB}")
		set(ZLIB_INCLUDE_DIRS "${ZLIB_INCLUDE_DIR}")
	elseif(${PropertyPrefix}OBS_REFERENCE)
		set(ZLIB_LIBRARIES "${OBS_DEPENDENCIES_DIR}/bin/zlib.lib")
		set(ZLIB_INCLUDE_DIRS "${OBS_DEPENDENCIES_DIR}/include")
	elseif(${PropertyPrefix}OBS_PACKAGE)
		set(ZLIB_LIBRARIES "${OBS_DEPENDENCIES_DIR}/bin/zlib.lib")
		set(ZLIB_INCLUDE_DIRS "${OBS_DEPENDENCIES_DIR}/include")	
	elseif(${PropertyPrefix}OBS_DOWNLOAD)
		set(ZLIB_LIBRARIES "${obsdeps_SOURCE_DIR}/win${BITS}/bin/zlib.lib")
		set(ZLIB_INCLUDE_DIRS "${obsdeps_SOURCE_DIR}/win${BITS}/include")
	endif()
	CacheSet(ZLIB_LIBRARY_DEBUG ${ZLIB_LIBRARIES})
	CacheSet(ZLIB_LIBRARY_RELEASE ${ZLIB_LIBRARIES})
	CacheSet(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIRS})
	set(ZLIB_FOUND ON)
endif()

# libzip
set(LIBZIP_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/third-party/libzip/${BITS}/include")
set(LIBZIP_LIBRARIES "${PROJECT_SOURCE_DIR}/third-party/libzip/${BITS}/lib/zip.lib")
set(LIBZIP_BINARIES "${PROJECT_SOURCE_DIR}/third-party/libzip/${BITS}/bin/zip.dll")

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
	"source/source-labels.hpp"
	"source/source-labels.cpp"
	"source/obs/obs-browser-panel.hpp"
	"source/obs/obs-source-factory.hpp"
	"source/ui/ui.hpp"
	"source/ui/ui.cpp"
	"source/ui/ui-browser.hpp"
	"source/ui/ui-browser.cpp"
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
	${LIBZIP_LIBRARIES}
)
set(PROJECT_LIBRARIES_DELAYED )
set(PROJECT_INCLUDE_DIRS
	${CURL_INCLUDE_DIRS}
	${LIBZIP_INCLUDE_DIRS}
	${libobs_INCLUDE_DIRS}
)
set(PROJECT_DEFINES )

if(WIN32)
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
if(WIN32)
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
		CXX_STANDARD ${_CXX_STANDARD}
		CXX_STANDARD_REQUIRED ON
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

# UI
if(HAVE_OBS_FRONTEND)
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
endif()

# Clang Tools
if(HAVE_CLANG)
	clang_format(
		TARGETS ${PROJECT_NAME}
		DEPENDENCY
		VERSION 9.0.0
	)
endif()

################################################################################
# Installation
################################################################################

if(${PropertyPrefix}OBS_NATIVE)
	install_obs_plugin_with_data(${PROJECT_NAME} data)
	install(FILES ${LIBZIP_BINARIES}
		DESTINATION "./obs-plugins/${BITS}bit/")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy
			"${LIBZIP_BINARIES}"
			"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/obs-plugins/${BITS}bit/"
		VERBATIM)
else()
	if(UNIX_LOCAL_STRUCTURE)
		install(
			TARGETS ${PROJECT_NAME}
			RUNTIME DESTINATION "./plugins/${PROJECT_NAME}/bin/${BITS}bit/" COMPONENT Runtime
			LIBRARY DESTINATION "./plugins/${PROJECT_NAME}/bin/${BITS}bit/" COMPONENT Runtime		
		)

		install(
			DIRECTORY "data/"
			DESTINATION "./plugins/${PROJECT_NAME}/data/"
		)
		
		add_custom_target(
			PACKAGE_7Z
			${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.7z" --format=7zip --
				"${CMAKE_INSTALL_PREFIX}/plugins/${PROJECT_NAME}"
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
		)
		add_custom_target(
			PACKAGE_ZIP
			${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.zip" --format=zip --
				"${CMAKE_INSTALL_PREFIX}/plugins/${PROJECT_NAME}"
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
		)
	else()
		install(
			TARGETS ${PROJECT_NAME}
			RUNTIME DESTINATION "./obs-plugins/${BITS}bit/" COMPONENT Runtime
			LIBRARY DESTINATION "./obs-plugins/${BITS}bit/" COMPONENT Runtime
		)
		install(FILES ${LIBZIP_BINARIES}
			DESTINATION "./obs-plugins/${BITS}bit/")
		if(MSVC)
			install(
				FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
				DESTINATION "./obs-plugins/${BITS}bit/"
				OPTIONAL
			)
		endif()
	
		install(
			DIRECTORY "data/"
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
endif()
