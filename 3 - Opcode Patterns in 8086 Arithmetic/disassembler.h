typedef struct Disassembler
{
	u8 *out_base;
	u8 *out_at;
	u8 *out_end;

	bool   error;
	String error_message;
} Disassembler;

function void InitializeDisassembler(Disassembler *disasm, u8 *output, size_t output_capacity);
function void DisassembleInstruction(Disassembler *disasm, Instruction *inst);
function String DisassemblerResult(Disassembler *disasm);
