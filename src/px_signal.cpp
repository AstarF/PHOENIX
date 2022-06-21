#include "px_signal.h"
#include "px_log.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
px_signal::px_signal(key_t signalkey, unsigned short num){

	semid = semget(signalkey, 1, IPC_CREAT | 0666);
    union semun sem_args;
    sem_args.val = num;
    sem_opt_down = { 0, -1, SEM_UNDO };
    sem_opt_up = { 0, 1, SEM_UNDO };
    int ret = semctl(semid, 0, SETVAL, sem_args);//0代表对1个信号来量初始化，即有1个资源
    if (-1 == ret)
    {
        px_log::get_log_instance().write("semctl failed.");
        printf("errno %d\n", errno);
    }
}

px_signal::~px_signal(){
//do nothing
}

//int current = 8;

void px_signal::down() {
    //current--;
    //px_log::get_log_instance().write(current);
    int ret = semop(semid, &sem_opt_down, 1);
    if (-1 == ret)
    {
        px_log::get_log_instance().write("semop failed.");
        printf("errno %d\n", errno);
    }
}

void px_signal::up() {
    //current++;
    //px_log::get_log_instance().write(current);
    int ret = semop(semid, &sem_opt_up, 1);
    if (-1 == ret)
    {
        px_log::get_log_instance().write("semop failed.");
    }
}