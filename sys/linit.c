#include <stdio.h>
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <lock.h>
extern struct lock_table_entry;
void lockInit(void){
	int iterator;
	for ( iterator = 0; iterator < NLOCKS; ++iterator)
	{
		lock_table_entry[iterator].lockState = LOCK_NOT_AVAILABLE;
	}
}
