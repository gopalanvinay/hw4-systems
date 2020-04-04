#include "cache.hh"
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <assert.h>

// Boost Libraries
#include <boost/beast/core.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <boost/algorithm/string/split.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <thread>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------


// Echoes back all received WebSocket messages
void do_session(tcp::socket& socket){
    try
    {
        // Construct the stream by moving in the socket
        websocket::stream<tcp::socket> ws{std::move(socket)};

        // Set a decorator to change the Server of the handshake
        ws.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-sync");
            }));

        // Accept the websocket handshake
        ws.accept();

        for(;;)
        {
            // This buffer will hold the incoming message
            beast::flat_buffer buffer;
            //http::request<http::dynamic_body> request_;
            // The socket for the currently connected client.

            // The request message.
            http::request<http::dynamic_body> request_;

            // The response message.
            http::response<http::dynamic_body> response_;
            // Read a message
            ws.read(buffer);
            // https://www.boost.org/doc/libs/master/libs/beast/example/http/server/small/http_server_small.cpp
            switch(buffer.method())
            {
                case http::verb::get:
                    Cache::size_type* size;
                    Cache::val_type resp = cache.get(buffer.body(), size);
                    if (resp == nullptr) {
                        printf("KEY NOT IN CACHE"); // send over ws instead
                    } else {
                        printf("This is key");
                    }
                    break;
                case http::verb::put:
                    //split by '=' buffer.body()
                    key_type key;
                    val_type val;
                    size_type size;
                    cache.set(key, val, size);
                    break;
                case http::verb::delete_:
                    if (cache.del(buffer.body())) {
                        printf("%s deleted", buffer.body());
                    } else {
                        printf("not deleted");
                    }
                    break;
                case http::verb::head:

                    break;
                case http::verb::post:
                    if (buffer.body() == "reset") {
                        cache.reset();
                    } else {
                        return "NOT FOUND";
                    }
                    break;
                default:
                    // We return responses indicating an error if
                    // we do not recognize the request method.
                    response_.result(http::status::bad_request);
                    response_.set(http::field::content_type, "text/plain");
                    beast::ostream(response_.body())
                        << "Invalid request-method '"
                        << std::string(request_.method_string())
                        << "'";
                    break;
            }
            // // print -> ws.write(message here)
            Cache cache(100);
        }
    }
    catch(beast::system_error const& se)
    {
        // This indicates that the session was closed
        if(se.code() != websocket::error::closed)
            std::cerr << "Error: " << se.code().message() << std::endl;
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    int opt;
    Cache::size_type maxmem = 20;
    std::string host_string = "127.0.0.1";
    std::string port_string = "4000";
    int8_t threads = 0;

    while((opt = getopt(argc, argv, "m:s:p:t:")) != -1)
    {
        switch(opt)
        {
            case 'm':
                maxmem = atoi(optarg);
                break;
            case 's':
                host_string = optarg ;
                break;
            case 'p':
                port_string = optarg;
                break;
            case 't':
                threads = atoi(optarg);
                break;
        }
    }

    // To stop compiler from complaining since threads is unused
    threads += 5;

    // Test to see getopt is working
    Cache cache(maxmem);
    cache.set("x","5",1);
    Cache::size_type valsize;
    assert(cache.get("x",valsize) == "5");

    try
    {
        auto const address = net::ip::make_address(host_string);
        // Convert port_string to const char * using c_str() method
        auto const port = static_cast<unsigned short>(std::atoi(port_string.c_str()));

        // The io_context is required for all I/O
        net::io_context ioc{1};

        // The acceptor receives incoming connections
        tcp::acceptor acceptor{ioc, {address, port}};
        for(;;)
        {
            // This will receive the new connection
            tcp::socket socket{ioc};

            // Block until we get a connection
            acceptor.accept(socket);

            // Launch the session, transferring ownership of the socket
            std::thread{std::bind(
                &do_session,
                std::move(socket))}.detach();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
