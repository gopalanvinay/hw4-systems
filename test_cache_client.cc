#include "cache.hh"
#include <cstdlib>
#include <iostream>

// Main
int main() {
    std::cout << "Connecting to 127.0.0.1:4000" << std::endl;
    try {
        Cache cache("127.0.0.1","4000");
        std::cout << "Connection successful" << std::endl;
        cache.set("hello","two", 3);
        cache.set("me","10", 2);
        cache.set("too","2", 1);
        std::cout << "SET successful" << std::endl;
        Cache::size_type s;
        std::string res = "two";
        assert(cache.get("hello", s) == res);
        res = "10";
        assert(cache.get("me", s) == res);
        res = "2";
        assert(cache.get("too", s) == res);
        std::cout << "GET successful" << std::endl;
        assert(cache.del("hello") == true);
        assert(cache.get("hello", s) == nullptr);
        assert(cache.del("empty") == false);
        std::cout << "DEL successful" << std::endl;
        assert(cache.space_used() == 3); // "10","2"
        std::cout << "Space_used successful" << std::endl;
        cache.reset();
        assert(cache.get("hello", s) == nullptr);
        assert(cache.get("me", s) == nullptr);
        assert(cache.get("too", s) == nullptr);
        std::cout << "RESET successful" << std::endl;
    } catch(std::exception const& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
