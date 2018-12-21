TARGET = bbapi
EXTRA_DIR = /lib/modules/$(shell uname -r)/extra/
obj-m += $(TARGET).o
$(TARGET)-objs := api.o simple_cdev.o
SUBDIRS := $(filter-out scripts/., $(wildcard */.))

OS!=uname -s
SUDO_Linux=sudo
SUDO_FreeBSD=doas
SUDO := ${SUDO_${OS}}
ccflags-y := -DBIOSAPIERR_OFFSET=0
ccflags-y := -DUNAME_S=\"${OS}\"

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	- ${SUDO} rmmod $(TARGET)_button
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
	rm -rf build/
	rm -f *.c~ *.h~ *.bin unittest example
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

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

sensors_example_with_c: sensors_example.cpp
	$(CC) -x c --std=c17 -Wall -pedantic $> -o $@.bin
	${SUDO} ./$@.bin


unittest: binaries
	${SUDO} ./build/$@.bin


${TARGET}.ko: api.c
	make -f Makefile.$(OS)

load: ${TARGET}.ko
	${SUDO} make -f Makefile.$(OS) $@

unload:
	${SUDO} make -f Makefile.$(OS) $@

.PHONY: clean display_example sensors_example unittest $(SUBDIRS)
