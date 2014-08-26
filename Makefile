CC=g++
CFLAGS=-c -Wall -g -pg
LDFLAGS=-g -pg
SOURCES=data_pool.cc util.cc sync_mgr.cc version.cc sk_index.cc status.cc memory_mgr.o fm_persistence_mgr.cc entdb.cc
HPP=skiplist.hpp
LIBRARY=entdb.a

OBJECTS=$(SOURCES:.cc=.o)
SKOBJECTS=$(HPP:.hpp=.o)

all: $(LIBRARY)
	mv *.o *.a build/

$(LIBRARY): $(OBJECTS) $(SKOBJECTS)
	rm -f $@
	ar -rs $@ $^

$(SKOBJECTS): $(HPP)
	$(CC) -c -o $@ $^

.cc.o:
	$(CC) $(CFLAGS) $< -o $@
clean:
	rm -f *.o
