a.out: main.cpp
	g++ -std=c++14 main.cpp
test: main.cpp
	g++ -std=c++14 -DTEST -o test main.cpp
	./test
demo: a.out
	./a.out < input/input00.txt