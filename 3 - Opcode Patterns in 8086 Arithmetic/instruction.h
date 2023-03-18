typedef u8 Mnemonic;
enum Mnemonic
{
	Mnemonic_None,

	ADD,
	MOV,
	OR,
	ADC,
	SBB,
	AND,
	SUB,
	XOR,
	CMP,

	JO,
	JNO,
	JB,
	JAE,
	JE,
	JNE,
	JBE,
	JA,
	JS,
	JNS,
	JP,
	JPO,
	JL,
	JGE,
	JLE,
	JG,

	LOOPNE,
	LOOPE,
	LOOP,
	JCXZ,

	Mnemonic_Count,
};

global String mnemonic_names[Mnemonic_Count] =
{
	[Mnemonic_None] = StringLitConst("invalid mnemonic"),

	[ADD]    = StringLitConst("add"),
	[MOV]    = StringLitConst("mov"),
	[OR]     = StringLitConst("or"),
	[ADC]    = StringLitConst("adc"),
	[SBB]    = StringLitConst("sbb"),
	[AND]    = StringLitConst("and"),
	[SUB]    = StringLitConst("sub"),
	[XOR]    = StringLitConst("xor"),
	[CMP]    = StringLitConst("cmp"),

	[JO]     = StringLitConst("jo"),
	[JNO]    = StringLitConst("jno"),
	[JB]     = StringLitConst("jb"),
	[JAE]    = StringLitConst("jae"),
	[JE]     = StringLitConst("je"),
	[JNE]    = StringLitConst("jne"),
	[JBE]    = StringLitConst("jbe"),
	[JA]     = StringLitConst("ja"),
	[JS]     = StringLitConst("js"),
	[JNS]    = StringLitConst("jns"),
	[JP]     = StringLitConst("jp"),
	[JPO]    = StringLitConst("jpo"),
	[JL]     = StringLitConst("jl"),
	[JGE]    = StringLitConst("jge"),
	[JLE]    = StringLitConst("jle"),
	[JG]     = StringLitConst("jg"),

	[LOOPNE] = StringLitConst("loopne"),
	[LOOPE]  = StringLitConst("loope"),
	[LOOP]   = StringLitConst("loop"),
	[JCXZ]   = StringLitConst("jcxz"),
};

typedef u8 Flags;
enum Flags
{
	S = 1 << 0,
	W = 1 << 1,
	D = 1 << 2,
	V = 1 << 3,
	Z = 1 << 4,

	InstructionFlag_HasData = 1 << 5,
};

typedef u8 Register;
enum Register
{
	Reg_None,
	AL, CL, DL, BL, AH, CH, DH, BH,
	AX, CX, DX, BX, SP, BP, SI, DI,
};

global String register_names[] =
{
	StringLitConst("Invalid Register"),

	StringLitConst("al"),
	StringLitConst("cl"),
	StringLitConst("dl"),
	StringLitConst("bl"),
	StringLitConst("ah"),
	StringLitConst("ch"),
	StringLitConst("dh"),
	StringLitConst("bh"),

	StringLitConst("ax"),
	StringLitConst("cx"),
	StringLitConst("dx"),
	StringLitConst("bx"),
	StringLitConst("sp"),
	StringLitConst("bp"),
	StringLitConst("si"),
	StringLitConst("di"),
};

typedef struct EffectiveAddress
{
	Register reg1;
	Register reg2;
	s16 disp;
} EffectiveAddress;

typedef struct RegisterPair
{
	Register reg1, reg2;
} RegisterPair;

global RegisterPair eac_register_table[] = 
{
	{ BX, SI },
	{ BX, DI },
	{ BP, SI },
	{ BP, DI },
	{ SI },
	{ DI },
	{ BP },
	{ BX },
};

typedef u8 OperandKind;
enum OperandKind
{
	Operand_Reg,
	Operand_Mem,
};

typedef struct Operand
{
	OperandKind kind;
	union
	{
		Register reg;
		EffectiveAddress mem;
	};
} Operand;

typedef struct Instruction
{
	Mnemonic mnemonic;
	Operand dst;
	Operand src;
	Flags flags;
	union
	{
		struct
		{
			s8 data_lo;
			u8 data_hi;
		};
		s16 data;
	};

	u32 source_byte_offset;
	u32 source_byte_count;
} Instruction;

function void InstSetData(Instruction *inst, s16 data)
{
	inst->data = data;
	inst->flags |= InstructionFlag_HasData;
}
