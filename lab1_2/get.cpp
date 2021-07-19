#include "get.h"
#include "ui_get.h"
#include <QTimer>
//类的成员函数
Get::Get(QWidget *parent):
    QWidget(parent),
    ui(new Ui::Get)
{
    ui->setupUi(this);
    ui->setupUi(this);
    /*获取信号灯*/
    key=ftok("./",66);
    semid=semget(key,4,0666);

    /*打开原文件*/
    writefile=fopen("/home/lm/untitled/input.txt","rb");
    if(writefile==NULL){
        printf("No File\n");
        exit(0);
    }
    struct stat statbuf;
    stat("/home/lm/untitled/input.txt",&statbuf);
    filesize=statbuf.st_size;   //获得要读取的文件的大小
    printf("filesize:%d\n",filesize);

    /*获取共享内存*/

     key=ftok("./",68);
     shmid=shmget(key,sizeof(BUF),0666);
     bufdata=(BUF*)shmat(shmid,0,0);

    /*start后每秒调用一次GetBuf*/
     QTimer *timer = new QTimer(this);
     connect(timer,SIGNAL(timeout()),this,SLOT(GetBuf()));
     timer->start(TIME);

}

void Get::GetBuf(){
    printf("get flag:%d\n",flag);
    if(flag==0){        //确保在读完最后一个缓冲区之后不再继续读
        P(semid,1);
        printf("Get %d\n",count);
        memset(bufdata->buf,'\0',LENGTH);   //清零，防止后面数据不完全覆盖导致len有问题
        bufdata->size=bufdata->state=0;
        count++;
        QString getstr="Get: ";
        ui->label->setText(getstr.append(QString::number(count)));
        int len=fread(bufdata->buf,sizeof (char),LENGTH,writefile);
        filesize -=len;
        printf("len %d, filesize: %d\n",len,filesize);
        if(filesize==0){
            //当在读完当前缓冲区内容后文件剩余大小为0，则当前缓冲区是最后一个被读的缓冲区
            bufdata->state=1;
            bufdata->size=len;
            flag=1;
            fclose(writefile);
            V(semid,0);
        }
        else{
            bufdata->size=LENGTH;
            bufdata->state=0;

            V(semid,0);
        }
    }
}

Get::~Get(){
    if(ui)
        delete  ui;
}



