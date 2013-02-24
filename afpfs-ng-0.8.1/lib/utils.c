
/*
 *  utils.c
 *
 *  Copyright (C) 2006 Alex deVries
 *
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <utime.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <ucontext.h>

#include "afp.h"
#include "utils.h"
#include "afp_internal.h"
#include "afp_protocol.h"

struct afp_path_header_long {
	unsigned char type;
	unsigned char len;
}  __attribute__((__packed__)) ;

struct afp_path_header_unicode {
	uint8_t type;
	uint32_t hint;
	uint16_t unicode;
}  __attribute__((__packed__)) ;

int translate_path(struct afp_volume * volume,
	char *incoming, char * outgoing)
{
	return 0;
}

unsigned short utf8_to_string(char * dest, char * buf, unsigned short maxlen)
{
	return copy_from_pascal_two(dest,buf+4,maxlen);
}

unsigned char unixpath_to_afppath(
	struct afp_server * server,
	char * buf)
{
	unsigned char encoding = server->path_encoding;
	char *p =NULL, *end;
	unsigned short len;

	switch (encoding) {
	case kFPUTF8Name: {
			unsigned short *len_p = NULL;
			len_p = (void *) buf + 5;
			p=buf+7;
			len=ntohs(*len_p);
		}
		break;
	case kFPLongName: {
			unsigned char *len_p = NULL;
			len_p = (void *) buf + 1;
			p=buf+2;
			len=(*len_p);
		}
	}
	end=p+len;

	while (p<end) {
		if (*p=='/') *p='\0';
		p++;
	}
	return 0;
}

void afp_unixpriv_to_stat(struct afp_file_info *fp, 
	struct stat *stat)
{
	memset(stat,0,sizeof(*stat));
	if (fp->unixprivs.permissions) 
		stat->st_mode=fp->unixprivs.permissions;
	else 
		stat->st_mode=fp->unixprivs.ua_permissions;
	stat->st_uid=fp->unixprivs.uid;
	stat->st_gid=fp->unixprivs.gid;
}


unsigned char copy_from_pascal(char *dest, char *pascal,unsigned int max_len) 
{

	unsigned char len;

	if (!pascal) return 0;
	len=*pascal;

	if (max_len<len) len=max_len;

	memset(dest,0,max_len);
	memcpy(dest,pascal+1,len);
	return len;
}

unsigned short copy_from_pascal_two(char *dest, char *pascal,unsigned int max_len) 
{

	unsigned short * len_p = (unsigned short *) pascal;
	unsigned short len = ntohs(*len_p);

	if (max_len<len) len=max_len;
	if (len==0) return 0;
	memset(dest,0,max_len);
	memcpy(dest,pascal+2,len);
	return len;
}

unsigned char copy_to_pascal(char *dest, const char *src) 
{
	unsigned char len = (unsigned char) strlen(src);
	dest[0]=len;

	memcpy(dest+1,src,len);
	return len;
}

unsigned short copy_to_pascal_two(char *dest, const char *src) 
{
	unsigned short * sendlen = (void *) dest;
	char * data = dest + 2;

	unsigned short len ;
	if (!src) {
		dest[0]=0;
		dest[1]=0;
		return 2;
	}
	len = (unsigned short) strlen(src);
	*sendlen=htons(len);
	memcpy(data,src,len);
	return len;
}

unsigned char sizeof_path_header(struct afp_server * server)
{
	switch (server->path_encoding) {
	case kFPUTF8Name:
		return(sizeof(struct afp_path_header_unicode));
	case kFPLongName:
		return(sizeof(struct afp_path_header_long));
	}
	return 0;
}


void copy_path(struct afp_server * server, char * dest, const char * pathname, unsigned char len)
{

	char tmppathname[255];
	unsigned char encoding = server->path_encoding;
	struct afp_path_header_unicode * header_unicode = (void *) dest;
	struct afp_path_header_long * header_long = (void *) dest;
	unsigned char offset, header_len, namelen;

	switch (encoding) {
	case kFPUTF8Name:
		header_unicode->type=encoding;
		header_unicode->hint=htonl(0x08000103);
		offset = 5;
		header_len = sizeof(struct afp_path_header_unicode);
		namelen=copy_to_pascal_two(tmppathname,pathname);
		memcpy(dest+offset,tmppathname,namelen+2);
		break;
	case kFPLongName:
		header_long->type=encoding;
		offset = 1;
		header_len = sizeof(struct afp_path_header_long);
		namelen=copy_to_pascal(tmppathname,pathname) ;
		memcpy(dest+offset,tmppathname,namelen+1);
	}
}

int invalid_filename(struct afp_server * server, const char * filename) 
{

	unsigned int maxlen=0;
	int len;
	char * p, *q;

	len = strlen(filename);

	if ((len==1) && (*filename=='/')) return 0;

	/* From p.34, each individual file can be 255 chars for > 30
	   for Long or short names.  UTF8 is "virtually unlimited" */

	if (server->using_version->av_number < 30) 
		maxlen=31; 
	else
		if (server->path_encoding==kFPUTF8Name) 
			maxlen=1024;
		else 
			maxlen=255;


	p=filename+1;
	while ((q=strchr(p,'/'))) {
		if (q>p+maxlen)
			return 1;
		p=q+1;
		if (p>filename+len)
			return 0;
	}

	if (strlen(filename)-(p-filename)>maxlen)
		return 1;

	return 0;

}

int my_pthread_create (pthread_t *__newthread,
			   pthread_attr_t *__attr,
			   void *(*__start_routine)(void *),
			   void *__arg)
{
	int rc = 0;
	pthread_attr_t myattr;
	pthread_attr_init (&myattr);
	pthread_attr_setdetachstate (&myattr, PTHREAD_CREATE_DETACHED);
	rc = pthread_create(__newthread,(__attr==NULL)?&myattr:__attr,
			__start_routine,__arg);
	pthread_attr_destroy (&myattr);
	return rc;
}

typedef struct { char name[10]; int id; char description[40]; } signal_def;
 
#define FUNCTION_DEPTH  128
static char *progname = NULL;
signal_def signal_data[] =
{
	{ "SIGHUP", SIGHUP, "Hangup (POSIX)" },
	{ "SIGINT", SIGINT, "Interrupt (ANSI)" },
	{ "SIGQUIT", SIGQUIT, "Quit (POSIX)" },
	{ "SIGILL", SIGILL, "Illegal instruction (ANSI)" },
	{ "SIGTRAP", SIGTRAP, "Trace trap (POSIX)" },
	{ "SIGABRT", SIGABRT, "Abort (ANSI)" },
	{ "SIGIOT", SIGIOT, "IOT trap (4.2 BSD)" },
	{ "SIGBUS", SIGBUS, "BUS error (4.2 BSD)" },
	{ "SIGFPE", SIGFPE, "Floating-point exception (ANSI)" },
	{ "SIGKILL", SIGKILL, "Kill, unblockable (POSIX)" },
	{ "SIGUSR1", SIGUSR1, "User-defined signal 1 (POSIX)" },
	{ "SIGSEGV", SIGSEGV, "Segmentation violation (ANSI)" },
	{ "SIGUSR2", SIGUSR2, "User-defined signal 2 (POSIX)" },
	{ "SIGPIPE", SIGPIPE, "Broken pipe (POSIX)" },
	{ "SIGALRM", SIGALRM, "Alarm clock (POSIX)" },
	{ "SIGTERM", SIGTERM, "Termination (ANSI)" },
	{ "SIGCHLD", SIGCHLD, "Child status has changed (POSIX)" },
	{ "SIGCLD", SIGCLD, "Same as SIGCHLD (System V)" },
	{ "SIGCONT", SIGCONT, "Continue (POSIX)" },
	{ "SIGSTOP", SIGSTOP, "Stop, unblockable (POSIX)" },
	{ "SIGTSTP", SIGTSTP, "Keyboard stop (POSIX)" },
	{ "SIGTTIN", SIGTTIN, "Background read from tty (POSIX)" },
	{ "SIGTTOU", SIGTTOU, "Background write to tty (POSIX)" },
	{ "SIGURG", SIGURG, "Urgent condition on socket (4.2 BSD)" },
	{ "SIGXCPU", SIGXCPU, "CPU limit exceeded (4.2 BSD)" },
	{ "SIGXFSZ", SIGXFSZ, "File size limit exceeded (4.2 BSD)" },
	{ "SIGVTALRM", SIGVTALRM, "Virtual alarm clock (4.2 BSD)" },
	{ "SIGPROF", SIGPROF, "Profiling alarm clock (4.2 BSD)" },
	{ "SIGWINCH", SIGWINCH, "Window size change (4.3 BSD, Sun)" },
	{ "SIGIO", SIGIO, "I/O now possible (4.2 BSD)" },
	{ "SIGPOLL", SIGPOLL, "Pollable event occurred (System V)" },
	{ "SIGPWR", SIGPWR, "Power failure restart (System V)" },
	{ "SIGSYS", SIGSYS, "Bad system call" },
};

static  void    sig_handler(int sig, siginfo_t *info, void *data)
{
	ucontext_t *ucontext = (ucontext_t*)data;
	void   *bt[FUNCTION_DEPTH];
	char   **messages = (char **)NULL;
	int    bt_size;
	int    i;
#if defined(CONF_BACKTRACE_MIPS32)
	static unsigned long long pc = 0;
#endif
	signal_def *d = NULL;

	for (i = 0; i < sizeof(signal_data) / sizeof(signal_def); i++)
		if (sig == signal_data[i].id) {
			d = &signal_data[i];
			break;
		}
	if (d)
		fprintf(stderr, "\033[1;33m%s: Got signal 0x%02X (%s): %s, try to do backtrace\033[0m\n", 
		        progname, sig, signal_data[i].name, signal_data[i].description);
	else
		fprintf(stderr, "\033[1;33m%s: Got signal 0x%02X, try to do backtrace\033[0m\n", 
		        progname, sig);

#if defined(CONF_BACKTRACE_MIPS32)
	if (!pc)
		pc = ucontext->uc_mcontext.pc;
	else {
		if (pc == ucontext->uc_mcontext.pc) {
			raise(sig);
		}
		pc = ucontext->uc_mcontext.pc;
	}

	bt_size = sigbbacktrace_mips32(bt, FUNCTION_DEPTH, ucontext);
#elif defined(CONF_BACKTRACE_ARM)
	//bt_size = sigbacktrace_arm(bt, FUNCTION_DEPTH);
	bt_size = backtrace(bt, FUNCTION_DEPTH);
	printf("bt_size=%d\n",bt_size);


#else
	bt_size = backtrace(bt, FUNCTION_DEPTH);
#endif

	if (bt_size > 0) {
		messages = backtrace_symbols(bt, bt_size);
		/* skip first stack frame (points here) */
		fprintf(stderr, "\033[32;40;1m [backtrace] Execution path:\033[0m\n");
		for (i = 0; i < bt_size; ++i)
			fprintf(stderr, "\033[31;40;1m [backtrace] %s\033[0m\n", messages[i]);
	}
	fflush(stderr);
	signal (sig, SIG_DFL);
	raise (sig);
}

int sighandler_register(char *filename)
{
	struct sigaction sigact;
	int ret = 0;
    
	progname = strdup(filename);

	memset(&sigact, 0, sizeof(sigact));

	sigact.sa_sigaction = sig_handler;
	sigact.sa_flags = SA_SIGINFO|SA_NOMASK;
	if((ret = sigaction(SIGSEGV, &sigact, NULL)) < 0) {
		perror("sigaction");
		return ret;
	}
	if((ret = sigaction(SIGBUS, &sigact, NULL)) < 0) {
		perror("sigaction");
		return ret;
	}
	return 0;
}
