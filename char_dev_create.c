#include <linux/module.h>  
#include <linux/types.h>  
#include <linux/fs.h>  
#include <linux/errno.h>  
#include <linux/mm.h>  
#include <linux/sched.h>  
#include <linux/init.h>  
#include <linux/cdev.h>
#include <asm/device.h>  //下面这三个头文件是由于动态创建需要加的
#include <linux/device.h>
#include <asm/io.h>  
#include <asm/uaccess.h>  
#include <linux/timer.h>  
#include <asm/atomic.h>  
#include <linux/slab.h>  
#include <linux/device.h>  
#include <linux/kernel.h>

#include "debug.h"
#include "char_dev_create.h"

static int major;
static struct cdev char_net_cdev;
static struct class *cls;

extern struct char_net_priv net_priv; 

static int char_net_open(struct inode *inode, struct file *filp)
{
    debug_print("%s()\n",__func__);
    
    return 0;
}

static int char_net_close(struct inode *inode, struct file *filp)
{
    debug_print("%s()\n",__func__);
    
    return 0;
}

static ssize_t char_net_read(struct file *filp,char *buf,size_t count,loff_t *offp)
{
    debug_print("%s()\n",__func__);
    
    return 0;
}

static ssize_t char_net_write(struct file *filp,char *buf,size_t count,loff_t *offp)
{
    debug_print("%s()\n",__func__);
    
    return 0;
}

static long char_net_ioctl(struct file *file,unsigned int cmd, unsigned long arg)
{
    debug_print("%s()\n",__func__);
    
    return 0;
}

static struct file_operations char_net_fops = 
{
    .owner = THIS_MODULE,
    .open = char_net_open,
    .release = char_net_close,
    .read = char_net_read,
    .write = char_net_write,
    .unlocked_ioctl = char_net_ioctl,

};

int char_dev_create(void)
{
    debug_print("%s()\n",__func__);
    
    dev_t devid;
    int ret = -1;

    if(major)
    {
        devid = MKDEV(major,0);
        ret = register_chrdev_region(devid,1,"char_net");
    }
    else
    {
        ret = alloc_chrdev_region(&devid,0,MAX_DEVICE_NUM,"char_net");
        if(ret < 0)
        {
            printk("error alloc ,ret is %d\n",ret);
            return -1;
        }
        major = MAJOR(devid);
    }
    
    debug_print("devid is %d,major is %d\n",devid,major);
    cdev_init(&char_net_cdev,&char_net_fops);
    cdev_add(&char_net_cdev,devid,1);
    
    
    cls = class_create(THIS_MODULE,"char_net");
    device_create(cls,NULL,MKDEV(major,0),NULL,"char_net0");
    
    return 0;
}

int char_dev_destroy(void)
{
    
    debug_print("%s()\n",__func__);
    
    device_destroy(cls,MKDEV(major,0));
    class_destroy(cls);
    
    cdev_del(&char_net_cdev);
    unregister_chrdev_region(MKDEV(major,0),1);
    
    return 0;
}

