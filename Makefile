a.out:
	g++ -std=c++17 main.cpp
test:
	g++ -std=c++17 -DTEST -o test main.cpp
	./test
demo: a.out
	./a.out < input/input00.txt