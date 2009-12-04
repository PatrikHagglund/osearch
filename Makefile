#DEBUG = DEBUG
DEFINES = $(DEBUG)
CPPFLAGS = $(DEFINES:%=-D %)

WARN = -fdiagnostics-show-option -Werror -Wall -Wno-parentheses -Wextra -Wmissing-declarations -Wsign-conversion -Wc++0x-compat -Wredundant-decls

CXXFLAGS = -g
#CXXFLAGS = -Os -flto -fwhole-program
GXX = g++ -static
GXX = /vboxshare/uabpath/llvm//build-gcc/Release+Asserts/bin/clang++ 
#GXX=g++
CXX = $(GXX) --std=c++11 $(WARN) $(CPPFLAGS) $(CXXFLAGS)

-include *.mk

osearch: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDLIBS)

%.o: %.cc
	$(CXX) -MMD -c $<

clean:
	$(RM) *.o *.d osearch
	$(RM) *~
	$(RM) -r html latex

-include *.d
