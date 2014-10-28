#include <unistd.h>
#include "rte_backtrace.h"

int main(void)
{
	signal_init();
	
	while(1){
		sleep(1);
	}

	return 0;
}
