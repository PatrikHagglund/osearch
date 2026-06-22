CC=gcc -std=c99
# Libraries go in LDLIBS (placed after the source by make's implicit link
# rule) so libm.so's AS_NEEDED libmvec resolves the vectorized sin/cos
# (_ZGVbN2v_*) that -O3 -ffast-math emits. In LDFLAGS they precede the
# source and those symbols go undefined.
LDLIBS=-lm -lrt
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

