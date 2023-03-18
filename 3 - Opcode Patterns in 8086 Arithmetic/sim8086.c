#include "common.h"
#include "instruction.h"
#include "decoder.h"
#include "disassembler.h"

//
//
//

#include "decoder.c"
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

global u8 g_input [1 << 16];
global u8 g_output[1 << 16];

#if 0
typedef struct ArgumentDescription
{
	String string;
	String explanation;
} ArgumentDescription;

typedef enum Argument
{
	Argument_ShowBytes,
	Argument_ShowBits,
	Argument_InputFile,
} Argument;

global ArgumentDescription program_arguments[] =
{
	[Argument_ShowBytes] =
	{
		.string      = StringLitConst("show-bytes"), 
		.explanation = StringLitConst("Add a comment to each disassembled line showing the original bytes in hexadecimal")
	},

	[Argument_ShowBits] =
	{ 
		.string      = StringLitConst("show-bits"),  
		.explanation = StringLitConst("Add a comment to each disassembled line showing the original bytes in binary")
	},

	[Argument_InputFile] =
	{ 
		.string = StringLitConst("%s"),
	}
};

typedef struct ArgumentParser
{
	int arg_at;
	int arg_count;
	char **args;

	int match_count;
	Argument *match_list;

	size_t string_count;
	String strings[10];
} ArgumentParser;

function Argument MatchArgument(ArgumentParser *parser)
{
	if (parser->arg_at >= parser->arg_count)
	{
		return -1;
	}

	char *arg = parser->args[parser->arg_at++];
	while (*arg)
	{
		char *start = arg;

		switch (*arg++)
		{
			case '-':
			{
			} break;

			case '"':
			{
			} break;
		}
		if (arg[0] == '-')
		{
		}
	}
}
#endif

int main(int argument_count, char **arguments)
{
	if (argument_count != 2)
	{
		fprintf(stderr, "Incorrect arguments\n");
		return 1;
	}

	String file_name = { 0 };

	bool show_bytes      = true;
	int  show_bytes_base = 2;

#if 0
	ArgumentParser arg_parser;
	InitializeArgumentParser(&arg_parser, argument_count, arguments, program_arguments);

	while (Argument argument = MatchArgument(&arg_parser))
	{
		switch (argument)
		{
			case Argument_ShowBytes:
			{
				show_bytes = true;
			} break;

			case Argument_ShowBits:
			{
				show_bytes      = true;
				show_bytes_base = 2;
			} break;

			case Argument_InputFile:
			{
				file_name = arg_parser.strings[0];
			} break;

			default:
			{
				return 1;
			} break;
		}
	}
#endif

	file_name = StringFromCString(arguments[1]);

	size_t bytes_read = 0;
	ReadFileError error = ReadFileIntoMemory(arguments[1], g_input, sizeof(g_input), &bytes_read);
	if (error != ReadFileError_None)
	{
		fprintf(stderr, "Failed to read file '%s' into memory!\n", arguments[1]);
		return 1;
	}

	String input = 
	{
		.count = bytes_read,
		.bytes = g_input,
	};

	Buffer output =
	{
		.capacity = sizeof(g_output),
		.bytes    = g_output,
	};

	Decoder *decoder = &(Decoder){ 0 };
	InitializeDecoder(decoder, input);

	Disassembler *disasm = &(Disassembler){ 0 };
	DisassemblerParams disasm_params =
	{
		.input  = input,
		.output = output,

		.style =
		{
			.show_original_bytes      = show_bytes,
			.show_original_bytes_base = show_bytes_base,
		},
	};
	InitializeDisassembler(disasm, &disasm_params);

	printf("; disassembly for %.*s\n", StringExpand(file_name));
	printf("bits 16\n");

	for (;;)
	{
		Instruction inst;

		if (!DecodeNextInstruction(decoder, &inst))
		{
			if (decoder->error)
			{
				fprintf(stderr, "\nError while decoding %.*s:\n\t%.*s\n\n", StringExpand(file_name), StringExpand(decoder->error_message));
			}
			break;
		}

		DisassemblerResetOutput(disasm, output);
		DisassembleInstruction(disasm, &inst);

		if (disasm->error)
		{
			fprintf(stderr, "Error while disassembling %.*s:\n\t%.*s\n\n", StringExpand(file_name), StringExpand(disasm->error_message));
			break;
		}

		String result = DisassemblerResult(disasm);
		printf("%.*s", StringExpand(result));
	}

	return 0;
}
