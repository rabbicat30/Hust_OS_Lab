#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QListWidget>
#include <QListWidgetItem>
#include <QStringList>
#include <QDateTime>
#include <QProcess>
#include <QInputDialog>
#include <sys/sysinfo.h>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    timer=new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(show_comm_timeInfo()));//定期更新系统时间
    connect(timer,SIGNAL(timeout()),this,SLOT(timer_update_currentTabInfo()));
    connect(ui->tabWidget_INFO,SIGNAL(currentChanged()),this,SLOT(on_tabWidget_currentChanged()));//选择不同界面触发不同函数显示对应窗口，currentChanged监测界面切换
    connect(ui->pushButton_shutdown,SIGNAL(clicked()),this,SLOT(shutdown()));
    connect(ui->CreateButton,SIGNAL(clicked()),this,SLOT(create_pid()));
    connect(ui->KillButton,SIGNAL(clicked()),this,SLOT(kill_pid()));
    timer->start(1000);
}

void MainWindow::timer_update_currentTabInfo(){
    //更新信息
    int index=ui->tabWidget_INFO->currentIndex();
    //printf("index:%d\n",index);
    //定时器刷新内存tab页面，用于进度条动态显示以及系统信息页面需要刷新运行时长
    if(index==0||index==2||index==1)
        show_tab_Info(index);
}

void MainWindow::show_comm_timeInfo(){
    //状态栏显示当前时间
    QDateTime time;
    ui->label_time->setText(time.currentDateTime().toString("yyyy")+"."+\
                                                            time.currentDateTime().toString("M")+"."+\
                                                            time.currentDateTime().toString("d")+"."+\
                                                            time.currentDateTime().toString("h")+":"+\
                                                            time.currentDateTime().toString("m")+":"+\
                                                            time.currentDateTime().toString("s"));
}
void MainWindow::show_tab_Info(int index){
    QFile tmpfile;
    QString tmpstr;
    int pos;
    if(index==0){
        //内存信息
        tmpfile.setFileName("/proc/meminfo");
        if(!tmpfile.open(QIODevice::ReadOnly)){
            QMessageBox::warning(this,tr("warning"),tr(" open /proc/meminfo error"),QMessageBox::Yes);
            return ;
        }
        QString memTotal,memFree,memUsed,swapTotal,swapFree,swapUsed;//内存/交换分区总/生于/使用
        int nmt,nmf,nmu,nst,nsf,nsu;

        while(1){
            tmpstr=tmpfile.readLine();//读取一行的信息
            pos=tmpstr.indexOf("MemTotal");//查找位置
            if(pos!=-1){
                //打开文件，发现格式如MemTotal:        2005388 kB
                //查找大小所在位置，trimmed删除多余空格，转换为数字并且转换为MB单位
                memTotal=tmpstr.mid(pos+10,tmpstr.length()-13).trimmed();//-13去掉“ kB”三个字符
                nmt=memTotal.toInt()/1024;

            }
            else if(pos=tmpstr.indexOf("MemFree"),pos!=-1){
                memFree=tmpstr.mid(pos+9,tmpstr.length()-12).trimmed();
                nmf=memFree.toInt()/1024;
            }
            else if(pos=tmpstr.indexOf("SwapTotal"),pos!=-1){
                swapTotal=tmpstr.mid(pos+11,tmpstr.length()-14).trimmed();
                nst=swapTotal.toInt()/1024;
            }
            else if(pos=tmpstr.indexOf("SwapFree"),pos!=-1){
                swapFree=tmpstr.mid(pos+10,tmpstr.length()-13).trimmed();
                nsf=swapFree.toInt()/1024;
                break;//文件中swapfree信息是最后的，所以循环从这里退出
            }
          }
          //注意该文件中并没有显示已使用的信息，需要自己计算
        nmu=nmt-nmf;
        nsu=nst-nsf;
        memUsed=QString::number(nmu,10);//转换为qstring用于ui显示（十进制）
        swapUsed=QString::number(nsu,10);
        memTotal=QString::number(nmt,10);
        memFree=QString::number(nmf,10);
        swapTotal=QString::number(nst,10);
        swapFree=QString::number(nsf,10);

        ui->label_RAM_Used->setText(memUsed+" MB");
        ui->label_RAM_Free->setText(memFree+" MB");
        ui->label_RAM_Total->setText(memTotal+" MB");
        ui->label_SWAP_Used->setText(swapUsed+" MB");
        ui->label_SWAP_Free->setText(swapFree+" MB");
        ui->label_SWAP_Total->setText(swapTotal+" MB");

        ui->progressBar_RAM->setValue(nmu*100/nmt);//使用率
        ui->progressBar_SWAP->setValue(nsu*100/nst);
        tmpfile.close();

        //stat文件中，CPU，CPU0，CPU1，CPU2，CPU3每行参数的解释为：
        // user 从系统启动开始累计到当前时刻，用户态的CPU时间，不包含nice值为负进程
        // nice 从系统启动开始累计到当前时刻，nice值为负的进程所占用的CPU时间
        // system 从系统启动开始累计到当前时刻，核心时间
        // idle 从系统启动开始累计到当前时刻，除硬盘IO等待时间以外其他等待时间
        // iowait 从系统启动开始累计到当前时刻，硬盘IO等待时间
        // irq 从系统启动开始累计到当前时刻，硬中断时间
        // softirq 从系统启动开始累计到当前时刻，软中断时间
        //steal（通常缩写为 st），代表当系统运行在虚拟机中的时候，被其他虚拟机占用的 CPU 时间
        //guest（通常缩写为 guest），代表通过虚拟化运行其他操作系统的时间，也就是运行虚拟机的 CPU 时间
        //guest_nice（通常缩写为 gnice），代表以低优先级运行虚拟机的时间
        // CPU时间等于以上时间之和
        // intr给出中断信息
        //第一行CPU是所有有后缀CPU时间之和
        //CPU的使用率=1-空闲时间（idle）/CPU总时间
        int cputotal=0,cpufree=0;//CPU总时间和空闲时间
        tmpfile.setFileName("/proc/stat");
        if(!tmpfile.open(QIODevice::ReadOnly)){
            QMessageBox::warning(this,tr("warning"),tr("The stat open failed"));
            return ;
        }
        tmpstr=tmpfile.readLine();//“cpu”和后面的数字之间有两个空格
        for(int i=0;i<7;i++){
            int t=tmpstr.section(" ",i+2,i+2).toInt();//获得时间
            cputotal += t;
            if(i==3)
                cpufree =t;
        }
        tmpfile.close();
        ui->progressBar_CPU->setValue((cputotal-cpufree)*100/cputotal);//注意两次CPU总时间由于精度的问题可能为0
    }
    else if(index==1){
        //显示进程信息
        //进程信息
        //在/proc/pid/stat中，一行参数解释为：
        //pid 应用程序或命令的名字 任务状态 ppid  第18，动态优先级 第19，静态优先级
        //在/proc/pid/stam中，按行解释为：
        //(pages为单位),任务虚拟地址空间的大小  应用程序正在使用的物理内存大小
        ui->ListWidget_process->clear();
        QDir qd("/proc");
        QStringList qlist=qd.entryList();
        QString qs=qlist.join("\n");
        QString id_of_pro;
        bool ok;
        int find_start=3;
        int a,b,propid;
        //int totalPronum;//进程总数
        QString proName;//进程名
        QString proPpid;//父进程号
        QString proPri;//进程优先级
        QString proMem;//占用内存
        QListWidgetItem* title=new QListWidgetItem("PID\t"+QString::fromUtf8("名称")+"\t\t"
                                                   +"PPID\t"
                                                   +QString::fromUtf8("优先级")+"\t"
                                                   +QString::fromUtf8("占用内存")
                                                   ,ui->ListWidget_process );
        //循环读取进程
        while(1){
            a=qs.indexOf("\n",find_start);
            b=qs.indexOf("\n",a+1);
            find_start=b;
            id_of_pro=qs.mid(a+1,b-a-1);
            //totalPronum++;
            propid=id_of_pro.toInt(&ok,10);//获取进程号
            if(!ok)
                break;
            //打开进程号对应的进程状态文件
            tmpfile.setFileName("/proc/"+id_of_pro+"/stat");
            if(!tmpfile.open(QIODevice::ReadOnly)){
                QMessageBox::warning(this,tr("warning"),tr("The pid file open failed"),QMessageBox::Yes);
                return ;
            }
            tmpstr=tmpfile.readLine();
            a=tmpstr.indexOf("(");
            b=tmpstr.indexOf(")");//应用程序名以：（名字）的形式给出
            proName=tmpstr.mid(a+1,b-a-1);
            proName.trimmed();//删除两端的空格
            proPpid=tmpstr.section(" ",3,3);//父进程pid
            proPri=tmpstr.section(" ",17,17);//动态优先级
            proMem=tmpstr.section(" ",22,22);//虚拟地址空间的大小？？？？不是实际物理空间的大小,23
            QListWidgetItem* item=new QListWidgetItem(id_of_pro+"\t"
                                                      +proName+"\t\t"
                                                      +proPpid+"\t"
                                                      +proMem+"\t"
                                                      +proMem,
                                                      ui->ListWidget_process);//这种表述有时候无法和表头对齐，无语

            tmpfile.close();
        }
        //QString tmp=QString::number(totalPronum,10);
        //ui->label_pNum->setText(tmp);

    }
    else if(index==2){
        //系统信息
        //需要显示：主机名，系统启动时间，到目前为止运行的时间，系统版本号，CPU型号和主频大小
        //分别对应文件：/proc/ sys/kernel/hostname  uptime uptime  sys/kernel/ostype-sys/kernel/osrelease  cpuinfo
        ui->ListWidget_process->clear();

        tmpfile.setFileName("/proc/sys/kernel/hostname");
        if (!tmpfile.open(QIODevice::ReadOnly) )
        {
               QMessageBox::warning(this, tr("warning"), tr("The /proc/sys/kernel/hostname open failed!"), QMessageBox::Yes);
               return ;
        }
        QString hostname =tmpfile.readLine();//主机名
        ui->label_hostname->setText(hostname);
        tmpfile.close();

        get_boot_time();
        struct sysinfo info;
        sysinfo(&info);
        struct tm* ptm=nullptr;
        ptm=gmtime(&info.uptime);//gmtime:转换时间格式，uptime:启动到目前经过的时间
        char buf[30];
        sprintf(buf,"%02d:%02d:%02d",ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
        ui->label_runtime->setText(QString(buf));

        ///proc/sys/kernel/ostype中记录系统名（Linux），osrelease中记录版本号
        tmpfile.setFileName("/proc/sys/kernel/ostype");
        if (!tmpfile.open(QIODevice::ReadOnly) )
        {
               QMessageBox::warning(this, tr("warning"), tr("The /proc/sys/kernel/ostype open failed!"), QMessageBox::Yes);
               return ;
        }
        tmpstr=tmpfile.readLine();//第一行就是
        tmpfile.close();
        tmpfile.setFileName("/proc/sys/kernel/osrelease");
        if (!tmpfile.open(QIODevice::ReadOnly) )
        {
               QMessageBox::warning(this, tr("warning"), tr("The /proc/sys/kernel/release open failed!"), QMessageBox::Yes);
               return ;
        }
        QString tmpstr2=tmpfile.readLine();
        ui->label_System_type->setText(tmpstr+" "+tmpstr2);
        tmpfile.close();

        tmpfile.setFileName("/proc/cpuinfo");
        if (!tmpfile.open(QIODevice::ReadOnly) )
        {
               QMessageBox::warning(this, tr("warning"), tr("The /proc/cpuinfo open failed!"), QMessageBox::Yes);
               return ;
        }
        while(1){
            //查找CPU型号
            tmpstr=tmpfile.readLine();
            pos=tmpstr.indexOf("model name");
            if(pos!=-1){
                QString* cpuname=new QString(tmpstr.mid(pos+13,tmpstr.length()-13));
                ui->label_CPU_type->setText(*cpuname);
            }
            else if(pos=tmpstr.indexOf("cpu MHz"),pos!=-1){
                QString* cpufrq=new QString(tmpstr.mid(pos+11,tmpstr.length()-11));
                ui->label_CPU_frq->setText(*cpufrq);
                break;
            }
        }
        tmpfile.close();
    }
}

void MainWindow::on_tabWidget_INFO_currentChanged(){
    int index=ui->tabWidget_INFO->currentIndex();
    show_tab_Info(index);
}

void MainWindow::get_boot_time(){
    struct sysinfo info;
    time_t curtime=0,boottime=0;
    struct tm* pom=nullptr;
    sysinfo(&info);
    time(&curtime);//获得当前时间返回到curtime，以秒为单位
    if(curtime>info.uptime)
        boottime=curtime-info.uptime;
    else
        boottime=info.uptime-curtime;
    pom=localtime(&boottime);
    char boot_time_buf[30];
    sprintf(boot_time_buf,"%d.%d.%d %02d:%02d:%02d",pom->tm_year+1900,pom->tm_mon+1,pom->tm_mday, pom->tm_hour,pom->tm_min,pom->tm_sec);
    ui->label_boottime->setText(QString(boot_time_buf));
}
void MainWindow::create_pid(){
    //点击按钮创建进程
    QProcess* pro=new QProcess;
    QString newproc;
    bool ok;
    newproc=QInputDialog::getText(this,"Input ","Input the name of pid",QLineEdit::Normal,"",&ok);//设置输入对话框获取进程名
    pro->start(newproc);//start以子进程方式打开外部程序，外部进程的父进程为主程序，随主程序退出而退出
}

void MainWindow::kill_pid(){
    QFile tmpfile;
    QString tmpstr;
    //点击kill按键杀死一个进程
    //QProcess pro;
    QString input_pid;
    bool ok=false;
    input_pid=QInputDialog::getText(this,"Input ","Input the pid of process", QLineEdit::Normal,"",&ok);//设置输入对话框获取进程名
    if(ok){
        tmpfile.setFileName("/proc/" + input_pid + "/stat");
        if(!tmpfile.open(QIODevice::ReadOnly)){
            QMessageBox::warning(this,tr("warning"),tr("The pid file open failed"),QMessageBox::Yes);
            return ;
        }
        tmpstr=tmpfile.readLine();
        int a=tmpstr.indexOf("(");
        int b=tmpstr.indexOf(")");//应用程序名以：（名字）的形式给出
        QString proName=tmpstr.mid(a+1,b-a-1);
        proName.trimmed();//删除两端的空格
        QString proPpid=tmpstr.section(" ",3,3);//父进程pid
        QString proPri=tmpstr.section(" ",17,17);//动态优先级
        QString proMem=tmpstr.section(" ",22,22);//虚拟地址空间的大小？？？？不是实际物理空间的大小,23
        tmpfile.close();
        int result=QMessageBox::question(this,"Kill",input_pid+"\t"+proName+"\t"+proPpid+"\t"+proPri+"\t"+proMem+"\t",QMessageBox::Yes|QMessageBox::No |QMessageBox::Cancel,QMessageBox::NoButton);
        if(result==QMessageBox::Yes){
           QString cmd=QString("kill -9 %1").arg(input_pid);//强制杀死进程
           system(cmd.toLocal8Bit().data());
        }
    }
    //pro->start(newproc);
}
void MainWindow::shutdown(){
    QProcess pro;    //通过QProcess类来执行第三方程序
    QString cmd = QString("shutdown -h 1");//linux下执行1分钟后自动关机命令
    pro.start(cmd);    //执行命令cmd
    pro.waitForStarted();
    pro.waitForFinished();
    close();    //关闭上位机
}

MainWindow::~MainWindow()
{
    delete ui;
}

