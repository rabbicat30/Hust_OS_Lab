#ifndef MAIN_H
#define MAIN_H
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#define LENGTH 100
#define TIME 100 //设定界面的触发时间，每1s更新一次
union semun {
    int val; /* value for SETVAL */
    struct semid_ds *buf; /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* array for GETALL, SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void * __pad;
};
static void P(int semid,int index){
    struct sembuf sem;
    sem.sem_num=index;      //要操作的信号灯的编号置为index
    sem.sem_op=-1;          //执行-1操作
    sem.sem_flg=0;
    semop(semid,&sem,1);
    return ;
}

static void V(int semid,int index){
    struct sembuf sem;
    sem.sem_num=index;
    sem.sem_op=1;
    sem.sem_flg=0;
    semop(semid,&sem,1);
    return ;
}

typedef struct BUFF{     //定义缓冲区块结构
    int size;   //当前缓冲区的大小
    int state;  //判断是否为最后一次读取
    char buf[LENGTH];   //缓冲区内容

}BUF;

#endif // MAIN_H
