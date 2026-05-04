CC=gcc -std=c99
LDFLAGS=-lrt -lm
CFLAGS=-O3 -ffast-math
#CFLAGS=-g -DVERIFY

ALL=almabench distbench evobench fftbench huffbench linbench mat1bench treebench

all: $(ALL)

all2: CFLAGS+=-DLINK -x c main.ic 

all2: $(ALL)

run: $(ALL)
	for p in $(ALL); do echo; ./$$p -v; ./$$p -v; ./$$p -v; done

run2: $(ALL)
	while true; do ./almabench; done

clean:
	$(RM) $(ALL)
	$(RM) *~

