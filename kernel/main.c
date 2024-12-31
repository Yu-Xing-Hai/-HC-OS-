#include "print.h"
#include "init.h"

void main(void) {
	put_str("I am kernel\n");
	init_all();
   	asm volatile("sti"); 
   	/*the directive of "sti" is open interrupt, 
   	it set eflags's IF to 1,
   	so CPU can process the interrupt signal from 8259A.*/
	while(1);
}