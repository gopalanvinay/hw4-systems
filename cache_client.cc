#include "cache.hh"
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include <stdlib.h>     /* atoi */

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
#include <boost/algorithm/string.hpp>

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
        beast::flat_buffer buffer;

    public:
        Impl(std::string host, std::string port) : host_(host), port_(port) {}
        void connect() {
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

        void disconnect() {
            // Gracefully close the socket
            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            // not_connected happens sometimes
            // so don't bother reporting it.
            if(ec && ec != beast::errc::not_connected)
                throw beast::system_error{ec};
        }

        // Add a <key, value> pair to the cache.
        // If key already exists, it will overwrite the old value.
        // Both the key and the value are to be deep-copied (not just pointer copied).
        // If maxmem capacity is exceeded, enough values will be removed
        // from the cache to accomodate the new value. If unable, the new value
        // isn't inserted to the cache.
        void set(key_type key, val_type val, size_type size) {
            std::ignore = size;
            connect();
            boost::string_view str { (boost::format("/%s/%s")% key % val).str() };
            req.method(http::verb::put);
            req.target(str);
            http::write(stream, req);
            disconnect();
        }
        // doesnt do anything with size type yet

        // Retrieve a pointer to the value associated with key in the cache,
        // or nullptr if not found.
        // Sets the actual size of the returned value (in bytes) in val_size.
        val_type get(key_type key, size_type& val_size) {
            std::ignore = val_size; // implement later
            connect();
            boost::string_view str { (boost::format("/%s")% key).str() };
            req.method(http::verb::get);
            req.target(str);
            http::write(stream, req);
            http::response<http::string_body> res;
            http::read(stream, buffer, res);
            std::vector<std::string> vec;
            boost::split(vec, res.body(), boost::is_any_of("{}= "));
            disconnect();
            if (res.result() == http::status::ok) {
                return (val_type) vec[4].c_str();
            } else {
                return nullptr;
            }
        }

        // Delete an object from the cache, if it's still there
        bool del(key_type key) {
            connect();
            boost::string_view str { (boost::format("/%s")% key).str() };
            req.method(http::verb::delete_);
            req.target(str);
            http::write(stream, req);
            http::response<http::string_body> res;
            http::read(stream, buffer, res);
            disconnect();
            return res.result() == http::status::ok; // if key exists
        }

        // Compute the total amount of memory used up by all cache values (not keys)
        size_type space_used() {
            connect();
            req.method(http::verb::head);
            http::write(stream, req);
            http::response<http::string_body> res;
            http::read(stream, buffer, res);
            assert(res.result() == http::status::ok);
            disconnect();
            std::vector<std::string> vec;
            boost::split(vec, res.body(), boost::is_any_of(" "));
            return atoi(vec[1].c_str());
        }

        // Delete all data from the cache
        void reset() {
            connect();
            req.method(http::verb::post);
            req.target("/reset");
            http::write(stream, req);
            http::response<http::string_body> res;
            http::read(stream, buffer, res);
            assert(res.result() == http::status::ok);
            disconnect();
        }
};

// cache impl stuff

// constructor for the cache

Cache::Cache(std::string host, std::string port)
    : pImpl_(new Impl(host, port)) {}

Cache::~Cache() {
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
