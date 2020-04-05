CXX=clang++
CXXFLAGS=-Wall -Wextra -pedantic -Werror -std=c++17 -O0 -g
LDFLAGS=$(CXXFLAGS)
LIBS=-pthread
OBJ=$(SRC:.cc=.o)

all:  cache_server test_cache_lib test_cache_client test_evictors

sample_beast: cache_lib.o sample-beast.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_sample:
	./test_beast -s 127.0.0.1 -p 4000& curl -I 127.0.0.1:4000
	curl -X DELETE 127.0.0.1:4000/key

cache_server: cache_server.o cache_lib.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_evictors: test_evictors.o lru_evictor.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_cache_lib: test_cache_lib.o cache_lib.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_cache_client: test_cache_client.o cache_client.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cc %.hh
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

clean:
	rm -rf *.o test_cache_client test_cache_lib test_evictors cache_server

test: all
	./test_cache_lib
	./test_evictors
	echo "test_cache_client must be run manually against a running server"

valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all ./test_cache_lib
	valgrind --leak-check=full --show-leak-kinds=all ./test_evictors
