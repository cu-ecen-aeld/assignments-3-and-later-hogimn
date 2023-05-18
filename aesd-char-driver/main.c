/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Your Name Here"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    struct aesd_dev *dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    struct aesd_dev *dev = filp->private_data;
    if (mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS;
    }

    int bytes_remained = count;
    size_t entry_offset_byte_rtn;
    // Find the buffer entry for the given offset
    struct aesd_buffer_entry *entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &entry_offset_byte_rtn);
    if (!entry) {
        retval = -EFAULT;
        goto release;
    }

    // Calculate the number of bytes to read from the current entry
    int bytes_to_read = entry->size - entry_offset_byte_rtn;
    if (bytes_to_read > bytes_remained) {
        // If the remaining bytes to read is less than the bytes available in the entry,
        // adjust the bytes_to_read
        bytes_to_read = bytes_remained;
    }

    // Copy to the userspace buffer
    if (copy_to_user(buf, entry->buffptr + entry_offset_byte_rtn, bytes_to_read)) {
        retval = -EFAULT;
        goto release;
    }

    *f_pos += bytes_to_read;
    retval += bytes_to_read;

release:
    mutex_unlock(&dev->lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    struct aesd_dev *dev = filp->private_data;
    if (mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS;
    }

    // Allocate memory for the buffer entry or resize it if already allocated
    if (dev->entry.buffptr) {
        dev->entry.buffptr = (char *)krealloc(dev->entry.buffptr, dev->entry.size + count, GFP_KERNEL);
    } else {
        dev->entry.buffptr = (char *)kmalloc(sizeof(char) * count, GFP_KERNEL);
    }

    if (dev->entry.buffptr == NULL) {
        retval = -ENOMEM;
        goto release;
    }

    // Copy data from the userspace buffer
    if (copy_from_user((void *)dev->entry.buffptr + dev->entry.size, buf, count)) {
        retval = -EFAULT;
        goto release;
    }

    dev->entry.size += count;
    retval = count;

    // Check if the buffer entry contains a newline character (end of input)
    if (memchr(dev->entry.buffptr, '\n', dev->entry.size)) {
        // If the buffer at in_offs index is already allocated, free it
        if (dev->buffer.entry[dev->buffer.in_offs].buffptr) {
            kfree(dev->buffer.entry[dev->buffer.in_offs].buffptr);
            dev->total_bytes -= dev->buffer.entry[dev->buffer.in_offs].size;
        }

        // Add the current entry to the circular buffer
        aesd_circular_buffer_add_entry(&dev->buffer, &dev->entry);
        dev->total_bytes += dev->entry.size;
        dev->entry.buffptr = NULL;
        dev->entry.size = 0;
    }

release:
    mutex_unlock(&dev->lock);
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
