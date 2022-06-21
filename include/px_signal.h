#ifndef PX_SIGNAL
#define PX_SIGNAL
#include<sys/sem.h>
union semun {
	int val;                        /* Value for SETVAL */
	struct semid_ds* buf;            /* Buffer for IPC_STAT, IPC_SET */
	unsigned short* array;        /* Array for GETALL, SETALL */
	struct seminfo* __buf;         /* Buffer for IPC_INFO
	(Linux-specific) */
};

class px_signal {
public:
	px_signal(key_t signalkey, unsigned short num);
	~px_signal();
	void down();
	void up();
private:
	short unsigned int semid;
	struct sembuf sem_opt_down;
	struct sembuf sem_opt_up;
};


#endif // !PX_SIGNAL
