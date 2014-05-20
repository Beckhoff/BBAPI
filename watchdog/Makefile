TARGET = bbapi_wdt
EXTRA_DIR = /lib/modules/$(shell uname -r)/extra/
obj-m += $(TARGET).o
$(TARGET)-objs := watchdog.o
ccflags-y := -DDEBUG
#ccflags-y := -DDEVICE_CX5000 -DDEBUG
KBUILD_EXTRA_SYMBOLS := $(src)/../Module.symvers

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	- sudo rmmod $(TARGET)
	sudo mkdir -p $(EXTRA_DIR)
	sudo cp ./$(TARGET).ko $(EXTRA_DIR)
	sudo depmod -a
	sudo modprobe $(TARGET)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

# indent the source files with the kernels Lindent script
indent: watchdog.c watchdog.h
	../Lindent $?
