#include "stdio_compat.h"
#include <stdarg.h>
#include <windows.h>
#include "log.h"

void platformLog(const logType type, const char *format, va_list va) {
    char buffer[2048];
    const char* prefix = "";
    switch (type) {
        case LOG_TYPE_NORMAL:  prefix = "[Butterscotch INFO] "; break;
        case LOG_TYPE_WARNING: prefix = "[Butterscotch WARN] "; break;
        case LOG_TYPE_ERROR:   prefix = "[Butterscotch ERROR] "; break;
        case LOG_TYPE_DEBUG:   prefix = "[Butterscotch DEBUG] "; break;
    }
    char message[1800];
    vsnprintf(message, sizeof(message), format, va);
    snprintf(buffer, sizeof(buffer), "%s%s\n", prefix, message);
    OutputDebugStringA(buffer);
}
