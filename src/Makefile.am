lib_LTLIBRARIES=libripc.la
include_HEADERS=ripc.h common.h config.h memory.h resolver.h resources.h
libripc_la_SOURCES=ripc.c memory.c resolver.c 

if NETARCH_INFINIBAND
  SUBDIRS=infiniband
  libripc_la_LIBADD= infiniband/libripc.la
endif
if NETARCH_BGP
  SUBDIRS=bgp
  libripc_la_LIBADD= infiniband/libripc.la
endif
if NETARCH_LOCAL
  SUBDIRS=local
  libripc_la_LIBADD= local/libripc.la
endif
