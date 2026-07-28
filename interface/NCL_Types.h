#ifndef __OSAL_TYPES_H__
#define __OSAL_TYPES_H__

#include <cstdint>
#ifdef __cplusplus // avoid overloading
extern "C" {
#endif    

#include <stdint.h>

/**
 * @brief NCL_IN is used to identify inputs to an NCL functions
 * 
 */
#ifndef NCL_IN
#define NCL_IN
#endif // NCL_IN

/**
 * @brief NCL_OUT is used to identify outputs to an NCL functions
 * 
 */
#ifndef NCL_OUT
#define NCL_OUT
#endif // NCL_OUT

/**
 * @brief NCL_OUT is used to identify input and outputs to an NCL functions
 * 
 */
#ifndef NCL_INOUT
#define NCL_INOUT
#endif // NCL_INOUT

/**
 * @brief NCL_U8 is a 8 bit unsigned quantity that is byte aligned
 * 
 */
typedef uint8_t NCL_U8;

/**
 * @brief NCL_S8 is a 8 bit signed quantity that is byte aligned
 * 
 */
typedef int8_t NCL_S8;

/**
 * @brief NCL_U16 is a 16 bit unsigned quantity that is byte aligned
 * 
 */
typedef uint16_t NCL_U16;

/**
 * @brief NCL_S16 is a 16 bit signed quantity that is byte aligned
 * 
 */
typedef int16_t NCL_S16;

/**
 * @brief NCL_U32 is a 32 bit unsigned quantity that is byte aligned
 * 
 */
typedef uint32_t NCL_U32;

/**
 * @brief NCL_S32 is a 32 bit signed quantity that is byte aligned
 * 
 */
typedef int32_t NCL_S32;

/**
 * @brief NCL_BOOLEAN is used to represent a true or false value when passing parameters 
 * to and from the application and components
 */
typedef enum 
{
    NCL_FALSE = 0,
    NCL_TRUE = !NCL_FALSE,
    NCL_BOOLEAN_MAX = 0x7FFFFFFF
} NCL_BOOLEAN;

/**
 * @brief NCL_PTR is used to pass pointers between application and the component and more
 * 
 */
typedef void* NCL_PTR;

/**
 * @brief NCL_STRING is used to pass 'C' type strings between thé application
 * and the component and more.
 */f
typedef char* NCL_STRING;

/**
 * @brief NCL_TYPE type is used pass arrays of bytes such as buffers between the
 * application and the component and more.
 */
typedef unsigned char* NCL_BYTE;

/**
 * @brief NCL_UINTPTR depends on system address model. Itcan be 32bit or 64 bit
 * by system type.
 */
typedef uintptr_t NCL_UINTPTR;

/**
 * @brief NCL_HANDLETYPE define the public interface for the NCL Handle.
 * The core will not use this value internally, but the application should only use this
 */
typedef void* NCL_HANDLETYPE;

#ifdef __cplusplus
}
#endif

#endif // __OSAL_TYPES_H__