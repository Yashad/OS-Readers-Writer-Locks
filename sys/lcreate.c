#include <stdio.h>
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
extern struct lock_table_entry;
int lcreate (void){
	STATWORD ps;
	disable(ps);

	struct lock_table *lptr;
	   int nextLock = 0;
	int i;
	for(i=0;i<NLOCKS;i++){
		if(lock_table_entry[nextLock].lockState==LOCK_NOT_AVAILABLE){
			lock_table_entry[nextLock].lockState = LOCK_AVAILABLE;
			lptr=&lock_table_entry[nextLock];
			int j;
			for (j = 0; j < NPROC; ++j)
			{
				lptr->lockAttri[j].user=NOTREL;
				lptr->lockAttri[j].priority=-1;
			}
			restore(ps);
			return nextLock;
		}
		nextLock = (nextLock+1)%NLOCKS;
	}
	restore(ps);
	return SYSERR;
}