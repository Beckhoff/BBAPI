TARGET = bbapi
EXTRA_DIR = /lib/modules/$(shell uname -r)/extra/
obj-m += $(TARGET).o
$(TARGET)-objs := api.o simple_cdev.o
ccflags-y := -DDEBUG

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
	rm -f *.c~ *.h~ *.bin unittest

unittest: test_config.h unittest.cpp TcBaDevDef_gpl.h
	g++ unittest.cpp -o $@ -Wall -pedantic -std=c++0x -I../
	sudo ./$@

# indent the source files with the kernels Lindent script
indent: api.c api.h simple_cdev.c simple_cdev.h
	./Lindent $?
