// SPDX-License-Identifier: GPL OR Unlicense
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/platform_device.h>

#define DEVICE_NAME "dma_buffer"
#define BUFFER_SIZE (PAGE_SIZE * 256) // 1MB

#define DMA_BUF_IOCTL_GET_PHYS_ADDR _IOR (0xF0, 1, uint32_t)

static dev_t dev_num;
static struct cdev c_dev;
static struct class *cl;
static void *dma_virt_addr;
static dma_addr_t dma_handle;
static struct platform_device *dma_pdev;

static int done_chrdev_region = 0;
static int done_cdev_add = 0;
static int done_dev_reg = 0;
static int done_cl_create = 0;
static int done_dev_create = 0;
static int done_dma_alloc = 0;


static long dma_buffer_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    int result = 0;
    switch (cmd) {
        case DMA_BUF_IOCTL_GET_PHYS_ADDR:
            if(copy_to_user((void __user *)arg, &dma_handle, sizeof(dma_handle))) result = -EFAULT;
            break;
        default:
          result = -EINVAL;
    }
    return result;
}

static int dma_buffer_mmap(struct file *filp, struct vm_area_struct *vma){
    int ret;
    ret = dma_mmap_coherent(&dma_pdev->dev, vma, dma_virt_addr, dma_handle, BUFFER_SIZE);
    if (ret) {
        pr_err("dma_mmap_coherent failed\n");
        return ret;
    }
    return 0;
}

static int dma_buffer_open(struct inode *inode, struct file *filp){
    return 0;
}

static int dma_buffer_release(struct inode *inode, struct file *filp){
    return 0;
}

static struct file_operations dma_buffer_fops = {
    .owner = THIS_MODULE,
    .open = dma_buffer_open,
    .release = dma_buffer_release,
    .unlocked_ioctl = dma_buffer_ioctl,
    .mmap = dma_buffer_mmap,
};

static void close_all(void){
    if(done_dma_alloc) dma_free_coherent(&dma_pdev->dev, BUFFER_SIZE, dma_virt_addr, dma_handle);
    if(done_dev_create) device_destroy(cl, dev_num);
    if(done_cl_create) class_destroy(cl);
    if(done_dev_reg) platform_device_unregister(dma_pdev);
    if(done_cdev_add) cdev_del(&c_dev);
    if(done_chrdev_region) unregister_chrdev_region(dev_num, 1);
}

static int __init dma_buffer_init(void){

    int ret;
    
    if ((ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME)) < 0) {
        pr_err("Failed to allocate chrdev region\n");
        return ret;
    }
    done_chrdev_region = 1;

    cdev_init(&c_dev, &dma_buffer_fops);

    if ((ret = cdev_add(&c_dev, dev_num, 1)) < 0) {
        pr_err("Failed to add cdev\n");
        close_all();
        return ret;
    }
    done_cdev_add = 1;

    dma_pdev = platform_device_register_simple("dma_dummy", -1, NULL, 0);
    if (IS_ERR(dma_pdev)) {
        pr_err("Failed to register platform device\n");
        close_all();
        return PTR_ERR(dma_pdev);
    }
    done_dev_reg = 1;

    
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    cl = class_create("dma_buffer_class");
#else
    cl = class_create(THIS_MODULE, "dma_buffer_class");
#endif

    if (IS_ERR(cl)) {
        pr_err("Failed to create class\n");
        close_all();
        return PTR_ERR(cl);
    }
    done_cl_create = 1;

    
    if (IS_ERR(device_create(cl, NULL, dev_num, NULL, DEVICE_NAME))) {
        pr_err("Failed to create device\n");
        close_all();
        return -1;
    }
    done_dev_create = 1;

    
    dma_virt_addr = dma_alloc_coherent(&dma_pdev->dev, BUFFER_SIZE, &dma_handle, GFP_KERNEL);
    if (!dma_virt_addr) {
        pr_err("Failed to allocate DMA buffer\n");
        close_all();
        return -ENOMEM;
    }
    done_dma_alloc = 1;

    pr_info("DMA Buffer: loaded");
    pr_info("DMA buffer: virt_addr=%p, phys_addr=%pad\n", dma_virt_addr, &dma_handle);

    return 0;
}

static void __exit dma_buffer_exit(void){
    close_all();
    pr_info("DMA Buffer: unloaded\n");
}

module_init(dma_buffer_init);
module_exit(dma_buffer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ksjo");
MODULE_DESCRIPTION("DMA Buffer Kernel Module");
