# -lc++experimental
env CC=/usr/bin/clang CXX='/usr/bin/clang++ -std=gnu++2a -fuse-ld=lld -stdlib=libc++ -march=native -glldb -fsanitize=undefined -fsanitize=integer -fsanitize=nullability -DDEBUG' cmake -G Ninja -H. -Bbuild-clang
ninja -C build-clang -v
env CC=/usr/lib/gcc-snapshot/bin/gcc CXX='/usr/lib/gcc-snapshot/bin/g++ -std=gnu++2a -march=native -ggdb -fsanitize=undefined -DDEBUG' cmake -G Ninja -H. -Bbuild-gcc
ninja -C build-gcc -v
# -lc++experimental
env CC=/usr/bin/clang CXX='/usr/bin/clang++ -std=gnu++2a -fuse-ld=lld -stdlib=libc++ -march=native -g3 -gdwarf-5 -glldb -O3 -flto' cmake -G Ninja -H. -Bbuild-clang-opt
ninja -C build-clang-opt -v
env CC=/usr/lib/gcc-snapshot/bin/gcc CXX='/usr/lib/gcc-snapshot/bin/g++ -std=gnu++2a -march=native -g3 -gdwarf-5 -ggdb -O3 -flto --param early-inlining-insns=10000000 --param large-stack-frame-growth=1000000 --param large-function-growth=100000 -fopt-info-missed' cmake -G Ninja -H. -Bbuild-gcc-opt
ninja -C build-gcc-opt -v
#objdump -d build-clang-opt/osearch | c++filt | less
make -j
