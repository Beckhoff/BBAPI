TARGET = bbapi_display
EXTRA_DIR = /lib/modules/$(shell uname -r)/extra/
obj-m += $(TARGET).o
$(TARGET)-objs := display.o
ccflags-y := -DDEBUG
KBUILD_EXTRA_SYMBOLS := $(src)/../Module.symvers
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

install:
	- sudo rmmod $(TARGET)
	sudo mkdir -p $(EXTRA_DIR)
	sudo cp ./$(TARGET).ko $(EXTRA_DIR)
	sudo depmod -a
	sudo modprobe $(TARGET)

clean:
	make -C $(KDIR) M=$(PWD) clean

# indent the source files with the kernels Lindent script
indent: display.c
	../Lindent $?
