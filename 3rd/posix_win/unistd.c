#include "unistd.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <conio.h>
#include <Windows.h>
#include <WinSock2.h>

static LONGLONG get_ticks_per_second() {
  static LONGLONG ticks_per_second = -1;
  if (-1 == ticks_per_second) {
    LARGE_INTEGER ticks_per_sec = { 0 };
    if (QueryPerformanceFrequency(&ticks_per_sec))
      ticks_per_second = ticks_per_sec.QuadPart;
    else
      ticks_per_second = 0;
  }
  //
  return ticks_per_second;
}

//pid_t getpid() {
//	return (pid_t)GetCurrentProcessId();
//}

int kill(pid_t pid, int exit_code) {
	return TerminateProcess((HANDLE)pid, exit_code);
}


#define NANOSEC 1000000000
#define MICROSEC 1000000
const LONGLONG kMicrosecondsPerSecond = 1000 * 1000;

void usleep(size_t us) {
	if(us > 1000) {
		Sleep(us / 1000);
		return;
	}
	LONGLONG delta = get_ticks_per_second() / MICROSEC * us;
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	LONGLONG start = counter.QuadPart;
	for(;;) {
		QueryPerformanceCounter(&counter);
		if(counter.QuadPart - start >= delta)
			return;
	}
}

void sleep(size_t ms) {
	Sleep(ms);
}


void tick_now(struct timespec *ti) {
  LONGLONG ticks_per_second = get_ticks_per_second();
  if (ticks_per_second > 0) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);  // 返回windows启动的tick时间
    LONGLONG qpc_value = now.QuadPart;
    // Intentionally calculate microseconds in a round about manner to avoid
    // overflow and precision issues. Think twice before simplifying!
    LONGLONG whole_seconds = qpc_value / ticks_per_second;
    LONGLONG leftover_ticks = qpc_value % ticks_per_second;
    //LONGLONG microseconds = (whole_seconds * kMicrosecondsPerSecond) + ((leftover_ticks * kMicrosecondsPerSecond) / ticks_per_second);
    ti->tv_sec = (long)whole_seconds;
    ti->tv_nsec = (long)(leftover_ticks * kMicrosecondsPerSecond * 1000 / ticks_per_second);
  }
}


int clock_gettime(int what, struct timespec *ti) {
  
	switch(what) {
  case CLOCK_REALTIME: {
    //CLOCK_REALTIME:系统实时时间,随系统实时时间改变而改变,即从UTC1970-1-1 0:0:0开始计时。
    tick_now(ti);
    return 0;
  }
  case CLOCK_MONOTONIC: {
    //CLOCK_MONOTONIC:从系统启动这一刻起开始计时, 不受系统时间被用户改变的影响。
    tick_now(ti);
    return 0;
  }
  case CLOCK_THREAD_CPUTIME_ID: {
    // CLOCK_THREAD_CPUTIME_ID:本线程到当前代码系统CPU花费的时间
    tick_now(ti);
    return 0;
  }
	default:
		__asm int 3;
	}
	return -1;
}

_CRTIMP intptr_t __cdecl _get_osfhandle(_In_ int _FileHandle);

int flock(int fd, int flag) {
	// Not implemented
	//__asm int 3;
  HANDLE h = (HANDLE)_get_osfhandle(fd);
  if (INVALID_HANDLE_VALUE != h) {
    if (LockFile(h, 0, 0, 0, 0))
      return 0;
  }
  return -1;
}

void sigaction(int flag, struct sigaction *action, int param) {
	// Not implemented
	//__asm int 3;
}
void sigfillset(sigset_t* sigset) {
  // Not implemented
  //__asm int 3;
}

int pipe(int fd[2]) {

	int listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
  sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  //
  int sin_size = sizeof(sin);
	if(bind(listen_fd, (struct sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR
    || getsockname(listen_fd, (struct sockaddr*)&sin, &sin_size) == -1) {
		closesocket(listen_fd);
		return -1;
	}
	listen(listen_fd, 5);
	printf("Windows sim pipe() listen at %s:%d\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

	int client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(connect(client_fd, (struct sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
		closesocket(listen_fd);
		return -1;
	}

    struct sockaddr_in client_addr;
	int name_len = sizeof(client_addr);
	int client_sock = accept(listen_fd, (struct sockaddr*)&client_addr, &name_len);
	//FD_SET( clientSock, &g_fdClientSock);

	// TODO: close listen_fd

	fd[0] = client_sock;
	fd[1] = client_fd;
	return 0;

	////HANDLE hReadPipe, hWritePipe;
	//SECURITY_ATTRIBUTES sa;
	//sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	//sa.lpSecurityDescriptor = NULL;
	//sa.bInheritHandle = TRUE;
	//if(CreatePipe(&fd[0],&fd[1],&sa,0))
	//	return 0;
	//return -1;
}

int write(int fd, const void *ptr, size_t sz) {
  int err = 0;
  int ret = send(fd, ptr, sz, 0);
  if (-1 == ret) {
    err = WSAGetLastError();
    switch (err) {
    case WSAEWOULDBLOCK:
      //WSAEWOULDBLOCK是Output Buffer 已经满了，无法再写入数据。
      //有可能是接收方的接受缓冲区已被填满。等待FD_WRITE事件再发送。skynet已做了处理。by ZC. 2016-5-11.
      debug_log("windows send WSAEWOULDBLOCK");
      //EINTR、EAGAIN都需要重试，结合skynet的send_list_tcp与send_socket函数处理来看，与Windows下EWOULDBLOCK错误就是EAGAIN，
      //所以使用_set_errno(EAGAIN)更合适。by ZC
      _set_errno(EAGAIN);
      break;
    case WSAEINTR:
      debug_log("windows send WSAEINTR");
      _set_errno(EINTR);
      break;
    case WSAENOTSOCK:
    case WSAEBADF:
      debug_log("windows send WSAENOTSOCK");
      _set_errno(EIO);
      break;
    default:
      fprintf(stderr, "windows send error!!!");
      debug_log("windows send error!!!");
      _set_errno(EINTR);
    }
  }
  return ret;
}

int read(int fd, void *buffer, size_t sz) {

	// read console input
	if(fd == 0) {
		char *buf = (char *) buffer;
		while(_kbhit()) {
			char ch = _getch();
			*buf++ = ch;
			_putch(ch);
			if(ch == '\r') {
				*buf++ = '\n';
				_putch('\n');
			}
		}
		return buf - (char *) buffer;
	}

	WSABUF vecs[1];
	vecs[0].buf = buffer;
	vecs[0].len = sz;

    DWORD bytesRecv = 0;
    DWORD flags = 0;
    if(WSARecv(fd, vecs, 1, &bytesRecv, &flags, NULL, NULL)) {
		if(WSAGetLastError() == WSAECONNRESET)
			return 0;
        return -1;
	} else
        return bytesRecv;
	//DWORD read = 0;
	//if(ReadFile(fd, buffer, sz, &read, NULL) == TRUE)
	//	return read;
	//return -1;
}

int close(int fd) {
	shutdown(fd, SD_BOTH);
	closesocket(fd);
  return 0;
}

int daemon(int a, int b) {
	// Not implemented
	//__asm int 3;
	return 0;
}

char *strsep(char **stringp, const char *delim)
{
    char *s;
    const char *spanp;
    int c, sc;
    char *tok;
    if ((s = *stringp)== NULL)
        return (NULL);
    for (tok = s;;) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc =*spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}

FILE* s_file_log = NULL;
void debug_log(char* log) {
  if (NULL == s_file_log) {
    s_file_log = fopen("skynet_debug.log", "w");
    setvbuf(s_file_log, NULL, _IONBF, 0);
  }
  fprintf(s_file_log, log);
  fprintf(s_file_log, "\n");
  fflush(s_file_log);
}

void debug_box(char* msg) {
  MessageBoxA(0, msg, 0, 0);
}
