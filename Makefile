all: myrouter host

myrouter: my-router.cpp
	g++ -std=c++0x -w  my-router.cpp -pthread -o myrouter

host: host.cpp
	g++ -std=c++0x -w host.cpp -o host
