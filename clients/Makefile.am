bin_PROGRAMS=client server benchmark_resolver benchmark_resolver_server ripc_pingpong_server ripc_pingpong_client ripc_long_pingpong_server ripc_long_pingpong_client socket_pingpong_server socket_pingpong_client jetty_bench

include_HEADERS=../src/ripc.h

client_SOURCES=client.c
client_LDADD=../src/libripc.la
client_LDFLAGS=-static

server_SOURCES=server.c
server_LDADD=../src/libripc.la
server_LDFLAGS=-static

benchmark_resolver_SOURCES=benchmark_resolver.c
benchmark_resolver_LDADD=../src/libripc.la
benchmark_resolver_LDFLAGS=-static

benchmark_resolver_server_SOURCES=benchmark_resolver_server.c
benchmark_resolver_server_LDADD=../src/libripc.la
benchmark_resolver_server_LDFLAGS=-static

ripc_pingpong_server_SOURCES=ripc_pingpong_server.c
ripc_pingpong_server_LDADD=../src/libripc.la
ripc_pingpong_server_LDFLAGS=-static

ripc_pingpong_client_SOURCES=ripc_pingpong_client.c
ripc_pingpong_client_LDADD=../src/libripc.la
ripc_pingpong_client_LDFLAGS=-static

ripc_long_pingpong_server_SOURCES=ripc_long_pingpong_server.c
ripc_long_pingpong_server_LDADD=../src/libripc.la
ripc_long_pingpong_server_LDFLAGS=-static

ripc_long_pingpong_client_SOURCES=ripc_long_pingpong_client.c
ripc_long_pingpong_client_LDADD=../src/libripc.la
ripc_long_pingpong_client_LDFLAGS=-static

socket_pingpong_server_SOURCES=socket_pingpong_server.c
socket_pingpong_server_LDADD=../src/libripc.la
socket_pingpong_server_LDFLAGS=-static

socket_pingpong_client_SOURCES=socket_pingpong_client.c
socket_pingpong_client_LDADD=../src/libripc.la
socket_pingpong_client_LDFLAGS=-static

jetty_bench_SOURCES=jetty-bench.c
jetty_bench_LDADD=../src/libripc.la
jetty_bench_LDFLAGS=-static
