KDIR ?= /lib/modules/$(shell uname -r)/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm aloop.o aloop.ko aloop.mod* .aloop* Module* modules* .Module* .modules*