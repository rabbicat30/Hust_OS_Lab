//finish the file copy 
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include<stdlib.h>
int main (int argc, char* argv[]){
    //the true format is the srcfile path, the des path  
    if(argc<3){
        //the args inputed is less
        printf("Please Input More Args!\n");
        return -1;
    }
    const char* srcfile=argv[1];
    const char* desfile=argv[2];
    int fsrc=open(srcfile,O_RDONLY);
    if(fsrc == -1){
        perror("read error");
        exit(1);
    }
    int fdes=open(desfile,O_CREAT|O_WRONLY,0777);
    if(fdes==-1){
        perror("write error");
        exit(1);
    }
    struct stat statbuf;
    stat(srcfile,&statbuf);
    int filesize=statbuf.st_size;   //获得要读取的文件的大小
    printf("filesize=%d\n",filesize);

    char buf[filesize];
    int count=read(fsrc,buf,filesize);
    if(count==-1){
        perror("Read error! The file is empty!");
        exit(1);
    }
    if(write(fdes,buf,filesize)==-1){
        perror("Write Error!");
        exit(1);
    }
    close(fsrc);
    close(fdes);
}
