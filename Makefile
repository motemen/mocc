CFLAGS=-std=c11 -g -Werror -fprofile-instr-generate -fcoverage-mapping
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

mocc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

test: mocc
	LLVM_PROFILE_FILE=mocc.profraw ./mocc test/test.c > tmp.s && \
	llvm-profdata merge -sparse mocc.profraw -o mocc.profdata && \
	llvm-cov show ./mocc -instr-profile=mocc.profdata -format=html > coverage.html && \
	riscv64-$(RISCV_HOST)-gcc -static tmp.s -o test.out && \
	prove -v -e "spike $(RISCV)/riscv64-$(RISCV_HOST)/bin/pk" ./test.out
	prove -v ./test.sh

clean:
	rm -f mocc *.o *~ tmp* *.gcov *.gcda *.gcno

.PHONY: test clean

# cc -MM -MF - *.c
mocc.o: mocc.c mocc.h
codegen.o: codegen.c mocc.h
parse.o: parse.c mocc.h
tokenize.o: tokenize.c mocc.h
type.o: type.c mocc.h
util.o: util.c mocc.h
