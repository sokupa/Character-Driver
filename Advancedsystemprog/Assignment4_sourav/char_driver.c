#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h> // file_operations structure
#include <linux/cdev.h> // char driver : to register char device to kernel
#include <linux/device.h>
#include<linux/ioctl.h>
#include <asm/uaccess.h> // copy to user, copy to kernel
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/list.h>
#include <linux/string.h>
#define MYDEV_NAME "mycdrv"
#define SIZE 16*PAGE_SIZE
MODULE_LICENSE("GPL");            //license type -- this affects available functionality
MODULE_AUTHOR("sourav parmar");    
MODULE_DESCRIPTION("A simple Linux char driver for ASP");  
MODULE_VERSION("0.1");   
#define ASP_CHGACCDIR _IOW('Z', 1, int)
struct asp_mycdrv {
    struct list_head list;
    struct cdev dev;
    char *ramdisk;
    struct semaphore sem;
    int devNo;
    int dir; // 0 is forward 1 is reverse
};
static struct list_head head ;

static int Major;

static struct asp_mycdrv mycdev;
static ssize_t mycdrv_read(struct file *, char *, size_t, loff_t *);
static ssize_t mycdrv_write(struct file *,const char *, size_t, loff_t *);
static long mycdrv_ioctl(struct file *, unsigned int, unsigned long);
static loff_t mycdrv_llseek(struct file *filp, loff_t off, int whence);
static int mycdrv_open(struct inode *inode, struct file *file);
static int mycdrv_release(struct inode *inode, struct file *file);

struct class * myclass;
static int noOfDevices=3;
module_param(noOfDevices,int,S_IRUSR|S_IWUSR);

static struct file_operations fops={
    .owner = THIS_MODULE,
    .unlocked_ioctl=mycdrv_ioctl,
    .read=mycdrv_read,
    .llseek=mycdrv_llseek,
    .write=mycdrv_write,
    .open=mycdrv_open,
    .release=mycdrv_release
};

static int __init mymodule_init(void)
{
    int i = 0 ;
    struct asp_mycdrv *curr=NULL;
    struct list_head *listcrawl=NULL;
    INIT_LIST_HEAD(&head);
    Major = register_chrdev(0,MYDEV_NAME,&fops);
    //mycdev.devNo = Major;
    printk(KERN_DEBUG "Major Num :  %d\n", Major);
    if(Major < 0)
        printk(KERN_DEBUG"Error:Major Number Not Assigned\n");
    else
    {
        //struct asp_mycdrv *curr=NULL;
        myclass = class_create(THIS_MODULE,"chrdrv");
        register_chrdev_region(Major, noOfDevices, MYDEV_NAME);
        while( i < noOfDevices)
        {
            curr=kmalloc(sizeof(struct asp_mycdrv),GFP_KERNEL);
            if(!curr)
            {
                printk(KERN_DEBUG"Memory Allocation Failed for Dev\n");
                return -1;
            }
            curr->ramdisk =  kmalloc(SIZE,GFP_KERNEL);
            if( !(curr->ramdisk))
            {
                printk(KERN_DEBUG"Memory Allocation Failed for Ramdisk\n");
                return -1;
            }
            memset(curr->ramdisk , 0 ,SIZE);
            curr->devNo = i;
            curr->dir = 0;// init with forward direction
            sema_init(&(curr->sem),1);
            (curr->dev).dev = MKDEV(Major, i);
            list_add(&(curr->list),&head);
            i++;
        }
        i = 0 ;

        while(i < noOfDevices)
        {
            device_create(myclass,NULL,MKDEV(Major,i),NULL,"mycdrv%i",i);
            i++;
        }
        i = 0 ;
        list_for_each(listcrawl,&head) {
            struct asp_mycdrv* crawler = list_entry(listcrawl ,struct asp_mycdrv,list);
            cdev_init(&(crawler->dev),&fops);
            cdev_add(&(crawler->dev),MKDEV(Major,i++),1);
        }
    }

    return 0;
}

static void __exit mymodule_exit(void)
{
    int i = 0 ;
    struct asp_mycdrv *crawler  =NULL;
    struct list_head *curr = NULL;
    struct list_head *Next = NULL;
    // free in reverse order of allocation
    list_for_each(curr,&head) {
        crawler =list_entry(curr,struct asp_mycdrv,list);
        if(crawler && crawler->ramdisk)
            kfree(crawler->ramdisk); //free allocated ramdisk
        cdev_del(&(crawler->dev));//delete device
    }
    while( i < noOfDevices)
    {
        device_destroy(myclass,MKDEV(Major,i));
        ++i;
    }
    class_destroy(myclass);
    unregister_chrdev_region(Major,noOfDevices);
    unregister_chrdev(mycdev.devNo,MYDEV_NAME);
    list_for_each_safe(curr,Next,&head) {
        crawler=list_entry(curr,struct asp_mycdrv,list);
        list_del(curr);
        kfree(crawler);
    }
}
module_init(mymodule_init);
module_exit(mymodule_exit);

//Define fileops
static long mycdrv_ioctl(struct file *filp, unsigned int num, unsigned long param)
{
    struct asp_mycdrv* mycdev  = filp->private_data;
    int prev = 0;
    //int* cur = NULL;
    down(&(mycdev->sem));
    prev = mycdev->dir;
    switch(num) {
    case ASP_CHGACCDIR:
        mycdev->dir = *(int*)param;
        break;
    }
    up(&(mycdev->sem));
    printk(KERN_DEBUG"My Direction: Before=%d, new direction=%d\n", prev, mycdev->dir);
    return prev;
}

static loff_t mycdrv_llseek(struct file *file, loff_t offset, int whence)
{
    loff_t testpos;
    struct asp_mycdrv* mycdev  = file->private_data;
    down(&(mycdev->sem));
    switch (whence) {
    case SEEK_SET:
        testpos = offset;
        break;
    case SEEK_CUR:
        testpos = file->f_pos + offset;
        break;
    case SEEK_END:
        testpos = SIZE + offset;
        break;
    default:
        return -EINVAL;
    }
    testpos = testpos < SIZE ? testpos : SIZE;
    testpos = testpos >= 0 ? testpos : 0;
    file->f_pos = testpos;
    printk(KERN_DEBUG"Seeking to pos=%ld\n", (long)testpos);
    up(&(mycdev->sem));
    return testpos;
}


static ssize_t mycdrv_read(struct file * filep, char __user * buf, size_t sz, loff_t * ppos)
{
    struct asp_mycdrv* mycdev  = filep->private_data;
    char *myramdisk = mycdev->ramdisk;
    int nbytes =  0;
    int direction = 0;
    int i = 0 ;
    char* copybuff = kmalloc(SIZE,GFP_KERNEL);
    memset(copybuff,0,SIZE);
    direction = mycdev->dir;
    printk(KERN_DEBUG"Before READING function, nbytes=%d, pos=%d, sz=%d\n", nbytes, (int)*ppos, (int)sz);
    if(!direction)
    {
        if( *ppos+sz < SIZE)
            nbytes =  /*sz*/ strlen(myramdisk+*ppos) -copy_to_user( buf,myramdisk+*ppos , sz);
        *ppos += nbytes;

    }
    else
    {
        for( i = 0 ; (*ppos -i) >=0 && myramdisk[*ppos -i];++i)
            copybuff[i] = myramdisk[*ppos -i]; //start from char before '\0'
        copybuff[i] = '\0';
        nbytes =  /*sz*/strlen(copybuff) -copy_to_user(buf,copybuff,sz);
        *ppos -= nbytes;
        *ppos = *ppos>=0?*ppos:0;
    }

    printk(KERN_DEBUG"Reading to buf data %s \t copybuff %s \t%s \n",mycdev->ramdisk,copybuff, buf);
    printk(KERN_DEBUG"Finished READING function, nbytes=%d, pos=%d, sz=%d\n", nbytes, (int)*ppos, (int)sz);
    kfree(copybuff);
    return nbytes;
}
static int mycdrv_open(struct inode *inode, struct file *filep)
{    
    struct asp_mycdrv* mycdev = NULL;
    struct list_head *curr=NULL;
    printk(KERN_ALERT"OPEN \n");
    list_for_each(curr,&head) {
        mycdev=list_entry(curr,struct asp_mycdrv,list);
        if((MAJOR((mycdev->dev).dev)==imajor(inode)) && (MINOR((mycdev->dev).dev)==iminor(inode)))
        {
            filep->private_data = mycdev;
            break;
        }
    }

    if(mycdev == NULL)
    {
        printk(KERN_DEBUG"Device Not Found\n");
        return -1;
    }
    return 0;
}

static int mycdrv_release(struct inode *inode, struct file *file)
{
    return 0;
}
static ssize_t mycdrv_write(struct file *filp,const char *buf, size_t sz, loff_t *ppos)
{

    struct asp_mycdrv* mycdev  = NULL;
    int direction = 0,nbytes = 0,j=0;
    char* copybuff = NULL;
    mycdev = filp->private_data ;
    down(&(mycdev->sem));
    printk(KERN_ALERT"Writing 1\n");
    copybuff = kmalloc(SIZE,GFP_KERNEL);
    direction = mycdev->dir;
    memset(copybuff,0,SIZE);
    copy_from_user(copybuff,buf,sz);
    printk(KERN_DEBUG"Before Write offset ppos=%d filp->f_pos=%d current str=%s buf str =%s copybuff=%s\n",(int)*ppos, (int)filp->f_pos ,mycdev->ramdisk,buf, copybuff);
    if(strlen(copybuff) <= SIZE)
    {
        printk(KERN_ALERT"Writing 2\n");
        if(!direction) // forward direction
        {
            for(j = 0 ; j <strlen(copybuff) ;++j)
                mycdev->ramdisk[*ppos+j] = copybuff[j];
            *ppos += j;
        }
        else
        {
            for( j = 0; j<strlen(copybuff) && *ppos-j>=0 ;++j)
                mycdev->ramdisk[*ppos-j] = copybuff[j];
            *ppos = *ppos-j>=0?(*ppos-j):0;
        }
    }
    nbytes = j;
    printk(KERN_DEBUG"After Write offset ppos=%d filp->f_pos=%d current str=%s \n",(int)*ppos, (int)filp->f_pos ,mycdev->ramdisk);
  
    kfree(copybuff);
    up(&(mycdev->sem));
    return nbytes;
}
