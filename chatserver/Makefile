CFLAGS =	-I/usr/include -g -Wall -std=c++11
LIBS = -pthread -lboost_system -ljsoncpp
chatserver: chat_server.cpp AsioServer.cpp
	g++ $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f chatserver *~ 
