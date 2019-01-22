//lock.h
#ifndef _LOCK_H_
#define _LOCK_H_
#define	NLOCKS 50		
#define	NPROC		30		/*  allowed if not already done	*/	
#define	DELETED		-6	
	
#define LOCK_NOT_AVAILABLE  49
#define LOCK_AVAILABLE 52
#define LOCK_BLOCKED 51

#define READ 12
#define WRITE 13

#define WRITER 22
#define READER 23
#define PROCESS 24

#define CURRENT -1
#define WAIT 1
#define NOTREL 0

struct lockAttributes{
	int type;
	int user;
	int priority; 
	unsigned long waitTime;
};
struct lock_table{
	int lockState; 
	struct lockAttributes lockAttri[NPROC];
};
extern struct lock_table lock_table_entry[];
#endif