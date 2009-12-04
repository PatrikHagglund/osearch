BENCHMARKS=benchmarks/*.c

#for CONFIG in gcc43_full.acovea llvm-gcc40.acovea; do

    CONFIG=llvm-gcc40_mini.osearch
    ./osearch -i '-std=c99 -lm -lrt -Werror' config/$CONFIG $BENCHMARKS

#    for f in $BENCHMARKS; do
#	runacovea -config config/$CONFIG -input $f
#    done
#
#    for f in $BENCHMARKS; do
#	runacovea -size -config config/$CONFIG -input $f
#    done

#done
