CFLAGS=-std=c11 -g -Werror -Wno-error=format -fprofile-instr-generate -fcoverage-mapping
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
STAGE=1

mocc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

test: mocc
	LLVM_PROFILE_FILE=mocc.profraw ./mocc test/test.c > tmp.s && \
	llvm-profdata merge -sparse mocc.profraw -o mocc.profdata && \
	llvm-cov show ./mocc -instr-profile=mocc.profdata -format=html > coverage.html && \
	riscv64-$(RISCV_HOST)-gcc -static tmp.s test/helper.c -o test.out && \
	prove -v -e "spike $(RISCV)/riscv64-$(RISCV_HOST)/bin/pk" ./test.out
	prove -v ./test.sh

clean:
	rm -f mocc *.o *~ tmp* *.gcov *.gcda *.gcno

self.$(STAGE)/mocc: mocc
	./selfcompile.sh && riscv64-unknown-elf-gcc -static self.$(STAGE)/*.s -o self.$(STAGE)/mocc

selftest: self.$(STAGE)/mocc
	spike "$(RISCV)/riscv64-$(RISCV_HOST)/bin/pk" $< test/test.c | perl -nle 'print unless $$.==1 && /^bbl loader\r$$/' > self.$(STAGE)/test.s
	riscv64-$(RISCV_HOST)-gcc -static self.$(STAGE)/test.s test/helper.c -o self.$(STAGE)/test && \
	prove -v -e "spike $(RISCV)/riscv64-$(RISCV_HOST)/bin/pk" ./self.$(STAGE)/test


.PHONY: test clean

# cc -MM -MF - *.c
mocc.o: mocc.c mocc.h
codegen.o: codegen.c mocc.h
parse.o: parse.c mocc.h
tokenize.o: tokenize.c mocc.h
type.o: type.c mocc.h
util.o: util.c mocc.h
