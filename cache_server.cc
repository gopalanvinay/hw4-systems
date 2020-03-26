#include "cache.hh"
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <assert.h>

#include <cpprest/ws_client.h>
using namespace web;
using namespace web::websockets::client;

int main(int argc, char* argv[])
{
    int opt;
    Cache::size_type maxmem = 0;
    std::string server = "127.0.0.1";
    std::string port = "3000";
    int8_t threads = 0;

    while((opt = getopt(argc, argv, "m:s:p:t:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'm': 
                maxmem = atoi(optarg);
                break;
            case 's': 
                server = optarg ;
                break;
            case 'p':  
                port = optarg;
                break; 
            case 't':  
                threads = atoi(optarg);
                break;  
        } 
    }

    // To stop compiler from complaining
    threads += 5;

    // Test to see getopt is working
    Cache cache(maxmem);
    cache.set("x","5",1);
    Cache::size_type valsize;
    assert(strcmp(cache.get("x",valsize),"5") == 0);

    // Set up client
    websocket_client client;
    // Connect
    client.connect(U(server)).then([](){ /* We've finished connecting. */ });

    while(true){
        client.receive().then([](websocket_incoming_message msg) {
        return msg.extract_string();
    }).then([](std::string body) {
        std::cout << body << std::endl;
    });
    }

    websocket_outgoing_message msg;
    msg.set_utf8_message("I am a UTF-8 string! (Or close enough...)");
    client.send(msg).then([](){ /* Successfully sent the message. */ });

    client.receive().then([](websocket_incoming_message msg) {
        return msg.extract_string();
    }).then([](std::string body) {
        std::cout << body << std::endl;
    });

    return 0;

}