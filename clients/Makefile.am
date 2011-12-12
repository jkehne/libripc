bin_PROGRAMS=client server
include_HEADERS=../src/ripc.h config.h
client_SOURCES=client.c
client_LDADD=../src/libripc.la
client_LDFLAGS=-static
server_SOURCES=server.c
server_LDADD=../src/libripc.la
server_LDFLAGS=-static