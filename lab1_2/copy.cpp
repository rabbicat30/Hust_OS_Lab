#include "copy.h"
#include "ui_copy.h"
#include <QTimer>
//类的成员函数
Copy::Copy(QWidget *parent):
    QWidget(parent),
    ui(new Ui::Copy)
{
    ui->setupUi(this);
    ui->setupUi(this);

    /*获取信号灯*/
    key=ftok("./",66);
    semid=semget(key,4,0666);

    /*获取共享内存*/
    key_s=ftok("./",68);
    key_tt=ftok("../",68);
    shmid_s=shmget(key_s,sizeof (BUF),0666);
    shmid_t=shmget(key_tt,sizeof (BUF),0666);
    bufdata_s=(BUF*)shmat(shmid_s,0,0);
    bufdata_t=(BUF*)shmat(shmid_t,0,0);

    /*start后每秒调用一次CopyBuf*/
     QTimer *timer = new QTimer(this);
     connect(timer,SIGNAL(timeout()),this,SLOT(CopyBuf()));
     timer->start(TIME);
}

void Copy::CopyBuf(){
    if(flag==0){
        P(semid,0);     //sfull -1
        P(semid,3);     //tempty -1
        count++;
        QString getstr="Copy: ";
        ui->label->setText(getstr.append(QString::number(count)));
        memcpy(bufdata_t->buf,bufdata_s->buf,bufdata_s->size);//将缓冲区s中的内容复制到缓冲区t中
        bufdata_t->size=bufdata_s->size;
        bufdata_t->state=bufdata_s->state;
        if(bufdata_t->state==1)  //判断结束
            flag=1;
        V(semid,1);     //sempty +1
        V(semid,2);     //tfull +1
    }
}

Copy::~Copy(){
    if(ui)
        delete ui;
}
