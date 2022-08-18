.PHONY:all
all:parser http_server

parser:parser.cc
	g++ -o $@ $^ -lboost_system -lboost_filesystem -std=c++11

http_server:http_server.cc
	g++ -o $@ $^ -ljsoncpp -lpthread -std=c++11

.PHONY:clean
clean:
	rm -f parser http_server