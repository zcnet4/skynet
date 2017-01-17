#ifndef _UNISTD_H
#define _UNISTD_H

/* This file intended to serve as a drop-in replacement for
*  unistd.h on Windows
*  Please add functionality as neeeded
*/

#include <stdlib.h>
//#include <io.h>
//#include <getopt.h> /* getopt at: https://gist.github.com/ashelly/7776712 */
#include <process.h> /* for getpid() and the exec..() family */
#include <direct.h> /* for _getcwd() and _chdir() */

#define random rand
#define srandom srand
#define snprintf _snprintf

/* Values for the second argument to access.
These may be OR'd together.  */
#define R_OK    4       /* Test for read permission.  */
#define W_OK    2       /* Test for write permission.  */
//#define   X_OK    1       /* execute permission - unsupported in windows*/
#define F_OK    0       /* Test for existence.  */

#define access _access
#define dup2 _dup2
#define execve _execve
#define ftruncate _chsize
#define unlink _unlink
#define fileno _fileno
#define getcwd _getcwd
#define chdir _chdir
#define isatty _isatty
#define lseek _lseek
/* read, write, and close are NOT being #defined here, because while there are file handle specific versions for Windows, they probably don't work for sockets. You need to look at your app and consider whether to call e.g. closesocket(). */

#define ssize_t int

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
/* should be in some equivalent to <sys/types.h> */
#if _MSC_VER >= 1600
/*
MS VC++ 14.0 _MSC_VER = 1900 (Visual Studio 2015)
MS VC++ 12.0 _MSC_VER = 1800 (VisualStudio 2013)
MS VC++ 11.0 _MSC_VER = 1700 (VisualStudio 2012)
MS VC++ 10.0 _MSC_VER = 1600(VisualStudio 2010)
MS VC++ 9.0 _MSC_VER = 1500(VisualStudio 2008)
MS VC++ 8.0 _MSC_VER = 1400(VisualStudio 2005)
MS VC++ 7.1 _MSC_VER = 1310(VisualStudio 2003)
MS VC++ 7.0 _MSC_VER = 1300(VisualStudio .NET)
MS VC++ 6.0 _MSC_VER = 1200(VisualStudio 98)
MS VC++ 5.0 _MSC_VER = 1100(VisualStudio 97)
*/
  #include <stdint.h>
#else
  typedef __int8            int8_t;
  typedef __int16           int16_t;
  typedef __int32           int32_t;
  typedef __int64           int64_t;
  typedef unsigned __int8   uint8_t;
  typedef unsigned __int16  uint16_t;
  typedef unsigned __int32  uint32_t;
  typedef unsigned __int64  uint64_t;
#endif

#endif /* unistd.h  */

#pragma once
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>



#ifdef _MSC_VER
# define inline __inline
#include "atomic_lock.h"
#endif

#ifndef __inline
	#define __inline __inline
#endif

//typedef long pid_t;
//pid_t getpid();
int kill(pid_t pid, int exit_code);

// defined in WinSock2.h
__declspec(dllimport) int __stdcall gethostname(char *buffer, int len);
void usleep(size_t us);
void sleep(size_t ms);

//typedef struct timespec {
//	int tv_sec;
//	int tv_nsec;
//} timespec;

enum { CLOCK_THREAD_CPUTIME_ID, CLOCK_REALTIME, CLOCK_MONOTONIC };
int clock_gettime(int what, struct timespec *ti);

enum { LOCK_EX, LOCK_NB };
int flock(int fd, int flag);

#define _NSIG      64

#ifdef __i386__
# define _NSIG_BPW 32
#else
# define _NSIG_BPW 64
#endif

#define _NSIG_WORDS    (_NSIG / _NSIG_BPW)
typedef struct {
  unsigned long sig[_NSIG_WORDS];
} sigset_t;
# define SA_RESTART   0x10000000 /* Restart syscall on signal return.  */
struct sigaction {
  void(*sa_handler)(int);
  //void(*sa_sigaction)(int, siginfo_t *, void *);
  sigset_t sa_mask;
  int sa_flags;
  void(*sa_restorer)(void);
};
#define SIGHUP  1
#define SIGPIPE 13

void sigaction(int flag, struct sigaction *action, int param);
void sigfillset(sigset_t* sigset);

int pipe(int fd[2]);
int daemon(int a, int b);

char *strsep(char **stringp, const char *delim);

int write(int fd, const void *ptr, size_t sz);
int read(int fd, void *buffer, size_t sz);
int close(int fd);

void debug_log(char* log);
void debug_box(char* msg);


