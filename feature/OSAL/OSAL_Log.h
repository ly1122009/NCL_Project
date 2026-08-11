#ifndef __OSAL_LOG_H__
#define __OSAL_LOG_H__

#include "NCL_Core.h"
#include "NCL_Types.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _LOG_LEVEL {
  LOG_VERBOSE = 0,
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_NONE, /* disable all output */
} LOG_LEVEL;

NCL_ERRORTYPE NCL_OSAL_LogInit(const char *ident, int facility);
NCL_ERRORTYPE NCL_OSAL_LogSetLevel(LOG_LEVEL level);
void NCL_OSAL_Log(LOG_LEVEL level, const char *format, ...);
NCL_ERRORTYPE NCL_OSAL_LogDeinit(void);

#define LOGV(fmt, ...)                                                         \
  NCL_OSAL_Log(LOG_VERBOSE, "[%s:%d %s] " fmt, __FILE__, __LINE__, __func__,   \
               ##__VA_ARGS__)
#define LOGD(fmt, ...)                                                         \
  NCL_OSAL_Log(LOG_DEBUG, "[%s:%d %s] " fmt, __FILE__, __LINE__, __func__,     \
               ##__VA_ARGS__)
#define LOGI(fmt, ...)                                                         \
  NCL_OSAL_Log(LOG_INFO, "[%s:%d %s] " fmt, __FILE__, __LINE__, __func__,      \
               ##__VA_ARGS__)
#define LOGW(fmt, ...)                                                         \
  NCL_OSAL_Log(LOG_WARNING, "[%s:%d %s] " fmt, __FILE__, __LINE__, __func__,   \
               ##__VA_ARGS__)
#define LOGE(fmt, ...)                                                         \
  NCL_OSAL_Log(LOG_ERROR, "[%s:%d %s] " fmt, __FILE__, __LINE__, __func__,     \
               ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // __OSAL_LOG_H__