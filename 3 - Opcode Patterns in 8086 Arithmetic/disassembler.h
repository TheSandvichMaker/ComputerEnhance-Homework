typedef struct DisassembleParams
{
	String input_name;
	String input;

	u8    *output;
	size_t output_capacity;
} DisassembleParams;

function String Diassemble(DisassembleParams *params);
