#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
	#include <stddef.h>
	#include <stdbool.h>

	struct kernel_api
	{
		void *(*malloc)(size_t size);
		void (*memcpy)(void *dst, void *src, size_t size);
		void (*free)(void *ptr);
		void (*printf)(const char *fmt, ...);
		void (*bug_on)(bool condition);
		void* (*atomic_create)(int value);
		void (*atomic_inc)(void *atomic);
		bool (*atomic_dec_and_test)(void *atomic);
		int (*atomic_read)(void *atomic);
		void (*atomic_delete)(void *atomic);
	};

	int cpp_init(struct kernel_api *kapi);
	void cpp_deinit(void);
	struct kernel_api *get_kapi(void);

#define PRINTF(fmt, ...)	\
		get_kapi()->printf("kcpp: " fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
