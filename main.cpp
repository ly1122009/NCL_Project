#include <iostream>
#include <cstdint>
#include "Math.h"


int main()
{
    std::cout << "Hello World!\n";
    Math my_math;
    
    int32_t res = my_math.add(1,2);
    std::cout << "My res:" << res << std::endl;
    return 0;
}