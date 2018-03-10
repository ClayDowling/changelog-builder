VERSION_MAJOR=1
VERSION_MINOR=1

CFLAGS=-std=c11 -D_BSD_SOURCE

OBJS=$(patsubst %.c,%.o,$(wildcard *.c))

.PHONY: clean release

clbuilder: $(OBJS)
	$(CC) $^ -o $@

clean:
	del *.o

release: clbuilder
	git tag -a $(VERSION_MAJOR)_$(VERSION_MINOR) -m "Release $(VERSION_MAJOR).$(VERSION_MINOR)"
	git push origin $(VERSION_MAJOR)_$(VERSION_MINOR)