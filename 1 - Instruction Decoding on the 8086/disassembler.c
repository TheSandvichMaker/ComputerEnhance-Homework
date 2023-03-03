typedef enum OpCode
{
	Op_MOV = 0x22, // 0b100010
} OpCode;

typedef struct Disassembler
{
	String source;
	uchar *at;
	uchar *end;

	bool   error;
	String error_message;
} Disassembler;

static String register_names[2][8] =
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

function uchar ReadByte(Disassembler *state)
{
	uchar result = 0;
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

function void Disassemble(String bytes)
{
	Disassembler *state = &(Disassembler){
		.source = bytes,
		.at     = (uchar *)bytes.bytes,
		.end    = (uchar *)bytes.bytes + bytes.count,
	};

	while (BytesLeft(state))
	{
		uchar b1 = ReadByte(state);

		OpCode op = b1 >> 2;
		switch (op)
		{
			case Op_MOV:
			{
				uchar w = b1 & 0x1;
				uchar d = b1 & 0x2;

				uchar b2 = ReadByte(state);

				uchar mod = (b2 >> 6) & 0x3;
				uchar reg = (b2 >> 3) & 0x7;
				uchar r_m = (b2 >> 0) & 0x7;

				if (mod != 0x3)
				{
					DisassemblyError(state, StringLit("mod was not 0b11!"));
				}

				uchar dst = d ? reg : r_m;
				uchar src = d ? r_m : reg;

				String dst_name = register_names[w][dst];
				String src_name = register_names[w][src];

				printf("mov %.*s, %.*s\n", StringExpand(dst_name), StringExpand(src_name));
			} break;

			default:
			{
				DisassemblyError(state, StringLit("Unexpected opcode"));
			} break;
		}

		if (ThereWereDisassemblyErrors(state))
		{
			fprintf(stderr, "Disassembly error: %.*s\n", StringExpand(state->error_message));
			break;
		}
	}
}
