function void InitializeDisassembler(Disassembler *disasm, u8 *output, size_t output_capacity)
{
	ZeroStruct(disasm);
	disasm->out_base = output;
	disasm->out_at   = output;
	disasm->out_end  = output + output_capacity;
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

function void DisasmWriteInt(Disassembler *disasm, int i)
{
	if (i < 0)
	{
		DisasmWriteC(disasm, '-');
		i = -i;
	}

	u8 *start = disasm->out_at;

	do
	{
		int d = i % 10;
		DisasmWriteC(disasm, (char)('0' + d));

		i /= 10;
	}
	while (i > 0);

	u8 *end = disasm->out_at;

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

				case 'i':
				{
					int i = va_arg(args, int);
					if (print_positive_sign && i >= 0)
					{
						DisasmWriteC(disasm, '+');
					}
					DisasmWriteInt(disasm, i);
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
	}
}

function void DisassembleInstruction(Disassembler *disasm, Instruction *inst)
{
	String mnemonic = mnemonic_names[inst->mnemonic];

	DisasmWrite(disasm, "%s ", mnemonic);

	switch (inst->mnemonic)
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
			DisassembleOperand(disasm, &inst->dst);
			DisasmWriteC(disasm, ',');
			DisasmWriteC(disasm, ' ');

			if (inst->flags & InstructionFlag_HasData)
			{
				if (inst->flags & W)
				{
					DisasmWrite(disasm, "word %i", inst->data);
				}
				else
				{
					DisasmWrite(disasm, "byte %i", inst->data);
				}
			}
			else
			{
				DisassembleOperand(disasm, &inst->src);
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
			s16 jmp = (s16)inst->data + 2;
			DisasmWrite(disasm, "$%c%i", jmp >= 0 ? '+' : '-', Abs(jmp));
		} break;
	}

	DisasmWriteC(disasm, '\n');
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
