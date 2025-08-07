// log_macro.h
#pragma once
#include <spdlog/spdlog.h>
#define LOG_INFO(msg, ...)  SPDLOG_INFO(msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) SPDLOG_ERROR(msg, ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...) SPDLOG_DEBUG(msg, ##__VA_ARGS__)
#define LOG_WARN(msg, ...)  SPDLOG_WARN(msg, ##__VA_ARGS__)


#define LOG_INFO_ROOM(msg, ...) SPDLOG_INFO("[room {}] " msg, this->getId(), ##__VA_ARGS__)
#define LOG_ERROR_ROOM(msg, ...) SPDLOG_ERROR("[room {}] " msg, this->getId(), ##__VA_ARGS__)
#define LOG_DEBUG_ROOM(msg, ...) SPDLOG_DEBUG("[room {}] " msg, this->getId(), ##__VA_ARGS__)
#define LOG_WARN_ROOM(msg, ...) SPDLOG_WARN("[room {}] " msg, this->getId(), ##__VA_ARGS__)