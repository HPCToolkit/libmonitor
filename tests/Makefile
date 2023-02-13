#
#  Example test programs for libmonitor.
#
#  These tests are not integrated with autotools.  Just compile the
#  programs, run them with the monitor-run script in debug mode, and
#  observe the output.
#
#  Copyright (c) 2007-2023, Rice University.
#  See the file LICENSE for details.
#
#  $Id$
#

CC = gcc
CFLAGS = -g -O -Wall

THREAD_PROGRAMS = cancel exit side-exit shootdown thread_fork
NONTHREAD_PROGRAMS = emain hidden

PROGRAMS = $(THREAD_PROGRAMS) $(NONTHREAD_PROGRAMS)

.PHONY: all clean

all: $(PROGRAMS)

$(THREAD_PROGRAMS): %: %.c
	$(CC) -o $@ $(CFLAGS) $< -lpthread

emain: emain.c libearly.so
	$(CC) -o $@ $(CFLAGS) $< -L. -learly

hidden: hidden.c
	$(CC) -o $@ $(CFLAGS) $< -ldl

libearly.so: early.c
	$(CC) -o $@ $(CFLAGS) -shared -fPIC $< -ldl -lpthread

clean:
	rm -f *.o *.so a.out $(PROGRAMS)

