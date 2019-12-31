## Character device driver - LKM (loadable kernel module)
> This is an LKM that implements a characted device driver

### __WARNING__
>Although this LKM has been tested, I do not recommend loading it on your own machine and I do not give any guarantee that it will not crash your kernel or alter the memory on your machine.

#### Features
- Custom linked list to store bytes
- Can be accessed by parallel processes and is deadlock free
- __ioctl__ that can increase the initial 2MiB limit
- 4KiB limit for the messages
- Does not leak memory on rmmod
- Keeps track of current size

#### Implementation
It is implemented using the linked list defined in __`charDeviceDriver.h`__ and it is using linux semaphores to concurrency conflicts between multiple processes that might access the device.

The total number of critical sections is 4, one for each section that alters the list, the __MAX_LIST_SIZE__ or the current_size.
The semaphores are in:
- `dev_read` (push on list, __current_size__)
- `dev_write` (pop and remove_element on list, __current_size__)
- `dev_ioctl` (modify __MAX_LIST_SIZE__)
- `opsysmem_exit` (destroy2 on list)

#### Testing
There are several tests I wrote for this to ensure all the features listed above

To run my tests you need to copile from source. Do `make` in the root directory
Once make is done, you can start running the tests.

- The basic test is located in `/own`. Run it from the root directory with `./own/test.sh`

- The more advanced test is `/own/devtest.sh`. Run it the same way: `./own/devtest.sh`

- Finally, there is a more intensive test written for checking if the program leaks memory on exit.

    - Because `devtest.sh` and `test.sh` will load/unload the module, to run this you need to first insert the compiled module with `./own/ins.sh` then run the test which was compiled earlier when you ran `make`
        ```shell
        make
        ./own/ins.sh
        ./out/read
        ```

    - After this test is run, you should have a loaded module which loaded kernel space with ~1MiB of data. Removing the module with `rmmod` will call __`opsysmem_exit`__ which will deallocate all the memory.

#### Memory checking
- To check for memory leaks (__which the kernel does not detect automatically__), I have used ___kedr___
    > kedr is an analysys tool that checks memory at run time
- You can find it on [github/euspecter/kedr](https://github.com/euspecter/kedr)
- It __does not__ require recompiling the kernel
- But it __does__ require compiling from source because its features are kernel specific

## License
This project is licensed under the Apache License Version 2.0 - see the [LICENSE.md](LICENSE.md) file for details

## Built with
- [Make](https://www.gnu.org/software/make/manual/make.html) - GNU Make
- [CLion](https://www.jetbrains.com/clion/) - IDEA Project
- [valgrind](https://valgrind.org/) - memory testing

## Authors
- bem22 
