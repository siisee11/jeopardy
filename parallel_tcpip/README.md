# Concurrent TCP/IP networking

Server spawns producer thread per client.
Server spawns one consumer that consumes message from buffer.

```
g++ -std=c++11 -o server server.cpp -lpthread
gcc -o client client.c -lpthread
```
