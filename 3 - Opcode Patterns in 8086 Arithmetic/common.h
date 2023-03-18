#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#define function static inline
#define global   static

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef struct String
{
	size_t    count;
	const u8 *bytes;
} String;

function size_t CountCString(const char *c_string)
{
	size_t count = 0;

	while (*c_string++)
	{
		count++;
	}

	return count;
}

function String StringFromCString(const char *c_string)
{
	String result = 
	{
		.count = CountCString(c_string),
		.bytes = (const u8 *)c_string,
	};
	return result;
}

#define StringExpand(string) (int)((string).count), (char *)(string).bytes
#define StringLit(string) (String){ sizeof(string) - 1, (const u8 *)string }
#define StringLitConst(string) { sizeof(string) - 1, (const u8 *)string }

#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Abs(a) ((a) >= 0 ? (a) : -(a))

#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))
#define ZeroStruct(a) memset(a, 0, sizeof(*(a)))

#define thread_local __declspec(thread)
