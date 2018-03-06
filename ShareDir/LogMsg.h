#pragma once


#define LOG_MSG_LEVEL		LOG_LEVEL_DEBUG


typedef enum _tag_log_level_enum {
	LOG_LEVEL_DEBUG   = 1,
	LOG_LEVEL_INFO    = 2,
	LOG_LEVEL_WARNING = 3,
	LOG_LEVEL_ERROR   = 4,
} LOG_LEVEL_ENUM;



int log_msg_init(const char *log_file = NULL);
int log_msg(const char *msg, int level);