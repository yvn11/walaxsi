/*
 * =====================================================================================
 *
 *       Filename:  walaxsi.c
 *
 *    Description:  a demonstration of XSI IPC system call
 *
 *
 *        Version:  1.0
 *        Created:  31/12/12 16:48:54
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zex (zex), top_zlynch@yahoo.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <linux/stat.h>
#include <linux/types.h>
#include <linux/ipc.h>
#include <linux/sem.h>
#include <linux/shm.h>
#include <linux/msg.h>

#define  FILE_NAME_LEN 128			/*  */
#define	DEFAULT_SEM_NUM 7			/*  */
#define	DEFAULT_TEST_SEM 5			/*  */
#define	DEFAULT_FLAGS (IPC_CREAT|IPC_EXCL|0666)			/*  */
#define	INIT_SEM_VALUE 3			/*  */
#define	SHM_SGM_SIZE 128			/*  */
#define	MAX_MSG_SIZE 128			/*  */
#define	DEFAULT_MSG_TYPE 17			/*  */

struct msqdata {
	long mtype;
	char mtext[MAX_MSG_SIZE];
};

char* get_file_name(int fd)
{
	if(fd == -1)
		return 0;
	char *ret = malloc(FILE_NAME_LEN);
	char *path = malloc(FILE_NAME_LEN);

	sprintf(path, "/proc/self/fd/%d", fd);
	readlink(path, ret, FILE_NAME_LEN);

	free(path);
	return ret;
}

key_t get_key(char sym, key_t *key)
{
	char* path = malloc(FILE_NAME_LEN);
	char pathtmpl[16];

	switch(sym) {
	 case 's': {
		strcpy(pathtmpl, "wala_sem_XXXXXX");
		break;
	  }
	 case 'm': {
		strcpy(pathtmpl, "wala_shm_XXXXXX");
		break;
	  }
	 case 'q': {
		strcpy(pathtmpl, "wala_msq_XXXXXX");
		break;
	  }
	 default: {
		strcpy(pathtmpl, "wala_def_XXXXXX");
	  }
	}

	strcpy(path, get_file_name(mkstemp(pathtmpl)));
	*key = ftok(path, sym);
	free(path);
	
	return *key;
}

void print_xsi_info(char* type, int id)
{
	if (type[1] == 'e') {
		struct semid_ds seminfo;

		printf("reading information of semaphore set %d\n", id);
		if((id = semctl(id, DEFAULT_TEST_SEM, SEM_STAT, &seminfo)) == -1) {
			perror("semctl");
			semctl(id, 0, IPC_RMID, 0);
			exit(1);
		}

		printf("Owner: [%d, %d]\n"
			"Permissions: [%o]\n"
			"Last semop time: %s"
			"Last change time: %s"
			"Total semaphores in set: [%u]\n\n",
			seminfo.sem_perm.uid, seminfo.sem_perm.gid,
			seminfo.sem_perm.mode, ctime(&seminfo.sem_otime),
			ctime(&seminfo.sem_ctime), seminfo.sem_nsems);
	} else if (type[1] == 'h') {
		struct shmid_ds shminfo;

		printf("reading information of shared memory segment %d\n", id);
		if(shmctl(id, IPC_STAT, shminfo) == -1) {
			perror("shmctl");
			shmctl(id, 0, IPC_RMID, 0);
			exit(1);
		}

		printf("Owner: [%d, %d]\n"
			"Permissions: [%o]\n"
			"Segment size: [%d]\n"
			"Last attach time: %s"
			"Last change time: %s"
			"Creator pid: [%d]\n"
			"Last operator pid: [%d]\n"
			"Total attaches: [%d]\n\n",
			shminfo.shm_perm.uid, shminfo.shm_perm.gid,
			shminfo.shm_perm.mode, shminfo.shm_segsz,
			ctime(&shminfo.shm_dtime), ctime(&shminfo.shm_ctime),
			shminfo.shm_cpid, shminfo.shm_lpid, shminfo.shm_nattch);
	} else if (type[1] == 's') {
		struct msqid_ds msqinfo;

		printf("reading information of message queue %d\n", id);
		if (msgctl(id, IPC_STAT, msqinfo) == -1) {
			perror("msgctl");
			msgctl(id, IPC_RMID, 0);
			exit(1);
		}

		printf("Owner: [%d, %d]\n"
			"Permissions: [%o]\n"
			"Last msgsnd time: %s"
			"Last msgrcv time: %s"
			"Last change time: %s"
			"Total messages in queue: [%d]\n"
			"Maximum bytes in queue: [%d]\n\n",
			msqinfo.msg_perm.uid, msqinfo.msg_perm.gid,
			msqinfo.msg_perm.mode, ctime(&msqinfo.msg_stime),
			ctime(&msqinfo.msg_rtime), ctime(&msqinfo.msg_ctime),
			msqinfo.msg_qnum, msqinfo.msg_qbytes);
	} else
		fprintf(stderr, "XSI type error\n");
}

void try_sem()
{
	key_t key;
	int semid, tmp_semid;
	union semun semopt;
	int i;
	struct sembuf sbuf;
	struct semid_ds seminfo;

	get_key('s', &key);

	printf("---------------- semaphore ----------------\n");
	printf("creating semaphore set with key 0x%x\n", key);
	if((semid = semget(key, DEFAULT_SEM_NUM, DEFAULT_FLAGS)) == -1) {
		perror("semget");
		exit(1);
	}
	printf("created semaphore set of id %d\n\n", semid);

	semopt.val = INIT_SEM_VALUE;

	for(i = 0; i < DEFAULT_SEM_NUM; i++) {
		semctl(semid, i, SETVAL, semopt);
	}

//	if((tmp_semid = semget(semid, 0, DEFAULT_FLAGS)) == -1) {
//		perror("semget");
//		semctl(semid, 0, IPC_RMID, 0);
//		exit(1);
//	}
//	printf("%d\n", tmp_semid);

	printf("locking semaphore %d in set %d\n",
		DEFAULT_TEST_SEM, semid);

	sbuf.sem_num = DEFAULT_TEST_SEM;
	sbuf.sem_op = -1;
	sbuf.sem_flg = IPC_NOWAIT;

	//if(semop(semid, &sbuf, SEM_UNDO) == -1) {
	if(semop(semid, &sbuf, 1) == -1) {
		perror("semop");
		semctl(semid, 0, IPC_RMID, 0);
		exit(1);
	}
	printf("locked semaphore %d in set %d\n",
		DEFAULT_TEST_SEM, semid);
	printf("value of semaphore %d: [%d]\n\n",
		DEFAULT_TEST_SEM,
		semctl(semid, DEFAULT_TEST_SEM, GETVAL, 0));

//	print_xsi_info("sem", semid);

	if((semid = semctl(semid, 0, SEM_STAT, &seminfo)) == -1) {
		perror("semctl");
		semctl(semid, 0, IPC_RMID, 0);
		exit(1);
	}

	printf("current permissions %o\n", seminfo.sem_perm.mode);
	printf("changing permissions\n");
	seminfo.sem_perm.mode = 0664;
	semopt.buf = &seminfo;
	semctl(semid, 0, IPC_SET, semopt);
	printf("current permissions %o\n\n", seminfo.sem_perm.mode);

	printf("unlocking semaphore %d in set %d\n",
		DEFAULT_TEST_SEM, semid);

	sbuf.sem_num = DEFAULT_TEST_SEM;
	sbuf.sem_op = 1;
	sbuf.sem_flg = IPC_NOWAIT;

	//if(semop(semid, &sbuf, SEM_UNDO) == -1) {
	if(semop(semid, &sbuf, 1) == -1) {
		perror("semop");
		semctl(semid, 0, IPC_RMID, 0);
		exit(1);
	}
	printf("unlocked semaphore %d in set %d\n",
		DEFAULT_TEST_SEM, semid);
	printf("value of semaphore %d: [%d]\n\n",
		DEFAULT_TEST_SEM,
		semctl(semid, DEFAULT_TEST_SEM, GETVAL, 0));

//	if(semctl(semid, 0, SEM_STAT, &seminfo) == -1) {
//		perror("semctl");
//		semctl(semid, 0, IPC_RMID, 0);
//		exit(1);
//	}

	printf("removing semaphore set %d\n", semid);
	semctl(semid, 0, IPC_RMID, 0);
	printf("removed semaphore set %d\n\n", semid);
}

void try_shm()
{
	key_t key;
	int shmid;
	int child_a, child_b;
	char* shmaddr;
	char* message = "This's message from God";
	struct shmid_ds shminfo;

	get_key('m', &key);

	printf("------------ shared memory------------\n");
	printf("creating shared memory segment with key 0x%x\n", key);
	if((shmid = shmget(key, SHM_SGM_SIZE, DEFAULT_FLAGS)) == -1) {
		perror("shmget");
		exit(1);
	}
	printf("created shared memory segment of id %d\n\n", shmid);

	printf("attaching shared memory address\n");
	if((shmaddr = (char*)shmat(shmid, 0, 0)) == (char*)-1) {
		perror("shmat");
		shmctl(shmid, 0, IPC_RMID, 0);
		exit(1);
	}
	printf("attached shared memory address at 0x%x\n", shmaddr);

	printf("writting message: [%s]\n", message);
	strcpy(shmaddr, message);
	printf("wrote message to shared memory\n\n");

	if((child_a = fork()) == -1) {
		perror("fork");
		shmctl(shmid, 0, IPC_RMID, 0);
		exit(1);
	} else if (!child_a) {
		printf("child_a [%d] in %d\n", getpid(), getppid());
		printf("reading message in child_a[%d]\n", getpid());
		printf("read message: [%s]\n\n", shmaddr);
		
		message = "Hello, this's child_a speaking";

		//print_xsi_info("shm", shmid);

		printf("attaching shared memory address in child_a[%d]\n", getpid());
		if((shmaddr = (char*)shmat(shmid, 0, 0)) == (char*)-1) {
			perror("shmat");
			shmctl(shmid, 0, IPC_RMID, 0);
			exit(1);
		}
		printf("attached shared memory address at 0x%x\n", shmaddr);
		
		strcpy(shmaddr, message);
	
		if((shmid = shmctl(shmid, SHM_STAT, &shminfo)) == -1) {
			perror("shmctl");
			shmctl(shmid, IPC_RMID, 0);
			exit(1);
		}

		printf("current permissions %o\n", shminfo.shm_perm.mode);
		printf("changing permissions\n");
		shminfo.shm_perm.mode = 0664;
		shmctl(shmid, IPC_SET, &shminfo);
		printf("current permissions %o\n\n", shminfo.shm_perm.mode);

		/* child pid retrieved privilege of shmid */
		printf("removing shared memory segment %d\n", shmid);
		if (shmctl(shmid, 0, IPC_RMID ) == -1)
			perror("shmctl, IPC_RMID");
		else 
			printf("removed shared memory segment %d\n\n", shmid);
	} else {
		wait(NULL);
		printf("This's parent process speaking\n");

		printf("read message in parent: [%s]\n\n", shmaddr);

		if ((child_b = fork()) == -1) {
			perror("fork() child_b");
			exit(1);
		} else if (!child_b) {
			printf("child_b [%d] in %d\n", getpid(), getppid());
			printf("shmid already been removed in child_a\n");
		} else {
			wait(NULL);
		}
	}
//	printf("detaching shared memory address\n");
//	if(shmdt(shmaddr) == -1) {
//		perror("shmdt");
//	}
//
//	printf("detached shared memory address\n");

//	printf("removing shared memory segment %d\n", shmid);
//	if (shmctl(shmid, 0, IPC_RMID ) == -1) {
//		perror("shmctl, IPC_RMID");
//	}
//	printf("removed shared memory segment %d\n\n", shmid);
}

void try_msq()
{
	key_t key;
	int msqid;
	int childpid;
	char* message = "What the devil is this";
	struct msqdata md;
	struct msqid_ds msqinfo;

	get_key('q', &key);

	printf("------------ message queue------------\n");
	printf("creating message queue with key 0x%x\n", key);
	if((msqid = msgget(key, DEFAULT_FLAGS)) == -1) {
		perror("msgget");
		exit(1);
	}
	printf("created message queue of id %d\n\n", msqid);

	printf("sending message: [%s] of type [%d]\n", message, DEFAULT_MSG_TYPE);
	md.mtype = DEFAULT_MSG_TYPE;
	strcpy(md.mtext, message);
	if(msgsnd(msqid, &md, strlen(md.mtext)+1, 0) == -1) {
		perror("msgsnd");
		msgctl(msqid, IPC_RMID, 0);
		exit(1);
	}
	printf("sent message\n\n");

	if((childpid = fork()) == -1) {
		perror("fork");
		msgctl(msqid, IPC_RMID, 0);
		exit(1);
	} else if (!childpid) {
		printf("childpid [%d] in %d\n", getpid(), getppid());
		printf("reading message in child process\n");
		msgrcv(msqid, &md, MAX_MSG_SIZE, DEFAULT_MSG_TYPE, 0);
		printf("read message type: [%ld]\n"
			"read message data: [%s]\n\n",
			md.mtype, md.mtext);
	
		if((msqid = msgctl(msqid, MSG_STAT, &msqinfo)) == -1) {
			perror("msgctl");
			msgctl(msqid, IPC_RMID, 0);
			exit(1);
		}

		printf("current permissions %o\n", msqinfo.msg_perm.mode);
		printf("changing permissions\n");
		msqinfo.msg_perm.mode = 0664;
		msgctl(msqid, IPC_SET, &msqinfo);
		printf("current permissions %o\n\n", msqinfo.msg_perm.mode);

		message = "Hollowing is comming";
		printf("child sending message: [%s] of type [%d]\n", message, DEFAULT_MSG_TYPE);
		md.mtype = DEFAULT_MSG_TYPE;
		strcpy(md.mtext, message);
		if(msgsnd(msqid, &md, strlen(md.mtext)+1, 0) == -1) {
			perror("msgsnd");
			msgctl(msqid, IPC_RMID, 0);
			exit(1);
		}
		printf("sent message in child process\n\n");
	} else {
		wait(NULL);
		printf("This's parent process speaking\n");

		msgrcv(msqid, &md, MAX_MSG_SIZE, DEFAULT_MSG_TYPE, 0);
		printf("read message type: [%ld]\n"
			"read message data: [%s]\n\n",
			md.mtype, md.mtext);

		printf("removing message queue %d\n", msqid);
		if (msgctl(msqid, IPC_RMID, 0) == -1) 
			perror("msgctl, IPC_RMID");
		else 
			printf("removed message queue %d\n\n", msqid);
	}
}

int main(int argc, const char *argv[])
{
	int child_a, child_b, child_c;

	if ((child_a = fork()) == -1) {
		perror("fork() child_a");
		exit(1);
	} else if (!child_a) {
		printf("try_sem() process child_a [%d] in %d\n", getpid(), getppid());
		try_sem();
	} else {
		wait(NULL);

		if ((child_b = fork()) == -1) {
			perror("fork() child_b");
			exit(1);
		} else if (!child_b) {
			printf("try_shm() process child_b [%d] in %d\n", getpid(), getppid());
			try_shm();
		} else {
			wait(NULL);

			if ((child_c = fork()) == -1) {
				perror("fork() child_c");
				exit(1);
			} else if (!child_c) {
				printf("try_msq() process child_c [%d] in %d\n",
					getpid(), getppid());
				try_msq();
			} else {
				wait(NULL);
			}
		}
	}
	return 0;
}
