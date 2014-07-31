CC=g++
CFLAGS=-c -Wall -g -pg
INCLUDE_TEST=-I ..

all: fm_p_mgr_test.o fm_persistence_mgr.o fm_mgr_test  data_pool_test.o data_pool.o data_pool_test \
	memory_mgr_test.o memory_mgr.o memory_mgr_test util.o sk_index_test entdb_test.o entdb_test entdb.o \
	version.o version_test.o version_test

fm_mgr_test: fm_p_mgr_test.o fm_persistence_mgr.o
	$(CC) fm_p_mgr_test.o fm_persistence_mgr.o -o fm_mgr_test -g

fm_mgr_test.o: fm_mgr_test.cc ../fm_persistence_mgr.cc
	$(CC) -g fm_mgr_test.cc -c

fm_persistence_mgr.o: ../fm_persistence_mgr.cc
	$(CC) -g ../fm_persistence_mgr.cc -c

data_pool_test: data_pool_test.o data_pool.o
	$(CC) data_pool_test.o data_pool.o -o data_pool_test -g

data_pool_test.o: data_pool_test.cc ../data_pool.cc
	$(CC) -g data_pool_test.cc -c

data_pool.o: ../data_pool.cc
	$(CC) -g ../data_pool.cc -c

memory_mgr_test: memory_mgr_test.o data_pool.o memory_mgr.o fm_persistence_mgr.o util.o
	$(CC) -g memory_mgr.o memory_mgr_test.o data_pool.o fm_persistence_mgr.o util.o  -o memory_mgr_test 

memory_mgr_test.o: memory_mgr_test.cc ../memory_mgr.cc
	$(CC) -g memory_mgr_test.cc -c

memory_mgr.o: ../memory_mgr.cc 
	$(CC) -g ../memory_mgr.cc -c

util.o: ../util.cc
	$(CC) -g ../util.cc -c

sk_index.o: ../sk_index.cc 
	$(CC) -g ../sk_index.cc -c

sk_index_test: sk_index.o sk_index_test.o
	$(CC) -g sk_index.o sk_index_test.o -o sk_index_test

sk_index_test.o: sk_index_test.cc
	$(CC) -g sk_index_test.cc -c 

entdb_test: sk_index.o data_pool.o memory_mgr.o entdb.o entdb_test.o fm_persistence_mgr.o util.o
	$(CC) -g sk_index.o data_pool.o memory_mgr.o entdb.o entdb_test.o fm_persistence_mgr.o util.o -o entdb_test

entdb_test.o: entdb_test.cc
	$(CC) -g entdb_test.cc -c

entdb.o: ../entdb.cc
	$(CC) -g ../entdb.cc -c

version.o: ../version.cc
	$(CC) -g ../version.cc -c

version_test.o: version_test.cc
	$(CC) -g version_test.cc -c

version_test: version_test.o version.o
	$(CC) -g $^ -o version_test


clean:
	rm *.o /tmp/ent*
