// log_macro.h
#pragma once
#include <spdlog/spdlog.h>
#define LOG_INFO(msg, ...)  SPDLOG_INFO(msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) SPDLOG_ERROR(msg, ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...) SPDLOG_DEBUG(msg, ##__VA_ARGS__)
#define LOG_WARN(msg, ...)  SPDLOG_WARN(msg, ##__VA_ARGS__)