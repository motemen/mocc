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
9cv.o: 9cv.c 9cv.h codegen.h parse.h tokenize.h type.h util.h
codegen.o: codegen.c codegen.h parse.h tokenize.h type.h 9cv.h util.h
parse.o: parse.c parse.h tokenize.h type.h 9cv.h util.h
tokenize.o: tokenize.c tokenize.h util.h
type.o: type.c parse.h tokenize.h type.h util.h
util.o: util.c 9cv.h tokenize.h
