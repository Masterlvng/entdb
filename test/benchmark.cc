#include "../entdb.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

using namespace entdb;
using namespace std;

typedef struct asdf
{
    int a;
    float b;
} s_t;

std::string concate(std::string key, int i)
{
    stringstream ss;
    ss << key << i;
    return ss.str();
}

int main()
{
    Entdb db = Entdb();    
    db.Open("/tmp", "entssst");
    s_t s = {1,1.1};
    char payload[128];
    vector<char> input, ret;
    input.assign(payload, payload + 128);
    
    int i = 0;

    struct timeval start, end;
    gettimeofday(&start, NULL);
    for(; i < 1000000; i++)
    {
        db.Put(concate("sdffffcks",i), input);
    }
    
    gettimeofday(&end, NULL);
    unsigned long time_pass;
    time_pass = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;

    cout << "PUT 1000000 cost " << time_pass << " ms" << endl;
    float second_pass = time_pass / 1000.0;
    float rps = 1000000 / second_pass;
    printf("PUT qps: %.2f\n", rps);

    gettimeofday(&start, NULL);
    i = 0;
    for(; i < 1000000; i++)
    {
        db.Get(concate("sdffffcks",i), &ret);
    }
    
    gettimeofday(&end, NULL);
    time_pass = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;

    cout << "GET 1000000 cost " << time_pass << " ms" << endl;
    second_pass = time_pass / 1000.0;
    rps = 1000000 / second_pass;
    printf("GET qps: %.2f\n", rps);


    /* 
    map<string, string> m;

    vector<char> vs;
    string s("sdfsdfsdfiwejriwehrwerhksdferwerwerwerwer907897897werwernkqwhekqenmqwenwjxiwhxeqiuxqiowejqowehxqwoiehqoiweho1ohi231oi23knskdnfskdfskjdfhkjxcviojirperti[rot[[werjowpejrklwrlk ejoirer oi12344n2k34nl23j42l4j1l2jkl31jl23j1l3j1l23jl12j31kl3jl12j3l`j`l3jl1j2l3j1l23j1l23jj4j5j5j6j6j7j7j7j");

    int fd = open("test.log", O_RDWR|O_CREAT, 0644);
    int i = 0;
    for(;i < 1000000; ++i)
    {
        write(fd, s.c_str(), s.length());
    }
    vs.assign(s.begin(), s.end());
    struct timeval start, end;
    gettimeofday(&start, NULL);
    int i;
    for (i = 0; i < 1000000; i++)
    {
        db.Put(concate("ent_", i), vs);
    }
    gettimeofday(&end, NULL);
    unsigned long time_pass;
    time_pass = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
    cout << time_pass << " ms" << endl;
    gettimeofday(&start, NULL);
    for(i = 0; i < 1000000; i++)
    {
       db.Delete(concate("ent_", i));
    }

    gettimeofday(&end, NULL);

    time_pass = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
    cout << time_pass << " ms" << endl;
    */
    return 1;
}
