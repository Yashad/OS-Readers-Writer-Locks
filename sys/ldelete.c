#include <stdio.h>
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>

int ldelete (int lockdescriptor){
	STATWORD ps;
	disable(ps);

	lock_table_entry[lockdescriptor].lockState = LOCK_NOT_AVAILABLE;
	int pIter;
	for (pIter = 0; pIter < NPROC; ++pIter)
	{
		if(lock_table_entry[lockdescriptor].lockAttri[pIter].user!=NOTREL){
			lock_table_entry[lockdescriptor].lockAttri[pIter].user=NOTREL;
			lock_table_entry[lockdescriptor].lockAttri[pIter].priority=-1;
			proctab[pIter].p_locks[lockdescriptor]=0;
			proctab[pIter].pwaitret = DELETED;
			updatePriorities(pIter,lockdescriptor,0);
		}
		if(proctab[pIter].pstate == PRWAIT){
			ready(pIter,RESCHNO);
		}
	}
	resched();
	restore(ps);
	return OK;
}