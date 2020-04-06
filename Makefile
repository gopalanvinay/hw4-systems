CXX=clang++
CXXFLAGS=-Wall -Wextra -pedantic -Werror -std=c++17 -O0 -g
LDFLAGS=$(CXXFLAGS)
LIBS=-pthread
OBJ=$(SRC:.cc=.o)

all:  cache_server test_cache_lib test_cache_client test_evictors

cache_server: cache_server.o cache_lib.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_server:
	./cache_server -s 127.0.0.1 -p 4000 &
	curl -I 127.0.0.1:4000
	curl -X DELETE 127.0.0.1:4000/key
	curl -X HEAD 127.0.0.1:4000
	curl -X PUT 127.0.0.1:4000/Eric/110
	curl -X GET 127.0.0.1:4000/Eric
	curl -X DELETE 127.0.0.1:4000/Eric
	curl -X GET 127.0.0.1:4000/Eric
	curl -X PUT 127.0.0.1:4000/Eric/110
	curl -X PUT 127.0.0.1:4000/Disco/110001
	curl -X PUT 127.0.0.1:4000/Penny/11220
	curl -X PUT 127.0.0.1:4000/Test/10
	curl -X PUT 127.0.0.1:4000/he/0
	curl -X GET 127.0.0.1:4000/Penny
	curl -X HEAD 127.0.0.1:4000
	curl -X DELETE 127.0.0.1:4000/Penny
	curl -X HEAD 127.0.0.1:4000
	killall cache_server

test_cache_lib: fifo_evictor.o test_cache_lib.o cache_lib.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_cache_client: test_cache_client.o cache_client.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cc %.hh
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

clean:
	rm -rf *.o test_cache_client test_cache_lib test_evictors cache_server

test: all
	./test_cache_lib
	echo "test_cache_client must be run manually against a running server"

valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all ./test_cache_lib
	valgrind --leak-check=full --show-leak-kinds=all ./test_evictors
