CFLAGS=-std=c11 -g -Werror
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

9cv: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): 9cv.h

test: 9cv
	./9cv test/test.c > tmp.s && riscv64-unknown-elf-gcc tmp.s -o test.out && spike "$(RISCV)/riscv64-$(RISCV_HOST)/bin/pk" ./test.out
	prove -v ./test.sh

clean:
	rm -f 9cv *.o *~ tmp*

.PHONY: test clean
