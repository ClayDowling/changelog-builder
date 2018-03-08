CFLAGS=-std=c11

OBJS=$(patsubst %.c,%.o,$(wildcard *.c))

clbuilder: $(OBJS)
	$(CC) $^ -o $@