TARGET = bbapi
EXTRA_DIR = /lib/modules/$(shell uname -r)/extra/
obj-m += $(TARGET).o
$(TARGET)-objs := api.o simple_cdev.o
SUBDIRS := $(filter-out scripts/., $(wildcard */.))

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	- sudo rmmod $(TARGET)_disp
	- sudo rmmod $(TARGET)_power
	- sudo rmmod $(TARGET)_sups
	- sudo rmmod $(TARGET)_wdt
	- sudo rmmod $(TARGET)
	sudo mkdir -p $(EXTRA_DIR)
	sudo cp ./$(TARGET).ko $(EXTRA_DIR)
	sudo depmod -a
	sudo modprobe $(TARGET)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f *.c~ *.h~ *.bin unittest example

unittest: test_config.h unittest.cpp TcBaDevDef_gpl.h config_cx5000.h config_cx2030_cx2100-0004.h config_cx2030_cx2100-0904.h
	g++ unittest.cpp -lpthread -o $@ -Wall -pedantic -std=c++11 -I../ -Itools/
	sudo ./$@

display_example: display_example.cpp TcBaDevDef_gpl.h
	g++ display_example.cpp -lpthread -o $@ -Wall -pedantic -std=c++11 -I../
	sudo ./$@

sensors_example: sensors_example.cpp TcBaDevDef_gpl.h
	g++ sensors_example.cpp -o $@ -Wall -pedantic -std=c++11 -I../
	sudo ./$@

# indent the source files with the kernels Lindent script
indent: indent_files indent_subdirs

indent_files: api.c api.h display_example.cpp sensors_example.cpp simple_cdev.c simple_cdev.h
	./Lindent $?

indent_subdirs: $(SUBDIRS)

$(SUBDIRS):
	make indent -C $@

.PHONY: clean unittest $(SUBDIRS)
