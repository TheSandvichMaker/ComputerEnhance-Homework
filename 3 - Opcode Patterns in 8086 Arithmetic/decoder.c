global thread_local bool decoder_table_initialized;
global thread_local DecodeParams decode_params[256];
global thread_local u8 instruction_kinds[256];

function void RegisterPattern(Pattern *pattern)
{
	decode_params[pattern->b1].kind = pattern->decoder;
	decode_params[pattern->b1].flags = pattern->decode_flags;
	instruction_kinds[pattern->b1] = pattern->mnemonic;

	u8 b1_mask = pattern->b1_mask;

	if (!b1_mask)
	{
		b1_mask = 0b11111111;
	}

	for (u64 fill_pattern = 0; fill_pattern < 256; fill_pattern++)
	{
		u8 fill = (fill_pattern & ~b1_mask);
		if (fill != 0)
		{
			u8 lookup_pattern = (u8)(pattern->b1|fill);
			decode_params[lookup_pattern].kind = pattern->decoder;
			decode_params[lookup_pattern].flags = pattern->decode_flags;
			instruction_kinds[lookup_pattern] = pattern->mnemonic;
		}
	}
}

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

function s16 DecoderReadSX(Decoder *decoder, u8 w)
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

function Operand OperandFromRegister(Register reg)
{
	Operand result =
	{
		.kind = reg >= ES ? Operand_SegReg : Operand_Reg,
		.reg  = reg,
	};
	return result;
}

function Operand DecodeSegmentRegister(u8 seg_reg)
{
	Operand result =
	{
		.kind = Operand_SegReg,
		.reg  = ES + seg_reg,
	};
	return result;
}

function Operand DecodeEffectiveAddress(Decoder *decoder, u8 mod, u8 w, u8 r_m)
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

		ea->disp = DecoderReadDisp(decoder, mod, r_m);
		if (!(mod == 0x0 && r_m == 0x6))
		{
			ea->reg1 = eac_register_table[r_m].reg1;
			ea->reg2 = eac_register_table[r_m].reg2;
		}
	}

	return result;
}

function Operand OperandFromAddress(u16 address)
{
	Operand result =
	{
		.kind = Operand_Mem,
		.mem =
		{
			.disp = (s16)address,
		},
	};
	return result;
}

function bool DecodeNextInstruction(Decoder *decoder, Instruction *inst)
{
	ZeroStruct(inst);

	if (!DecoderBytesLeft(decoder))
	{
		return false;
	}

	inst->source_byte_offset = (u32)(decoder->at - decoder->base);

	u8 b1 = DecoderReadU8(decoder);
	inst->mnemonic = instruction_kinds[b1];

	DecodeParams *params = &decode_params[b1];
	switch (params->kind)
	{
		case Decode_MemToAccum:
		{
			u8 w = b1 & 0x1;

			inst->op1 = OperandFromRegister(w ? AX : AL);
			inst->op2 = OperandFromAddress(DecoderReadU16(decoder));
		} break;

		case Decode_AccumToMem:
		{
			u8 w = b1 & 0x1;

			inst->op1 = OperandFromAddress(DecoderReadU16(decoder));
			inst->op2 = OperandFromRegister(w ? AX : AL);
		} break;

		case Decode_RegMemToFromReg:
		{
			if (inst->mnemonic == Mnemonic_Immed)
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

			if (d)
			{
				inst->op1 = DecodeRegister(w, reg);
				inst->op2 = DecodeEffectiveAddress(decoder, mod, w, r_m);
			}
			else
			{
				inst->op1 = DecodeEffectiveAddress(decoder, mod, w, r_m);
				inst->op2 = DecodeRegister(w, reg);
			}
		} break;

		case Decode_ImmToRegMem:
		{
			u8 s = 0;
			if (params->flags & S)
			{
				s = (b1 >> 1) & 0x1;
			}

			u8 w = (b1 >> 0) & 0x1;

			u8 b2 = DecoderReadU8(decoder);

			if (inst->mnemonic == Mnemonic_Immed)
			{
				u8 op = (b2 >> 3) & 0x7;
				inst->mnemonic = immed_table[op];
			}

			u8 mod = (b2 >> 6) & 0x3;
			u8 r_m = (b2 >> 0) & 0x7;

			inst->op1 = DecodeEffectiveAddress(decoder, mod, w, r_m);

			s16 data;
			if (s)
			{
				data = DecoderReadS8(decoder);
			}
			else
			{
				data = ReadUx(decoder, w);
			}

			InstSetDataX(inst, s || w, data);
		} break;

		case Decode_ImmToReg:
		{
			u8 w   = (b1 >> 3) & 0x1;
			u8 reg = (b1 >> 0) & 0x7;

			inst->op1 = DecodeRegister(w, reg);

			s16 data = DecoderReadSX(decoder, w);
			InstSetDataX(inst, w, data);
		} break;

		case Decode_ImmToAccum:
		{
			u8 w = (b1 >> 0) & 0x1;

			if (inst->mnemonic == Mnemonic_Immed)
			{
				u8 op = (b1 >> 3) & 0x7;
				inst->mnemonic = immed_table[op];
			}

			inst->op1 = OperandFromRegister(w ? AX : AL);

			s16 data = DecoderReadSX(decoder, w);
			InstSetDataX(inst, w, data);
		} break;

		case Decode_JumpIpInc8:
		{
			s8 ip_inc8 = DecoderReadS8(decoder);
			InstSetData16(inst, ip_inc8); // 16 because I want it sign extended
		} break;

		case Decode_Reg:
		{
			u8 reg = b1 & 0x7;
			inst->op1 = DecodeRegister(1, reg);
		} break;

		case Decode_RegAccum:
		{
			u8 reg = b1 & 0x7;
			inst->op1 = OperandFromRegister(AX);
			inst->op2 = DecodeRegister(1, reg);
		} break;

		case Decode_SegReg:
		{
			u8 seg_reg = (b1 >> 3) & 0x3;
			inst->op1 = DecodeSegmentRegister(seg_reg);
		} break;

		case Decode_RegMem:
		{
			u8 b2 = DecoderReadU8(decoder);

			if (inst->mnemonic == Mnemonic_Grp2)
			{
				u8 op = (b2 >> 3) & 0x7;
				inst->mnemonic = grp2_table[op];
			}

			u8 mod = (b2 >> 6) & 0x3;
			u8 r_m = (b2 >> 0) & 0x7;

			inst->op1 = DecodeEffectiveAddress(decoder, mod, 1, r_m);
		} break;

		case Decode_IOFixedPort:
		{
			u8 w = b1 & 0x1;
			inst->op1 = OperandFromRegister(w ? AX : AL);
			InstSetData8(inst, DecoderReadU8(decoder));
		} break;

		case Decode_IOVariablePort:
		{
			u8 w = b1 & 0x1;
			inst->op1 = OperandFromRegister(w ? AX : AL);
			inst->op2 = OperandFromRegister(DX);
		} break;

		default:
		{
			DecoderError(decoder, StringLit("Unexpected bit pattern"));
		} break;
	}

	u32 source_byte_end = (u32)(decoder->at - decoder->base);
	inst->source_byte_count = source_byte_end - inst->source_byte_offset;

	return !ThereWereDecoderErrors(decoder);
}
