#include "cache.hh"
#include <cstdlib>
#include <iostream>

// Main
int main() {
    std::cout << "Connecting to 127.0.0.1:4000" << std::endl;
    try {
        Cache cache("127.0.0.1","4000");
        std::cout << "Connection sucsessful" << std::endl;
        cache.set("hello","1", 1);
        cache.set("me","10", 2);
        cache.set("too","2", 1);
        std::cout << "SET sucsessful" << std::endl;
        Cache::size_type s;
        std::cout << "GET sucsessful : " << cache.get("hello", s) << std::endl;
        assert(cache.del("hello") == true);
        assert(cache.del("empty") == false);
        std::cout << "DEL sucsessful" << std::endl;
        assert(cache.space_used() == 100);
        std::cout << "Space_used sucsessful" << std::endl;
        cache.reset();
        std::cout << "RESET sucsessful" << std::endl;
    } catch(std::exception const& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
