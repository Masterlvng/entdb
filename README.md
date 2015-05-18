EntDB -- Embedded Key-Value database based on share-memory
=====

## Feature
High read performance with mmap

```
#include "entdb.h"
using namespace entdb;
using namespace std;

typedef struct asdf
{
    int a;
    float b;
} s_t;

int main()
{
    Entdb db = Entdb();
    db.Open("/tmp", "entssst");
    s_t s = {1,1.1};

    vector<char> input, ret;
    input.assign((char*)&s, (char*)&s + sizeof(s));

    db.Put("sdff", input);
    //string value;
    db.Get("sdff", &ret);
    s_t ss;
    memcpy(&ss, ret.data(), ret.size());
    cout << ss.b << endl;
}


```

##PERFORMANCE
```
Performance @ Intel(R) Core(TM) i5 CPU 2.67GHz, 6G ram

PUT 1000000 cost 32588 ms
PUT qps: 30686.14

Get key not in db

GET 1000000 cost 3147 ms
GET qps: 317762.94

Get key all in db

GET 1000000 cost 8005 ms
GET qps: 124921.92


```



##TODO
ONLY SUPPORT SINGLE THREAD
ADD NETWORK SUPPORT
...
Maybe do it in another project

