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
	};

	int cpp_init(struct kernel_api *kapi);
	void cpp_deinit(void);
#ifdef __cplusplus
}
#endif
