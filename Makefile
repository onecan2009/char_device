ifneq ($(KERNELRELEASE),) 
#Kbuild语法的语句，指明模块源码中各个文件的依赖关系，以及生成的目标模块
#$(warning  $(src))#/home/board/kernel1.3/KernelDriver
#$(src) 是一个相对路径，它就是Makefile/Kbuild 文件所在的路径。同(obj) 就是编译目标保存的路径，默认就是源代码所在路

MODULE_NAME := char_dev

OBJDRIVER := char_net_platform.o  char_dev_create.o dwmac1000_core.o dwmac1000_dma.o gmac_init.o dwmac_lib.o norm_desc.o gmac_ctrl.o net_ctrl.o phy_ctrl.o



$(MODULE_NAME)-objs := $(OBJDRIVER)
obj-m := $(MODULE_NAME).o #生成模块 

else
DRIVERDIR := $(shell pwd)
KERNELDIR = /home/rk3288-sdk/gpu-rk3288-kernel/


all: clean

 #$(MAKE) -C $(KERNELDIR) 指明跳转到内核源码目录下读取那里的Makefile 
 #M=$(PWD) 意思是返回到当前目录继续读入、执行当前的Makefile。所以此文件执行两遍
 #“M=”选项的作用是，当用户需要以某个内核为基础编译一个外部模块的话，需要在make modules 命令中加入“M=dir 
 #modueles目标指向obj-m变量中设定的模块
	$(MAKE) -C $(KERNELDIR) M=$(DRIVERDIR) modules
	$(MAKE) copy


copy:
	echo "copy ko to remote Lark.."
	scp char_dev.ko root@192.168.1.9:/home/Lark/Desktop
clean:
	rm -rf *.o *.mod.c .*.cmd *.order *.symvers *.ko .tmp_versions
	
endif



