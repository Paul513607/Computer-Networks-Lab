all:
	g++ -std=c++17 -Wall ./server/server.cpp -l sqlite3 -g -o ./server/server.o
	g++ -std=c++17 -Wall ./client/client.cpp -l sqlite3 -g -o ./client/client.o
clean:
	rm server/server.o client/client.o