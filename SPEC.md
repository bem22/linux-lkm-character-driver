
These are the specifications for a device driver for a character device that implements a simple way of message passing. 
* The kernel maintains a list of messages. 
* To limit memory usage, a limit of 4KiB = 4*1024 bytes for each message is imposed, along with a limit of the size of all messages, which is initially 2MiB = 2*1024*1024 bytes.

## Device driver operations

The device driver performs the following operations:

- Creating the device (*/dev/opsysmem*), creates an empty list of messages.
- Removing the device deallocates all messages and removes the list of messages.
- Reading from the device returns one message, and removes this message from the kernel list. 
    - If the list of messages is empty, the reader returns **-EAGAIN**.
- Writing to the device stores the message in kernel space and adds it to the list if the message is below the maximum size, and the limit of the size of all messages wouldn't be surpassed with this message. 
    - If the message is too big, **-EINVAL** is returned
    - If the limit of the size of all messages was surpassed, **-EAGAIN** is returned.

An ioctl which sets a new maximum size of all messages is also provided. This operation succeeds only if the new maximum is bigger than the old maximum, or if the new maximum is bigger than the size of all messages currently held. The ioctl returns 0 on success and **-EINVAL** on failure.

## General coding
- The kernel module which implements this driver must be called **charDeviceDriver.ko**.
- The code does not use the kernel fifo.
- The code deals with multiple attempts at reading and writing at the same time. It minimises the time spent in critical sections. 
- The reader should obtain the messages in the same order as they were written.

## Blocking reads and writes 
The code also implements blocking reads and writes:
- Instead of returning **-EAGAIN** when reading and writing from the device, the reader is blocked until a message is available. 
- Similarly, the writer is blocked until there is room for the message (in the case of the writer). 

For the available kernel mechanisms to achieve blocking, see the section *``Wait queues and Wake events''* in the device driver documentation (https://www.linuxtv.org/downloads/v4l-dvb-internals/device-drivers/ch01s04.html). 

Two kernel modules are produced by this task, one named **charDeviceDriver.ko** and one named **charDeviceDriverBlocking.ko**. It is assumed that only one of these two modules will be loaded at any given time.

## Useful links
Introduction to writing Linux kernel modules, part 1
Introduction to writing Linux kernel modules, part 2
Linux kernel API
Browsing Linux kernel source
