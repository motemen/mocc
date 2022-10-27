CFLAGS=-std=c11 -g

9cv: 9cv.c

test: 9cv
	./test.sh

clean:
	rm -f 9cv *.o *~ tmp*

.PHONY: test clean