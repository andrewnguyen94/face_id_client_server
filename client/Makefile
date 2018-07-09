CFLAGS =	-I/usr/include -g -Wall -std=c++11
LIBS = -pthread -lboost_system -ljsoncpp
chatclient: chat_client.cpp AsioClient.cpp
	g++ $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f chatclient *~
