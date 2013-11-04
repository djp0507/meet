
#ifndef LOGCALLBACK_H
#define LOGCALLBACK_H

typedef enum LogPriority {
    LOG_UNKNOWN = 0,
    LOG_DEFAULT,    /* only for SetMinPriority() */
    LOG_VERBOSE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
    LOG_SILENT,     /* only for SetMinPriority(); must be last */
};

// ---------------------------------------------------------------------

/*
 * Simplified macro to send a verbose log message using the current LOG_TAG.
 */
#ifndef LOGV
#if LOG_NDEBUG
#define LOGV(...)   ((void)0)
#else
#define LOGV(...) ((void)LOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif
#endif

/*
 * Simplified macro to send a debug log message using the current LOG_TAG.
 */
#ifndef LOGMYD
#if LOG_NDEBUG
#define LOGD(...)   ((void)0)
#else
#define LOGD(...) ((void)LOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif
#endif

/*
 * Simplified macro to send an info log message using the current LOG_TAG.
 */
#ifndef LOGMYI
#define LOGI(...) ((void)LOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif


/*
 * Simplified macro to send a warning log message using the current LOG_TAG.
 */
#ifndef LOGMYW
#define LOGW(...) ((void)LOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

/*
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef LOGMYE
#define LOGE(...) ((void)LOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

/*
 * Log macro that allows you to specify a number for the priority.
 */
#ifndef LOGMY
#define LOG(priority, tag, ...) \
		__android_file_log_print(priority, tag, __VA_ARGS__)
#endif

namespace android {
	void android_log_callback(int level, char* text);

	void __android_file_log_print(int priority, const char *tag, const char *fmt, ...);
}
#endif
