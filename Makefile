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
	sudo chmod a+rw /dev/BBAPI

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f *.c~ *.h~ *.bin

asm: cxdisplay.c TcBaDevDef_gpl.h
	gcc cxdisplay.c -o test.asm -Wall -pedantic -std=c99 -S

test: cxdisplay.c TcBaDevDef_gpl.h
	gcc cxdisplay.c -o test.bin -Wall -pedantic -std=c99

test_asm: cxdisplay.c callbacks.S TcBaDevDef_gpl.h
	gcc cxdisplay.c callbacks.S -o test.bin -Wall -pedantic -std=c99

new_test: test.c test.S
	gcc test.S test.c -o test.bin -Wall -pedantic -std=c99

# indent the source files with the kernels Lindent script
indent: api.c api.h simple_cdev.c simple_cdev.h
	./Lindent $?
