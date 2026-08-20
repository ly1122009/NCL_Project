#include "OSAL_Log.h"
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define NCL_LOG_PREFIX "[NCL_LOG]"

static LOG_LEVEL g_maxLogLevel = LOG_INFO;
static struct timespec g_startTime = {0};

// Lấy Thread ID thực tế trên Linux
static pid_t _get_thread_id(void) {
#ifdef SYS_gettid
  return (pid_t)syscall(SYS_gettid);
#else
  return getpid();
#endif
}

// Tính thời gian trôi qua từ lúc Init (dạng giây.micro-giây)
static double _get_elapsed_time(void) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return (now.tv_sec - g_startTime.tv_sec) +
         (now.tv_nsec - g_startTime.tv_nsec) / 1e9;
}

// Lấy tên file ngắn gọn (bỏ đường dẫn thư mục)
static const char *_get_filename(const char *filepath) {
  const char *filename = strrchr(filepath, '/');
  return filename ? filename + 1 : filepath;
}

NCL_ERRORTYPE NCL_OSAL_LogInit(const char *ident, int facility) {
  (void)ident;
  (void)facility;
  clock_gettime(CLOCK_MONOTONIC, &g_startTime);
  return NCL_ErrorNone;
}

NCL_ERRORTYPE NCL_OSAL_LogSetLevel(LOG_LEVEL level) {
  g_maxLogLevel = level;
  return NCL_ErrorNone;
}

NCL_ERRORTYPE NCL_OSAL_LogDeinit(void) { return NCL_ErrorNone; }

void NCL_OSAL_Log(LOG_LEVEL level, const char *tag, const char *file, int line,
                  const char *format, ...) {
  if (level < g_maxLogLevel || level == LOG_NONE || format == NULL) {
    return;
  }

  const char *levelStr = "INFO ";
  switch (level) {
  case LOG_VERBOSE:
    levelStr = "VRBSE";
    break;
  case LOG_DEBUG:
    levelStr = "DEBUG";
    break;
  case LOG_INFO:
    levelStr = "INFO ";
    break;
  case LOG_WARNING:
    levelStr = "WARN ";
    break;
  case LOG_ERROR:
    levelStr = "ERROR";
    break;
  default:
    break;
  }

  double elapsed = _get_elapsed_time();

  // In theo chuẩn format kỳ vọng:
  // [%8.6f] [NCL_LOG] [PID: %d] [TID: %d] [%9.6f] [%s] [%-9s] %s:%d
  printf("[%8.6f] %s [PID: %d] [TID: %d] [%s] [%-9s] %s:%d ", elapsed,
         NCL_LOG_PREFIX, getpid(), _get_thread_id(), levelStr,
         tag ? tag : "APP", _get_filename(file), line);

  // In nội dung Message
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  printf("\n");
}
