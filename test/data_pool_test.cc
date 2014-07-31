#include "../data_pool.h"
#include <iostream>
#include <vector>

using namespace entdb;
using namespace std;

typedef struct ss
{
    int a;
    float b;
} ss_t;
int main()
{
    DataPool* dp = new DataPool;
    dp->Open("/tmp/entdb_data",102400);
    //string data("asdfasfsjfiowfihnkjdbvcjvhkwhfi3fhsdkfhskfhsdkjfhskjfhskfhskfdskfjsdfiwjeofjsfosjfosjfo");
    /*
    ss_t ss = {2, 2.2}; 
    vector<char> v;
    vector<char> s;
    v.assign((char*)&ss, (char*)&ss + sizeof(ss));
    
    ss_t b;

    dp->Write(0, v.size(), v);
    dp->Read(0, v.size(), &s);
     
    memcpy(&b, s.data(), s.size());
    cout << b.b << endl;
    */
    string value("sdfdferer2312312");
    vector<char> v(value.begin(), value.end());
    int off = 0;
    dp->Write(off, v.size(), v);
    off += v.size();
    dp->Write(off, v.size(), v);
    off += v.size();
    dp->Write(off, v.size(), v);
    off += v.size();
    dp->Write(off, v.size(), v);
    off += v.size();
    dp->Write(off, v.size(), v);
    off += v.size();
    dp->Write(off, v.size(), v);
    
    dp->Write(off, v.size(), v);
    off += v.size();
    dp->Write(off, v.size(), v);
    off += v.size();
    dp->Write(off, v.size(), v);
    off += v.size();
    dp->Write(off, v.size(), v);
    off += v.size();
    dp->Write(off, v.size(), v);
    off += v.size();
    dp->Write(off, v.size(), v);

    return 1;
}
