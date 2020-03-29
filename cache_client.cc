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
#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


int main(int argc, char* argv[]
)
{
    int opt;
    Cache::size_type maxmem = 20;
    std::string host = "127.0.0.1";
    std::string port = "4000";
    int8_t threads = 0;

    while((opt = getopt(argc, argv, "m:s:p:t:")) != -1)
    {
        switch(opt)
        {
            case 'm':
                maxmem = atoi(optarg);
                break;
            case 's':
                host = optarg ;
                break;
            case 'p':
                port = optarg;
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
        std::string text = "Testing";
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

        // Send the message
        ws.write(net::buffer(std::string(text)));

        // This buffer will hold the incoming message
        beast::flat_buffer buffer;

        // Read a message into our buffer
        ws.read(buffer);
        // /*
        // GET /key: Returns a JSON tuple with { key: , value: } pair (where is the resource in the request, and value is the appropriate value from the cache), or an error message if the key isn't in the cache.
        // PUT /k/v: create or replace a k,v pair in the cache. You may assume v is always a value and neither k nor v contain the '/' character or any other characters that would invalidate the URL, like a whitespace.
        // DELETE /key: Delete from the cache the key and value associated with key.
        // HEAD: Return just a header, regardless of any resources requested. The response must include the pair `Space-Used: `, where is the integer value returned by the Cache's `space_used()` method. You may include as many additional (header) lines as you wish, but as a bare minimum, you should return the HTTP version (1.1 is OK), Accept, and Content-Type with `application/json` (see here). You can can return these header fields in other types of requests as well, but they must be present in a HEAD request.
        // POST /reset: Upon receiving this message, the server calls Cache::reset to clean up all its data. Any other target than "/reset" should be ignored and return NOT FOUND.
        // */
        // std::string msg = beast::make_printable(buffer.data());

        // // Process the message here
        // std::vector<std::string> result;
        // boost::split(result, msg, boost::is_any_of(" "));


        // // print -> ws.write(message here)
        // switch(result[0])
        // {
        //     case 'GET':
        //         if (result[1] == "/key")
        //             val_type resp = cache.get();
        //             if (resp == nullptr) {
        //                 printf("KEY NOT IN CACHE");
        //             } else {
        //                 printf("%s\n", );
        //             }
        //         else
        //             ws.write(net::buffer(std::string("NOT FOUND")));
        //         break;
        //     case 'PUT':
        //         if (result[1] == "/reset")
        //             cache.reset();
        //         else
        //             printf("NOT FOUND");
        //         break;
        //     case 'DELETE':
        //         if (result[1] == "/key")
        //             cache.del(result[2]);
        //         break;
        //     case 'HEAD':
        //         Cache::size_type x = cache.space_used();
        //         // add x into the header
        //         ws.write(net::buffer(std::string("NOT FOUND")));
        //         break;
        //     case 'POST':
        //         if (result[1] == "/reset")
        //             cache.reset();
        //         else
        //             ws.write(net::buffer(std::string("NOT FOUND")));
        //         break;
        // }

        // Close the WebSocket connection
        ws.close(websocket::close_code::normal);

        // If we get here then the connection is closed gracefully

        // The make_printable() function helps print a ConstBufferSequence
        std::cout << beast::make_printable(buffer.data()) << std::endl;
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;

}
