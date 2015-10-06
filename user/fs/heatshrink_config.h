#ifndef HEATSHRINK_CONFIG_H
#define HEATSHRINK_CONFIG_H

/* Should functionality assuming dynamic allocation be used? */
#ifndef HEATSHRINK_DYNAMIC_ALLOC
#define HEATSHRINK_DYNAMIC_ALLOC 1
#endif

#if HEATSHRINK_DYNAMIC_ALLOC
#ifdef DB_ESP8266
	#include "user_config.h"

    #define HEATSHRINK_MALLOC(SZ) db_malloc(SZ, "Heatshrink alloc")
    #define HEATSHRINK_FREE(P, SZ) db_free(P)
#else
    /* Optional replacement of malloc/free */
    #define HEATSHRINK_MALLOC(SZ) malloc(SZ)
    #define HEATSHRINK_FREE(P, SZ) free(P)
#endif
#else
    /* Required parameters for static configuration */
    #define HEATSHRINK_STATIC_INPUT_BUFFER_SIZE 32
    #define HEATSHRINK_STATIC_WINDOW_BITS 8
    #define HEATSHRINK_STATIC_LOOKAHEAD_BITS 4
#endif

/* Turn on logging for debugging. */
#define HEATSHRINK_DEBUGGING_LOGS 1

/* Use indexing for faster compression. (This requires additional space.) */
#define HEATSHRINK_USE_INDEX 1

#ifdef HEATSHRINK_DEBUGGING_LOGS
#ifdef DB_ESP8266
		#include "user_config.h"
		
		#define LOG(...) debug(__VA_ARGS__)
		#define ASSERT(X) assert(X)
#else
		#include <stdio.h>
		#include <ctype.h>
		#include <assert.h>
		
		#define LOG(...) fprintf(stderr, __VA_ARGS__)
		#define ASSERT(X) assert(X)
#endif //DB_ESP8266
#else
#define LOG(...) /* no-op */
#define ASSERT(X) /* no-op */
#endif

#endif
