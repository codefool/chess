#include <iostream>
#include "dq.h"

struct Data
{
    int a;
    int b;
};

int main(int argc, char **argv)
{
    dreid::DiskQueue dq("/home/codefool/tmp", "testqueue", sizeof(Data));
    Data d;
    for(int j(0); j < 1024 * 1024; j++)
    {
        for(int i(0); i < 20; i++)
        {
            d.a = j * 100 + i;
            d.b = d.a * 2;
            dq.push((const dreid::dq_data_t)&d);
        }
        dq.pop((dreid::dq_data_t)&d);
        std::cout << d.a << ' ' << d.b << std::endl;
    }
    while( dq.pop((dreid::dq_data_t)&d) )
        std::cout << d.a << ' ' << d.b << std::endl;
    return 0;
}