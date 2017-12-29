# env CC=/usr/bin/clang-7 CXX='/usr/bin/clang++-7 -std=gnu++2a -stdlib=libc++ -glldb -fsanitize=undefined -fsanitize=integer -fsanitize=nullability -DDEBUG' cmake -G Ninja -H. -Bbuild-clang
# ninja -C build-clang -v
# env CC=/usr/lib/gcc-snapshot/bin/gcc CXX='/usr/lib/gcc-snapshot/bin/g++ -std=gnu++2a -ggdb -fsanitize=undefined -DDEBUG' cmake -G Ninja -H. -Bbuild-gcc
# ninja -C build-gcc -v
## -gdwarf-5 => clang LTO crash
# env CC=/usr/bin/clang-7 CXX='/usr/bin/clang++-7 -std=gnu++2a -stdlib=libc++ -g3 -gdwarf-4 -glldb -O3 -flto' cmake -G Ninja -H. -Bbuild-clang-opt
# ninja -C build-clang-opt -v
## -gdwarf-5 => /usr/bin/ld problem
# env CC=/usr/lib/gcc-snapshot/bin/gcc CXX='/usr/lib/gcc-snapshot/bin/g++ -std=gnu++2a -g3 -gdwarf-4 -ggdb -O3 -flto' cmake -G Ninja -H. -Bbuild-gcc-opt
# ninja -C build-gcc-opt -v
# objdump -d build-clang-opt/osearch | c++filt | less
cmake_minimum_required(VERSION 3.11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(/home/uabpath/GSL/include)
add_executable(osearch main.cc compile.cc  execute.cc  getopts.cc  measure.cc  point.cc  print.cc  read_conf.cc  search.cc  steps.cc)
target_link_libraries(osearch expat)
