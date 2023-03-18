typedef struct DisassemblerStyle
{
	bool show_original_bytes;
	int  show_original_bytes_base;
} DisassemblerStyle;

typedef struct DisassemblerParams
{
	String input;
	Buffer output;

	DisassemblerStyle style;
} DisassemblerParams;

typedef struct Disassembler
{
	String source;

	u8 *out_base;
	u8 *out_at;
	u8 *out_end;

	u8 *line_start;

	bool   error;
	String error_message;

	DisassemblerStyle style;
} Disassembler;

function void InitializeDisassembler(Disassembler *disasm, DisassemblerParams *params);
function void DisassembleInstruction(Disassembler *disasm, Instruction *inst);
function void DisassemblerResetOutput(Disassembler *disasm, Buffer output);
function String DisassemblerResult(Disassembler *disasm);
