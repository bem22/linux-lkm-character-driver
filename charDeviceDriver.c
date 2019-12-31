#include <linux/init.h>    // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>  // Core header for loading LKMs into the kernel
#include <linux/device.h>  // Header to support the kernel Driver Model
#include <linux/kernel.h>  // Contains types, macros, functions for the kernel
#include <linux/fs.h>      // Header for the Linux file system support
#include <linux/uaccess.h> // Required for the copy to user function
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/delay.h>

// Own includes
#include "charDeviceDriver.h"
#include "ioctl.h"

MODULE_LICENSE("GPL");                                              ///< The license type -- this affects available functionality
MODULE_AUTHOR("Mihai Emilian Buduroi meb648@student.bham.ac.uk");   ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux char driver to read and write"); ///< The description -- see modinfo
MODULE_VERSION("0.1");                                              ///< A version number to inform users

// These functions serve the custom linked list
static Node *pop(list *l, unsigned int index)
{
    struct Node *head = l->node;

    // Cut down index until reaching the desired position
    while (head)
    {
        if (index)
        {
            head = head->next;
            index--;
        }
        else
        {
            return head;
        }
    }

    // If you are here, the node does not exist
    return NULL;
}

static int push(list *l, Node *n)
{
    if (!n)
    {
        return -1;
    }

    // Do we have a node in the list?
    if (l->node)
    {
        n->next = l->node; // head:tail
    }
    else
    {
        n->next = NULL; // head:null
    }

    // Now make the list point to the head
    l->node = n;

    return 0;
}

static int remove_element(list *l, unsigned int index)
{
    struct Node *previous;
    struct Node *head;

    previous = NULL;
    head = l->node;

    // Swap head until index
    while (head)
    {
        // Is the index !0 and we have more nodes?
        if (index)
        {
            previous = head;
            head = head->next;
            index--;
        }
        else
        {
            if (previous)
            {
                previous->next = head->next;
            }
            else
            {
                l->node = head->next;
            }
            // Free memory, assign NULL pointer
            kfree(head->string);
            head->string = NULL;
            kfree(head);
            head = NULL;

            // Return success
            return 0;
        }
    }

    // No head? No problem!
    return -1;
}

static void init(list *l)
{
    l->node = NULL;
}

/* This used to be the deallocator function
 * Unfortunately deep recursion gives kernel panic
    static void destroyNode(Node *n) {
        if(n) {
            destroyNode(n->next);
            kfree(n->string);
            n->string = NULL;
            kfree(n);
            n = NULL;
        }
    }

    static void destroy(list *l) {
        if(l) { destroyNode(l->node); }
        l = NULL;
    }
 */

// This deallocator is itterative
static void destroy2(list *l)
{
    Node *head = l->node;
    Node *_next;
    while (head)
    {
        _next = head->next;
        kfree(head->string);
        kfree(head);
        head = _next;
    }
}

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init opsysmem_init(void)
{
    printk(KERN_INFO "charDeviceDriver: Initializing the charDeviceDriver LKM\n");

    if (SLEEP)
        msleep(300);

    // Try to dynamically allocate a major number for the device -- more difficult but worth it
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0)
    {
        printk(KERN_ALERT "charDeviceDriver failed to register a major number\n");
        return majorNumber;
    }

    printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, majorNumber);

    // Register the device class
    opsysmemClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(opsysmemClass))
    {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(opsysmemClass); // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "charDeviceDriver: device class registered correctly\n");

    // Initialize binary semaphore
    sema_init(&sem, 1);

    // Initialize the empty list of messages
    init(&l);

    printk(KERN_INFO "charDeviceDriver: device class created correctly\n"); // Made it! device was initialized

    return 0;
};

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit opsysmem_exit(void)
{
    // Deallocate the list of messages
    down(&sem); 
    destroy2(&l);
    up(&sem);

    class_unregister(opsysmemClass);             // unregister the device class
    class_destroy(opsysmemClass);                // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME); // unregister the major number
    printk(KERN_INFO "charDeviceDriver: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "opsysmem: Device successfully opened\n");
    return 0;
}

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    Node *msg;
    int ret;
    int error_count = 0;
    char *message;

    // Entering critical section
    down(&sem); //wait state

    if (SLEEP) { msleep(500); }

    msg = pop(&l, 0);

    // No message? No wait!
    if (!msg)
    {
        up(&sem);
        return -EAGAIN;
    }

    /* If the tool you use has a smaller buffer size than the length
     * of the message. This helps with avoiding ugly overflows
     */
    if (len < msg->length)
    {
        up(&sem);
        return -EINVAL;
    }

    ret = msg->length;

    if (DEBUG)
    {
        message = (char *)kmalloc(msg->length + 1, GFP_KERNEL);
        memset(message, 0, msg->length + 1);
        memcpy(message, msg->string, msg->length);
        message[msg->length] = '\0';
        printk(KERN_INFO "Write to user: %s", message);
        kfree(message);
    }

    // Write from kernel space to user space
    error_count = copy_to_user(buffer, msg->string, msg->length);

    if (error_count == 0)
    {
        current_size -= msg->length;
        if (DEBUG)
        {
            printk(KERN_INFO "List size decreased to (bytes):%d, %u \n", current_size, msg->length);
            printk(KERN_INFO "Current size: %d %zu", current_size, len);
        }

        remove_element(&l, 0);

        up(&sem); // Success and exit critical section

        return ret;
    }
    else
    {
        up(&sem); // Fail and exit critical section

        if (DEBUG)
        {
            printk(KERN_INFO "opsysmem: Failed to send %d characters to the user\n", error_count);
        }

        return -EFAULT; // Failed -- return a bad address message (i.e. -14)
    }
}

/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    Node *n;

    char *message; // Debug purposes only

    // Stop messages larger than 2 * 1024 bytes and empty strings
    if (len > MAX_MESSAGE_SIZE || len == 0)
    {
        return -EINVAL;
    }

    n = kmalloc(sizeof(Node), GFP_KERNEL);

    if (!n)
    {
        return -EAGAIN;
    } // Unsuccessful to allocate bytes for Node

    n->string = (char *)kmalloc(len, GFP_KERNEL);

    // Unsuccessful to alocate bytes for string
    if (!n->string)
    {
        kfree(n);
        return -EAGAIN;
    }

    n->length = len;

    // Write from user space to kernel space
    copy_from_user(n->string, buffer, len);

    down(&sem); // Enter critical section

    /* If the message will lead to overflowing 2MiB
     * deallocate the bytes for node and string and
     * release the semaphore
     */
    if (current_size + len > MAX_LIST_SIZE)
    {
        kfree(n->string);
        kfree(n);
        up(&sem);
        return -EAGAIN;
    }

    if (DEBUG)
    {
        message = (char *)kmalloc(n->length + 1, GFP_KERNEL);
        memset(message, 0, n->length + 1);
        memcpy(message, n->string, n->length);
        printk(KERN_INFO "Read from user: %s, with size: %d", message, n->length);
        kfree(message);
    }

    current_size += len;

    if (DEBUG)
    {
        printk(KERN_INFO "current_size:%d\n", current_size);
    }

    // Finally push the node to the list
    push(&l, n);

    up(&sem); // Exit critical section

    return len;
}

static long dev_ioctl(struct file *file,
                      unsigned int ioctl_num,
                      unsigned long ioctl_param)
{

    int new_size = ioctl_param;

    if (!(ioctl_num == MODIFY_MEMORY && new_size > 0))
    {
        return -EINVAL;
    }

    down(&sem); // Enter critical section

    if (new_size > MAX_LIST_SIZE || new_size > current_size)
    {
        MAX_LIST_SIZE = new_size;
        if (DEBUG)
        {
            printk(KERN_INFO "The new size is %u", MAX_LIST_SIZE);
        }
        up(&sem); // Exit critical section with success
        return 0;
    }
    else
    {
        up(&sem); // Exit critical section with error
        return -EINVAL;
    }
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "opsysmem: Device successfully closed\n");

    return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(opsysmem_init);
module_exit(opsysmem_exit);
