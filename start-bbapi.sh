 #!/bin/sh

# Insert Kernel Module
sudo insmod bbapi.ko
# Make File Read and writable
sudo chmod a+rw /dev/BBAPI
