.PHONY: all
all: sdbsearch

sdbsearch: sdbsearch.c
	gcc -Ofast -o sdbsearch sdbsearch.c

.PHONY: clean
clean:
	rm -f sdbsearch sdbsearch.exe