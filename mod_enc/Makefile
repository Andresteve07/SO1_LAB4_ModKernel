obj-m := mod_enc.o
KERNEL_DIR= /usr/src/linux-headers-$(shell uname -r)
all:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
clean:
	rm -rf *.o *.ko *.symvers *.order *.mod.c *~
