#include "skynet_log.h"
#include "skynet_timer.h"
#include "skynet.h"
#include "skynet_socket.h"
#include <string.h>
#include <time.h>

FILE * 
skynet_log_open(struct skynet_context * ctx, uint32_t handle) {
	const char * logpath = skynet_getenv("logpath");
	if (logpath == NULL)
		return NULL;
	size_t sz = strlen(logpath);
#ifdef _MSC_VER
  char tmp[1024 + 16];
#else
	char tmp[sz + 16];
#endif
	sprintf(tmp, "%s/%08x.log", logpath, handle);
	FILE *f = fopen(tmp, "ab");
	if (f) {
		uint32_t starttime = skynet_starttime();
		uint64_t currenttime = skynet_now();
		time_t ti = starttime + currenttime/100;
		skynet_error(ctx, "Open log file %s", tmp);
		fprintf(f, "open time: %u %s", (uint32_t)currenttime, ctime(&ti));
		fflush(f);
	} else {
		skynet_error(ctx, "Open log file %s fail", tmp);
	}
	return f;
}

void
skynet_log_close(struct skynet_context * ctx, FILE *f, uint32_t handle) {
	skynet_error(ctx, "Close log file :%08x", handle);
	fprintf(f, "close time: %u\n", (uint32_t)skynet_now());
	fclose(f);
}

static void
log_blob(FILE *f, void * buffer, size_t sz) {
	size_t i;
	uint8_t * buf = buffer;
	for (i=0;i!=sz;i++) {
		fprintf(f, "%02x", buf[i]);
	}
}

static void
log_socket(FILE * f, struct skynet_socket_message * message, size_t sz) {
	fprintf(f, "[socket] %d %d %d ", message->type, message->id, message->ud);

	if (message->buffer == NULL) {
		const char *buffer = (const char *)(message + 1);
		sz -= sizeof(*message);
		const char * eol = memchr(buffer, '\0', sz);
		if (eol) {
			sz = eol - buffer;
		}
		fprintf(f, "[%*s]", (int)sz, (const char *)buffer);
	} else {
		sz = message->ud;
		log_blob(f, message->buffer, sz);
	}
	fprintf(f, "\n");
	fflush(f);
}

void 
skynet_log_output(FILE *f, uint32_t source, int type, int session, void * buffer, size_t sz) {
	if (type == PTYPE_SOCKET) {
		log_socket(f, buffer, sz);
	} else {
		uint32_t ti = (uint32_t)skynet_now();
		fprintf(f, ":%08x %d %d %u ", source, type, session, ti);
		log_blob(f, buffer, sz);
		fprintf(f,"\n");
		fflush(f);
	}
}


//////////////////////////////////////////////////////////////////////////
//
FILE* s_skynet_debug_log = NULL;

void skynet_onexit() {
  my_log_output("skynet exit!!!");
  fclose(s_skynet_debug_log);
  s_skynet_debug_log = NULL;
}

void my_log_init() {
  //log文件名中加入pid和时间信息；
  time_t now = time(NULL);
  char Time[32] = { 0 };
  strftime(Time, 20, "%Y%m%d%H%M%S", localtime(&now));
  mkdir("log", 0777);
  char logfile[256];
  snprintf(logfile, sizeof(logfile), "log/skynet_%u_%s.debug", getpid(), Time);
  //
  s_skynet_debug_log = fopen(logfile, "w");
  setvbuf(s_skynet_debug_log, NULL, _IONBF, 0);
  atexit(skynet_onexit);
}

void my_log_output(char* log) {
  time_t now = time(NULL);
  char Time[32] = { 0 };
  strftime(Time, 20, "%Y%m%d%H%M%S", localtime(&now));

  fprintf(s_skynet_debug_log, "[%s] %s", Time, log);
  fprintf(s_skynet_debug_log, "\n");
  fflush(s_skynet_debug_log);
}

