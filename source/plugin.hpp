#pragma once
#include <obs-module.h>
#include <util/platform.h>
#include "version.hpp"

// Log Functionality
#define DLOG_PREFIX "[Own3d.TV]"
#define DLOG_(level, ...) blog(level, DLOG_PREFIX " " __VA_ARGS__)
#define DLOG_ERROR(...) DLOG_(LOG_ERROR, __VA_ARGS__)
#define DLOG_WARNING(...) DLOG_(LOG_WARNING, __VA_ARGS__)
#define DLOG_INFO(...) DLOG_(LOG_INFO, __VA_ARGS__)
#define DLOG_DEBUG(...) DLOG_(LOG_DEBUG, __VA_ARGS__)
