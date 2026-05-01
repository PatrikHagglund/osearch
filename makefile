# Tested IDEs:
## Theia (clangd is working, a bit slow),
## atom (clang-tidy is working, OK),
## Netbeans (crashed at startup)

# Tested iwyu, but libc++ seems implying wrong suggestions (eg <iosfwd> for std::string)
## iwyu_tool -p .

# TODO:

## lsp-mode

## <=>
### Ensure that constexpr is used.

## compile-time tests for obj_t operators (how is the infinite arithmetic working?)
## <=>


## check generated code (*next*: point.cc, main.cc)
## non-POD?
## ctags/clangd
## bazel
## code coverage

## new C++ features
### <=> when available
### constraints (GSL variant?), Google test?, compile-time unit tests?
### constexpr std::string and std::vector
### modules
### lambdas? (constexpr)
### Concepts?

## replace libexpat, and try sanitizers
## more constexpr
### how to remove std::string from opt_reg_t?
### compile time string concatenation using constexpr
### https://github.com/akrzemi1/static_string
## fix NONLINT
### noexcept on global functions?
### static member functions returning void?

## Doxygen (clang-doc), interface
## check global variables in lldb
## reconsider some compiler warnings
## More of C++ Core Guidelines
## less std::string, more czstring, not_null
### gsl::multi_span has run-time checks, i.e. non-zero overhead

# TODO: Test type traits, such as is_literal_type, is_trivially_copyable, and
# is_standard_layout (see http://en.cppreference.com/w/cpp/types)

DEBUG := DEBUG
DEFINES := $(DEBUG)
GSL := -I $(HOME)/GSL/include
CPPFLAGS := $(DEFINES:%=-D %) $(GSL)
STD := -std=gnu++26
LLVM_VER=15

# generic
WARN := -Wall -Wextra -Werror
# for Clang
#WARN = -Weverything -Wno-pedantic -Wno-c++98-compat -Wno-c++98-compat-bind-to-temporary-copy -Wno-padded -Wno-global-constructors -Wno-weak-vtables -Wno-exit-time-destructors

# /usr/lib/gcc-snapshot/bin/g++ -Q --help=warning | sed -e 's/^\s*\(\-\S*\)\s*\[\w*\]/\1 /gp;d' | tr -d '\n'
#cc1plus: warning: command line option '-Waliasing' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Walign-commons' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wampersand' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wargument-mismatch' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Warray-temporaries' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wassign-intercept' is valid for ObjC/ObjC++ but not for C++
#cc1plus: warning: command line option '-Wbad-function-cast' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wc++-compat' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wc-binding-type' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wc90-c99-compat' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wc99-c11-compat' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wcharacter-truncation' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wcompare-reals' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wconversion-extra' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wdeclaration-after-statement' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wdesignated-init' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wdiscarded-array-qualifiers' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wdiscarded-qualifiers' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wdo-subscript' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wduplicate-decl-specifier' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wfunction-elimination' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wimplicit' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wimplicit-function-declaration' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wimplicit-int' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wimplicit-interface' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wimplicit-procedure' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wincompatible-pointer-types' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wint-conversion' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Winteger-division' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wintrinsic-shadow' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wintrinsics-std' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wjump-misses-init' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wline-truncation' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wmissing-parameter-type' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wmissing-prototypes' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wnested-externs' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wold-style-declaration' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wold-style-definition' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Woverride-init' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Woverride-init-side-effects' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wpointer-sign' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wpointer-to-int-cast' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wproperty-assign-default' is valid for ObjC/ObjC++ but not for C++
#cc1plus: warning: command line option '-Wprotocol' is valid for ObjC/ObjC++ but not for C++
#cc1plus: warning: command line option '-Wreal-q-constant' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wrealloc-lhs' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wrealloc-lhs-all' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wselector' is valid for ObjC/ObjC++ but not for C++
#cc1plus: warning: command line option '-Wshadow-ivar' is valid for ObjC/ObjC++ but not for C++
#cc1plus: warning: command line option '-Wstrict-prototypes' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wstrict-selector-match' is valid for ObjC/ObjC++ but not for C++
#cc1plus: warning: command line option '-Wsurprising' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wtabs' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wtarget-lifetime' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wtraditional' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wtraditional-conversion' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wundeclared-selector' is valid for ObjC/ObjC++ but not for C++
#cc1plus: warning: command line option '-Wundefined-do-loop' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wunderflow' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wunsuffixed-float-constants' is valid for C/ObjC but not for C++
#cc1plus: warning: command line option '-Wunused-dummy-argument' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wuse-without-only' is valid for Fortran but not for C++
#cc1plus: warning: command line option '-Wzerotrip' is valid for Fortran but not for C++
#cc1plus: error: command line option '-frequire-return-statement' is valid for Go but not for C++ [-Werror]
# GCC 10 will not have C++ -fanalyzer support: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=93288
#WARN_GXX := -fanalyzer
WARN_GXX := -Waddress -Waggressive-loop-optimizations -Walloc-zero -Walloca -Warray-bounds -Wattribute-alias -Wattributes -Wbool-compare -Wbool-operation -Wbuiltin-declaration-mismatch -Wbuiltin-macro-redefined -Wc++11-compat -Wc++14-compat -Wc++17-compat -Wcast-align -Wcast-align=strict -Wcast-function-type -Wcast-qual -Wchar-subscripts -Wclass-memaccess -Wclobbered -Wcomment -Wconditionally-supported -Wconversion-null -Wcoverage-mismatch -Wcpp -Wctor-dtor-privacy -Wdangling-else -Wdate-time -Wdelete-incomplete -Wdelete-non-virtual-dtor -Wdeprecated -Wdeprecated-declarations -Wdisabled-optimization -Wdiv-by-zero -Wdouble-promotion -Wduplicated-branches -Wduplicated-cond -Wempty-body -Wendif-labels -Wenum-compare -Wexpansion-to-defined -Wextra -Wextra-semi -Wfloat-conversion -Wfloat-equal -Wformat-contains-nul -Wformat-extra-args -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat-y2k -Wformat-zero-length -Wframe-address -Wfree-nonheap-object -Whsa -Wif-not-aligned -Wignored-attributes -Wignored-qualifiers -Winherited-variadic-ctor -Winit-self -Winline -Wint-in-bool-context -Wint-to-pointer-cast -Winvalid-memory-model -Winvalid-offsetof -Winvalid-pch -Wliteral-suffix -Wlogical-not-parentheses -Wlogical-op -Wlong-long -Wlto-type-mismatch -Wmain -Wmaybe-uninitialized -Wmemset-elt-size -Wmemset-transposed-args -Wmisleading-indentation -Wmissing-attributes -Wmissing-braces -Wmissing-declarations -Wmissing-field-initializers -Wmissing-include-dirs -Wmultichar -Wmultiple-inheritance -Wmultistatement-macros -Wnarrowing -Wnoexcept -Wnoexcept-type -Wnon-template-friend -Wnon-virtual-dtor -Wnonnull -Wnonnull-compare -Wnull-dereference -Wodr -Wold-style-cast -Wopenmp-simd -Woverflow -Woverlength-strings -Woverloaded-virtual -Wpacked -Wpacked-bitfield-compat -Wpacked-not-aligned -Wparentheses -Wpmf-conversions -Wpointer-arith -Wpointer-compare -Wpragmas -Wpsabi -Wredundant-decls -Wregister -Wreorder -Wrestrict -Wreturn-local-addr -Wreturn-type -Wsequence-point -Wshadow -Wshadow=compatible-local -Wshadow=local -Wshift-count-negative -Wshift-count-overflow -Wshift-negative-value -Wsign-compare -Wsign-promo -Wsized-deallocation -Wsizeof-array-argument -Wsizeof-pointer-div -Wsizeof-pointer-memaccess -Wstack-protector -Wstrict-null-sentinel -Wstringop-truncation -Wsubobject-linkage -Wsuggest-attribute=cold -Wsuggest-attribute=const -Wsuggest-attribute=format -Wsuggest-attribute=malloc -Wsuggest-attribute=noreturn -Wsuggest-attribute=pure -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch -Wswitch-bool -Wswitch-default -Wswitch-enum -Wswitch-unreachable -Wsync-nand -Wsynth -Wtautological-compare -Wterminate -Wtrampolines -Wtrigraphs -Wtype-limits -Wundef -Wuninitialized -Wunknown-pragmas -Wunsafe-loop-optimizations -Wunused -Wunused-but-set-parameter -Wunused-but-set-variable -Wunused-function -Wunused-label -Wunused-local-typedefs -Wunused-macros -Wunused-parameter -Wunused-result -Wunused-value -Wunused-variable -Wuseless-cast -Wvarargs -Wvariadic-macros -Wvector-operation-performance -Wvirtual-inheritance -Wvirtual-move-assign -Wvolatile-register-var -Wwrite-strings
# -Wzero-as-null-pointer-constant problem with spaceship operator
# -Wpedantic # allow language extensions
# -Wtemplates # we allow templates
# -Wnamespaces # we allow namespaces
# -Wvla # we allow VLAs
# -Waggregate-return # we allow aggregate returns
# -Wsystem-headers # exclude system headers
# -Wabi-tag # we don't use abi_tag
# -Wconversion # hard to follow strictly, use -fsanitize=integer instead
## maybe later
# -Wpadded
# -Weffc++
# -Wsign-conversion # may warnings from GSL!

#WARN_GXX := --all-warnings --extra-warnings

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

# FIXME: Reconsider some of these:
# -Wno-pedantic
# -Wno-c++98-compat
# -Wno-c++98-compat-bind-to-temporary-copy
# -Wno-padded
# -Wno-global-constructors
# -Wno-weak-vtables
# -Wno-exit-time-destructors

CXXFLAGS =
#CXXFLAGS = -O3 -flto
#GXX = g++ # 7.3.0
#GXX = g++-8 --std=c++2a # 8.0.1
GXX = /usr/lib/gcc-snapshot/bin/g++ -pipe -fsanitize=undefined $(WARN_GXX) #-fsanitize=leak -fsanitize=address #-fsanitize=thread

#GXX = clang++ # 4.0.1
#GXX = clang++-libc++ # 4.0.1
#GXX = clang++-libc++ --analyze -Xanalyzer -analyzer-output=text
#GXX = clang++-libc++ -fsanitize=memory
#GXX = clang++-7 -stdlib=libc++
#GXX = clang++-7 --alnalyze -Xanalyzer -analyzer-output=text
#GXX = clang++-7 -stdlib=libc++ -fstandalone-debug
#GXX = clang++ -stdlib=libc++ -fsanitize=undefined -fsanitize=integer -fsanitize=nullability
#GXX = clang++-7 -fsanitize=memory

#GXX = clang++-7
# ls *.h > headers.list
# modularize-7 -module-map-path=modules.map headers.list
#GXX = clang++-7
#GXX += -fmodules-ts -Rmodule-build -fmodule-map-file=modules.map -fmodule-map-file=modules-gcc-7.2.1.map -fimplicit-modules

CXX = $(GXX) $(STD) $(CPPFLAGS) $(WARN) $(CXXFLAGS)

# android-cloexec-fopen
# modernize-use-auto2
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
# cppcoreguidelines-pro-type-vararg # no fix-it
# cppcoreguidelines-pro-type-member-init
# cppcoreguidelines-pro-type-const-cast
# cppcoreguidelines-pro-type-reinterpret-cast
# cppcoreguidelines-pro-bounds-pointer-arithmetic
# cppcoreguidelines-special-member-functions
# cppcoreguidelines-interfaces-global-init
# cppcoreguidelines-owning-memory
# exclude:
# cppcoreguidelines-pro-bounds-constant-array-index # using at() is not zero-cost, use -fsanitize=bounds/-fsanitize=bounds-strict (GCC only)
# cppcoreguidelines-init-variables # use valgrind/MSan instead
# -google-runtime-int # ftell and strtoul returns 'long' and 'unsigned long', rather than of a fix size.

# Incoherent guidelines
#
# hicpp-explicit-conversions
# hicpp
# cert
# google
# llvm

# TODO:
# cppcoreguidelines-avoid-non-const-global-variables

CTIDY = clang-tidy-$(LLVM_VER) -header-filter='.*' -warnings-as-errors='*' -checks='*'
CTIDY = clang-tidy-$(LLVM_VER) -header-filter='.*,-gsl*' -warnings-as-errors='*' -checks='*,-cppcoreguidelines-pro-bounds-constant-array-index,-cppcoreguidelines-init-variables,-modernize-use-trailing-return-type,-readability-identifier-length,-google-runtime-int,-llvm-include-order,-fuchsia-default-arguments*,-fuchsia-overloaded-operator,-fuchsia-statically-constructed-objects,-cert-err58-cpp,-cert-msc32-c,cert-msc51-cpp,-llvmlibc-*,-cppcoreguidelines-avoid-non-const-global-variables,-altera-struct-pack-align,-altera-unroll-loops,-altera-id-dependent-backward-branch'

-include *.mk

osearch: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDLIBS)

%.o: %.cc
	$(CTIDY) -fix $< -- -stdlib=libc++ -I /usr/lib/llvm-$(LLVM_VER)/include/c++/v1 $(STD) $(CPPFLAGS) && $(CXX) -c -g $<
	$(CXX) -c -g $<
#	$(CXX) -S -save-temps -O3 $<

# next step: debug hard_compile, missing output file
run: osearch
	./test.sh

clang-format:
	clang-format-$(LLVM_VER) -style=LLVM *.cc -i

clean:
	$(RM) *.o *.d *.s *.ll *.ii osearch
	$(RM) *~
	$(RM) -r html latex
	$(RM) -r build-*

-include *.d
