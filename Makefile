VERSION_MAJOR=1
VERSION_MINIOR=0

CFLAGS=-std=c11 -D_BSD_SOURCE

OBJS=$(patsubst %.c,%.o,$(wildcard *.c))

clbuilder: $(OBJS)
	$(CC) $^ -o $@

clean:
	del *.o