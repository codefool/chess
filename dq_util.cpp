#include <iostream>
#include "dq.h"

struct Data
{
    int a;
    int b;
};

int main(int argc, char **argv)
{
    DiskQueue dq("/home/codefool/tmp", "testqueue", sizeof(Data));
    Data d;
    for(int i(0); i < 10; i++)
    {
        d.a = i;
        d.b = i * 2;
        dq.push((const dq_data_t)&d);
    }
    while( dq.pop((dq_data_t)&d) )
        std::cout << d.a << ' ' << d.b << std::endl;
    return 0;
}