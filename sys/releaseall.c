#include <stdio.h>
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>

int releaseall (int numlocks, ...){
        STATWORD ps;
        disable(ps);
        unsigned long *a;
        int lcount=numlocks;
    a = (unsigned long *)(&numlocks) + (numlocks);
    int releaseLockArray[NLOCKS];
    int count=0;
	while(numlocks>0){
         releaseLockArray[count]=*a--;
         count++;
	numlocks--;
    }
    int error = 0;
    int i;       
    for(i=0;i<lcount;i++){
        releaseLock(currpid, releaseLockArray[i],0);
        }
	if(proctab[currpid].p_locks[releaseLockArray[i]]==0){
		
                restore(ps);
				return SYSERR;
        }
        restore(ps);
        return OK;
}

int releaseLock(int pid, int ldes,int killed){
        struct lock_table *lptr;
        
       lptr=&lock_table_entry[ldes];
		lptr->lockAttri[pid].user=NOTREL;
                        lptr->lockAttri[pid].priority = -1;
                        proctab[pid].p_locks[ldes]=0;
                        updatePriorities(pid,ldes,killed);
		int isLast = 1;
        if(lock_table_entry[ldes].lockAttri[pid].type==READ){
                int pIter;
                for (pIter = 0; pIter < NPROC; ++pIter)
                {
                        if(lptr->lockAttri[pIter].type==READ && pIter!=pid && lptr->lockAttri[pIter].user==CURRENT){
                                isLast = 0;
                        }
                }
		if(isLast){
        int nextWriter=getHighestPriority(lptr,WRITER);
        int nextReader = getHighestPriority(lptr,READER);
        if(nextReader != -1  && nextWriter != -1){
			if( lptr->lockAttri[nextReader].priority > lptr->lockAttri[nextWriter].priority){
				wakeAllReadersGreaterThanPriority(lptr,nextWriter);
			} else { 
						lptr->lockState=LOCK_BLOCKED;
                        lptr->lockAttri[nextWriter].user=CURRENT;
                        lptr->lockAttri[nextWriter].waitTime = 0;
                        updateLockPriorities(nextWriter,ldes,killed);
                        proctab[nextWriter].pstate = PRREADY;
                        ready(nextWriter, RESCHNO);
			}
		} else if(nextWriter != -1 ){

			lptr->lockState=LOCK_BLOCKED;
                        lptr->lockAttri[nextWriter].user=CURRENT;
                        lptr->lockAttri[nextWriter].waitTime = 0;
                        updateLockPriorities(nextWriter,ldes,killed);
                        proctab[nextWriter].pstate = PRREADY;
                        ready(nextWriter, RESCHNO);
		} else if(nextReader != -1){
			wakeAllReadersGreaterThanPriority(lptr,-1);
		} else {
			lptr->lockState=LOCK_AVAILABLE;
		}
        }
		return 1;
	} else if(lock_table_entry[ldes].lockAttri[pid].type==WRITE){

	int nextWriter=getHighestPriority(lptr,WRITER);
        int nextReader = getHighestPriority(lptr,READER);
		if(nextReader < nextWriter){
			lptr->lockState=LOCK_BLOCKED;
				lptr->lockAttri[nextWriter].user=CURRENT;
				lptr->lockAttri[nextWriter].waitTime = 0;
				updateLockPriorities(nextWriter,ldes,killed);
				proctab[nextWriter].pstate = PRREADY;
				ready(nextWriter, RESCHNO);	
			} else {
				wakeAllReadersGreaterThanPriority(lptr,-1);
				
			}
	return 1;
}
return 1;
}


int getHighestPriority(struct lock_table *lptr,int type){
		int readerPID = -1;
        int readerPrio=-1;
        int pIter;
		
        for (pIter = 0; pIter < NPROC; ++pIter)
        {
				if(type==READER){
				if(lptr->lockAttri[pIter].type==READ && lptr->lockAttri[pIter].user==WAIT){
                        if(readerPrio < lptr->lockAttri[pIter].priority){
                                readerPID = pIter;
                                readerPrio = lptr->lockAttri[pIter].priority;
                        }
                        else if(readerPrio == lptr->lockAttri[pIter].priority&&readerPrio!=-1){
                                if(lptr->lockAttri[readerPID].waitTime>lptr->lockAttri[pIter].waitTime){
                                        readerPID=pIter;
                                }
                        }
                }	
				}else{
					if(lptr->lockAttri[pIter].type==WRITE && lptr->lockAttri[pIter].user==WAIT){
                        if(readerPrio < lptr->lockAttri[pIter].priority){
                                readerPID = pIter;
                                readerPrio = lptr->lockAttri[pIter].priority;
                        }
                        else if(readerPrio == lptr->lockAttri[pIter].priority&&readerPrio!=-1){
                                if(lptr->lockAttri[readerPID].waitTime>lptr->lockAttri[pIter].waitTime){
                                        readerPID=pIter;
                                }
                        }
					}
					
				}
        } 
		return readerPID;
}

int wakeAllReadersGreaterThanPriority(struct lock_table *lptr,int priority){
	int pIter;
	for (pIter = 0; pIter < NPROC; ++pIter){
			if(lptr->lockAttri[pIter].type==READ && lptr->lockAttri[pIter].user==WAIT && lptr->lockAttri[pIter].priority>= priority){
					
					lptr->lockAttri[pIter].waitTime = 0;
					lptr->lockState=LOCK_BLOCKED;
					proctab[pIter].pstate = PRREADY;
					lptr->lockAttri[pIter].user=CURRENT;
					ready(pIter, RESCHNO);
			}
	}
}


void updatePriorities(int pid, int LD, int killed){

        struct pentry *pptr,*npptr;
        pptr=&proctab[pid];

        struct lock_table *lptr;
        lptr=&lock_table_entry[LD];
        if(killed!=1){
              if(pptr->pinherited!=pptr->pprio){
                        chprio(pid,pptr->pinherited);
                        pptr->pinherited=-1;
                }
                
                int lIterator;
                for (lIterator = 0; lIterator < NLOCKS; ++lIterator){
                        if(pptr->p_locks[lIterator]==1)
                        {
                            
                                updateLockPriorities(pid,lIterator,killed);
                        }
                }  

        }else{
            int pIterator;
                int highWaitProcPrio=-1;
				int i;
					for (i = 0; i < NPROC; ++i)
					{
					if(lock_table_entry[LD].lockAttri[i].user==WAIT  && lock_table_entry[LD].lockAttri[i].priority>highWaitProcPrio && i!=pid){
							highWaitProcPrio= proctab[i].pprio;
						}
					}
					if(pptr->pprio > highWaitProcPrio){
                        for (pIterator = 0; pIterator < NPROC; pIterator++){
                                if(lptr->lockAttri[pIterator].user!=NOTREL && proctab[pIterator].pinherited!=-1){
                                        if(proctab[pIterator].pinherited<highWaitProcPrio){
                                                chprio(pIterator,highWaitProcPrio);
                                        }else{
                                                chprio(pIterator,proctab[pIterator].pinherited);
                                                proctab[pIterator].pinherited = -1;
                                        }
                                        int lockIterator;
                                        npptr=&proctab[pIterator];
                                        for(lockIterator=0;lockIterator<NLOCKS;lockIterator++){
                                                if(npptr->p_locks[lockIterator]==1)
                                                updatePriorities(pIterator,lockIterator,1);
                                        }
                                }
                        }
                }    	
        }
        }

