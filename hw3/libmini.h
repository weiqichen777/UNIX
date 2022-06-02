#ifndef __LIBMINI_H__
#define __LIBMINI_H__           /* avoid reentrant */

//#include "stdbool.h"

typedef long long size_t;
typedef long long ssize_t;
typedef long long off_t;
typedef int mode_t;
typedef int uid_t;
typedef int gid_t;
typedef int pid_t;
typedef _Bool bool;
#define NULL ((void*) 0)

extern long errno;

/*system call number*/
#define SYS_WRITE           1
#define SYS_RT_SIGACTION    13
#define SYS_RT_PROCMASK     14
#define SYS_PAUSE           34
#define SYS_NANOSLEEP       35
#define SYS_ALARM           37
#define SYS_EXIT            60
#define SYS_RT_SIGPEND      127

/* Values for the HOW argument to `sigprocmask'.  */
#define SIG_BLOCK     0                 /* Block signals.  */
#define SIG_UNBLOCK   1                 /* Unblock signals.  */
#define SIG_SETMASK   2                 /* Set the set of blocked signals.  */

#define SIG_DFL	((__sighandler_t)0)	/* default signal handling */
#define SIG_IGN	((__sighandler_t)1)	/* ignore signal */
#define SIG_ERR	((__sighandler_t)-1)	/* error return from signal */

/*perror*/
#define	PERRMSG_MIN	0
#define	PERRMSG_MAX	34

/* from /usr/include/x86_64-linux-gnu/asm/signal.h */
#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGBUS		 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGURG		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGIO		29
#define SIGPOLL		SIGIO

/* from /usr/include/asm-generic/errno-base.h */
#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	ESRCH		 3	/* No such process */
#define	EINTR		 4	/* Interrupted system call */
#define	EIO		 5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	E2BIG		 7	/* Argument list too long */
#define	ENOEXEC		 8	/* Exec format error */
#define	EBADF		 9	/* Bad file number */
#define	ECHILD		10	/* No child processes */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK		15	/* Block device required */
#define	EBUSY		16	/* Device or resource busy */
#define	EEXIST		17	/* File exists */
#define	EXDEV		18	/* Cross-device link */
#define	ENODEV		19	/* No such device */
#define	ENOTDIR		20	/* Not a directory */
#define	EISDIR		21	/* Is a directory */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* File table overflow */
#define	EMFILE		24	/* Too many open files */
#define	ENOTTY		25	/* Not a typewriter */
#define	ETXTBSY		26	/* Text file busy */
#define	EFBIG		27	/* File too large */
#define	ENOSPC		28	/* No space left on device */
#define	ESPIPE		29	/* Illegal seek */
#define	EROFS		30	/* Read-only file system */
#define	EMLINK		31	/* Too many links */
#define	EPIPE		32	/* Broken pipe */
#define	EDOM		33	/* Math argument out of domain of func */
#define	ERANGE		34	/* Math result not representable */
#define	ENOSYS		38	/* Invalid system call number */

#define __SIGRTMIN        32
#define SIGCANCEL       __SIGRTMIN
#define SIGSETXID       (__SIGRTMIN + 1)

# define __sigmask(sig) \
    (((unsigned long int) 1) << (((sig) - 1) % (8 * sizeof (unsigned long int))))
# define __sigword(sig) \
    (((sig) - 1) / (8 * sizeof (unsigned long int)))
# define __sigaddset(set, signo)                                        \
    (__extension__ ({                                                \
        unsigned long int __mask = __sigmask (signo);                        \
        unsigned long int __word = __sigword (signo);                        \
        (set)->sig[__word] |= __mask;                                \
        (void)0;                                                        \
    }))
# define __sigismember(set, signo)                                \
    (__extension__ ({                                                \
        unsigned long int __mask = __sigmask (signo);                        \
        unsigned long int __word = __sigword (signo);                        \
        (set)->sig[__word] & __mask ? 1 : 0;                        \
    }))

/*sigaction*/
#define NSIG 64
#define NSIG_BPW 64
#define NSIG_WORDS (NSIG / NSIG_BPW)

# define STUB(act, sigsetsize) (sigsetsize)

typedef struct {
  unsigned long sig[NSIG_WORDS];
} sigset_t;

typedef void (*__sighandler_t) (int);
typedef __sighandler_t sighandler_t;

#define SA_RESTORER 0x04000000
typedef struct {
  int si_signo;
  int si_errno;
  int si_code;
  // ignoring
} siginfo_t;

struct sigaction {
  union {
    sighandler_t sa_handler;
    void (*sa_sigaction)(int, siginfo_t *, void*);
  };
  unsigned long sa_flags;
  void (*sa_restorer)(void);
  sigset_t sa_mask;
};

/*time structure*/
struct timespec {
	long	tv_sec;		/* seconds */
	long	tv_nsec;	/* nanoseconds */
};

struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* microseconds */
};

struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of DST correction */
};

/*jump buffer*/
typedef struct jmp_buf_s {
	long long reg[8];
	sigset_t mask;
} jmp_buf[1];

/* system calls */
long sys_write(int fd, const void *buf, size_t count);
long sys_pause();
long sys_alarm(unsigned int seconds);
long sys_nanosleep(struct timespec *rqtp, struct timespec *rmtp);
long sys_rt_sigpending(sigset_t *set, size_t sigsetsize);
long sys_rt_sigprocmask(int how, const sigset_t *set, sigset_t *oset, size_t sigsetsize);
long sys_rt_sigaction(int sig, const struct sigaction *act, struct sigaction *oact, size_t sigsetsize);

/* wrappers */
ssize_t write(int fd, const void *buf, size_t count);
int pause();
unsigned int alarm(unsigned int seconds);
unsigned int sleep(unsigned int seconds);
int setjmp(jmp_buf env);    //in syscall.asm
int longjmp(jmp_buf env, int val);  //in syscall.asm

/*signal*/
int sigemptyset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
//static inline bool __is_internal_signal(int sig);
int sigprocmask (int how, const sigset_t *set, sigset_t *oset);
int sigpending (sigset_t *set);
int sigismember (const sigset_t *set, int signo);
sighandler_t signal(int signum, sighandler_t handler);
int sigaction(int signum, const struct sigaction * act, struct sigaction * oldact);

/*utils*/
size_t strlen(const char *s);
void *memset (void *dest, int val, size_t len);
void *memcpy(void *dest, const void *src, long long len);
void perror(const char *prefix);

#endif  /* __LIBMINI_H__ */