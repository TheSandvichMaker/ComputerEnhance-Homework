typedef u8 DecodeKind;
enum DecodeKind
{
	Decode_INVALID,

	Decode_MemToAccum,
	Decode_AccumToMem,
	Decode_RegMemToFromReg,
	Decode_ImmToRegMem,
	Decode_ImmToReg,
	Decode_ImmToAccum,
	Decode_JumpIpInc8,
};

global Mnemonic immed_table[] =
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

typedef u8 DecodeFlags;
enum DecodeFlags
{
	DecoderFlag_HasS = 1 << 0,
};

typedef struct DecodeParams
{
	DecodeKind kind;
	DecodeFlags flags;
} DecodeParams;

global thread_local bool decoder_table_initialized;
global thread_local DecodeParams decode_params[256];
global thread_local u8 instruction_kinds[256];

typedef struct Pattern
{
	u8 b1, b1_mask;
	Mnemonic mnemonic;
	DecodeKind decoder;
	Flags decode_flags;
} Pattern;

function void RegisterPattern(Pattern *pattern)
{
	decode_params[pattern->b1].kind = pattern->decoder;
	decode_params[pattern->b1].flags = pattern->decode_flags;
	instruction_kinds[pattern->b1] = pattern->mnemonic;

	// TODO(daniel): Figure out a way to not have to iterate all possible bit patterns for the fill
	for (u64 fill_pattern = 0; fill_pattern < 256; fill_pattern++)
	{
		u8 fill = (fill_pattern & ~pattern->b1_mask);
		if (fill != 0)
		{
			u8 lookup_pattern = (u8)(pattern->b1|fill);
			decode_params[lookup_pattern].kind = pattern->decoder;
			decode_params[lookup_pattern].flags = pattern->decode_flags;
			instruction_kinds[lookup_pattern] = pattern->mnemonic;
		}
	}
}

global Pattern patterns[] =
{
	{ .b1 = 0b10001000, .b1_mask = 0b11111100, .mnemonic = MOV, .decoder = Decode_RegMemToFromReg },
	{ .b1 = 0b11000110, .b1_mask = 0b11111110, .mnemonic = MOV, .decoder = Decode_ImmToRegMem     },
	{ .b1 = 0b10110000, .b1_mask = 0b11110000, .mnemonic = MOV, .decoder = Decode_ImmToReg        },
	{ .b1 = 0b10100000, .b1_mask = 0b11111110, .mnemonic = MOV, .decoder = Decode_MemToAccum      },
	{ .b1 = 0b10100010, .b1_mask = 0b11111110, .mnemonic = MOV, .decoder = Decode_AccumToMem      },

	// Immed group
	{ .b1 = 0b00000000, .b1_mask = 0b11000100, .decoder = Decode_RegMemToFromReg                               },
	{ .b1 = 0b10000000, .b1_mask = 0b11111100, .decoder = Decode_ImmToRegMem, .decode_flags = DecoderFlag_HasS },
	{ .b1 = 0b00000100, .b1_mask = 0b11000110, .decoder = Decode_ImmToAccum                                    },

	{ .b1 = 0b01110000, .mnemonic = JO,  .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01110001, .mnemonic = JNO, .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01110010, .mnemonic = JB,  .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01110011, .mnemonic = JAE, .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01110100, .mnemonic = JE,  .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01110101, .mnemonic = JNE, .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01110110, .mnemonic = JBE, .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01110111, .mnemonic = JA,  .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01111000, .mnemonic = JS,  .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01111001, .mnemonic = JNS, .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01111010, .mnemonic = JP,  .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01111011, .mnemonic = JPO, .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01111100, .mnemonic = JL,  .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01111101, .mnemonic = JGE, .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01111110, .mnemonic = JLE, .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b01111111, .mnemonic = JG,  .decoder = Decode_JumpIpInc8 },

	{ .b1 = 0b11100000, .mnemonic = LOOPNE, .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b11100001, .mnemonic = LOOPE,  .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b11100010, .mnemonic = LOOP,   .decoder = Decode_JumpIpInc8 },
	{ .b1 = 0b11100011, .mnemonic = JCXZ,   .decoder = Decode_JumpIpInc8 },
};

function void InitializeDecoderTable(void)
{
	for (u64 i = 0; i < ArrayCount(patterns); i++)
	{
		RegisterPattern(&patterns[i]);
	}

	decoder_table_initialized = true;
}

function void InitializeDecoder(Decoder *decoder, String source)
{
	decoder->base = (u8 *)source.bytes;
	decoder->at   = decoder->base;
	decoder->end  = decoder->base + source.count;

	if (!decoder_table_initialized)
	{
		InitializeDecoderTable();
	}
}

function void DecoderError(Decoder *decoder, String message)
{
	if (!decoder->error)
	{
		decoder->error_message = message;
	}

	decoder->error = true;
}

function bool ThereWereDecoderErrors(Decoder *decoder)
{
	return decoder->error;
}

function bool DecoderBytesLeft(Decoder *decoder)
{
	if (decoder->error)
	{
		return false;
	}

	return decoder->at < decoder->end;
}

function u8 DecoderReadU8(Decoder *decoder)
{
	u8 result = 0;
	if (decoder->at < decoder->end)
	{
		result = *decoder->at++;
	}
	else
	{
		DecoderError(decoder, StringLit("Expected byte!"));
	}
	return result;
}

function u16 DecoderReadU16(Decoder *decoder)
{
	u16 result = DecoderReadU8(decoder) | (DecoderReadU8(decoder) << 8);
	return result;
}

function s8 DecoderReadS8(Decoder *decoder)
{
	return (s8)DecoderReadU8(decoder);
}

function s16 DecoderReadS16(Decoder *decoder)
{
	return (s16)DecoderReadU16(decoder);
}

function u16 ReadUx(Decoder *decoder, u8 w)
{
	return w ? DecoderReadU16(decoder) : DecoderReadU8(decoder);
}

function s16 DecoderReadSx(Decoder *decoder, u8 w)
{
	return w ? DecoderReadS16(decoder) : DecoderReadS8(decoder);
}

function s16 DecoderReadDisp(Decoder *decoder, u8 mod, u8 r_m)
{
	s16 disp = 0;

	if (mod == 0x0 && r_m == 0x6)
	{
		disp = DecoderReadS16(decoder);
	}
	else if (mod == 0x1)
	{
		disp = DecoderReadS8(decoder);
	}
	else if (mod == 0x2)
	{
		disp = DecoderReadS16(decoder);
	}

	return disp;
}

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

function bool DecodeNextInstruction(Decoder *decoder, Instruction *inst)
{
	if (!DecoderBytesLeft(decoder))
	{
		return false;
	}

	ZeroStruct(inst);

	u8 b1 = DecoderReadU8(decoder);

	inst->mnemonic = instruction_kinds[b1];

	DecodeParams *params = &decode_params[b1];
	switch (params->kind)
	{
		case Decode_MemToAccum:
		{
			u8 w = b1 & 0x1;

			inst->dst.kind     = Operand_Reg;
			inst->dst.reg      = w ? AX : AL;

			inst->src.kind     = Operand_Mem;
			inst->src.mem.disp = DecoderReadS16(decoder);

			if (w) inst->flags |= W;
		} break;

		case Decode_AccumToMem:
		{
			u8 w = b1 & 0x1;

			inst->dst.kind     = Operand_Mem;
			inst->dst.mem.disp = DecoderReadS16(decoder);

			inst->src.kind     = Operand_Reg;
			inst->src.reg      = w ? AX : AL;

			if (w) inst->flags |= W;
		} break;

		case Decode_RegMemToFromReg:
		{
			if (!inst->mnemonic)
			{
				u8 op = (b1 >> 3) & 0x7;
				inst->mnemonic = immed_table[op];
			}

			u8 w = (b1 >> 0) & 0x1;
			u8 d = (b1 >> 1) & 0x1;

			u8 b2 = DecoderReadU8(decoder);

			u8 mod = (b2 >> 6) & 0x3;
			u8 reg = (b2 >> 3) & 0x7;
			u8 r_m = (b2 >> 0) & 0x7;

			s16 disp = DecoderReadDisp(decoder, mod, r_m);

			if (d)
			{
				inst->dst = DecodeRegister(w, reg);
				inst->src = DecodeEffectiveAddress(mod, w, r_m, disp);
			}
			else
			{
				inst->dst = DecodeEffectiveAddress(mod, w, r_m, disp);
				inst->src = DecodeRegister(w, reg);
			}

			if (w) inst->flags |= W;
			if (d) inst->flags |= D;
		} break;

		case Decode_ImmToRegMem:
		{
			u8 s = 0;
			if (params->flags & DecoderFlag_HasS)
			{
				s = (b1 >> 1) & 0x1;
			}

			u8 w = (b1 >> 0) & 0x1;

			u8 b2 = DecoderReadU8(decoder);

			if (!inst->mnemonic)
			{
				u8 op = (b2 >> 3) & 0x7;
				inst->mnemonic = immed_table[op];
			}

			u8 mod = (b2 >> 6) & 0x3;
			u8 r_m = (b2 >> 0) & 0x7;

			s16 disp = DecoderReadDisp(decoder, mod, r_m);
			inst->dst = DecodeEffectiveAddress(mod, w, r_m, disp);

			s16 data;
			if (s)
			{
				data = DecoderReadS8(decoder);
			}
			else
			{
				data = ReadUx(decoder, w);
			}

			InstSetData(inst, data);

			if (w) inst->flags |= W;
			if (s) inst->flags |= S;
		} break;

		case Decode_ImmToReg:
		{
			u8 w   = (b1 >> 3) & 0x1;
			u8 reg = (b1 >> 0) & 0x7;

			inst->dst = DecodeRegister(w, reg);

			s16 data = DecoderReadSx(decoder, w);
			InstSetData(inst, data);

			if (w) inst->flags |= W;
		} break;

		case Decode_ImmToAccum:
		{
			u8 w = (b1 >> 0) & 0x1;

			if (!inst->mnemonic)
			{
				u8 op = (b1 >> 3) & 0x7;
				inst->mnemonic = immed_table[op];
			}

			inst->dst.kind = Operand_Reg;
			inst->dst.reg  = w ? AX : AL;

			s16 data = DecoderReadSx(decoder, w);
			InstSetData(inst, data);

			if (w) inst->flags |= W;
		} break;

		case Decode_JumpIpInc8:
		{
			s8 ip_inc8 = DecoderReadS8(decoder);
			InstSetData(inst, ip_inc8);
		} break;

		default:
		{
			DecoderError(decoder, StringLit("Unexpected bit pattern"));
		} break;
	}

	return !ThereWereDecoderErrors(decoder);
}
