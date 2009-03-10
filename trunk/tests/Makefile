#
#  Example test programs for libmonitor.
#
#  These tests are not integrated with autotools.  Just compile the
#  programs, run them with the monitor-run script in debug mode, and
#  observe the output.
#
#  $Id$
#

CC = gcc
CFLAGS = -g -O -Wall

PROGRAMS = emain exit thread_fork

all: $(PROGRAMS)

emain: emain.c libearly.so
	$(CC) -o $@ $(CFLAGS) $< -L. -learly

libearly.so: early.c
	$(CC) -o $@ $(CFLAGS) -shared -fPIC $< -ldl -lpthread

exit: exit.c
	$(CC) -o $@ $(CFLAGS) $< -lpthread

thread_fork: thread_fork.c
	$(CC) -o $@ $(CFLAGS) $< -lpthread

clean:
	rm -f *.o *.so a.out $(PROGRAMS)

