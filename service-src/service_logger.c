#include "skynet.h"
#include "skynet_env.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#ifdef _MSC_VER
#include <direct.h>
#include <Windows.h>
#include <locale.h>
#endif
struct logger {
	FILE * handle;
	int close;
};

struct logger *
logger_create(void) {
	struct logger * inst = skynet_malloc(sizeof(*inst));
	inst->handle = NULL;
	inst->close = 0;
	return inst;
}

void
logger_release(struct logger * inst) {
	if (inst->close) {
		fclose(inst->handle);
	}
	skynet_free(inst);
}

static int
_logger(struct skynet_context * context, void *ud, int type, int session, uint32_t source, const void * msg, size_t sz) {
	struct logger * inst = ud;
#ifdef _MSC_VER
	fwprintf(inst->handle, L"[:%08x] ",source);
  fwrite(msg, sz, 1, inst->handle);
	fwprintf(inst->handle, L"\n");
#else
	fprintf(inst->handle, "[:%08x] ",source);
	fwrite(msg, sz , 1, inst->handle);
	fprintf(inst->handle, "\n");
#endif
	fflush(inst->handle);

	return 0;
}

#ifdef _MSC_VER
static int Mkdir(const char* dir) {
	return _mkdir(dir);
}
#else
static int Mkdir(const char* dir) {
	return mkdir(dir, 0777);
}
#endif

int
logger_init(struct logger * inst, struct skynet_context *ctx, const char * parm) {
	if (parm) {
		//log文件名中加入pid和时间信息；
		time_t now = time(NULL);
		char Time[32] = { 0 };
		strftime(Time, 20, "%Y%m%d_%H%M%S", localtime(&now));

		const char * logpath = skynet_getenv("logpath");
		Mkdir(logpath);

		char logfile[256];
		snprintf(logfile, sizeof(logfile), "%s/%s_%u_%s.%s", logpath, parm, getpid(), Time, "log");

		inst->handle = fopen(logfile, "w");
		if (inst->handle == NULL) {
			return 1;
		}
		inst->close = 1;
	} else {
		inst->handle = stdout;
	}
#ifdef _MSC_VER
	_wsetlocale(0, L"chs");
#endif
	if (inst->handle) {
		skynet_callback(ctx, inst, _logger);
		skynet_command(ctx, "REG", ".logger");
		return 0;
	}
	return 1;
}
