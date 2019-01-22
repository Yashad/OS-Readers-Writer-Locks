#include <stdio.h>
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <lock.h>
#include <proc.h>

extern struct lock_table_entry;

extern int ctr1000;

int lock(int ldes1, int type, int priority) {
  STATWORD ps;
  disable(ps);
  if (lock_table_entry[ldes1].lockState == LOCK_NOT_AVAILABLE) {
	   restore(ps);
    return DELETED;
  } 
 struct lock_table * lptr;
  lptr = & lock_table_entry[ldes1];
  struct pentry * pptr;
  pptr = & proctab[currpid];
 /*The lock algorthm where we update the lock table according to the conditions*/
    switch (type) {
    case WRITE:
     
      if (lptr -> lockState == LOCK_AVAILABLE) {
        lptr -> lockState = LOCK_BLOCKED;
        lptr -> lockAttri[currpid].type = WRITE;
        lptr -> lockAttri[currpid].user = CURRENT;
        lptr -> lockAttri[currpid].priority = priority;
        pptr -> p_locks[ldes1] = 1;
        updateLockPriorities(currpid, ldes1, 0);
        restore(ps);
        return OK;
      } else {
        //Lock is blocked 
        lptr -> lockAttri[currpid].type = WRITE;
        lptr -> lockAttri[currpid].user = WAIT;
        lptr -> lockAttri[currpid].priority = priority;
        lptr -> lockAttri[currpid].waitTime = ctr1000;
        pptr -> pstate = PRWAIT;
        pptr -> p_locks[ldes1] = 1;
        
        updateLockPriorities(currpid, ldes1, 0);
        resched();
        restore(ps);
        return pptr -> pwaitret;
      }
      break;
    case READ:
      if (lptr -> lockState == LOCK_AVAILABLE) {
        lptr -> lockState = LOCK_BLOCKED;
        lptr -> lockAttri[currpid].type = READ;
        lptr -> lockAttri[currpid].user = CURRENT;
        lptr -> lockAttri[currpid].priority = priority;
        pptr -> p_locks[ldes1] = 1;
       
        updateLockPriorities(currpid, ldes1, 0);
        restore(ps);
        return OK;
      } else {
		int i;
		int writePriority=0;
		for (i = 0; i < NPROC; ++i) {
			if (lock_table_entry[ldes1].lockAttri[i].user == CURRENT && lock_table_entry[ldes1].lockAttri[i].type == WRITE) {
			writePriority=lock_table_entry[ldes1].lockAttri[i].priority;
			}
		}
        if (writePriority == 0) {
		  int highestWritePrio;
          highestWritePrio = getHighestWait(ldes1,WRITER);
          if (priority >= highestWritePrio) {
         
         //   kprintf("Lock Busy. Priority Greater than waiting Writers. Acquired\n %d", highestWritePrio);
            lptr -> lockAttri[currpid].type = READ;
            lptr -> lockAttri[currpid].user = CURRENT;
            lptr -> lockAttri[currpid].priority = priority;

            
            pptr -> p_locks[ldes1] = 1;
          

            updateLockPriorities(currpid, ldes1, 0);
           

            restore(ps);
            return OK;

          } else {
            lptr -> lockAttri[currpid].type = READ;
            lptr -> lockAttri[currpid].user = WAIT;
            lptr -> lockAttri[currpid].priority = priority;
            lptr -> lockAttri[currpid].waitTime = ctr1000;
            pptr -> pstate = PRWAIT;
            pptr -> p_locks[ldes1] = 1;
            updateLockPriorities(currpid, ldes1, 0);
            resched();
            restore(ps);
            return pptr -> pwaitret;
          }
        } else {
          //kprintf("Lock busy. But less than waiting writer. Wait \n");
          lptr -> lockAttri[currpid].type = READ;
          lptr -> lockAttri[currpid].user = WAIT;
          lptr -> lockAttri[currpid].priority = priority;
          lptr -> lockAttri[currpid].waitTime = ctr1000;
          //resched
          pptr -> pstate = PRWAIT;
          pptr -> p_locks[ldes1] = 1;
         
          updateLockPriorities(currpid, ldes1, 0);

          resched();

          restore(ps);
          return pptr -> pwaitret;
        }
      }
    break;
    default:
    restore(ps);
    return SYSERR;
    break;

  }

restore(ps);
return SYSERR;
}

void updateLockPriorities(int pid, int lockDescriptor, int beingKilled) {

  struct pentry * pptr, * npptr;
  pptr = & proctab[pid];

  struct lock_table * lptr;
  lptr = & lock_table_entry[lockDescriptor];
  if (lptr -> lockAttri[pid].user != WAIT) {

    
    int highWaitProcPrio = getHighestWait(lockDescriptor,PROCESS);

    if (pptr -> pprio < highWaitProcPrio) {
      if (proctab[pid].pinherited == -1) {
        proctab[pid].pinherited = proctab[pid].pprio;
      }
      chprio(pid, highWaitProcPrio);
      int lockIterator;
      for (lockIterator = 0; lockIterator < NLOCKS; lockIterator++) {
        npptr = & proctab[pid];
        if (npptr -> p_locks[lockIterator] == 1)
          updateLockPriorities(pid, lockIterator, 0);
      }
    }
  } else {
    int pIterator;
   // kprintf("Waiting Process %s %d being pinheritederited in %d\n", proctab[pid].pname, pid, lockDescriptor);
    for (pIterator = 0; pIterator < NPROC; pIterator++) {
      if (lptr -> lockAttri[pIterator].user == CURRENT && pptr -> pprio > proctab[pIterator].pprio) {
        if (proctab[pIterator].pinherited == -1) {
          proctab[pIterator].pinherited = proctab[pIterator].pprio;
        }
        chprio(pIterator, pptr -> pprio);
        int lockIterator;
        for (lockIterator = 0; lockIterator < NLOCKS; lockIterator++) {
          npptr = & proctab[pIterator];
          if (npptr -> p_locks[lockIterator] == 1)
            updateLockPriorities(pIterator, lockIterator, beingKilled);
        }
      }

    }
  }
}

int getHighestWait(int lockDescriptor, int type) {
    int retVal = -1;
    int i;
	for (i = 0; i < NPROC; ++i) {
			if (type == WRITER) {
        
            if (lock_table_entry[lockDescriptor].lockAttri[i].user == WAIT && lock_table_entry[lockDescriptor].lockAttri[i].type == WRITE && lock_table_entry[lockDescriptor].lockAttri[i].priority > retVal) {
                retVal = lock_table_entry[lockDescriptor].lockAttri[i].priority;
            }
			}
     else {
     
            if (lock_table_entry[lockDescriptor].lockAttri[i].user == WAIT && lock_table_entry[lockDescriptor].lockAttri[i].priority > retVal) {
                retVal = proctab[i].pprio;
            }
         }
	}
    return retVal;
}
