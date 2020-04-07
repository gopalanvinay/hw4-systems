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
        std::cout << "SET sucsessful" << std::endl;
        Cache::size_type s;
        cache.get("hello", s);
        std::cout << "GET sucsessful" << std::endl;
    } catch(std::exception const& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
