all: my-router.cpp
	g++ -std=c++0x my-router.cpp -pthread -o myrouter
