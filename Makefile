CFLAGS=-std=c11 -D_BSD_SOURCE

OBJS=$(patsubst %.c,%.o,$(wildcard *.c))

clbuilder: $(OBJS)
	$(CC) $^ -o $@