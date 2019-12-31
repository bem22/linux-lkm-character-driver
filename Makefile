obj-m+=charDeviceDriver.o

CUR = ${shell pwd}

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules -lpthread -lrt
	mkdir ${CUR}/out
	${CC} ${CUR}/own/charRead.c -o ${CUR}/out/read
	${CC} ${CUR}/own/charWrite.c -o ${CUR}/out/write
	${CC} ${CUR}/ioctl.c -o ${CUR}/out/ioctl

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm -rf ${CUR}/out