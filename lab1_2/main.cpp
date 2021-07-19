#include "mainwindow.h"
#include <QApplication>
#include "main.h"
#include "get.h"   
#include "put.h"
#include "copy.h"
int main(int argc, char *argv[])
{
    printf("main\n");
    //实现三个进程的誊抄
    //设置两个缓冲区s和t，get将文件读到缓冲区s中，copy将数据从s中读到t中，put将t中的数据读到输出文件中
        int shmid_s=0;
        int shmid_t=0;

        key_t key_s=ftok("./",68);
        key_t key_tt=ftok("../",68);
        if((shmid_s=shmget(key_s,sizeof(BUF),IPC_CREAT|0666))==-1){
            puts("shm_s creat failed\n");
            exit(1);
        }
        printf("shmget\n");
         BUF * buff = (BUF *)shmat(shmid_s, NULL, 0);
         buff->size = 0;
         buff->state=0;
         if((shmid_t=shmget(key_tt,sizeof(BUF),IPC_CREAT|0666))==-1){
             puts("shm_t creat failed!\n");
             exit(1);
        }
         buff = (BUF *)shmat(shmid_t, NULL, 0);
         buff->size = 0;
         buff->state=0;
        printf("shm finished\n");

        //创建信号灯集
        int sem_id;
        key_t key_sem=ftok("./",66);
        if((sem_id=semget(key_sem,4,IPC_CREAT|0666))==-1){
            puts("sem created failed\n");
            exit(1);
        }
        union semun arg;
        //s信号灯
        arg.val=0;//sfull
        semctl(sem_id,0,SETVAL,arg);
        arg.val=1;//sempty
        semctl(sem_id,1,SETVAL,arg);
        //t信号灯
        arg.val=0;//tfull
        semctl(sem_id,2,SETVAL,arg);
        arg.val=1;//tempty
        semctl(sem_id,3,SETVAL,arg);
        printf("sem finished\n");

        /*创建子进程*/
        static pid_t p1;
        if((p1=fork())==0){                 //the first
            printf("pid1 get create\n");
            QApplication a(argc, argv); //定义并创建应用程序
            QString title="Get";
            Get w;  //定义并创建窗口
            w.show();   //显示窗口
            a.exec();   //应用程序运行
            exit(0);
        }
        static pid_t p2;
        if((p2=fork())==0){             //the second
                printf("pid2 copy create\n");
                QApplication a(argc, argv); //定义并创建应用程序
                QString title="Copy";
                Copy w;  //定义并创建窗口
                w.show();   //显示窗口
                a.exec();   //应用程序运行
                exit(0);
            }

        static pid_t p3;
        if((p3=fork())==0){         //the third
            printf("pid3 put create\n");
            QApplication a(argc, argv); //定义并创建应用程序
            QString title="Put";
            Put w;  //定义并创建窗口
            w.show();   //显示窗口
            a.exec();   //应用程序运行
            exit(0);
        }

        printf("main create\n");
        QApplication a(argc, argv); //定义并创建应用程序                        //the father
        /*删除信号灯*/
        semctl(sem_id,0,IPC_RMID,arg);
        semctl(sem_id,1,IPC_RMID,arg);
        semctl(sem_id,2,IPC_RMID,arg);
        semctl(sem_id,3,IPC_RMID,arg);


         /*删除共享内存*/
        shmctl(shmid_s,IPC_RMID,0);
        shmctl(shmid_t,IPC_RMID,0);
        return a.exec();



}
