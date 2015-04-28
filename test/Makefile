CC=g++
CFLAGS= -g -pg
INCLUDE_TEST=-I ..

all: fm_p_mgr_test.o fm_persistence_mgr.o fm_mgr_test  data_pool_test.o data_pool.o data_pool_test \
	memory_mgr_test.o memory_mgr.o memory_mgr_test util.o sk_index_test entdb_test.o entdb_test entdb.o \
	version.o version_test.o version_test sync_mgr_sign_test sync_mgr_wait_test sync_mgr.o sync_mgr_test.o \
	sync_mgr_test1.o benchmark.o benchmark

fm_mgr_test: fm_p_mgr_test.o fm_persistence_mgr.o
	$(CC) $(CFLAGS) fm_p_mgr_test.o fm_persistence_mgr.o -o fm_mgr_test

fm_mgr_test.o: fm_mgr_test.cc ../fm_persistence_mgr.cc
	$(CC) $(CFLAGS) fm_mgr_test.cc -c

fm_persistence_mgr.o: ../fm_persistence_mgr.cc
	$(CC) $(CFLAGS) ../fm_persistence_mgr.cc -c

data_pool_test: data_pool_test.o data_pool.o
	$(CC) data_pool_test.o data_pool.o -o data_pool_test -g

data_pool_test.o: data_pool_test.cc ../data_pool.cc
	$(CC) -g data_pool_test.cc -c

data_pool.o: ../data_pool.cc
	$(CC) $(CFLAGS) ../data_pool.cc -c

memory_mgr_test: memory_mgr_test.o data_pool.o memory_mgr.o fm_persistence_mgr.o util.o version.o sync_mgr.o
	$(CC) -g  $^  -o memory_mgr_test -lpthread  

memory_mgr_test.o: memory_mgr_test.cc ../memory_mgr.cc ../sync_mgr.cc
	$(CC) -g memory_mgr_test.cc -c

memory_mgr.o: ../memory_mgr.cc 
	$(CC) $(CFLAGS) ../memory_mgr.cc -c

util.o: ../util.cc
	$(CC) $(CFLAGS) ../util.cc -c

sk_index.o: ../sk_index.cc 
	$(CC) $(CFLAGS) ../sk_index.cc -c

sk_index_test: sk_index.o sk_index_test.o version.o sync_mgr.o
	$(CC) -g $^ -o sk_index_test -lpthread

sk_index_test.o: sk_index_test.cc
	$(CC) -g sk_index_test.cc -c 

entdb_test: sk_index.o data_pool.o memory_mgr.o entdb.o entdb_test.o fm_persistence_mgr.o util.o version.o sync_mgr.o
	$(CC) -g $^ -o entdb_test -lpthread

entdb_test.o: entdb_test.cc
	$(CC) -g entdb_test.cc -c

benchmark: sk_index.o data_pool.o memory_mgr.o entdb.o benchmark.o fm_persistence_mgr.o util.o version.o sync_mgr.o
	$(CC) $(CFLAGS) $^ -o benchmark -lpthread

benchmark.o: benchmark.cc
	$(CC) $(CFLAGS) benchmark.cc -c



entdb.o: ../entdb.cc
	$(CC) $(CFLAGS) ../entdb.cc -c

version.o: ../version.cc
	$(CC) $(CFLAGS) ../version.cc -c

version_test.o: version_test.cc
	$(CC) -g version_test.cc -c

version_test: version_test.o version.o
	$(CC) -g $^ -o version_test

sync_mgr.o: ../sync_mgr.cc
	$(CC) $(CFLAGS) $^ -c

sync_mgr_test.o: ./sync_mgr_test.cc
	$(CC) -g $^ -c 

sync_mgr_test1.o: ./sync_mgr_test1.cc
	$(CC) -g $^ -c 

sync_mgr_sign_test: sync_mgr.o sync_mgr_test.o
	$(CC) -g $^ -o sync_mgr_sign_test -lpthread

sync_mgr_wait_test: sync_mgr.o sync_mgr_test1.o
	$(CC) -g $^ -o sync_mgr_wait_test -lpthread


clean:
	rm *.o /tmp/ent* *_test
