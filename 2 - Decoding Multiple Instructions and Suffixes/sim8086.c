#include "common.h"
#include "disassembler.h"

//
//
//

#include "disassembler.c"

//
//
//

typedef enum ReadFileError
{
	ReadFileError_None,
	ReadFileError_CouldNotOpenFile,
	ReadFileError_DestinationTooSmall,
	ReadFileError_Other,
} ReadFileError;

function ReadFileError ReadFileIntoMemory(const char *file_name, void *destination, size_t destination_size, size_t *bytes_read)
{
	ReadFileError result = ReadFileError_None;

	FILE *f = fopen(file_name, "rb");

	if (!f)
	{
		result = ReadFileError_CouldNotOpenFile;
		goto bail;
	}

	fseek(f, 0, SEEK_END);
	int file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (file_size == -1)
	{
		result = ReadFileError_Other;
		goto bail;
	}

	size_t read_size = file_size;

	if (destination_size < file_size)
	{
		result = ReadFileError_DestinationTooSmall;
		goto bail;
	}

	if (fread(destination, 1, read_size, f) != read_size)
	{
		result = ReadFileError_Other;
		goto bail;
	}

	if (bytes_read)
	{
		*bytes_read = read_size;
	}

bail:
	if (f)
	{
		fclose(f);
	}

	return result;
}

global u8 g_bytes[1 << 16];

int main(int argument_count, char **arguments)
{
	if (argument_count != 2)
	{
		fprintf(stderr, "Usage: %s [8086 binary to disassemble]\n", arguments[0]);
		return 1;
	}

	size_t bytes_read = 0;

	ReadFileError error = ReadFileIntoMemory(arguments[1], g_bytes, sizeof(g_bytes), &bytes_read);
	if (error != ReadFileError_None)
	{
		fprintf(stderr, "Failed to read file '%s' into memory!\n", arguments[1]);
		return 1;
	}

	String bytes = 
	{
		.count = bytes_read,
		.bytes = g_bytes,
	};

	printf("; disassembly for %s\n", arguments[1]);
	Disassemble(bytes);

	return 0;
}
