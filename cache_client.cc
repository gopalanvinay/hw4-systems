#include "cache.hh"
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <assert.h>

// Boost Libraries
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/format.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

struct cache_item {
    Cache::val_type val;
    Cache::size_type size;
};

class Cache::Impl {
    public:
        // Add a <key, value> pair to the cache.
        // If key already exists, it will overwrite the old value.
        // Both the key and the value are to be deep-copied (not just pointer copied).
        // If maxmem capacity is exceeded, enough values will be removed
        // from the cache to accomodate the new value. If unable, the new value
        // isn't inserted to the cache.
        void set(key_type key, val_type val, size_type size) {
            std::string query = (boost::format("SET /%s/%d") % key % val).str();
            ws.write(net::buffer(query));
        }

        // Retrieve a pointer to the value associated with key in the cache,
        // or nullptr if not found.
        // Sets the actual size of the returned value (in bytes) in val_size.
        val_type get(key_type key, size_type& val_size) const {
            std::string query = (boost::format("GET %s") % key).str();
            ws.write(net::buffer(query));
            // This buffer will hold the incoming message
            beast::flat_buffer buffer;
            // Read a message into our buffer
            ws.read(buffer);
            // The make_printable() function helps print a ConstBufferSequence
            beast::make_printable(buffer.data()) << std::endl;
        }

        // Delete an object from the cache, if it's still there
        bool del(key_type key) {
            std::string query = (boost::format("DEL %s") % key).str();
            ws.write(net::buffer(query));
            // This buffer will hold the incoming message
            beast::flat_buffer buffer;
            // Read a message into our buffer
            ws.read(buffer);
            // The make_printable() function helps print a ConstBufferSequence
            beast::make_printable(buffer.data()) << std::endl;
        }

        // Compute the total amount of memory used up by all cache values (not keys)
        size_type space_used() const {
            ws.write(net::buffer("HEAD"));
            // This buffer will hold the incoming message
            beast::flat_buffer buffer;
            // Read a message into our buffer
            ws.read(buffer);
            // The make_printable() function helps print a ConstBufferSequence
            beast::make_printable(buffer.data()) << std::endl;
        }

        // Delete all data from the cache
        void reset() {
            ws.write(net::buffer("POST /reset"));
        }

        // This function is used to test the resizing using max_load_factor

        /*
        int get_bucket_count() const{
        return table.bucket_count();
        }
        */

        Impl(std::string host, std::string port : host(host), port(port)) {
            try {
                // The io_context is required for all I/O
                net::io_context ioc;

                // These objects perform our I/O
                tcp::resolver resolver{ioc};
                websocket::stream<tcp::socket> ws{ioc};

                // Look up the domain name
                auto const results = resolver.resolve(host, port);

                // Make the connection on the IP address we get from a lookup
                net::connect(ws.next_layer(), results.begin(), results.end());

                // Set a decorator to change the User-Agent of the handshake
                ws.set_option(websocket::stream_base::decorator(
                    [](websocket::request_type& req)
                    {
                        req.set(http::field::user_agent,
                            std::string(BOOST_BEAST_VERSION_STRING) +
                                " websocket-client-coro");
                    }));
                // Perform the websocket handshake
                ws.handshake(host, "/");
            }
            catch(std::exception const& e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
};

// cache impl stuff

// constructor for the cache

Cache::Cache(std::string host, std::string port)
    : pImpl_(new Impl(host, port)) {}

Cache::~Cache() {
    // Close the WebSocket connection
    // If we get here then the connection is closed gracefully
    ws.close(websocket::close_code::normal);
};

void Cache::set(key_type key, val_type val, size_type size) {
    Cache::pImpl_->set(key, val, size);
}

// Retrieve a pointer to the value associated with key in the cache,
// or nullptr if not found.
// Sets the actual size of the returned value (in bytes) in val_size.
Cache::val_type Cache::get(key_type key, Cache::size_type& val_size) const {
    return Cache::pImpl_->get(key, val_size);
}

// Delete an object from the cache, if it's still there
bool Cache::del(key_type key){
    return Cache::pImpl_->del(key);
}

// Compute the total amount of memory used up by all cache values (not keys)
Cache::size_type Cache::space_used() const {
    return Cache::pImpl_->space_used();
}
// Delete all data from the cache
void Cache::reset(){
    Cache::pImpl_->reset();
}
