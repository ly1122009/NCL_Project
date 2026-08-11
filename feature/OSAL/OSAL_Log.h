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
NCL_ERRORTYPE NCL_OSAL_LogDeinit(void);

// Hàm Core xử lý format log (nhận thêm tag, file, line)
void NCL_OSAL_Log(LOG_LEVEL level, const char *tag, const char *file, int line,
                  const char *format, ...);

// Cập nhật các Macro truyền tag vào NCL_OSAL_Log
#define LOGV(tag, fmt, ...)                                                    \
  NCL_OSAL_Log(LOG_VERBOSE, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGD(tag, fmt, ...)                                                    \
  NCL_OSAL_Log(LOG_DEBUG, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGI(tag, fmt, ...)                                                    \
  NCL_OSAL_Log(LOG_INFO, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...)                                                    \
  NCL_OSAL_Log(LOG_WARNING, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGE(tag, fmt, ...)                                                    \
  NCL_OSAL_Log(LOG_ERROR, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // __OSAL_LOG_H__