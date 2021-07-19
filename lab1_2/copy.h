#ifndef COPY_H
#define COPY_H

#include "main.h"
#include <QWidget>
QT_BEGIN_NAMESPACE
namespace Ui { class Copy; }  //用于描述界面组件
QT_END_NAMESPACE
class Copy: public QWidget    //实现一个窗体类
{
    Q_OBJECT

public:
    explicit Copy(QWidget *parent = nullptr);
    ~Copy();

public slots:
    void CopyBuf();  //获取缓冲区内容函数

private:
    Ui::Copy *ui;             //指针 ui 是指向可视化设计的界面，后面会看到要访问界面上的组件，都需要通过这个指针 ui。
    //在定义实现誊抄进程的成员变量
    int count=0;
    int flag=0;
    int shmid_s=0;  //在初始化时使用方括号清零，就可以不用用到memset函数
    int shmid_t=0;
    int semid;
    key_t key,key_s,key_tt;
    BUF* bufdata_s={0};
    BUF* bufdata_t={0};
};

#endif // COPY_H
