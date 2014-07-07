TARGET = bbapi
EXTRA_DIR = /lib/modules/$(shell uname -r)/extra/
obj-m += $(TARGET).o
$(TARGET)-objs := api.o simple_cdev.o
#ccflags-y := -DDEBUG

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
	rm -f *.c~ *.h~ *.bin unittest example

unittest: test_config.h unittest.cpp TcBaDevDef_gpl.h config_cx5000.h config_cx2030_cx2100-0004.h config_cx2030_cx2100-0904.h
	g++ unittest.cpp -lpthread -o $@ -Wall -pedantic -std=c++11 -I../
	sudo ./$@

example: display_example.cpp
	g++ display_example.cpp -lpthread -o $@ -Wall -pedantic -std=c++11 -I../
	sudo ./$@

# indent the source files with the kernels Lindent script
indent: api.c api.h simple_cdev.c simple_cdev.h
	./Lindent $?
