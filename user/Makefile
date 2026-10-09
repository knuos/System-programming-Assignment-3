CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2

all: hello_user

hello_user: hello_user.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f hello_user

.PHONY: all clean
