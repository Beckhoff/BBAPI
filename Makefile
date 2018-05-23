TARGET = bbapi
EXTRA_DIR = /lib/modules/$(shell uname -r)/extra/
obj-m += $(TARGET).o
$(TARGET)-objs := api.o simple_cdev.o
SUBDIRS := $(filter-out scripts/., $(wildcard */.))

SUDO=sudo

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	- ${SUDO} rmmod $(TARGET)_disp
	- ${SUDO} rmmod $(TARGET)_power
	- ${SUDO} rmmod $(TARGET)_sups
	- ${SUDO} rmmod $(TARGET)_wdt
	- ${SUDO} rmmod $(TARGET)
	${SUDO} mkdir -p $(EXTRA_DIR)
	${SUDO} cp ./$(TARGET).ko $(EXTRA_DIR)
	${SUDO} depmod -a
	${SUDO} modprobe $(TARGET)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f *.c~ *.h~ *.bin unittest example

# indent the source files with the kernels Lindent script
indent: indent_files indent_subdirs

indent_files: api.c api.h display_example.cpp sensors_example.cpp simple_cdev.c simple_cdev.h
	./Lindent $?

indent_subdirs: $(SUBDIRS)

$(SUBDIRS):
	make indent -C $@

binaries:
	cmake -H. -Bbuild
	cmake --build build

display_example: binaries
	${SUDO} ./build/$@.bin

sensors_example: binaries
	${SUDO} ./build/$@.bin

unittest: binaries
	${SUDO} ./build/$@.bin

.PHONY: clean display_example sensors_example unittest $(SUBDIRS)
