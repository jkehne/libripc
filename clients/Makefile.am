bin_PROGRAMS=client server ripc_pingpong_server ripc_pingpong_client socket_pingpong_server socket_pingpong_client

include_HEADERS=../src/ripc.h config.h common.h

client_SOURCES=client.c
client_LDADD=../src/libripc.la
client_LDFLAGS=-static

server_SOURCES=server.c
server_LDADD=../src/libripc.la
server_LDFLAGS=-static

ripc_pingpong_server_SOURCES=ripc_pingpong_server.c
ripc_pingpong_server_LDADD=../src/libripc.la
ripc_pingpong_server_LDFLAGS=-static

ripc_pingpong_client_SOURCES=ripc_pingpong_client.c
ripc_pingpong_client_LDADD=../src/libripc.la
ripc_pingpong_client_LDFLAGS=-static

socket_pingpong_server_SOURCES=socket_pingpong_server.c
socket_pingpong_server_LDADD=../src/libripc.la
socket_pingpong_server_LDFLAGS=-static

socket_pingpong_client_SOURCES=socket_pingpong_client.c
socket_pingpong_client_LDADD=../src/libripc.la
socket_pingpong_client_LDFLAGS=-static