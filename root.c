#include "rte_backtrace.h"

int main(void)
{
	int ret=0;
	ret = signal_init();
	
	while(1){
		sleep(1);
	}

	return 0;
}
