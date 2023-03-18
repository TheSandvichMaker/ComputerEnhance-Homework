typedef struct Decoder
{
	u8 *base;
	u8 *at;
	u8 *end;

	bool   error;
	String error_message;
} Decoder;

function void InitializeDecoder(Decoder *decoder, String source);
function bool DecodeNextInstruction(Decoder *decoder, Instruction *result);
function bool ThereWereDecoderErrors(Decoder *decoder);

//
//
//

typedef u8 DecodeKind;
enum DecodeKind
{
	Decode_INVALID,

	Decode_Single,
	Decode_Reg,
	Decode_RegAccum,
	Decode_SegReg,
	Decode_RegMem,
	Decode_RegMemToFromReg,
	Decode_ImmToRegMem,
	Decode_ImmToReg,
	Decode_ImmToAccum,
	Decode_MemToAccum,
	Decode_AccumToMem,
	Decode_JumpIpInc8,
	Decode_IOFixedPort,
	Decode_IOVariablePort,
};

global Mnemonic immed_table[] =
{
	ADD,
	OR,
	ADC,
	SBB,
	AND,
	SUB,
	XOR,
	CMP,
};

global Mnemonic group_one[] =
{
	TEST,
	Mnemonic_None,
	NOT,
	NEG,
	MUL,
	IMUL,
	DIV,
	IDIV,
};

global Mnemonic grp2_table[] =
{
	INC,
	DEC,
	CALL,
	CALL,
	JMP,
	JMP,
	PUSH,
	Mnemonic_None,
};

typedef struct DecodeParams
{
	DecodeKind kind;
	Flags flags;
} DecodeParams;

typedef struct Pattern
{
	u8 b1, b1_mask;
	Mnemonic mnemonic;
	DecodeKind decoder;
	Flags decode_flags;
} Pattern;

global Pattern patterns[] =
{
	{ .b1 = 0b10001000, .b1_mask = 0b11111100, .mnemonic = MOV,            .decoder = Decode_RegMemToFromReg,                     },
	{ .b1 = 0b11000110, .b1_mask = 0b11111110, .mnemonic = MOV,            .decoder = Decode_ImmToRegMem                          },
	{ .b1 = 0b10110000, .b1_mask = 0b11110000, .mnemonic = MOV,            .decoder = Decode_ImmToReg                             },
	{ .b1 = 0b10100000, .b1_mask = 0b11111110, .mnemonic = MOV,            .decoder = Decode_MemToAccum                           },
	{ .b1 = 0b10100010, .b1_mask = 0b11111110, .mnemonic = MOV,            .decoder = Decode_AccumToMem                           },

	{ .b1 = 0b00000000, .b1_mask = 0b11000100, .mnemonic = Mnemonic_Immed, .decoder = Decode_RegMemToFromReg,                     },
	{ .b1 = 0b10000000, .b1_mask = 0b11111100, .mnemonic = Mnemonic_Immed, .decoder = Decode_ImmToRegMem,      .decode_flags = S  },
	{ .b1 = 0b00000100, .b1_mask = 0b11000110, .mnemonic = Mnemonic_Immed, .decoder = Decode_ImmToAccum                           },

	{ .b1 = 0b01110000, .b1_mask = 0b11111111, .mnemonic = JO,             .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01110001, .b1_mask = 0b11111111, .mnemonic = JNO,            .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01110010, .b1_mask = 0b11111111, .mnemonic = JB,             .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01110011, .b1_mask = 0b11111111, .mnemonic = JAE,            .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01110100, .b1_mask = 0b11111111, .mnemonic = JE,             .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01110101, .b1_mask = 0b11111111, .mnemonic = JNE,            .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01110110, .b1_mask = 0b11111111, .mnemonic = JBE,            .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01110111, .b1_mask = 0b11111111, .mnemonic = JA,             .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01111000, .b1_mask = 0b11111111, .mnemonic = JS,             .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01111001, .b1_mask = 0b11111111, .mnemonic = JNS,            .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01111010, .b1_mask = 0b11111111, .mnemonic = JP,             .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01111011, .b1_mask = 0b11111111, .mnemonic = JPO,            .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01111100, .b1_mask = 0b11111111, .mnemonic = JL,             .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01111101, .b1_mask = 0b11111111, .mnemonic = JGE,            .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01111110, .b1_mask = 0b11111111, .mnemonic = JLE,            .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b01111111, .b1_mask = 0b11111111, .mnemonic = JG,             .decoder = Decode_JumpIpInc8                           },

	{ .b1 = 0b11100000, .b1_mask = 0b11111111, .mnemonic = LOOPNE,         .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b11100001, .b1_mask = 0b11111111, .mnemonic = LOOPE,          .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b11100010, .b1_mask = 0b11111111, .mnemonic = LOOP,           .decoder = Decode_JumpIpInc8                           },
	{ .b1 = 0b11100011, .b1_mask = 0b11111111, .mnemonic = JCXZ,           .decoder = Decode_JumpIpInc8                           },

	{ .b1 = 0b11111111, .b1_mask = 0b11111111, .mnemonic = Mnemonic_Grp2,  .decoder = Decode_RegMem                               },
	{ .b1 = 0b01010000, .b1_mask = 0b11111000, .mnemonic = PUSH,           .decoder = Decode_Reg                                  },
	{ .b1 = 0b00000110, .b1_mask = 0b11100111, .mnemonic = PUSH,           .decoder = Decode_SegReg                               },

	{ .b1 = 0b10001111, .b1_mask = 0b11111111, .mnemonic = POP,            .decoder = Decode_RegMem                               },
	{ .b1 = 0b01011000, .b1_mask = 0b11111000, .mnemonic = POP,            .decoder = Decode_Reg                                  },
	{ .b1 = 0b00000111, .b1_mask = 0b11100111, .mnemonic = POP,            .decoder = Decode_SegReg                               },

	{ .b1 = 0b10000110, .b1_mask = 0b11111110, .mnemonic = XCHG,           .decoder = Decode_RegMemToFromReg,                     },
	{ .b1 = 0b10010000, .b1_mask = 0b11111000, .mnemonic = XCHG,           .decoder = Decode_RegAccum,                            },

	{ .b1 = 0b11100100, .b1_mask = 0b11111110, .mnemonic = IN,             .decoder = Decode_IOFixedPort,                         },
	{ .b1 = 0b11101100, .b1_mask = 0b11111110, .mnemonic = IN,             .decoder = Decode_IOVariablePort,                      },
	{ .b1 = 0b11100110, .b1_mask = 0b11111110, .mnemonic = OUT,            .decoder = Decode_IOFixedPort,                         },
	{ .b1 = 0b11101110, .b1_mask = 0b11111110, .mnemonic = OUT,            .decoder = Decode_IOVariablePort,                      },
};
