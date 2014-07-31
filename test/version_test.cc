#include "../version.h"

#include <iostream>
#include <unistd.h>

using namespace std;
using namespace entdb;

int main()
{
    Version v;
    v.Open("/tmp/version");
    unsigned int i;
    for (i = 0; i < 1000000; i++)
    {
        v.IncVersion(1);
        cout << v.CurVersion(1) << endl;
    }
    return 1;
}
