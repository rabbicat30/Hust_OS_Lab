#include "put.h"
#include "ui_put.h"
#include<QTimer>
Put::Put(QWidget *parent):
    QWidget(parent),
    ui(new Ui::Put)
{
    ui->setupUi(this);
    ui->setupUi(this);
    /*获取信号灯*/
    key=ftok("./",66);
    semid=semget(key,4,0666);

    /*打开原文件*/
    readfile=fopen("/home/lm/untitled/output.txt","wb");
    if(readfile==NULL){
        printf("Open Output.txt failed\n");
        exit(0);
     }

    /*获取共享内存*/
    key=ftok("../",68);      //应该是t缓冲区对应的文件
    shmid=shmget(key,sizeof (BUF),0666);
    bufdata=(BUF*)shmat(shmid,0,0);


    /*start后每秒调用一次GetBuf*/
    QTimer *timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(PutBuf()));
    timer->start(TIME);
}

void Put::PutBuf(){
    if(flag==0){
        P(semid,2);
        count++;
        QString putstr = "PUT: ";
        ui->label->setText(putstr.append(QString::number(count)));
        fwrite(bufdata->buf,sizeof(char),bufdata->size,readfile);
        if(bufdata->state==1){
            fclose(readfile);
            flag=1;
        }
        V(semid,3);
    }
}

Put::~Put(){
    if(ui)
        delete ui;
}
