all:
	$(MAKE) hash
	$(MAKE) build
hash:
	cp ./utilities/file-system/hashlib2plus/trunk/src/libhl++.a ./exec/libhl++.o
build:
	g++ -std=c++17 -Wall ./server/server.cpp -lpthread -ldl -l sqlite3 exec/libhl++.o -I ./utilities -g -o ./server/server.o
	g++ -std=c++17 -Wall ./client/client.cpp -lpthread -ldl -l sqlite3 exec/libhl++.o -I ./utilities -g -o ./client/client.o
clean:
	rm server/server.o client/client.o