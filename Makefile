CFLAGS=-std=c11 -g -Werror -fprofile-instr-generate -fcoverage-mapping
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

9cv: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): 9cv.h

test: 9cv
	LLVM_PROFILE_FILE=9cv.profraw ./9cv test/test.c > tmp.s && riscv64-$(RISCV_HOST)-gcc -static tmp.s -o test.out && prove -v -e "spike $(RISCV)/riscv64-$(RISCV_HOST)/bin/pk" ./test.out
	prove -v ./test.sh

clean:
	rm -f 9cv *.o *~ tmp* *.gcov *.gcda *.gcno

.PHONY: test clean

# cc -MM -MF - *.c
9cv.o: 9cv.c 9cv.h
codegen.o: codegen.c 9cv.h
parse.o: parse.c 9cv.h
tokenize.o: tokenize.c 9cv.h
type.o: type.c 9cv.h
util.o: util.c 9cv.h
