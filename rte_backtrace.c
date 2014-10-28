#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <signal.h>
#include <dlfcn.h>

extern unsigned long linux_syms_addresses[] __attribute__((weak, section("data")));
extern unsigned long linux_syms_num __attribute__((weak, section("data")));
extern char linux_syms_names[] __attribute__((weak, section("data")));

static const char *const siglist[] = {
	"Signal 0",
	"SIGHUP",
	"SIGINT",
	"SIGQUIT",
	"SIGILL",
	"SIGTRAP",
	"SIGABRT/SIGIOT",
	"SIGBUS",
	"SIGFPE",
	"SIGKILL",
	"SIGUSR1",
	"SIGSEGV",
	"SIGUSR2",
	"SIGPIPE",
	"SIGALRM",
	"SIGTERM",
	"SIGSTKFLT",
	"SIGCHLD",
	"SIGCONT",
	"SIGSTOP",
	"SIGTSTP",
	"SIGTTIN",
	"SIGTTOU",
	"SIGURG",
	"SIGXCPU",
	"SIGXFSZ",
	"SIGVTALRM",
	"SIGPROF",
	"SIGWINCH",
	"SIGIO",
	"SIGPOLL",
	NULL
};

static unsigned long rte_get_func_index(unsigned long addr)
{
	unsigned long low, high, mid;
	if((addr<linux_syms_addresses[0])||(addr>linux_syms_addresses[linux_syms_num-1])){
		return 0;
	}
	low=0;
	high=linux_syms_num;
	while(high-low>1){
		mid=(low+high)/2;
		if(linux_syms_addresses[mid]<=addr)
			low = mid;
		else
			high = mid;
	}
	if((low==0)||((linux_syms_num-1)==low)){
		return 0;
	}

	return low;
}

static char *rte_get_dlname(unsigned long epc, unsigned long *base)
{
	int ret;
	Dl_info info;

	ret =dladdr((void *)epc, &info);
	if(ret){
		*base = (unsigned long)info.dli_fbase;
		return (char *)info.dli_fname;
	}
	return NULL;
}

static char *rte_get_func_name(unsigned long idx)
{
	char *tmp = NULL;
	int len = 0;
	int i=0;
	tmp = linux_syms_names;
	len = *tmp;
	tmp++;
	for(i=0;i<idx;i++){
		tmp += len;
		len = *tmp;
		tmp++;
	}
	return tmp;
}

static unsigned long get_addr_from_string(char *str)
{
	unsigned long addr=0;
	char *tmp=str;	
	char val=0;
	int start=0;
	while(*tmp != ']'){
		if(start==0){
			if((*tmp=='[')&&(*(tmp+1)=='0')&&(*(tmp+2)=='x')){
				start = 1;
				tmp = tmp+2;
			}
		}else{
			val = *tmp;
			if((val>='0')&&(val<='9')){
				val = val - '0';
			}else if((val>='a')&&(val<='f')){
				val = val - 'a' + 10;
			}
			addr <<= 4;
			addr |= val;
		}
		tmp++;
	}
	return addr;
}

static void rte_show_backtrace(ucontext_t *ucp, int sig)
{
	char *func_name=NULL;	
	unsigned long epc; 
	unsigned long func_index=0, func_addr=0;
	unsigned long dl_base=0;
	int start_flag=0;
	mcontext_t *context=NULL;
	void *bt[32];
	int bt_size;
	char **bt_sym;
	int i;

	context = &(ucp->uc_mcontext);
	epc = context->gregs[REG_RIP];
	if(sig!=SIGUSR2){
		printf("host_daemon exception for %s(%d)\n", siglist[sig], sig);
		printf("RIP:%16lx EFL:%16lx ERR:%16lx TRAPNO:%16lx\n", 
				(unsigned long)context->gregs[REG_RIP], (unsigned long)context->gregs[REG_EFL], 
				(unsigned long)context->gregs[REG_ERR], (unsigned long)context->gregs[REG_TRAPNO]);
		printf("RAX:%16lx RBX:%16lx RCX:%16lx RDX:%16lx\n", 
				(unsigned long)context->gregs[REG_RAX], (unsigned long)context->gregs[REG_RBX],
				(unsigned long)context->gregs[REG_RCX], (unsigned long)context->gregs[REG_RDX]);
		printf("RSI:%16lx RDI:%16lx RSP:%16lx RBP:%16lx\n", 
				(unsigned long)context->gregs[REG_RSI], (unsigned long)context->gregs[REG_RDI],
				(unsigned long)context->gregs[REG_RSP], (unsigned long)context->gregs[REG_RBP]);
		printf("R8 :%16lx R9 :%16lx R10:%16lx R11:%16lx\n", 
				(unsigned long)context->gregs[REG_R8], (unsigned long)context->gregs[REG_R9],
				(unsigned long)context->gregs[REG_R10], (unsigned long)context->gregs[REG_R11]);
		printf("R12:%16lx R13:%16lx R14:%16lx R15:%16lx\n", 
				(unsigned long)context->gregs[REG_R12], (unsigned long)context->gregs[REG_R13],
				(unsigned long)context->gregs[REG_R14], (unsigned long)context->gregs[REG_R15]);
		printf("CSGSFS:%16lx\n", (unsigned long)context->gregs[REG_CSGSFS]);
	}
	printf("Backtrace:\n");
	bt_size = backtrace(bt, 32);
	bt_sym = backtrace_symbols(bt, bt_size);
	for(i=0;i<bt_size;i++){
		func_addr = get_addr_from_string(bt_sym[i]);
		if(func_addr==epc){
			start_flag = 1;
		}
		if(start_flag){
			func_index = rte_get_func_index(func_addr);
			if(0==func_index){
				func_name = rte_get_dlname(epc, &dl_base);
				printf("\t[<0x%lx>] %s\n", epc-dl_base, func_name);
				} else{
					func_name = rte_get_func_name(func_index);
					printf("\t[<0x%lx>] %s\n", func_addr, func_name);
				}
		}
	}
	if(start_flag==0){
		func_index = rte_get_func_index(epc);
		if(0==func_index){
			func_name = rte_get_dlname(epc, &dl_base);
			printf("\t[<0x%lx>] %s\n", dl_base, func_name);
		} else{
			func_name = rte_get_func_name(func_index);
			printf("\t[<0x%lx>] %s\n", epc, func_name);
		}
	}
	free(bt_sym);
}

static void sigsem_int(int sig, siginfo_t *sig_info, void *uc)
{
	if(uc){
		rte_show_backtrace((ucontext_t *)uc, sig);
	}
	return;
}

static void sigsem_exec(int sig, siginfo_t *sig_info, void *uc)
{
	struct sigaction sa;
	if(uc){
		rte_show_backtrace((ucontext_t *)uc, sig);
	}
	if(sig == SIGUSR2){
		return;
	}
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(sig, &sa, NULL);
	raise(sig);

	return;
}

static void *signal_set(int sig, void (*func)(int, siginfo_t *, void *))
{
	struct sigaction action;
	struct sigaction old;
	action.sa_sigaction = func;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_RESTART | SA_SIGINFO;
	if(sigaction(sig, &action, &old)<0){
		return SIG_ERR;
	}else{
		return old.sa_handler;
	}
}

int signal_init(void)
{
	signal_set(SIGINT, sigsem_int);
	signal_set(SIGSEGV, sigsem_exec);
	signal(SIGPIPE, SIG_IGN);
	signal_set(SIGBUS, sigsem_exec);
	signal_set(SIGILL, sigsem_exec);
	signal_set(SIGFPE, sigsem_exec);
	signal_set(SIGABRT, sigsem_exec);
	signal_set(SIGUSR2, sigsem_exec);

	return 0;
}
