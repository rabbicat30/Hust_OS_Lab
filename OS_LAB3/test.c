#include <sys/types.h> 
#include <sys/stat.h> 
#include <string.h> 
#include <stdio.h> 
#include <fcntl.h> 
#include <stdlib.h>
#include <unistd.h>

int main(void){
    char in[1024],out[1024];
    memset(in,0,sizeof(in));
    memset(out,0,sizeof(out));
    printf("Input the string:\n");
    gets(in);

    int fd=open("/dev/mydriver5",O_RDWR,S_IRUSR|S_IWUSR);//open the device
    if(fd>0){
        read(fd,&out,sizeof(out));
        printf("Before innput, the message in mydriver is: %s\n",out);

        write(fd,in,sizeof(in));//put the input to device
        memset(out,0,sizeof(out));//must memset
        read(fd,out,sizeof(out));//get the buf from the device and print
        printf("After input, the message changed to: %s\n",out);
        sleep(1);
    }
    else{
        printf("Failed\n");
        return -1;
    }
    close(fd);
    return 0;
}
