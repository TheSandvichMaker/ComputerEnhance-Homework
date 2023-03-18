#define MNEMONICS(_)   \
	_(ADD)             \
	_(MOV)             \
	_(OR)              \
	_(ADC)             \
	_(SBB)             \
	_(AND)             \
	_(SUB)             \
	_(XOR)             \
	_(CMP)             \
                       \
	_(TEST)            \
	_(NOT)             \
	_(NEG)             \
	_(MUL)             \
	_(IMUL)            \
	_(DIV)             \
	_(IDIV)            \
                       \
	_(POP)             \
                       \
	_(INC)             \
	_(DEC)             \
	_(CALL)            \
	_(JMP)             \
	_(PUSH)            \
                       \
	_(JO)              \
	_(JNO)             \
	_(JB)              \
	_(JAE)             \
	_(JE)              \
	_(JNE)             \
	_(JBE)             \
	_(JA)              \
	_(JS)              \
	_(JNS)             \
	_(JP)              \
	_(JPO)             \
	_(JL)              \
	_(JGE)             \
	_(JLE)             \
	_(JG)              \
                       \
	_(LOOPNE)          \
	_(LOOPE)           \
	_(LOOP)            \
	_(JCXZ)            \
	_(XCHG)            \
	                   \
	_(IN)              \
	_(OUT)             \

#define Mnemonic(name) name,

typedef u8 Mnemonic;
enum Mnemonic
{
	Mnemonic_None,

	MNEMONICS(Mnemonic)

	// These are only used for signalling to the decoder
	Mnemonic_Immed,
	Mnemonic_Grp,
	Mnemonic_Grp2,

	Mnemonic_Count,
};

#define mnemonic_names(name) [name] = StringLitConst(#name),

global String mnemonic_names[Mnemonic_Count] =
{
	[Mnemonic_None] = StringLitConst("<null mnemonic>"),
	MNEMONICS(mnemonic_names)
};

typedef u8 Flags;
enum Flags
{
	S = 1 << 0,
	W = 1 << 1,
	D = 1 << 2,
	V = 1 << 3,
	Z = 1 << 4,

	InstructionFlag_DataLO = 1 << 5,
	InstructionFlag_DataHI = 1 << 6,
};

typedef u8 Register;
enum Register
{
	Reg_None,
	AL, CL, DL, BL, AH, CH, DH, BH,
	AX, CX, DX, BX, SP, BP, SI, DI,
	ES, CS, SS, DS,
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

	StringLitConst("es"), 
	StringLitConst("cs"), 
	StringLitConst("ss"), 
	StringLitConst("ds"),
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
	Operand_None,
	Operand_Reg,
	Operand_SegReg,
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
	Operand op1;
	Operand op2;
	Flags flags;
	union
	{
		struct
		{
			u8 data_lo;
			u8 data_hi;
		};
		s16 data;
	};
	u32 source_byte_offset;
	u32 source_byte_count;
} Instruction;

function void InstSetData8(Instruction *inst, u8 data)
{
	inst->data_lo = data;
	inst->data_hi = 0;
	inst->flags |= InstructionFlag_DataLO;
}

function void InstSetData16(Instruction *inst, s16 data)
{
	inst->data = data;
	inst->flags |= InstructionFlag_DataLO|InstructionFlag_DataHI;
}

function void InstSetDataX(Instruction *inst, u8 w, s16 data)
{
	if (w)
	{
		InstSetData16(inst, data);
	}
	else
	{
		InstSetData8(inst, (u8)data);
	}
}
