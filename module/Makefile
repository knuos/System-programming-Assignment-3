# 커널 소스 트리 경로 — 새 커널로 부팅한 뒤에는 기본값으로 충분하다.
#   make KDIR=~/linux-7.2.2   처럼 직접 지정해도 된다.
KDIR ?= /lib/modules/$(shell uname -r)/build

obj-m := tick_hello.o

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

.PHONY: all clean
