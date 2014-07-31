#include "../sk_index.h"

#include <iostream>

using namespace std;
using namespace entdb;

int main()
{
    SKIndex ski;
    ski.Open("/tmp/entIndex", 1024);
    string s("asdf");
    /*
    ski.Put(s, 112, 1233);

    ski.Put("qweqwe", 112, 1233);

    ski.Put("qweqwerr11", 112, 1233);
    ski.Put("123rqweq", 112, 1233);
    ski.Put("plplpl23r", 112, 1233);
    ski.Put("plplo0-01234", 112, 1233);
    ski.Put("zdaweqwe21", 112, 1233);
    //uint64_t off, size;
    //ski.Get("asdf", &off, &size);
    //cout << off <<", "<< size << endl;
    cout << ski.NumIndex() << endl;
    */
    return 0;
}

