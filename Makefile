PNAME := warp7p
CXX := g++
CXXFLAGS := -O2 --std=c++11 -Wall -Werror -pedantic
LDFLAGS := -llodepng -s
RM := rm -f

all: $(PNAME)

$(PNAME): $(PNAME).cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	$(RM) $(PNAME)
