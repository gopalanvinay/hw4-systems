# HW4: Let's network

#### Completed by Eric Boettner and Vinay Gopalan.

Note: We didn't use any code written by our classmates. We fixed our previous cache bugs to continue this assignment.

In this exercise we used our cache code to access the cache from multiple clients over the network, like Memcached does. For this, we implemented a RESTful API using the `Boost` library made for C++. Furthermore, to parse command line args we made use of the `getopt` function built for command line parsing.

By default, our server runs on localhost (127.0.0.1) port 4000. To build this project, type `make`. Run the `./cache_server` with your chosen arguments (although for testing purposes we only used default values). To test the server, we have added a test_cache_server module in the Makefile which performs numerous `curl` operations and tests if the server is set up correctly. To test the client, run `./test_cache_client`.

We have run network tests for all the RESTful methods. We decided to use both common key-value pairs such as `("hello","two")` as well as uncommon ones such as null values `("null","")` and memory exceeding values. Our goal was to set a benchmark to make sure that all basic/normal queries should be processed correctly by our network.

While testing, we ran into a few problems which were fixed. We realized we were not using the space_used to decide whether an object exceeds the maxmem of the Cache. Furthermore, we learned that in order to prevent segmentation faults and different copies of the cache, we needed to open&close a new connection for each request in order to bind one cache object to the server's lifetime. There were several issues with getting the right datatype from Beast to the Cache and vice-versa, many of which were solved with the help of shared pointers and casting.
