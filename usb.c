#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include "libudev.h"
#undef asmlinkage
#ifdef __i386__
#define asmlinkage __attribute__((regparm(0)))
#else
#define asmlinkage 
#endif
//#define UDEV_MAX(a,b) ((a) > (b) ? (a) : (b))
//#define udev_list_entry_foreach(entry, first) for (entry = first;entry != NULL;entry = udev_list_entry_get_next(entry))
static int debug;
static int udev_exit;
static char device_path[1024]={0};
#define CDROM 0
#define REDISK 1
static void asmlinkage sig_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM)
		udev_exit = 1;
}
/***get mount path from /proc/mounts,file format as:
#devnodepath mountpath xx
/dev/sr0 /medis/root
***/
void* get_path_from_proc_mounts(const char *devnode,int devicetype)
{
        FILE *fp;               //文件指针
        char find_str[100];     //存储字符串的数组
        int     line=0; 
        int split1=0,split2=0,splittmp=0,splitcount=0;  
        char file_str[1024];
        fp=fopen("/proc/mounts","r");//创建的文件
        if(fp==NULL)
        {
                printf("open error\n");
                return NULL;
        }
        //printf("input string to find:%s\n",devnode);
        //gets(find_str);               //获取输入的字符串
        while(fgets(file_str,sizeof(file_str),fp))//逐行循环读取文件，直到文件结束 
        {
                line++;
                if(strstr(file_str,devnode))  //检查字符串是否在该行中，如果在，则输出该行
                {
//                        printf("%s in %d :%s\n",devnode,line,file_str);
                        //在该字符串中查找第1个空格和第2个空格的位置
                        for (splittmp=0;splittmp<1024;splittmp++)
                        {
                                if (file_str[splittmp] == ' '  && splitcount ==0)
                                {
          //                      printf("find 1st ' ' at:%d \n",splittmp);
                                split1 = splittmp;
                                splitcount++;
                                continue;
                                }
                                if (file_str[splittmp] == ' '  && splitcount ==1)
                                {
            //                    printf("find 2nd ' ' at:%d \n",splittmp);
                                split2 = splittmp;
        			memset(device_path,0,1024);
	                    strncpy(device_path, file_str+split1,split2-split1+1);//有数据的部分复制 只复制6长度的
                                break;
                                }


                        }
                }
        }
        fclose(fp);//关闭文件，结束

}
static void print_device(struct udev_device *device, const char *source, int env)
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	int cdrom=0;
	int redisk=0;
	int changeaction=0,addaction=0;
	const char *devnode=udev_device_get_devnode(device);
	//print udev event
/*	printf("%-6s[%llu.%06u] action:%-8s;devpath:%s;devnode:%s (%s)/n",
	       source,
	       (unsigned long long) tv.tv_sec, (unsigned int) tv.tv_usec,
	       udev_device_get_action(device),
	       udev_device_get_devpath(device),
	       udev_device_get_devnode(device),
	       udev_device_get_subsystem(device));*/
	if (env) {
		struct udev_list_entry *list_entry;
		udev_list_entry_foreach(list_entry, udev_device_get_properties_list_entry(device))
		{
/*			printf("%s=%s/n",
			       udev_list_entry_get_name(list_entry),
			       udev_list_entry_get_value(list_entry));*/
			const char * name = udev_list_entry_get_name(list_entry);
			const char * value = udev_list_entry_get_value(list_entry);
			if (!strcmp(name,"ACTION") && !strcmp(value,"change"))
                        {       
                                changeaction=1;
                                //printf("get change action\n");
                        }
			if (!strcmp(name,"ACTION") && !strcmp(value,"add"))
                        {
                                addaction=1;
                                //printf("get add action\n");
                        }

			//check cdrom
			if (!strcmp(name,"ID_CDROM_MEDIA_STATE")&& !strcmp(value,"complete")&&changeaction==1)
			{
				printf("get cdrom media_state\n");
			        get_path_from_proc_mounts(devnode,CDROM);    
                                printf("get cdrom event,the cdrom path is:%s\n",device_path);
                                break;

			}
			//check redisk
                        if (!strcmp(name,"DEVPATH")&& !strcmp(value,"/devices/virtual/block/dm-0")&&addaction==1)
                        {
                                printf("get redisk state\n");
				usleep(2000000);
				// reddisk's devpath is changed from dm-0 to RedDisk by ZHONGFU APP
	                        get_path_from_proc_mounts("/dev/mapper/RedDisk",REDISK);    
                                printf("get redisk event,the redisk path is:%s\n",device_path);
                                break;

                        }



			
		}
	}
}
int udevadm_monitor(struct udev *udev)
{
	int env = 0;
	struct udev_monitor *kernel_monitor = NULL;
	fd_set readfds;
	int rc = 0;

	
	kernel_monitor = udev_monitor_new_from_netlink(udev, "udev"); //listen kernel udev event 
	if (kernel_monitor == NULL) 
	{
		rc = 3;
		printf("udev_monitor_new_from_netlink() error/n");
		goto out;
	}
	if (udev_monitor_enable_receiving(kernel_monitor) < 0) 
	{
		rc = 4;
		goto out;
	}
	
	//printf("/n");
	while (!udev_exit) {
		int fdcount;
		//printf("wait event!\n");
		FD_ZERO(&readfds);
		if (kernel_monitor != NULL)
			FD_SET(udev_monitor_get_fd(kernel_monitor), &readfds);
		// udev get info from kernel using socket method
		fdcount = select(udev_monitor_get_fd(kernel_monitor)+1,
				 &readfds, NULL, NULL, NULL);
		if (fdcount < 0) {
			if (errno != EINTR)
				fprintf(stderr, "error receiving uevent message: %m/n");
			continue;
		}
		if ((kernel_monitor != NULL) && FD_ISSET(udev_monitor_get_fd(kernel_monitor), &readfds)) {
			struct udev_device *device;
			device = udev_monitor_receive_device(kernel_monitor);
			if (device == NULL)
				continue;
			print_device(device, "UEVENT", 1);
			udev_device_unref(device);
		}
	}
out:
	udev_monitor_unref(kernel_monitor);
	return rc;
}
int main(int argc, char *argv[])
{
	struct udev *udev;
	int rc = 1;
	printf("start udev!\n");
	udev = udev_new();
	if (udev == NULL)
		goto out;
	udevadm_monitor(udev);
	goto out;
	rc = 2;
out:
	udev_unref(udev);
	return rc;
}
