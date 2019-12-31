#!/bin/bash

#globals
ret=0
d=/dev/opsysmem
executable=charDeviceDriver

function run(){
    $1 #execute
    #check errors
    tmp=$?
    if [ $tmp -ne 0 ]; then
        ret=$tmp
    fi
    return $tmp
}


function reset() {
	for a in {1..2000} 
	do
		head -n 1 < $d 2>/dev/null >/dev/null
	done
	return 0
}

function reload() {
    rmmod $executable 2>/dev/null >/dev/null
    rm -f $d
	

    insmod ./$executable.ko
	
	major=`dmesg | tail -n 6 | sed -n "s/\[[0-9. ]*\] 'mknod \/dev\/opsysmem c \([0-9]*\) 0'\./\1/p"`
    case $major in
    "" )
		return 1
	esac
	mknod $d c $major 0

	return 0;
}

function unload() {
	rmmod $executable 2>/dev/null >/dev/null
	rm -f $d 
	return 0;
}

function test_1(){
    echo -en "Test 1: \n"
	t="testcase 1"
	
	#cleanup
	rmmod $executable 2>/dev/null >/dev/null
	rm -f $d
	
	#load kernel driver
    echo -en " load kernel driver:\t"
	insmod ./$executable.ko
	if [ $? -ne 0 ]
	then
		echo -e "ERROR: insmod failed"
		return 1
    else
        echo "ok"
	fi

	#mknod
    echo -en " mknod:\t\t\t"
    major=`dmesg | tail -n 4 | sed -n "s/\[[0-9. ]*\] 'mknod \/dev\/opsysmem c \([0-9]*\) 0'\./\1/p"`
    case $major in
	"" )
	    echo -e "ERROR: cannot find major number in /var/log/syslog, probably the chardev-module isn't implemented properly";
	    return 1
    esac

	if [ $major -eq 0 ]
	then
	    echo -e "ERROR: major = 0, probably the module isn't implemented"
	    rmmod $executeable
	    return 1
	fi
	mknod $d c $major 0 

	if [ $? -ne 0 ]
	then
		echo -e "ERROR: mknod command failed"
		rmmod $executeable
		return 1
    else
        echo "ok"
	fi
	
	#check file
    echo -en " ls $d:\t"
	line=`ls $d 2>/dev/null`
	if [ "$line" != "$d" ]
	then
		echo -e "ERROR: file $d does not exist after loading the kernel module"
		rmmod $executable
		return 1
    else
        echo "ok"
	fi
	
	#write test
    echo -en " write test:\t\t"
	echo "$t" > $d 2>/dev/null
	if [ $? -ne 0 ]
	then
		echo -e "ERROR: writing $d failed"
		rmmod $executable
		return 1
    else
        echo "ok"
	fi

	#read test
    echo -en " read test:\t\t"
	r=`head -n 1 < $d`
	if [ $? -ne 0 ]
	then
		echo -e "ERROR: reading $d failed"
		rmmod $executable
		return 1
	fi
	
	#check if same was read
	if [ "$r" != "$t" ]
	then
		echo -e "ERROR: $d: could not read what was written before"
		rmmod $executable
		return 1
    else
        echo "ok"
	fi
	return 0;
}

function test_2() {
    echo  -en "Test 2: \t\t"
    
    for a in {1..50}
    do 
        cat own/input.txt > $d
    done
    if [ $? -ne 0 ]
	then
		echo -e "Write fail"
		rmmod $executable
		return 1
    else
        echo "Write ok"
	fi
    return 0;
}

function test_3() { 
    echo  -en "Test 3: \t\t"
    
    for a in {1..100}
    do 
        cat own/input2.txt > $d 2>/dev/null
    done
    if [ $? -ne 0 ]
	then
		echo -e "Write fail"
		rmmod $executable
		return 1
    else
        echo "Write ok"
	fi
    return 0;
}

function test_4() { 
	echo -en "Test 4: \t\t"
	./out/ioctl $d 5000000
	if [ $? -ne 0 ]
	then
		echo -e "IOCTL fail"
		rmmod $executable
		return 1
    else
        echo "IOCTL ok"
	fi
}
function test_5() {
	echo -en "Test 5: \t\t"
    for a in {1..100}
    do 
        cat own/input.txt > $d 2>/dev/null
    done
	if [ $? -ne 0 ]
	then
		echo -e "FAIL"
		rmmod $executable
		return 1
    else
        echo "Write ok"
	fi
}

function test_6() {
	echo -en "Test 6: \t\t"	
    for a in {1..200}
    do 
        cat own/input.txt > $d 2>/dev/null
    done
	if [ $? -ne 0 ]
	then
		echo -e "FAIL"
		rmmod $executable
		return 1
    else
        echo "Write ok"
	fi
}

function test_7() {
	echo -en "Test 7: \t\t"
	cat own/input.txt > $d 2>/dev/null & ./out/ioctl $d 5 2>/dev/null & ./out/ioctl $d 2000 2>/dev/null & ./out/ioctl $d 1000000 2>/dev/null

	if [ $? -ne 0 ]
	then
		echo -e "FAIL"
		rmmod $executable
		return 1
    else
        echo "Fork ok"
	fi
}

unload

run test_1

reload

run test_2

run test_3

run test_4

run test_5

run test_6

reset

run test_7

unload

exit $ret
