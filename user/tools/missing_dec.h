//I have not been able to find the original source of this file. I think
//Martin Harizanov wrote it. I have modified it as I went along.
//Martin Groenholdt

//Missing function prototypes in include folders. Gcc will warn on these if we don't define 'em anywhere.
//MOST OF THESE ARE GUESSED! but they seem to swork and shut up the compiler.

#ifndef MISSING_DEC_H
#define MISSING_DEC_H

#include "ets_sys.h"
#include "c_types.h"
#include <stdarg.h>

typedef s8 err_t;

extern int atoi(const char *nptr);

extern void ets_install_putc1(void *routine);

extern void ets_isr_attach(int intr, void *handler, void *arg);
extern void ets_isr_mask(unsigned intr);
extern void ets_isr_unmask(unsigned intr);

extern int ets_memcmp(const void *s1, const void *s2, size_t n);
extern void *ets_memcpy(void *dest, const void *src, size_t n);
extern void *ets_memset(void *s, int c, size_t n);

extern int ets_printf(const char *format, ...)  __attribute__ ((format (printf, 1, 2)));
extern int ets_sprintf(char *str, const char *format, ...)  __attribute__ ((format (printf, 2, 3)));
extern int ets_vsprintf(char *str, const char *format, va_list argptr);
extern int ets_vsnprintf(char *buffer, size_t sizeOfBuffer, const char *format, va_list argptr);

extern int ets_str2macaddr(void *, void *);

extern void ets_bzero(void *s, size_t n);

extern int ets_strcmp(const char *s1, const char *s2);
extern char *ets_strcpy(char *dest, const char *src);
extern size_t ets_strlen(char *s);
extern int ets_strncmp(const char *s1, const char *s2, int len);
extern char *ets_strncpy(char *dest, const char *src, size_t n);
extern char *ets_strstr(const char *haystack, const char *needle);

extern void ets_timer_arm_new(ETSTimer *a, int b, int c, int isMstimer);
extern void ets_timer_disarm(ETSTimer *a);
extern void ets_timer_setfn(ETSTimer *t, ETSTimerFunc *fn, void *parg);

extern int os_printf(const char *format, ...)  __attribute__ ((format (printf, 1, 2)));
extern int os_snprintf(char *str, size_t size, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
extern int os_printf_plus(const char *format, ...)  __attribute__ ((format (printf, 1, 2)));

extern void *pvPortMalloc(size_t size, const char *str, int lineno);
extern void *pvPortCalloc(size_t size, const char *str, int lineno);
extern void *pvPortRealloc(void *ptr, size_t size, const char *str, int lineno);
extern void *pvPortZalloc(size_t size, const char *str, int lineno);
extern void vPortFree(void *ptr, const char *str, int lineno);

extern void uart_div_modify(int no, unsigned int freq);

extern void ets_delay_us(int us);

extern void slop_wdt_feed();
extern void ets_wdt_enable(void);
extern void ets_wdt_disable(void);
extern void wdt_feed(void);

void _xtos_set_exception_handler();
void _ResetVector();

#endif
