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
