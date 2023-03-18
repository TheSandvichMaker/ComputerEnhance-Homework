function void InitializeDisassembler(Disassembler *disasm, DisassemblerParams *params)
{
	ZeroStruct(disasm);
	disasm->source   = params->input;
	disasm->out_base = params->output.bytes;
	disasm->out_at   = params->output.bytes;
	disasm->out_end  = params->output.bytes + params->output.capacity;
	disasm->style    = params->style;
}

function void DisassemblyError(Disassembler *disasm, String message)
{
	if (!disasm->error)
	{
		disasm->error_message = message;
	}

	disasm->error = true;
}

function bool ThereWereDisassemblyErrors(Disassembler *disasm)
{
	return disasm->error;
}

function size_t DisasmWriteLeft(Disassembler *disasm)
{
	return disasm->out_end - disasm->out_at;
}

function void DisasmAnchorLine(Disassembler *disasm)
{
	disasm->line_start = disasm->out_at;
}

function void DisasmWriteS(Disassembler *disasm, String string)
{
	size_t count_left  = DisasmWriteLeft(disasm);
	size_t count_write = Min(count_left, string.count);
	for (size_t i = 0; i < count_write; i++)
	{
		*disasm->out_at++ = string.bytes[i];
	}

	if (count_write < string.count)
	{
		DisassemblyError(disasm, StringLit("Output buffer overflow!"));
	}
}

function void DisasmWriteSLower(Disassembler *disasm, String string)
{
	size_t count_left  = DisasmWriteLeft(disasm);
	size_t count_write = Min(count_left, string.count);
	for (size_t i = 0; i < count_write; i++)
	{
		u8 b = string.bytes[i];
		if (b >= 'A' && b <= 'Z')
		{
			b = 'a' + (b - 'A');
		}
		*disasm->out_at++ = b;
	}

	if (count_write < string.count)
	{
		DisassemblyError(disasm, StringLit("Output buffer overflow!"));
	}
}


function void DisasmWriteC(Disassembler *disasm, char c)
{
	if (disasm->out_at < disasm->out_end)
	{
		*disasm->out_at++ = (u8)c;
	}
	else
	{
		DisassemblyError(disasm, StringLit("Output buffer overflow!"));
	}
}

function void DisasmAlignLine(Disassembler *disasm, int align)
{
	s64 to_write = align - (disasm->out_at - disasm->line_start);
	for (s64 i = 0; i < to_write; i++)
	{
		DisasmWriteC(disasm, ' ');
	}
}

global u8 int_to_char[] =
{
	[0]  = '0',
	[1]  = '1',
	[2]  = '2',
	[3]  = '3',
	[4]  = '4',
	[5]  = '5',
	[6]  = '6',
	[7]  = '7',
	[8]  = '8',
	[9]  = '9',
	[10] = 'a',
	[11] = 'b',
	[12] = 'c',
	[13] = 'd',
	[14] = 'e',
	[15] = 'f',
};

typedef struct IntFormat
{
	int base;
	int min_length;
} IntFormat;

function void DisasmWriteI(Disassembler *disasm, int i, IntFormat *format)
{
	int base       = format->base;
	int min_length = format->min_length;

	if (!base)
	{
		base = 10;
	}

	if (base < 2)
	{
		base = 2;
	}

	if (base > 16)
	{
		base = 16;
	}

	if (i < 0)
	{
		DisasmWriteC(disasm, '-');
		i = -i;
	}

	u8 *start = disasm->out_at;

	do
	{
		int d = i % base;
		DisasmWriteC(disasm, int_to_char[d]);

		i /= base;
	}
	while (i > 0);

	u8 *end = disasm->out_at;

	s64 leading_zeroes = min_length - (end - start);
	for (s64 zero_index = 0; zero_index < leading_zeroes; zero_index++)
	{
		DisasmWriteC(disasm, '0');
	}

	end = disasm->out_at;

	while (start < end)
	{
		u8 t = *--end;
		*end = *start;
		*start++ = t;
	}
}

function void DisasmWrite(Disassembler *disasm, const char *fmt, ...)
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
					DisasmWriteC(disasm, va_arg(args, char));
				} break;

				case 's':
				{
					DisasmWriteS(disasm, va_arg(args, String));
				} break;

				case 'm':
				{
					DisasmWriteSLower(disasm, va_arg(args, String));
				} break;

				case 'i':
				{
					int i = va_arg(args, int);
					if (print_positive_sign && i >= 0)
					{
						DisasmWriteC(disasm, '+');
					}
					DisasmWriteI(disasm, i, &(IntFormat){ .base = 10 });
				} break;

				case 'x':
				{
					u32 x = va_arg(args, u32);
					DisasmWriteI(disasm, x, &(IntFormat){ .base = 16 });
				} break;

				case '%':
				{
					DisasmWriteC(disasm, '%');
				} break;

				default:
				{
					DisasmWriteC(disasm, f);
				} break;
			}
		}
		else
		{
			DisasmWriteC(disasm, f);
		}

		if (ThereWereDisassemblyErrors(disasm))
		{
			break;
		}
	}

	va_end(args);
}

function void DisassembleOperand(Disassembler *disasm, Operand *operand)
{
	switch (operand->kind)
	{
		case Operand_Reg:
		case Operand_SegReg:
		{
			DisasmWrite(disasm, "%s", register_names[operand->reg]);
		} break;

		case Operand_Mem:
		{
			EffectiveAddress *ea = &operand->mem;

			DisasmWriteC(disasm, '[');

			if (ea->reg1)
			{
				DisasmWrite(disasm, "%s", register_names[ea->reg1]);
				if (ea->reg2)
				{
					DisasmWrite(disasm, " + %s", register_names[ea->reg2]);
				}
			}

			if (ea->disp)
			{
				if (ea->reg1)
				{
					DisasmWrite(disasm, " %c ", ea->disp >= 0 ? '+' : '-');
				}

				DisasmWrite(disasm, "%i", Abs(ea->disp));
			}

			DisasmWriteC(disasm, ']');
		} break;

		case Operand_None:
		{
			/* don't do anything! */
		} break;
	}
}

function void DisassembleData(Disassembler *disasm, Instruction *inst)
{
	if (inst->flags & InstructionFlag_DataLO)
	{
		if (inst->flags & InstructionFlag_DataHI)
		{
			DisasmWrite(disasm, "word %i", inst->data);
		}
		else
		{
			DisasmWrite(disasm, "byte %i", inst->data);
		}
	}
}

function void DisassembleInstruction(Disassembler *disasm, Instruction *inst)
{
	String mnemonic = mnemonic_names[inst->mnemonic];

	if (mnemonic.count == 0)
	{
		mnemonic = StringLit("<invalid mnemonic>");
	}

	DisasmAnchorLine(disasm);
	DisasmWrite(disasm, "%m ", mnemonic);

	switch (inst->mnemonic)
	{
		case ADD:
		case MOV:
		case OR:
		case ADC:
		case SBB:
		case AND:
		case SUB:
		case XOR:
		case CMP:
		case XCHG:
		case IN:
		{
			DisassembleOperand(disasm, &inst->op1);
			DisasmWriteC(disasm, ',');
			DisasmWriteC(disasm, ' ');

			if (inst->flags & InstructionFlag_DataLO)
			{
				DisassembleData(disasm, inst);
			}
			else
			{
				DisassembleOperand(disasm, &inst->op2);
			}
		} break;

		case OUT:
		{
			if (inst->flags & InstructionFlag_DataLO)
			{
				DisassembleData(disasm, inst);
			}
			else
			{
				DisassembleOperand(disasm, &inst->op2);
			}
			DisasmWriteC(disasm, ',');
			DisasmWriteC(disasm, ' ');

			DisassembleOperand(disasm, &inst->op1);
		} break;

		case PUSH:
		case POP:
		{
			if (inst->op1.kind == Operand_Mem)
			{
				DisasmWrite(disasm, "word ");
			}
			DisassembleOperand(disasm, &inst->op1);
		} break;

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
			s16 jmp = (s16)inst->data + 2;
			DisasmWrite(disasm, "$%c%i", jmp >= 0 ? '+' : '-', Abs(jmp));
		} break;
	}

	if (disasm->style.show_original_bytes)
	{
		DisasmAlignLine(disasm, 32);
		DisasmWrite(disasm, " ;");

		for (size_t i = 0; i < inst->source_byte_count; i++)
		{
			u8 byte = disasm->source.bytes[inst->source_byte_offset + i];

			int base       = disasm->style.show_original_bytes_base;
			int min_length = 0;

			// log(base; 256)
			for (int counter = 256; counter > 1; counter /= base)
			{
				min_length += 1;
			}

			DisasmWriteC(disasm, ' ');
			IntFormat fmt =
			{
				.base       = base,
				.min_length = min_length,
			};
			DisasmWriteI(disasm, byte, &fmt);
		}
	}

	DisasmWriteC(disasm, '\n');
}

function void DisassemblerResetOutput(Disassembler *disasm, Buffer output)
{
	disasm->out_base = output.bytes;
	disasm->out_at   = output.bytes;
	disasm->out_end  = output.bytes + output.capacity;
}

function String DisassemblerResult(Disassembler *disasm)
{
	String result =
	{
		.count = disasm->out_at - disasm->out_base,
		.bytes = disasm->out_base,
	};
	return result;
}
