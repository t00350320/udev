功能说明：
1、当前只支持光盘、红盘（reddisk）的热插拔
2、udevadm_monitor函数需要一直处于执行状态，才能动态捕捉usb端口的实时状态。
可以起一个线程，一直运行这个usb探测函数即可。
3、当识别到有usb事件时，就进入print_device函数，打印设备信息。在这个函数中
	static char device_path[1024]={0};
	
    printf("get cdrom event,the cdrom path is:%s\n",device_path);
	即捕捉到的光盘的mount路径。
	
	printf("get redisk event,the redisk path is:%s\n",device_path);
	即捕捉到的红盘的mount路径


编译说明：
1、udev用到的头文件libudev.h需要放到当前编译目录下。
2、保证系统/lib/x86_64-linux-gnu/目录下，存在libudev.so
3、gcc usb.c -ludev -L/lib/x86_64-linux-gnu/ -o usb