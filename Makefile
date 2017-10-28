DEBUG = DEBUG
DEFINES = $(DEBUG)
CPPFLAGS = $(DEFINES:%=-D %)

# generic
WARN = -Werror -Wall -Wextra
# for Clang
WARN = -Weverything -Wno-padded -Wno-c++98-compat -Wno-c++98-compat-bind-to-temporary-copy -Wno-weak-vtables -Wno-exit-time-destructors -Wno-global-constructors

# -Wextra-semi
# -Wshadow-field-in-constructor
# -Wundef
# -Wold-style-cast
# -Wmissing-noreturn
# -Wreserved-id-macro
# -Wshorten-64-to-32
# -Wconversion
# -Wweak-vtables
# -Wmissing-variable-declarations
# -Wdeprecated
# -Wdocumentation-unknown-command

# -Wc++98-compat
# -Wc++98-compat-bind-to-temporary-copy
# -Wpadded
# -Wweak-vtables
# -Wexit-time-destructors
# -Wglobal-constructors

CXXFLAGS = -g
#CXXFLAGS = -O3 -flto
GXX=g++
GXX = g++-7
GXX = /usr/lib/gcc-snapshot/bin/g++

#GXX = clang++-libc++
#GXX = clang++-libc++ --analyze -Xanalyzer -analyzer-output=text
#GXX = clang++-libc++ -fsanitize=memory
#GXX = clang++-6.0
#GXX = clang++-6.0 --analyze -Xanalyzer -analyzer-output=text
#GXX = clang++-6.0
#GXX = clang++-6.0 -fsanitize=undefined -fsanitize=integer -fsanitize=nullability
#GXX = clang++-6.0 -fsanitize=memory

GXX = clang++-6.0
# ls *.h > headers.list
# modularize-6.0 -module-map-path=modules.map headers.list
GXX = clang++-6.0 -fmodules-ts -Rmodule-build -fmodule-map-file=modules.map -fmodule-map-file=modules-gcc-7.2.1.map -fimplicit-modules


CXX = $(GXX) --std=c++2a $(CPPFLAGS) $(WARN) $(CXXFLAGS)

# android-cloexec-fopen
# modernize-use-auto
# modernize-loop-convert
# modernize-pass-by-value
# modernize-use-default-member-init
# modernize-use-equals-default
# modernize-use-override
# modernize-use-using # sometimes no fix-it?
# modernize-use-equals-default
# modernize-pass-by-value
# performance-unnecessary-value-param
# misc-unused-using-decls
# misc-unconventional-assign-operator # no fix-it
# misc-static-assert
# misc-string-compare
# readability-braces-around-statements
# readability-redundant-declaration
# readability-redundant-member-init
# readability-inconsistent-declaration-parameter-name
# readability-implicit-bool-conversion
# readability-container-size-empty
# readability-else-after-return
# readability-named-parameter
# llvm-include-order
# -llvm-header-guard
# cppcoreguidelines-pro-type-member-init
# -cppcoreguidelines-pro-type-vararg # no fix-it
# -cppcoreguidelines-pro-type-const-cast
# cppcoreguidelines-pro-bounds-constant-array-index # no fix-it?
# -hicpp-vararg
# hicpp-explicit-conversions
# hicpp
# cert
# google
# llvm

CTIDY = clang-tidy-6.0 -header-filter='.*' -warnings-as-errors='*' -checks='*'
CTIDY = clang-tidy-6.0 -header-filter='.*' -warnings-as-errors='*' -checks='android-*,modernize-*,performance-*,misc-*,readability-*,bugprone-*,boost-*,llvm-*,-llvm-header-guard'
#CTIDY = clang-tidy-6.0 -header-filter='.*' -warnings-as-errors='*' -checks='android-*,modernize-*,performance-*,misc-*,readability-*,hicpp-explicit-conversions' -fix

-include *.mk

osearch: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDLIBS)

%.o: %.cc
	#$(CTIDY) $< -- --std=c++2a $(CPPFLAGS)
	$(CXX) -c $<

run: osearch
	./test.sh

clang-format:
	clang-format-6.0 -style=LLVM *.cc -i

clean:
	$(RM) *.o *.d osearch
	$(RM) *~
	$(RM) -r html latex

-include *.d
