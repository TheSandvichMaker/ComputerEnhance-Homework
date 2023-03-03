#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define function static inline

typedef unsigned char uchar;

typedef struct String
{
	size_t       count;
	const uchar *bytes;
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
		.bytes = (const uchar *)c_string,
	};
	return result;
}

#define StringExpand(string) (int)((string).count), (char *)(string).bytes
#define StringLit(string) (String){ sizeof(string) - 1, (const uchar *)string }
#define StringLitConst(string) { sizeof(string) - 1, (const uchar *)string }
