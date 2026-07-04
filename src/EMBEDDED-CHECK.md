# Checking for Embedded-Friendly Code Generation

Steps to verify that only "zero overhead" C++ constructs are used
(both in code size and performance).

## Binary size and sections

```sh
size build/osearch
nm -C --size-sort build/osearch | tail -30
readelf -S build/osearch | grep -E '\.gcc_except|\.eh_frame|\.typeinfo'
size -A build/osearch | grep -E 'eh_frame|gcc_except'
```

## Unwanted runtime features

```sh
# RTTI
nm -C build/osearch | grep -c 'typeinfo\|type_name'

# Exceptions
nm -C build/osearch | grep -c '__cxa_throw\|__cxa_allocate_exception\|__gxx_personality'

# Heap allocation
nm -C build/osearch | grep -c ' malloc\| operator new\| operator delete'

# Heavy std library (iostream, locale)
nm -C build/osearch | grep -c 'std::locale\|std::iostream'
```

## Compile with -fno-exceptions -fno-rtti

```sh
cmake -G Ninja -B build-noexcept \
  -DCMAKE_CXX_FLAGS="-fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables"
ninja -C build-noexcept
```

If it builds clean, the code doesn't rely on those features.

## Inspect hot-path assembly

```sh
objdump -dC build/osearch | sed -n '/<search()>:/,/^$/p'
objdump -dC build/osearch | sed -n '/<get_next/,/^$/p'
objdump -dC build/osearch | sed -n '/<skip/,/^$/p'
```

## Source-level grep for embedded-unfriendly constructs

```sh
grep -rn 'std::string\|std::vector\|std::map' *.cc *.hh
grep -rn 'virtual ' *.hh
grep -rn 'throw\|try\|catch' *.cc *.hh
grep -rn '\bnew \|delete ' *.cc *.hh
```

## Link map

```sh
# Add -Wl,-Map=osearch.map to link flags, then:
grep -E '\.o\)' osearch.map | sort -t'(' -k2 | uniq
```
