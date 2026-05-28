CXX=clang++
CFLAGS=-Wall -Wextra -ggdb -std=c++23
LDFLAGS=-lreadline
yash: main.cpp
	$(CXX) $(CFLAGS) -o yash main.cpp $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJS) yash
