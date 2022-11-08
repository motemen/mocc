CFLAGS=-std=c11 -g -Werror
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

9cv: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): 9cv.h

test: 9cv
	./9cv test/test.c > tmp.s && riscv64-$(RISCV_HOST)-gcc -static tmp.s -o test.out && prove -v -e "spike $(RISCV)/riscv64-$(RISCV_HOST)/bin/pk" ./test.out
	prove -v ./test.sh

clean:
	rm -f 9cv *.o *~ tmp*

.PHONY: test clean

# cc -MM -MF - *.c
9cv.o: 9cv.c 9cv.h codegen.h parse.h tokenize.h util.h
codegen.o: codegen.c codegen.h parse.h tokenize.h 9cv.h util.h
parse.o: parse.c parse.h tokenize.h 9cv.h util.h
tokenize.o: tokenize.c tokenize.h util.h
util.o: util.c 9cv.h tokenize.h
