typedef enum Opcode
{
	Op_INVALID,

	Op_MOV_RegMemToFromReg,
	Op_MOV_ImmToRegMem,
	Op_MOV_ImmToReg,
	Op_MOV_MemToAccum,
	Op_MOV_AccumToMem,

	Op_Common_RegMemToFromReg,
	Op_Common_ImmToRegMem,
	Op_Common_ImmToAccum,

	Op_JumpIpInc8,
} Opcode;

global thread_local bool opcode_lookup_initialized;
global thread_local u8   opcode_lookup[256];

function void RegisterOpcode(u8 pattern, u8 fill_mask, Opcode opcode)
{
	opcode_lookup[pattern] = opcode;

	// TODO(daniel): Figure out a way to not have to iterate all possible bit patterns for the fill
	for (u64 fill_pattern = 0; fill_pattern < 256; fill_pattern++)
	{
		u8 fill = (fill_pattern & fill_mask);
		if (fill != 0)
		{
			u8 lookup_pattern = (u8)(pattern|fill);
			opcode_lookup[lookup_pattern] = opcode;
		}
	}
}

function void InitializeOpcodeLookup(void)
{
	//
	// MOV:
	//

	RegisterOpcode(0b10001000, 0b00000011, Op_MOV_RegMemToFromReg);
	RegisterOpcode(0b11000110, 0b00000001, Op_MOV_ImmToRegMem);
	RegisterOpcode(0b10110000, 0b00001111, Op_MOV_ImmToReg);
	RegisterOpcode(0b10100000, 0b00000001, Op_MOV_MemToAccum);
	RegisterOpcode(0b10100010, 0b00000001, Op_MOV_AccumToMem);

	//
	// Shared between ADD, OR, ADC, SBB, AND, SUB, XOR, CMP:
	//

	RegisterOpcode(0b00000000, 0b00111011, Op_Common_RegMemToFromReg);
	RegisterOpcode(0b10000000, 0b00000011, Op_Common_ImmToRegMem);
	RegisterOpcode(0b00000100, 0b00111001, Op_Common_ImmToAccum);

	//
	//
	//

	RegisterOpcode(0b01110000, 0b00001111, Op_JumpIpInc8);
	RegisterOpcode(0b11100000, 0b00000011, Op_JumpIpInc8);
	// RegisterOpcode(0b11101001, 0b00000000, Op_JumpOrLoop);
	// RegisterOpcode(0b11101010, 0b00000000, Op_JumpOrLoop);
	// RegisterOpcode(0b11101011, 0b00000000, Op_JumpIpInc8);
	
	//
	//
	//

	opcode_lookup_initialized = true;
}

typedef struct Disassembler
{
	String source_name;
	String source;
	u8 *at;
	u8 *end;

	bool   error;
	String error_message;
} Disassembler;

global String register_names[2][8] =
{
	// W = 0
	{
		StringLitConst("al"),
		StringLitConst("cl"),
		StringLitConst("dl"),
		StringLitConst("bl"),
		StringLitConst("ah"),
		StringLitConst("ch"),
		StringLitConst("dh"),
		StringLitConst("bh"),
	},

	// W = 1
	{
		StringLitConst("ax"),
		StringLitConst("cx"),
		StringLitConst("dx"),
		StringLitConst("bx"),
		StringLitConst("sp"),
		StringLitConst("bp"),
		StringLitConst("si"),
		StringLitConst("di"),
	},
};

global String effective_address_calculations[8] =
{
	StringLitConst("bx + si"),
	StringLitConst("bx + di"),
	StringLitConst("bp + si"),
	StringLitConst("bp + di"),
	StringLitConst("si"),
	StringLitConst("di"),
	StringLitConst("bp"),
	StringLitConst("bx"),
};

global String common_instruction_patterns[] =
{
	[0b000] = StringLitConst("add"),
	[0b001] = StringLitConst("or"),
	[0b010] = StringLitConst("adc"),
	[0b011] = StringLitConst("sbb"),
	[0b100] = StringLitConst("and"),
	[0b101] = StringLitConst("sub"),
	[0b110] = StringLitConst("xor"),
	[0b111] = StringLitConst("cmp"),
};

global String jump_mnemonics[2][16] =
{
	{
		// 0111 ones
		StringLitConst("jo"),
		StringLitConst("jno"),
		StringLitConst("jb"),  
		StringLitConst("jnb"), 
		StringLitConst("je"),  
		StringLitConst("jne"),  
		StringLitConst("jbe"),  
		StringLitConst("jnbe"),  
		StringLitConst("js"),  
		StringLitConst("jns"),  
		StringLitConst("jp"),  
		StringLitConst("jnp"),  
		StringLitConst("jl"),  
		StringLitConst("jnl"),  
		StringLitConst("jle"),  
		StringLitConst("jnle"),  
	},

	{
		// 1110 ones
		StringLitConst("loopnz"),
		StringLitConst("loopz"),
		StringLitConst("loop"),
		StringLitConst("jcxz"),
	},
};

function void DisassemblyError(Disassembler *state, String message)
{
	state->error = true;
	state->error_message = message;
}

function bool ThereWereDisassemblyErrors(Disassembler *state)
{
	return state->error;
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

function s16 ReadDisp(Disassembler *state, u8 mod)
{
	s16 disp = 0;

	if (mod == 0x1)
	{
		disp = ReadS8(state);
	}
	else if (mod == 0x2)
	{
		disp = ReadS16(state);
	}

	return disp;
}

function void PrintMemoryOperand(u8 r_m, s16 disp)
{
	String mem_string = effective_address_calculations[r_m];

	if (disp)
	{
		printf("[%.*s %c %d]", StringExpand(mem_string), disp >= 0 ? '+' : '-', disp >= 0 ? disp : -disp);
	}
	else
	{
		printf("[%.*s]", StringExpand(mem_string));
	}
}

function void Disassemble(String source_name, String bytes)
{
	if (!opcode_lookup_initialized)
	{
		InitializeOpcodeLookup();
	}

	Disassembler *state = &(Disassembler){
		.source_name = source_name,
		.source      = bytes,
		.at          = (u8 *)bytes.bytes,
		.end         = (u8 *)bytes.bytes + bytes.count,
	};

	printf("; disassembly for %.*s\n", StringExpand(state->source_name));
	printf("bits 16\n");

	while (BytesLeft(state))
	{
		u8 b1 = ReadU8(state);

		Opcode opcode = opcode_lookup[b1];

		switch (opcode)
		{
			//
			// MOV:
			//

			case Op_MOV_RegMemToFromReg:
			{
				// Register/memory to/from register

				u8 w = (b1 >> 0) & 0x1;
				u8 d = (b1 >> 1) & 0x1;

				u8 b2 = ReadU8(state);

				u8 mod = (b2 >> 6) & 0x3;
				u8 reg = (b2 >> 3) & 0x7;
				u8 r_m = (b2 >> 0) & 0x7;

				if (mod == 0x3)
				{
					// Register-to-register

					u8 dst = d ? reg : r_m;
					u8 src = d ? r_m : reg;

					String dst_string = register_names[w][dst];
					String src_string = register_names[w][src];

					printf("mov %.*s, %.*s\n", StringExpand(dst_string), StringExpand(src_string));
				}
				else
				{
					// Register-to-memory/memory-to-register

					String reg_string = register_names[w][reg];

					printf("mov ");

					if (mod == 0x0 &&
						r_m == 0x6)
					{
						// Direct address

						// Probably, nobody cares to see a negative address
						u16 disp = ReadU16(state);

						if (d)
						{
							printf("%.*s, [%u]", StringExpand(reg_string), disp);
						}
						else
						{
							printf("[%u], %.*s", disp, StringExpand(reg_string));
						}
					}
					else
					{
						s16 disp = ReadDisp(state, mod);

						if (d)
						{
							printf("%.*s, ", StringExpand(reg_string));
							PrintMemoryOperand(r_m, disp);
						}
						else
						{
							PrintMemoryOperand(r_m, disp);
							printf(", %.*s", StringExpand(reg_string));
						}
					}

					printf("\n");
				}
			} break;

			case Op_MOV_ImmToRegMem:
			{
				// Immediate to register/memory

				u8 w = b1 & 0x1;

				u8 b2 = ReadU8(state);

				u8 mod = (b2 >> 6) & 0x3;
				u8 r_m = (b2 >> 0) & 0x7;

				s16 disp = ReadDisp(state, mod);
				s16 data = ReadSx(state, w);

				printf("mov ");

				if (mod == 0x3)
				{
					String reg_string = register_names[w][r_m];
					printf("%.*s, ", StringExpand(reg_string));
				}
				else
				{
					PrintMemoryOperand(r_m, disp);
					printf(", ");
				}

				if (w)
				{
					printf("word %d\n", data);
				}
				else
				{
					printf("byte %d\n", data);
				}
			} break;

			case Op_MOV_ImmToReg:
			{
				// Immediate to register

				u8 w   = (b1 >> 3) & 0x1;
				u8 reg = (b1 >> 0) & 0x7;

				s16 data = ReadSx(state, w);

				String reg_string = register_names[w][reg];

				if (data < 0)
				{
					printf("mov %.*s, %d ; %u\n", StringExpand(reg_string), data, (u16)data);
				}
				else
				{
					printf("mov %.*s, %d\n", StringExpand(reg_string), data);
				}
			} break;

			case Op_MOV_MemToAccum:
			{
				// Memory to accumulator

				u8 w = b1 & 0x1;

				u16 addr = ReadU16(state);

				printf("mov %s, [%u]\n", w ? "ax" : "al", addr);
			} break;

			case Op_MOV_AccumToMem:
			{
				// Accumulator to memory

				u8 w = b1 & 0x1;

				u16 addr = ReadU16(state);

				printf("mov [%u], %s\n", addr, w ? "ax" : "al");
			} break;

			//
			// Common (shared decoding patterns for ADD, ADC, SUB, SBB, CMP, ..?)
			//

			case Op_Common_RegMemToFromReg:
			{
				// Register/memory to/from register

				u8 op = (b1 >> 3) & 0x7;
				String op_string = common_instruction_patterns[op];

				u8 w = (b1 >> 0) & 0x1;
				u8 d = (b1 >> 1) & 0x1;

				u8 b2 = ReadU8(state);

				u8 mod = (b2 >> 6) & 0x3;
				u8 reg = (b2 >> 3) & 0x7;
				u8 r_m = (b2 >> 0) & 0x7;

				printf("%.*s ", StringExpand(op_string));

				if (mod == 0x3)
				{
					// Register-to-register

					u8 dst = d ? reg : r_m;
					u8 src = d ? r_m : reg;

					String dst_string = register_names[w][dst];
					String src_string = register_names[w][src];

					printf("%.*s, %.*s\n", StringExpand(dst_string), StringExpand(src_string));
				}
				else
				{
					// Register-to-memory/memory-to-register

					String reg_string = register_names[w][reg];

					if (mod == 0b00 &&
						r_m == 0b110)
					{
						// Direct address

						// Probably, nobody cares to see a negative address
						u16 disp = ReadU16(state);

						if (d)
						{
							printf("%.*s, [%u]", StringExpand(reg_string), disp);
						}
						else
						{
							printf("[%u], %.*s", disp, StringExpand(reg_string));
						}
					}
					else
					{
						s16 disp = ReadDisp(state, mod);

						if (d)
						{
							printf("%.*s, ", StringExpand(reg_string));
							PrintMemoryOperand(r_m, disp);
						}
						else
						{
							PrintMemoryOperand(r_m, disp);
							printf(", %.*s", StringExpand(reg_string));
						}
					}

					printf("\n");
				}
			} break;

			case Op_Common_ImmToRegMem:
			{
				// This does not translate 1:1 to the mov one because the s bit
				// influences the behaviour in a fun way that makes it not work
				// quite the same.

				u8 s = (b1 >> 1) & 0x1;
				u8 w = (b1 >> 0) & 0x1;

				u8 b2 = ReadU8(state);

				u8 op = (b2 >> 3) & 0x7;
				String op_name = common_instruction_patterns[op];

				u8 mod = (b2 >> 6) & 0x3;
				u8 r_m = (b2 >> 0) & 0x7;

				printf("%.*s ", StringExpand(op_name));

				if (mod == 0x3)
				{
					String reg_string = register_names[w][r_m];
					printf("%.*s, ", StringExpand(reg_string));
				}
				else
				{
					if (mod == 0b00 &&
						r_m == 0b110)
					{
						// Direct address

						u16 disp = ReadU16(state);
						printf("[%u], ", disp);
					}
					else
					{
						s16 disp = ReadDisp(state, mod);

						PrintMemoryOperand(r_m, disp);
						printf(", ");
					}
				}

				s16 data;
				if (s)
				{
					data = ReadS8(state);
				}
				else
				{
					data = ReadUx(state, w);
				}

				// TODO(daniel): When do we actually care to print word/byte explicitly?
				if (w)
				{
					printf("word %d\n", data);
				}
				else
				{
					printf("byte %d\n", data);
				}
			} break;

			case Op_Common_ImmToAccum:
			{
				u8 w  = (b1 >> 0) & 0x1;

				u8 op = (b1 >> 3) & 0x7;
				String op_string = common_instruction_patterns[op];

				s16 data = ReadSx(state, w);

				printf("%.*s %s, %d\n", StringExpand(op_string), w ? "ax" : "al", data);
			} break;

			//
			//
			//

			case Op_JumpIpInc8:
			{
				s8 ip_inc8 = ReadS8(state);

				int mnemonic_pattern_shift = (b1 >> 4) == 0b1110;
				String mnemonic = jump_mnemonics[mnemonic_pattern_shift][b1 & 15];
				printf("%.*s $%+d\n", StringExpand(mnemonic), ip_inc8 + 2);
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

	if (ThereWereDisassemblyErrors(state))
	{
		fprintf(stderr, "Disassembly error: %.*s\n", StringExpand(state->error_message));
	}
}
