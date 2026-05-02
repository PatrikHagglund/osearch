cmake_minimum_required(VERSION 3.22)
project(osearch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(EXPAT REQUIRED)
find_path(GSL_INCLUDE_DIR gsl/gsl
  PATHS
    $ENV{HOME}/GSL/include
    /home/uabpath/GSL/include
  REQUIRED)

add_executable(osearch
  main.cc my_rand.cc obj.cc point.cc compile.cc
  execute.cc getopts.cc measure.cc print.cc
  read_conf.cc search.cc steps.cc)

target_include_directories(osearch PRIVATE ${GSL_INCLUDE_DIR})
target_link_libraries(osearch PRIVATE EXPAT::EXPAT)

# Warning flags (migrated from legacy makefile, verified clean on GCC 16)
target_compile_options(osearch PRIVATE
  -Wall -Wextra -Werror
  -Waddress -Waggressive-loop-optimizations -Walloc-zero -Walloca
  -Warray-bounds -Wattributes -Wbool-compare -Wbool-operation
  -Wbuiltin-declaration-mismatch -Wbuiltin-macro-redefined
  -Wcast-align -Wcast-align=strict -Wcast-function-type -Wcast-qual
  -Wchar-subscripts -Wclass-memaccess -Wclobbered -Wcomment
  -Wconditionally-supported -Wconversion-null -Wcoverage-mismatch
  -Wctor-dtor-privacy -Wdangling-else -Wdate-time -Wdelete-incomplete
  -Wdelete-non-virtual-dtor -Wdeprecated -Wdeprecated-declarations
  -Wdisabled-optimization -Wdiv-by-zero -Wdouble-promotion
  -Wduplicated-branches -Wduplicated-cond -Wempty-body -Wendif-labels
  -Wenum-compare -Wexpansion-to-defined -Wextra-semi -Wfloat-conversion
  -Wfloat-equal -Wformat-contains-nul -Wformat-extra-args
  -Wformat-nonliteral -Wformat-security -Wformat-signedness
  -Wformat-y2k -Wformat-zero-length -Wframe-address -Wfree-nonheap-object
  -Wif-not-aligned -Wignored-attributes -Wignored-qualifiers
  -Winherited-variadic-ctor -Winit-self -Winline -Wint-in-bool-context
  -Wint-to-pointer-cast -Winvalid-memory-model -Winvalid-offsetof
  -Winvalid-pch -Wliteral-suffix -Wlogical-not-parentheses -Wlogical-op
  -Wlto-type-mismatch -Wmain -Wmaybe-uninitialized -Wmemset-elt-size
  -Wmemset-transposed-args -Wmisleading-indentation -Wmissing-attributes
  -Wmissing-braces -Wmissing-declarations -Wmissing-field-initializers
  -Wmissing-include-dirs -Wmultichar -Wmultiple-inheritance
  -Wmultistatement-macros -Wnarrowing -Wnoexcept -Wnoexcept-type
  -Wnon-template-friend -Wnon-virtual-dtor -Wnonnull -Wnonnull-compare
  -Wnull-dereference -Wodr -Wold-style-cast -Wopenmp-simd -Woverflow
  -Woverlength-strings -Woverloaded-virtual -Wpacked -Wpacked-bitfield-compat
  -Wpacked-not-aligned -Wparentheses -Wpmf-conversions -Wpointer-arith
  -Wpointer-compare -Wpragmas -Wredundant-decls -Wregister -Wreorder
  -Wrestrict -Wreturn-local-addr -Wreturn-type -Wsequence-point -Wshadow
  -Wshadow=compatible-local -Wshadow=local -Wshift-count-negative
  -Wshift-count-overflow -Wshift-negative-value -Wsign-compare -Wsign-promo
  -Wsized-deallocation -Wsizeof-array-argument -Wsizeof-pointer-div
  -Wsizeof-pointer-memaccess -Wstack-protector -Wstrict-null-sentinel
  -Wsubobject-linkage -Wsuggest-attribute=cold -Wsuggest-attribute=const
  -Wsuggest-attribute=format -Wsuggest-attribute=malloc
  -Wsuggest-attribute=noreturn -Wsuggest-attribute=pure
  -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override
  -Wswitch -Wswitch-bool -Wswitch-default -Wswitch-enum
  -Wswitch-unreachable -Wsync-nand -Wsynth -Wtautological-compare
  -Wterminate -Wtrampolines -Wtrigraphs -Wtype-limits -Wundef
  -Wuninitialized -Wunknown-pragmas -Wunused -Wunused-but-set-parameter
  -Wunused-but-set-variable -Wunused-function -Wunused-label
  -Wunused-local-typedefs -Wunused-macros -Wunused-parameter
  -Wunused-result -Wunused-value -Wunused-variable -Wuseless-cast
  -Wvarargs -Wvariadic-macros -Wvector-operation-performance
  -Wvirtual-inheritance -Wvirtual-move-assign -Wvolatile-register-var
  -Wwrite-strings
)

# Doxygen documentation target (run: ninja -C build doc)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating documentation with Doxygen")
endif()

# ---------------------------------------------------------------------------
# Tests (run: ctest --test-dir build [--output-on-failure])
# ---------------------------------------------------------------------------
enable_testing()

# Reuse the same warning flags as the main target.
get_target_property(_osearch_warn_opts osearch COMPILE_OPTIONS)

# Helper: register a self-contained test executable.
function(osearch_add_test name)
  set(sources ${ARGN})
  add_executable(${name} ${sources})
  target_include_directories(${name} PRIVATE ${GSL_INCLUDE_DIR})
  target_compile_options(${name} PRIVATE ${_osearch_warn_opts})
  add_test(NAME ${name} COMMAND ${name})
endfunction()

# Compile-time-only tests for obj_t. "Passing" == "compiles".
osearch_add_test(test_obj_static tests/test_obj_static.cc)

# Runtime tests for point_t. Defines its own `conf` global, so read_conf.cc
# is intentionally NOT linked in.
osearch_add_test(test_point tests/test_point.cc point.cc)

# Runtime tests for steps_t / delta_t. Stubs out conf, progress, measure(),
# and my_rand(); never calls any method that would touch them.
osearch_add_test(test_steps tests/test_steps.cc steps.cc point.cc)
