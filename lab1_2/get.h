#ifndef GET_H
#define GET_H

#include "main.h"
#include <QWidget>
QT_BEGIN_NAMESPACE
namespace Ui { class Get; }  //用于描述界面组件
QT_END_NAMESPACE
class Get: public QWidget    //实现一个窗体类
{
    Q_OBJECT

public:
    explicit Get(QWidget *parent = nullptr);
    ~Get();

public slots:
    void GetBuf();  //获取缓冲区内容函数

private:
    Ui::Get *ui;             //指针 ui 是指向可视化设计的界面，后面会看到要访问界面上的组件，都需要通过这个指针 ui。
    //在定义实现誊抄进程的成员变量
    int count=0;
    int flag=0;
    FILE* writefile;
    int filesize;
    int shmid=0;  //在初始化时使用方括号清零，就可以不用用到memset函数
    int semid;
    key_t key;
    BUF* bufdata={0};
};

#endif // GET_H
