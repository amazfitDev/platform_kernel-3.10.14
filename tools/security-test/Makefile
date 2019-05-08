# Makefile for USB tools
CROSS_COMPILE = mips-linux-gnu-
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip
PTHREAD_LIBS = -lpthread
WARNINGS = -Wall -Wextra
CFLAGS = $(WARNINGS) -g $(PTHREAD_LIBS) -I../include --static

all: sec_test
%: %.c
	$(CC) $(CFLAGS) -o $@ $^
	$(STRIP) sec_test

clean:
	$(RM) sec_test
