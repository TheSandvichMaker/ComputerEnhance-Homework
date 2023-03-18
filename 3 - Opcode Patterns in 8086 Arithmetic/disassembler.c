typedef u8 Instruction;
enum Instruction
{
	Inst_None,

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

	Inst_Count,
};

global String mnemonics[Inst_Count] =
{
	[Inst_None] = StringLitConst("inst_none"),

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

typedef u8 DecoderKind;
enum DecoderKind
{
	Decoder_INVALID,

	Decoder_MOV_MemToAccum,
	Decoder_MOV_AccumToMem,
	Decoder_RegMemToFromReg,
	Decoder_ImmToRegMem,
	Decoder_ImmToReg,
	Decoder_ImmToAccum,
	Decoder_JumpIpInc8,
};

typedef u8 DecoderFlags;
enum DecoderFlags
{
	DecoderFlag_HasS = 1 << 0,
};

typedef struct Decoder
{
	DecoderKind kind;
	DecoderFlags flags;
} Decoder;

global thread_local bool decoder_table_initialized;
global thread_local Decoder decoders[256];
global thread_local u8 instruction_kinds[256];

typedef struct Pattern
{
	u8 b1, b1_mask;
	Instruction inst;
	DecoderKind decoder;
	Flags decode_flags;
} Pattern;

function void RegisterPattern(Pattern *pattern)
{
	decoders[pattern->b1].kind = pattern->decoder;
	decoders[pattern->b1].flags = pattern->decode_flags;
	instruction_kinds[pattern->b1] = pattern->inst;

	// TODO(daniel): Figure out a way to not have to iterate all possible bit patterns for the fill
	for (u64 fill_pattern = 0; fill_pattern < 256; fill_pattern++)
	{
		u8 fill = (fill_pattern & ~pattern->b1_mask);
		if (fill != 0)
		{
			u8 lookup_pattern = (u8)(pattern->b1|fill);
			decoders[lookup_pattern].kind = pattern->decoder;
			decoders[lookup_pattern].flags = pattern->decode_flags;
			instruction_kinds[lookup_pattern] = pattern->inst;
		}
	}
}

global Pattern patterns[] =
{
	{ .b1 = 0b10001000, .b1_mask = 0b11111100, .inst = MOV, .decoder = Decoder_RegMemToFromReg },
	{ .b1 = 0b11000110, .b1_mask = 0b11111110, .inst = MOV, .decoder = Decoder_ImmToRegMem     },
	{ .b1 = 0b10110000, .b1_mask = 0b11110000, .inst = MOV, .decoder = Decoder_ImmToReg        },
	{ .b1 = 0b10100000, .b1_mask = 0b11111110, .inst = MOV, .decoder = Decoder_MOV_MemToAccum  },
	{ .b1 = 0b10100010, .b1_mask = 0b11111110, .inst = MOV, .decoder = Decoder_MOV_AccumToMem  },

	// Immed group
	{ .b1 = 0b00000000, .b1_mask = 0b11000100, .decoder = Decoder_RegMemToFromReg },
	{ .b1 = 0b10000000, .b1_mask = 0b11111100, .decoder = Decoder_ImmToRegMem, .decode_flags = DecoderFlag_HasS },
	{ .b1 = 0b00000100, .b1_mask = 0b11000110, .decoder = Decoder_ImmToAccum      },

	{ .b1 = 0b01110000, .inst = JO,  .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01110001, .inst = JNO, .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01110010, .inst = JB,  .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01110011, .inst = JAE, .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01110100, .inst = JE,  .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01110101, .inst = JNE, .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01110110, .inst = JBE, .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01110111, .inst = JA,  .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01111000, .inst = JS,  .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01111001, .inst = JNS, .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01111010, .inst = JP,  .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01111011, .inst = JPO, .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01111100, .inst = JL,  .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01111101, .inst = JGE, .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01111110, .inst = JLE, .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b01111111, .inst = JG,  .decoder = Decoder_JumpIpInc8 },

	{ .b1 = 0b11100000, .inst = LOOPNE, .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b11100001, .inst = LOOPE,  .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b11100010, .inst = LOOP,   .decoder = Decoder_JumpIpInc8 },
	{ .b1 = 0b11100011, .inst = JCXZ,   .decoder = Decoder_JumpIpInc8 },
};

function void InitializeDecoderTable(void)
{
	for (u64 i = 0; i < ArrayCount(patterns); i++)
	{
		RegisterPattern(&patterns[i]);
	}

	decoder_table_initialized = true;
}

typedef struct Disassembler
{
	String source_name;
	String source;
	u8 *at;
	u8 *end;
	
	u8 *out_base;
	u8 *out_at;
	u8 *out_end;

	bool   error;
	String error_message;
} Disassembler;

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

function Operand DecodeRegister(u8 w, u8 reg)
{
	Operand result =
	{
		.kind = Operand_Reg,
		.reg  = w ? AX + reg : AL + reg,
	};
	return result;
}

function Operand DecodeEffectiveAddress(u8 mod, u8 w, u8 r_m, s16 disp)
{
	Operand result = { 0 };

	if (mod == 0x3)
	{
		result = DecodeRegister(w, r_m);
	}
	else
	{
		result.kind = Operand_Mem;
		EffectiveAddress *ea = &result.mem;

		ea->disp = disp;
		if (!(mod == 0x0 && r_m == 0x6))
		{
			ea->reg1 = eac_register_table[r_m].reg1;
			ea->reg2 = eac_register_table[r_m].reg2;
		}
	}

	return result;
}

typedef struct DecodedInstruction
{
	Instruction inst;
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
} DecodedInstruction;

global Instruction immed_table[] =
{
	[0b000] = ADD,
	[0b001] = OR,
	[0b010] = ADC,
	[0b011] = SBB,
	[0b100] = AND,
	[0b101] = SUB,
	[0b110] = XOR,
	[0b111] = CMP,
};

function void DisassemblyError(Disassembler *state, String message)
{
	if (!state->error)
	{
		state->error_message = message;
	}

	state->error = true;
}

function bool ThereWereDisassemblyErrors(Disassembler *state)
{
	return state->error;
}

function size_t DsmWriteLeft(Disassembler *state)
{
	return state->out_end - state->out_at;
}

function void DsmWriteS(Disassembler *state, String string)
{
	size_t count_left  = DsmWriteLeft(state);
	size_t count_write = Min(count_left, string.count);
	for (size_t i = 0; i < count_write; i++)
	{
		*state->out_at++ = string.bytes[i];
	}

	if (count_write < string.count)
	{
		DisassemblyError(state, StringLit("Output buffer overflow!"));
	}
}

function void DsmWriteC(Disassembler *state, char c)
{
	if (state->out_at < state->out_end)
	{
		*state->out_at++ = (u8)c;
	}
	else
	{
		DisassemblyError(state, StringLit("Output buffer overflow!"));
	}
}

function void DsmWriteInt(Disassembler *state, int i)
{
	if (i < 0)
	{
		DsmWriteC(state, '-');
		i = -i;
	}

	u8 *start = state->out_at;

	do
	{
		int d = i % 10;
		DsmWriteC(state, (char)('0' + d));

		i /= 10;
	}
	while (i > 0);

	u8 *end = state->out_at;

	while (start < end)
	{
		u8 t = *--end;
		*end = *start;
		*start++ = t;
	}
}

function void DsmWrite(Disassembler *state, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	while (*fmt)
	{
		char f = *fmt++;

		if (f == '%')
		{
			f = *fmt++;

			bool print_positive_sign = false;
			if (f == '+')
			{
				print_positive_sign = true;
				f = *fmt++;
			}

			switch (f)
			{
				case 'c':
				{
					DsmWriteC(state, va_arg(args, char));
				} break;

				case 's':
				{
					DsmWriteS(state, va_arg(args, String));
				} break;

				case 'i':
				{
					int i = va_arg(args, int);
					if (print_positive_sign && i >= 0)
					{
						DsmWriteC(state, '+');
					}
					DsmWriteInt(state, i);
				} break;

				case '%':
				{
					DsmWriteC(state, '%');
				} break;

				default:
				{
					DsmWriteC(state, f);
				} break;
			}
		}
		else
		{
			DsmWriteC(state, f);
		}

		if (ThereWereDisassemblyErrors(state))
		{
			break;
		}
	}

	va_end(args);
}

function bool BytesLeft(Disassembler *state)
{
	if (state->error)
	{
		return false;
	}

	return state->at < state->end;
}

function u8 ReadU8(Disassembler *state)
{
	u8 result = 0;
	if (state->at < state->end)
	{
		result = *state->at++;
	}
	else
	{
		DisassemblyError(state, StringLit("Expected byte!"));
	}
	return result;
}

function u16 ReadU16(Disassembler *state)
{
	u16 result = ReadU8(state) | (ReadU8(state) << 8);
	return result;
}

function s8 ReadS8(Disassembler *state)
{
	return (s8)ReadU8(state);
}

function s16 ReadS16(Disassembler *state)
{
	return (s16)ReadU16(state);
}

function u16 ReadUx(Disassembler *state, u8 w)
{
	return w ? ReadU16(state) : ReadU8(state);
}

function s16 ReadSx(Disassembler *state, u8 w)
{
	return w ? ReadS16(state) : ReadS8(state);
}

function s16 ReadDisp(Disassembler *state, u8 mod, u8 r_m)
{
	s16 disp = 0;

	if (mod == 0x0 && r_m == 0x6)
	{
		disp = ReadS16(state);
	}
	else if (mod == 0x1)
	{
		disp = ReadS8(state);
	}
	else if (mod == 0x2)
	{
		disp = ReadS16(state);
	}

	return disp;
}

function void InstSetData(DecodedInstruction *decoded, s16 data)
{
	decoded->data = data;
	decoded->flags |= InstructionFlag_HasData;
}

function void DecodeNextInstruction(Disassembler *state, DecodedInstruction *decoded)
{
	ZeroStruct(decoded);

	u8 b1 = ReadU8(state);

	decoded->inst = instruction_kinds[b1];

	Decoder *decoder = &decoders[b1];
	switch (decoder->kind)
	{
		//
		// MOV:
		//

		case Decoder_MOV_MemToAccum:
		{
			// Memory to accumulator

			u8 w = b1 & 0x1;

			decoded->dst.kind     = Operand_Reg;
			decoded->dst.reg      = w ? AX : AL;

			decoded->src.kind     = Operand_Mem;
			decoded->src.mem.disp = ReadS16(state);

			if (w) decoded->flags |= W;
		} break;

		case Decoder_MOV_AccumToMem:
		{
			// Accumulator to memory

			u8 w = b1 & 0x1;

			decoded->dst.kind     = Operand_Mem;
			decoded->dst.mem.disp = ReadS16(state);

			decoded->src.kind     = Operand_Reg;
			decoded->src.reg      = w ? AX : AL;

			if (w) decoded->flags |= W;
		} break;

		//
		// Common (shared decoding patterns for ADD, ADC, SUB, SBB, CMP, ..?)
		//

		case Decoder_RegMemToFromReg:
		{
			// Register/memory to/from register

			if (!decoded->inst)
			{
				u8 op = (b1 >> 3) & 0x7;
				decoded->inst = immed_table[op];
			}

			u8 w = (b1 >> 0) & 0x1;
			u8 d = (b1 >> 1) & 0x1;

			u8 b2 = ReadU8(state);

			u8 mod = (b2 >> 6) & 0x3;
			u8 reg = (b2 >> 3) & 0x7;
			u8 r_m = (b2 >> 0) & 0x7;

			s16 disp = ReadDisp(state, mod, r_m);

			if (d)
			{
				decoded->dst = DecodeRegister(w, reg);
				decoded->src = DecodeEffectiveAddress(mod, w, r_m, disp);
			}
			else
			{
				decoded->dst = DecodeEffectiveAddress(mod, w, r_m, disp);
				decoded->src = DecodeRegister(w, reg);
			}

			if (w) decoded->flags |= W;
			if (d) decoded->flags |= D;
		} break;

		case Decoder_ImmToRegMem:
		{
			u8 s = 0;
			if (decoder->flags & DecoderFlag_HasS)
			{
				s = (b1 >> 1) & 0x1;
			}

			u8 w = (b1 >> 0) & 0x1;

			u8 b2 = ReadU8(state);

			if (!decoded->inst)
			{
				u8 op = (b2 >> 3) & 0x7;
				decoded->inst = immed_table[op];
			}

			u8 mod = (b2 >> 6) & 0x3;
			u8 r_m = (b2 >> 0) & 0x7;

			s16 disp = ReadDisp(state, mod, r_m);
			decoded->dst = DecodeEffectiveAddress(mod, w, r_m, disp);

			s16 data;
			if (s)
			{
				data = ReadS8(state);
			}
			else
			{
				data = ReadUx(state, w);
			}

			InstSetData(decoded, data);

			if (w) decoded->flags |= W;
			if (s) decoded->flags |= S;
		} break;

		case Decoder_ImmToReg:
		{
			// Immediate to register

			u8 w   = (b1 >> 3) & 0x1;
			u8 reg = (b1 >> 0) & 0x7;

			decoded->dst = DecodeRegister(w, reg);

			s16 data = ReadSx(state, w);
			InstSetData(decoded, data);

			if (w) decoded->flags |= W;
		} break;

		case Decoder_ImmToAccum:
		{
			u8 w = (b1 >> 0) & 0x1;

			if (!decoded->inst)
			{
				u8 op = (b1 >> 3) & 0x7;
				decoded->inst = immed_table[op];
			}

			decoded->dst.kind = Operand_Reg;
			decoded->dst.reg  = w ? AX : AL;

			s16 data = ReadSx(state, w);
			InstSetData(decoded, data);

			if (w) decoded->flags |= W;
		} break;

		//
		//
		//

		case Decoder_JumpIpInc8:
		{
			s8 ip_inc8 = ReadS8(state);
			InstSetData(decoded, ip_inc8);
		} break;

		//
		//
		//

		default:
		{
			DisassemblyError(state, StringLit("Unexpected bit pattern"));
		} break;
	}
}

function void DisassembleOperand(Disassembler *state, Operand *operand)
{
	switch (operand->kind)
	{
		case Operand_Reg:
		{
			DsmWrite(state, "%s", register_names[operand->reg]);
		} break;

		case Operand_Mem:
		{
			EffectiveAddress *ea = &operand->mem;

			DsmWriteC(state, '[');

			if (ea->reg1)
			{
				DsmWrite(state, "%s", register_names[ea->reg1]);
				if (ea->reg2)
				{
					DsmWrite(state, " + %s", register_names[ea->reg2]);
				}
			}

			if (ea->disp)
			{
				if (ea->reg1)
				{
					DsmWrite(state, " %c ", ea->disp >= 0 ? '+' : '-');
				}

				DsmWrite(state, "%i", Abs(ea->disp));
			}

			DsmWriteC(state, ']');
		} break;
	}
}

function void DisassembleInstruction(Disassembler *state, DecodedInstruction *decoded)
{
	String mnemonic = mnemonics[decoded->inst];

	DsmWrite(state, "%s ", mnemonic);

	switch (decoded->inst)
	{
		// Binary operators
		case ADD:
		case MOV:
		case OR:
		case ADC:
		case SBB:
		case AND:
		case SUB:
		case XOR:
		case CMP:
		{
			DisassembleOperand(state, &decoded->dst);
			DsmWriteC(state, ',');
			DsmWriteC(state, ' ');

			if (decoded->flags & InstructionFlag_HasData)
			{
				if (decoded->flags & W)
				{
					DsmWrite(state, "word %i", decoded->data);
				}
				else
				{
					DsmWrite(state, "byte %i", decoded->data);
				}
			}
			else
			{
				DisassembleOperand(state, &decoded->src);
			}
		} break;

		// IP-INC-8
		case JO:
		case JNO:
		case JB:
		case JAE:
		case JE:
		case JNE:
		case JBE:
		case JA:
		case JS:
		case JNS:
		case JP:
		case JPO:
		case JL:
		case JGE:
		case JLE:
		case JG:
		case LOOPNE:
		case LOOPE:
		case LOOP:
		case JCXZ:
		{
			s16 jmp = (s16)decoded->data + 2;
			DsmWrite(state, "$%c%i", jmp >= 0 ? '+' : '-', Abs(jmp));
		} break;
	}

	DsmWriteC(state, '\n');
}

function String Disassemble(DisassembleParams *params)
{
	if (!decoder_table_initialized)
	{
		InitializeDecoderTable();
	}

	Disassembler *state = &(Disassembler){
		.source_name = params->input_name,
		.source      = params->input,
		.at          = (u8 *)params->input.bytes,
		.end         = (u8 *)params->input.bytes + params->input.count,
		.out_base    = params->output,
		.out_at      = params->output,
		.out_end     = params->output + params->output_capacity,
	};

	DsmWrite(state, "; disassembly for %.*s\n", StringExpand(state->source_name));
	DsmWriteS(state, StringLit("bits 16\n"));

	while (BytesLeft(state))
	{
		DecodedInstruction decoded;
		DecodeNextInstruction(state, &decoded);

		if (ThereWereDisassemblyErrors(state))
		{
			fprintf(stderr, "Error while disassembling %.*s:\n\t%.*s\n\n", StringExpand(state->source_name), StringExpand(state->error_message));
			break;
		}
		else
		{
			DisassembleInstruction(state, &decoded);
		}
	}

	String result = 
	{
		.bytes = state->out_base,
		.count = state->out_at - state->out_base,
	};

	return result;
}
