#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
	#include <stddef.h>
	struct kernel_api
	{
		void *(*malloc)(size_t size);
		void (*memcpy)(void *dst, void *src, size_t size);
		void (*free)(void *ptr);
		void (*printf)(const char *fmt, ...);
		void (*bug_on)(bool condition);
	};

	int cpp_init(struct kernel_api *kapi);
	void cpp_deinit(void);
	struct kernel_api *cpp_get_kapi(void);

#define PRINTF(fmt, ...)	\
		cpp_get_kapi()->printf("kcpp: " fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
