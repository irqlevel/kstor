#undef TRACE_SYSTEM
#define TRACE_SYSTEM kstor

#if !defined(_KAPI_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _KAPI_TRACE_H

#include <linux/tracepoint.h>

#define KAPI_TRACE_MSG_CHARS 200

TRACE_EVENT(msg,
        TP_PROTO(const char *message),
        TP_ARGS(message),

        TP_STRUCT__entry(
                __dynamic_array(char, message, KAPI_TRACE_MSG_CHARS)
        ),

        TP_fast_assign(
                char *output = __get_str(message);

                snprintf(output, KAPI_TRACE_MSG_CHARS, "%s", message);
                output[KAPI_TRACE_MSG_CHARS - 1] = '\0';
        ),

        TP_printk("%s", __get_str(message))
);

#endif /* _KAPI_TRACE_H */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ../system
#define TRACE_INCLUDE_FILE trace
#include <trace/define_trace.h>
