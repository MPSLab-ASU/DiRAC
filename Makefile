PROGRAM = DiRAC

CXX = g++ -O3 -static -g
CFLAGS = -std=c++11
SRCS = $(wildcard ./src/*.cc)
INC = -I./include

all:
	mkdir -p out
	$(CXX) $(CFLAGS) -o out/$(PROGRAM) $(SRCS) $(INC)

clean:
	rm -rf $(PROGRAM) out
