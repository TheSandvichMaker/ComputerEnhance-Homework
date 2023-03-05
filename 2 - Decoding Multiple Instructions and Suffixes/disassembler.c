#define BIT_PATTERN(a, b, c, d, e, f, g, h) \
	(((a) << 7)|((b) << 6)|((c) << 5)|((d) << 4)|((e) << 3)|((f) << 2)|((g) << 1)|((h) << 0))

typedef struct Disassembler
{
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

function void Disassemble(String bytes)
{
	Disassembler *state = &(Disassembler){
		.source = bytes,
		.at     = (u8 *)bytes.bytes,
		.end    = (u8 *)bytes.bytes + bytes.count,
	};

	printf("bits 16\n");

	while (BytesLeft(state))
	{
		u8 b1 = ReadU8(state);

		//
		// MOV:
		//

		if ((b1 >> 2) == 0x22)
		{
			// Register/memory to/from register

			u8 w = b1 & 0x1;
			u8 d = b1 & 0x2;

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
		}
		else if ((b1 >> 1) == 0x63)
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
		}
		else if ((b1 >> 4) == 0xB)
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
		}
		else if ((b1 >> 1) == 0x50)
		{
			// Memory to accumulator

			u8 w = b1 & 0x1;

			u16 addr = ReadU16(state);

			printf("mov %s [%u]\n", w ? "ax" : "al", addr);
		}
		else if ((b1 >> 1) == 0x51)
		{
			// Accumulator to memory

			u8 w = b1 & 0x1;

			u16 addr = ReadU16(state);

			printf("mov [%u] %s\n", addr, w ? "ax" : "al");
		}
		else
		{
			DisassemblyError(state, StringLit("Unexpected opcode"));
		}
	}

	if (ThereWereDisassemblyErrors(state))
	{
		fprintf(stderr, "Disassembly error: %.*s\n", StringExpand(state->error_message));
	}
}
