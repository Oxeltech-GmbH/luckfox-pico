# <kernel_sources>/Documentation/kbuild/modules.txt.

RELEASE := $(shell uname -r)
KDIR ?= /lib/modules/$(RELEASE)/build
CMD := $(MAKE) -C $(KDIR) M=$(PWD)

all default:
	$(CMD)

install:
	$(CMD) modules_install

clean:
	$(CMD) clean
