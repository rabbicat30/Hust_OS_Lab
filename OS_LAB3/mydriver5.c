#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/slab.h>

#if CONFIG_MODVERSIONS ==1
#define MODVERSIONS
#endif

#define DEVICE_NUM 0  //deviece number randomly

int device_num =0;//save the device num after being created successfully
char* buffer;
int open_nr=0;//the pid num

//function declaration 
int mydriver_open(struct inode* inode, struct file* filp);
int mydriver_release(struct inode* inode, struct file* filp);
ssize_t mydriver_read(struct file* file, char __user *buf,size_t count, loff_t *f_pos);
ssize_t mydriver_write(struct file* file, const char __user* buf,size_t count,loff_t *f_pos);

//fill the entrances of struct file_operations
struct file_operations mydriver_fops = {
    .read=mydriver_read,
    .write=mydriver_write,
    .open=mydriver_open,
    .release=mydriver_release,
};

//open 
int mydriver_open(struct inode* inode, struct file* filp){
    printk("\nThe main device is %d, and the slave device is %d\n",MAJOR(inode->i_rdev),MINOR(inode->i_rdev));
    if(open_nr==0)  //the first pid
    {
        open_nr++;  //the num of pid which is usinf the dev
        try_module_get(THIS_MODULE);//add 1 for the current module
        return 0;
    }
    else{
        printk(KERN_ALERT"There has already another process open the devive\n");//guaqi
        return -1;
    }
}

//read
ssize_t mydriver_read(struct file* file, char __user *buf,size_t count, loff_t *f_pos){
    if(copy_to_user(buf,buffer,count))
        return -1;
    return count;

}

//write 
ssize_t mydriver_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos){
    if(copy_from_user(buffer,buf,count))
        return -1;
    return count;
} 

int mydriver_release(struct inode *inode, struct file* filp){
    open_nr--;
    printk("The device is released\n");
    module_put(THIS_MODULE);
    return 0;
}

//init
static int __init mydriver_init(void){
    printk(KERN_ALERT "Init the device\n");
    buffer =  (char *) kmalloc(1024, GFP_KERNEL);
    int result;
    result=register_chrdev(0,"mydriver5",&mydriver_fops);
    if(result<0){
        printk(KERN_WARNING "mydriver:register failed\n");
        return -1;
    }
    else{
        printk("mydriver: register successfully\n");
        device_num=result;
        return 0;
     }
}

//exit
static void __exit mydriver_exit(void){
    printk(KERN_ALERT "Unregister...\n");
    unregister_chrdev(device_num,"mydriver5");
    kfree(buffer);
    printk("Unregister successfully\n");
}

module_init (mydriver_init);
module_exit (mydriver_exit);

MODULE_LICENSE("GPL");
