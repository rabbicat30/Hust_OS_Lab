#ifndef MAINWINDOW_H
#define MAINWINDOW_H
//定义资源和事件响应函数
#include <QMainWindow>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QMessageBox>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QTimer* timer;//计时器
    //int index;

private slots:
    void show_tab_Info(int index);//显示窗口信息
    //void show_tab3Info();//显示开发者信息
    void show_comm_timeInfo();//状态栏显示当前时间
    void timer_update_currentTabInfo();//更新状态信息
    void create_pid();//创建进程
    void kill_pid();//杀死进程
    void shutdown();//按键关机
    void on_tabWidget_INFO_currentChanged();//点击tab界面触发调用对应的窗口函数
    void get_boot_time();//获得系统运行时间，因为需要刷新所以单独作为函数
};
#endif // MAINWINDOW_H
