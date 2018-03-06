#include <stdafx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#include "LogMsg.h"


static char LOG_FILE_NAME[MAX_PATH] = "./LogMsg.txt";



static BOOL log_init = FALSE;

static char *log_level_strs[] = {
	"-------",
	"DEBUG  ",
	"INFO   ",
	"WARNING",
	"ERROR  ",
};


static void get_log_time_str(char *buff, int size)
{
	time_t timep;
	struct tm *p;

	time(&timep);
	p=localtime(&timep);

	_snprintf(buff, size, "%04d-%02d-%02d %02d:%02d:%02d", (1900 + p->tm_year), (1 + p->tm_mon),  p->tm_mday,
		p->tm_hour, p->tm_min, p->tm_sec);
}

int log_msg_init(const char *log_file)
{
	FILE *f;
	char log_time[MAX_PATH];

	if (log_init) {
		return 0;
	}
	
	if (NULL != log_file) {
		strncpy(LOG_FILE_NAME, log_file, sizeof(LOG_FILE_NAME));
	}

	f = fopen(LOG_FILE_NAME, "w");
	if (f == NULL) {
		return -1;
	}

	get_log_time_str(log_time, sizeof(log_time));
	fprintf(f, "[%s] log start, log level is set to %s\r\n\r\n", log_time, log_level_strs[LOG_MSG_LEVEL]);

	fclose(f);
	log_init = TRUE;
	return 0;
}

int log_msg(const char *msg, int level)
{
	FILE *f;
	char log_time[MAX_PATH];


	if (level < LOG_MSG_LEVEL) {
		return 0;
	}

	if (msg == NULL) {
		return -1;
	}

	f = fopen(LOG_FILE_NAME, "a");
	if (f == NULL) {
		return -1;
	}

	get_log_time_str(log_time, sizeof(log_time));
	fprintf(f, "[%s] %s: %s\r\n", log_time, log_level_strs[level], msg);

	fclose(f);
	return 0;
}

