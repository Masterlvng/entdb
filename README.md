EntDB -- Embedded Key-Value database based on share-memory
=====

## Feature
Support data synchronization between processes
High performance

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

##TODO
Data Snapshot
More Test
