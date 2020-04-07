#include "cache.hh"
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <assert.h>

// Boost Libraries
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/format.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http/dynamic_body.hpp>
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
    private:
        std::string host_, port_;

        // These objects perform our I/O
        net::io_context ioc;
        tcp::resolver resolver{ioc};
        beast::tcp_stream stream{ioc};

        beast::http::request<http::empty_body> req;
        beast::http::request<http::string_body> res;
    public:
        Impl(std::string host, std::string port) : host_(host), port_(port)
        {
            try {
                // Look up the domain name
                auto const results = resolver.resolve(host_, port_);
                // Make the connection on the IP address we get from a lookup
                stream.connect(results);
            }
            catch(std::exception const& e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }

        // Add a <key, value> pair to the cache.
        // If key already exists, it will overwrite the old value.
        // Both the key and the value are to be deep-copied (not just pointer copied).
        // If maxmem capacity is exceeded, enough values will be removed
        // from the cache to accomodate the new value. If unable, the new value
        // isn't inserted to the cache.
        void set(key_type key, val_type val, size_type size) {
            boost::string_view str { (boost::format("/%s/%s")% key % val).str() };
            req.method(http::verb::put);
            req.target(str);
            req.keep_alive(true);
            http::write(stream, req);
            std::cout << size << std::endl;
        }
        // doesnt do anything with size type yet

        // Retrieve a pointer to the value associated with key in the cache,
        // or nullptr if not found.
        // Sets the actual size of the returned value (in bytes) in val_size.

        val_type get(key_type key, size_type& val_size) {
            boost::string_view str { (boost::format("/%s")% key).str() };
            req.method(http::verb::get);
            req.target(str);
            http::write(stream, req);
            beast::flat_buffer buffer;
            //http::read(stream, buffer, res);
            // Write the message to standard out
            std::cout << res << val_size << std::endl;
            val_type temp = "TEMP ANSWER";
            return temp;
        }

        // Delete an object from the cache, if it's still there
        /*
        bool del(key_type key) {
            beast::http::request<beast::http::string_body> req;
            req.method(beast::http::verb::delete_);
            req.target("/");
            req.set(beast::http::field::content_type, "application/x-www-form-urlencoded");
            req.body() = "key=" + key;
            req.prepare_payload();
            ws.write(req);
            // This buffer is used for reading and must be persisted
            beast::flat_buffer buffer;
            // Declare a container to hold the response
            http::response<http::dynamic_body> res;
            // Receive the HTTP response
            http::read(stream, buffer, res);
            return true; // fix here
        }

        // Compute the total amount of memory used up by all cache values (not keys)
        size_type space_used() const {
            beast::http::request<beast::http::string_body> req;
            req.method(beast::http::verb::head);
            req.target("/");
            req.prepare_payload();
            ws.write(req);
            // This buffer is used for reading and must be persisted
            beast::flat_buffer buffer;
            // Declare a container to hold the response
            http::response<http::dynamic_body> res;
            // Receive the HTTP response
            http::read(stream, buffer, res);
        }

        // Delete all data from the cache
        void reset() {
            beast::http::request<beast::http::string_body> req;
            req.method(beast::http::verb::post);
            req.target("/");
            req.set(beast::http::field::content_type, "application/x-www-form-urlencoded");
            req.body() = "reset";
            req.prepare_payload();
            ws.write(net::buffer("POST /reset"));
        }

        // This function is used to test the resizing using max_load_factor


        int get_bucket_count() const{
        return table.bucket_count();
        }

        */

        void close(){
            // Gracefully close the socket
            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);

            // not_connected happens sometimes
            // so don't bother reporting it.
            if(ec && ec != beast::errc::not_connected)
                throw beast::system_error{ec};
        }
};

// cache impl stuff

// constructor for the cache

Cache::Cache(std::string host, std::string port)
    : pImpl_(new Impl(host, port)) {}

Cache::~Cache() {
    pImpl_->close();
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

/*

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

*/
