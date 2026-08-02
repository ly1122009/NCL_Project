#include <iostream>
#include <cstdint>
#include "Math.h"
#include "OSAL_Thread.h"
#include "OSAL_Semaphore.h"
#include "OSAL_Mutex.h"
#include "OSAL_Memory.h"
#include "NCL_Types.h"
#include "NCL_Core.h"

int main()
{
    std::cout << "Hello World!\n";
    Math my_math;
    
    int32_t res = my_math.add(1,2);
    std::cout << "My res:" << res << std::endl;
    return 0;
}