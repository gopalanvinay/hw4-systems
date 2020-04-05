//
// Copyright (c) 2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP server, small
//
//------------------------------------------------------------------------------

#include "cache.hh"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

Cache cache(1000);

class http_connection : public std::enable_shared_from_this<http_connection> {
    public:
        http_connection(tcp::socket socket) : socket_(std::move(socket)) {
        }
        // Initiate the asynchronous operations associated with the connection.
        void start()
        {
            read_request();
        }

    private:
        // The socket for the currently connected client.
        tcp::socket socket_;
        // The buffer for performing reads.
        beast::flat_buffer buffer_{8192};

        // The request message.
        http::request<http::dynamic_body> request_;

        // The response message.
        http::response<http::dynamic_body> response_;

        // Asynchronously receive a complete request message.
        void read_request() {
            auto self = shared_from_this();

            http::async_read(
                socket_,
                buffer_,
                request_,
                [self](beast::error_code ec,
                    std::size_t bytes_transferred)
                {
                    boost::ignore_unused(bytes_transferred);
                    if(!ec)
                        self->process_request();
                });
        }

        // Determine what needs to be done with the request message.
        void process_request() {
            response_.version(request_.version());
            response_.keep_alive(false);

            key_type key;
            Cache::val_type val;
            Cache::size_type size = 0;
            switch(request_.method()) {
                case http::verb::get:
                    val = cache.get((key_type) request_.target(), size);
                    if (val == nullptr) {
                        printf("KEY NOT IN CACHE"); // send over ws instead
                    } else {
                        printf("This is key");
                    }
                    break;
                case http::verb::put:
                    //split by '=' buffer.body()
                    cache.set(key, val, size);
                    break;
                case http::verb::delete_:
                    if (cache.del((key_type) request_.target())) {
                        printf("key deleted");
                    } else {
                        printf("not deleted");
                    }
                    break;
                case http::verb::head:
                    printf("do this");
                    break;
                case http::verb::post:
                    if (request_.target() == "reset") {
                        cache.reset();
                    } else {
                        printf("NOT FOUND");
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

            write_response();
        }


        // Asynchronously transmit the response message.
        void write_response() {
            auto self = shared_from_this();

            response_.set(http::field::content_length, response_.body().size());

            http::async_write(
                socket_,
                response_,
                [self](beast::error_code ec, std::size_t)
                {
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                });
        }
};

// "Loop" forever accepting new connections.
void http_server(tcp::acceptor& acceptor, tcp::socket& socket) {
  acceptor.async_accept(socket, [&](beast::error_code ec)
      {
          if(!ec)
              std::make_shared<http_connection>(std::move(socket))->start();
          http_server(acceptor, socket);
      });
}

int main(int argc, char* argv[]) {
    try
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
        // initialize cache
        //Cache cache(maxmem);

        auto const address = net::ip::make_address(port_string.c_str());
        unsigned short port = static_cast<unsigned short>(std::atoi(host_string.c_str()));

        net::io_context ioc{1};
        Cache cache(1000);
        tcp::acceptor acceptor{ioc, {address, port}};
        tcp::socket socket{ioc};
        http_server(acceptor, socket);

        ioc.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
