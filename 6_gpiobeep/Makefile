KERNELDIR := /home/yb/linux/IMX6ULL/linux/My_linux/linux-imx-rel_imx_4.1.15_2.1.0_ga 
CURRENT_PATH := $(shell pwd) 
obj-m := gpiobeep.o 
 
build: kernel_modules 
 
kernel_modules: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules 
clean:   
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean 