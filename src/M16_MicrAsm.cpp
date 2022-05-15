#include "include/M16_MicrAsm.h"

#include <cstdarg>

namespace m16 {
	micrasm_error micrasm_error::generr(const char* fmt, ...) {
		constexpr size_t BUF_SIZE = 512;

		va_list a;

		char buf[BUF_SIZE];

		va_start(a, fmt);
		vsnprintf(buf, BUF_SIZE, fmt, a);
		va_end(a);

		return micrasm_error(std::string(buf));
	}

	void micrasm::emitWord(word val) {
		code[PC++] = (val >> 8);
		code[PC++] = (val & 0xff);
	}

	word micrasm::readWord(word at) {
		return (code[at] << 8) | code[at + 1];
	}

	void micrasm::writeWord(word at, word v) {
		code[at] = v >> 8;
		code[at + 1] = v & 0xff;
	}

	char micrasm::peekChar() {
		return *current;
	}

	char micrasm::peekNextChar() {
		if (peekChar() == '\0') return '\0';

		return current[1];
	}

	char micrasm::nextChar() {
		if (peekChar() == '\0') return '\0';

		current++;
		return current[-1];
	}

	bool micrasm::matchChar(char c) {
		if (peekChar() == '\0') return false;
		if (peekChar() != c) return false;

		current++;
		return true;
	}

	bool micrasm::checkRest(int _start, int length, const char* rest) {
		if (current - start == _start + length &&
			memcmp(start + _start, rest, length) == 0) {
			start = current;
			return true;
		}

		return true;
	}

	void micrasm::skipWhitespace() {
		while (true) {
			char c = peekChar();
			switch (c) {
			case ' ':
			case '\t':
			case '\r':
				break;
			default:
				start = current;
				return;
			}

			nextChar();
		}
	}

	void micrasm::skipComment() {
		skipWhitespace();				// Skip all whitespaces to something meaningful

		if (matchChar(';')) {		// If first met char is ';', then skip till '\n'
			while (peekChar() != '\n' && peekChar() != '\0') nextChar();

			nextChar();						// Consume '\n'
			line++;								// Go to newline

			lineHasLabelDecl = false;
			return;
		} else if (matchChar('\n')) {		// If first met char is '\n', there is no comment
			line++;								// Go to newline

			lineHasLabelDecl = false;
			return;
		} else if (peekChar() == '\0') {
			lineHasLabelDecl = false;
			return;
		}

		// Otherwise, there is something, which shouldn't be there
		throw micrasm_error::generr("line %d: Unexpected character '%s'", line, peekChar());
	}

	bool micrasm::scanIdent() {
		//First char in identifier must be always a letter
		if (isAlpha(peekChar()) || peekChar() == '.') {
			nextChar();

			// Then, identifier can consist of letters and digits
			while (isAlphaOrDigit(peekChar())) nextChar();

			return true;
		}

		return false;
	}

	word micrasm::scanUnsignedWord(int size) {
		start = current;

		skipWhitespace();
		// Numbers in MicrAsm always start with:
		// '#' - decimal;
		// 'b' - binary;
		// 'o' - octodecimal;
		// 'x' - hexadecimal;

		unsigned long parsed;
		char* finalChar;

		if (matchChar('#')) {
			parsed = strtoul(current, &finalChar, 10);
		} else if (matchChar('b')) {
			parsed = strtoul(current, &finalChar, 2);
		} else if (matchChar('o')) {
			parsed = strtoul(current, &finalChar, 8);
		} else if (matchChar('x')) {
			parsed = strtoul(current, &finalChar, 16);
		} else {
			throw micrasm_error::generr("line %d: Unknown number specifier '%s'", line, peekChar());
		}

		if (parsed > (1 << size) - 1) throw micrasm_error::generr("line %d: Number exceds %d-bit unsigned range.", line, size);

		current = finalChar;

		return (word)parsed;
	}

	int16_t micrasm::scanSignedWord(int size) {
		start = current;

		skipWhitespace();

		long parsed;
		char* finalChar;

		// otherwise parse number
		if (matchChar('#')) {
			parsed = strtol(current, &finalChar, 10);
		} else if (matchChar('b')) {
			parsed = strtol(current, &finalChar, 2);
		} else if (matchChar('o')) {
			parsed = strtol(current, &finalChar, 8);
		} else if (matchChar('x')) {
			parsed = strtol(current, &finalChar, 16);
		} else if (isAlpha(peekChar())) { // it is, in fact, a label
			start = current;

			// Scan characters or digits till something
			while (isAlphaOrDigit(peekChar())) {
				nextChar();
			}

			std::string conv(start, current - start);

			// check if label is defined
			if (labels.contains(conv)) {
				word absAddr = labels.at(conv) >> 1;
				word curAddr = (PC + 2) >> 1;

				int16_t difference = absAddr - curAddr;

				if (difference < -((1 << (size - 1)) - 1) ||
					difference >((1 << (size - 1)) - 1))
					throw micrasm_error::generr("line %d: Label '%s' is not reachable.", line, conv);

				start = current;
				return difference;
			}

			labelsToPatch.push(patchedLabel(conv, PC, size, line));

			start = current;
			return 0;
		} else {
			throw micrasm_error::generr("line %d: Unknown number specifier '%s'", line, peekChar());
		}
		if (parsed < -((1 << (size - 1)) - 1) ||
			parsed > ((1 << (size - 1)) - 1)) 
			throw micrasm_error::generr("line %d: Number exceeds %d-bit signed range.", line, size);

		current = finalChar;

		return (int16_t)parsed;
	}

	byte micrasm::scanRegister() {
		skipWhitespace();

		if (matchChar('r')) {
			if (isDigit(peekChar())) {
				byte regNum = nextChar() - '0';

				if (regNum > 7) throw micrasm_error::generr("line %d: Only registers R0 through R7 are available.", line);
				
				return regNum;
			} else throw micrasm_error::generr("line %d: Register identifier cannot contain any letters apart leading 'r'.", line);
		} else throw micrasm_error::generr("line %d: Register identifier expected.", line);
	}

	bool micrasm::scanRegisterOrSignedNumber(int16_t* result, int size) {
		skipWhitespace();

		if (peekChar() == 'r') {
			*result = scanRegister();
			return true;
		}

		*result = scanSignedWord(size);
		return false;
	}

	void micrasm::strzPseudoOp(int line) {
		start = current;

		skipWhitespace();

		if (matchChar('"')) {
			while (peekChar() != '"') {
				// Allow some character escape sequences
				if (matchChar('\\')) {
					switch (peekChar()) {
					case '0': code[PC++] = '\0'; nextChar(); break;
					case 'a': code[PC++] = '\a'; nextChar(); break;
					case 'b': code[PC++] = '\b'; nextChar(); break;
					case 'f': code[PC++] = '\f'; nextChar(); break;
					case 'n': code[PC++] = '\n'; nextChar(); break;
					case 'r': code[PC++] = '\r'; nextChar(); break;
					case 't': code[PC++] = '\t'; nextChar(); break;
					case 'v': code[PC++] = '\v'; nextChar(); break;
					case '\\': code[PC++] = '\\'; nextChar(); break;
					default:
						code[PC++] = '\\';
						code[PC++] = nextChar(); break;
					}

					continue;
				}

				code[PC++] = nextChar();		
			}

			code[PC++] = '\0';
			if (!matchChar('"')) throw micrasm_error::generr("line %d: Unterminated string in '.strnz'.", line);

		} else throw micrasm_error::generr("line %d: '\"' expected after '.strz'.", line);
	}

	void micrasm::datPseudoOp(int line) {
		start = current;

		// Start scanning numbers
		do {
			word num = scanSignedWord(16);

			code[PC++] = num >> 8;
			code[PC++] = num & 0xff;

			skipWhitespace();
		} while (matchChar(','));
	}

	// Note: this function reserves space!
	void micrasm::blkPseudoOp(int line) {
		start = current;

		word spaceToReserve = scanUnsignedWord(16);

		if (MAX_MEM_SIZE - PC < spaceToReserve) throw micrasm_error::generr("line %d: Space needed to be reserved is too large", line);
		PC += spaceToReserve;
	}

	void micrasm::origPseudoOp(int line) {
		start = current;

		skipWhitespace();

		word startPos = scanUnsignedWord(16);

		PC = startPos;
	}

	void micrasm::RRIRtypeOp(byte opcode, int line) {
		start = current;

		byte rd = scanRegister();

		skipWhitespace();
		if (!matchChar(',')) throw micrasm_error::generr("line %d: 'add' receives only 3 operands, got 1.", line);

		byte rs1 = scanRegister();

		skipWhitespace();
		if (!matchChar(',')) throw micrasm_error::generr("line %d: 'add' receives only 3 operands, got 2.", line);

		int16_t rs2;
		word op = 0;
		if (scanRegisterOrSignedNumber(&rs2, 5)) {
			op = opcode << 12;
			op |= ((byte)rd & 0x7) << 9;
			op |= ((byte)rs1 & 0x7) << 6;
			op |= 0 << 5;
			op |= (byte)rs2 & 0x7;
		} else {
			op = opcode << 12;
			op |= ((byte)rd & 0x7) << 9;
			op |= ((byte)rs1 & 0x7) << 6;
			op |= 1 << 5;
			op |= rs2 & 31;
		}

		emitWord(op);
	}

	bool micrasm::brOp(int line) {
		const char* savedCurrent = current;
		current = start + 2;
		int stage = 0;

		bool n = false;
		bool z = false;
		bool p = false;

		// 'br' is special opcode.
		// all of its types are listed below:
		// -> br, brn, brnz, brz, brzp, bro, brnzp
		// They could be parsed as differen tokens, but they aren't.
		while (current < savedCurrent) {
			switch (nextChar()) {
			case 'n':
				if (stage > 0 || n) {
					current = savedCurrent;
					return false;
				}

				n = true;
				stage = 1;

				break;
			case 'z':
				if (stage > 1 || z) {
					current = savedCurrent;
					return false;
				}

				z = true;
				stage = 2;

				break;
			case 'p':
				if (stage > 2 || p) {
					current = savedCurrent;
					return false;
				}

				p = true;
				stage = 3;

				break;
			default:
				current = savedCurrent;
				return false;
			}
		}

		int16_t offset = scanSignedWord(9);

		word op = n << 11;
		op |= z << 10;
		op |= p << 9;
		op |= offset & 0x1ff;

		emitWord(op);

		return true;
	}

	void micrasm::leaOp(int line) {
		byte src = scanRegister();

		skipWhitespace();
		if (!matchChar(',')) throw micrasm_error::generr("line %d: 'lea' receives only 2 operands, got 1.", line);

		int16_t offset = scanSignedWord(9);

		word op = 0b1110 << 12;
		op |= ((byte)src & 0x7) << 9;
		op |= offset & 511;

		emitWord(op);
	}

	void micrasm::RRBtypeOp(byte opcode, int line) {
		byte dest = scanRegister();

		skipWhitespace();
		if (!matchChar(',')) throw micrasm_error::generr("line %d: 'ldr' receives only 3 operands, got 1.", line);

		byte base = scanRegister();

		skipWhitespace();
		if (!matchChar(',')) throw micrasm_error::generr("line %d: 'ldr' receives only 3 operands, got 2.", line);

		int16_t offset = scanSignedWord(6);

		word op = opcode << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)base & 0x7) << 6;
		op |= offset & 63;

		emitWord(op);
	}

	void micrasm::trapOp(int line) {
		start = current;

		skipWhitespace();
		int16_t rd = scanUnsignedWord(8);

		word op = 0b1111 << 12;
		op |= rd & 0xff;

		emitWord(op);
	}

	void micrasm::jsrOp(int line) {
		int16_t offset = 0;
			
		word op = 0b0100 << 12;
		if (scanRegisterOrSignedNumber(&offset, 11)) {
			op |= 0 << 11;
			op |= ((byte)offset & 0x7) << 6;
		} else {
			op |= 1 << 11;
			op |= offset & 2047;
		}

		emitWord(op);
	}

	void micrasm::jmpOp(int line) {
		byte base = scanRegister();

		word op = 0b1100 << 12;
		op |= ((byte)base & 0x7) << 6;

		emitWord(op);
	}

	void micrasm::notOp(int line) {
		byte dest = scanRegister();

		skipWhitespace();
		if (!matchChar(',')) throw micrasm_error::generr("line %d: 'not' receives only 2 operands, got 1.", line);

		byte src = scanRegister();

		word op = 0b1001 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src & 0x7) << 6;

		emitWord(op);
	}

	void micrasm::divModOp(bool isDIV, int line) {
		byte dest = scanRegister();

		skipWhitespace();
		if (!matchChar(',')) throw micrasm_error::generr("line %d: '%s' receives only 3 operands, got 1.", line, std::string(start, current - start));

		byte src1 = scanRegister();

		skipWhitespace();
		if (!matchChar(',')) throw micrasm_error::generr("line %d: '%s' receives only 3 operands, got 2.", line, std::string(start, current - start));

		byte src2 = scanRegister();

		word op = 0b1011 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;
		op |= !isDIV << 5;
		op |= (byte)src2 & 0x7;

		emitWord(op);
	}

	void micrasm::shiftOp(bool isLeft, bool isArith, int line) {
		byte dest = scanRegister();

		skipWhitespace();
		if (!matchChar(',')) throw micrasm_error::generr("line %d: '%s' receives only 3 operands, got 1.", line, std::string(start, current - start));

		byte src = scanRegister();

		skipWhitespace();
		if (!matchChar(',')) throw micrasm_error::generr("line %d: '%s' receives only 3 operands, got 2.", line, std::string(start, current - start));

		word imm4 = scanUnsignedWord(4);

		word op = 0b1101 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src & 0x7) << 6;
		op |= isArith << 5;
		op |= isLeft << 4;
		op |= (byte)imm4 & 0xf;

		emitWord(op);
	}

	void micrasm::codeFinalize() {
		// Check for earlier unfound labels
		while (labelsToPatch.size() > 0) {
			patchedLabel& label = labelsToPatch.top();

			if (!labels.contains(label.name))
				throw micrasm_error::generr("line %d: Can't find the label declaration with the name '%s'.", label.line, label.name);

			word absAddr = labels.at(label.name) >> 1;
			word curAddr = (label.address + 2) >> 1;

			int16_t difference = absAddr - curAddr;

			if (difference < -((1 << (label.offsetSize - 1)) - 1) ||
				difference >((1 << (label.offsetSize - 1)) - 1))
				throw micrasm_error::generr("line %d: Label '%s' is not reachable.", label.line, label.name);

			word instr = readWord(label.address);
			instr &= ~((1 << label.offsetSize) - 1);
			instr |= difference & ((1 << label.offsetSize) - 1);
			writeWord(label.address, instr);

			labelsToPatch.pop();
		}
	}

	void micrasm::assemble(const char* source) {
		line = 1;
		PC = 0;

		if (code != nullptr) delete[] code;

		code = new byte[MAX_MEM_SIZE];
		memset(code, 0, MAX_MEM_SIZE);

		start = current = source;
		lineHasLabelDecl = false;

		while (peekChar() != '\0') {
			start = current;

			skipWhitespace();

			if (peekChar() == '\n' || peekChar() == ';') {
				skipComment();

				// After skipping comment, line-assembling procedure must be restarted
				continue;
			}

			// If no EOL or ';' met, there must be a label or an opcode
			// If first letter is digit, then it is an exceptional situation
			if (isDigit(peekChar())) throw micrasm_error::generr("line %d: label or opcode expected", line);
			
			// Anyway scan identifier
			scanIdent();

			// Now test if scanned identifier was a label declaration
			if (peekChar() == ':') {
				// I dont think that two or more lables pointing to the same address is a good idea,
				// so I disallow this behaviour
				if (lineHasLabelDecl) {
					throw micrasm_error::generr("line %d: This line has label already declared.", line);
				}

				// Convert label c string to c++ string
				std::string conv(start, (int)(current - start));

				// Skip ':'
				nextChar();

				// Check whether there is a a label with the same name already declared
				if (labels.contains(conv)) throw micrasm_error::generr("line %d: Label '%s' already exist.", line, conv);

				// Put label into labels table
				labels.emplace(conv, PC);

				// Mark, that current line has a label
				lineHasLabelDecl = true;

				// Restart the scanning process
				continue;
			} else {
				// If identifier does not have ':' at the end, it must be an opcode

				bool isLegitOpcode = false; // This var is used to show if identifier is really an opcode

				// Wind up the scanning thing!
				switch (start[0]) {
				case '.': // If first letter is '.', it is, in fact, a pseudo_op
					if (current - start > 1) {
						switch (start[1]) {
						case 's': // .strz <string>
							if (checkRest(2, 3, "trz")) {
								isLegitOpcode = true;
								strzPseudoOp(line);
							}
							break;
						case 'd': // .dat <w1>, ...
							if (checkRest(2, 2, "at")) {
								isLegitOpcode = true;
								datPseudoOp(line);
							}
							break;
						case 'b': // .blk <num>
							if (checkRest(2, 2, "lk")) {
								isLegitOpcode = true;
								blkPseudoOp(line);
							}
							break;
						case 'o': // .orig <num>
							if (checkRest(2, 3, "rig")) {
								isLegitOpcode = true;
								origPseudoOp(line);
							}
							break;
						}
					}
					break;
				case 'a':
					if (current - start > 1) {
						switch (start[1]) {
						case 'd':
							if (checkRest(2, 1, "d")) {
								isLegitOpcode = true;
								RRIRtypeOp(0b0001, line);
							}
							break;
						case 'n':
							if (checkRest(2, 1, "d")) {
								isLegitOpcode = true;
								RRIRtypeOp(0b0101, line);
							}
							break;
						case 'r':
							if (checkRest(2, 3, "shf")) {
								isLegitOpcode = true;
								shiftOp(false, true, line);
							}
							break;
						}
					}
					break;
				case 'b':
					if (current - start > 1 && start[1] == 'r') {
						isLegitOpcode = brOp(line);
					}
					break;
				case 'd':
					if (checkRest(1, 2, "iv")) {
						isLegitOpcode = true;
						divModOp(true, line);
					}
					break;
				case 'h':
					if (checkRest(1, 2, "lt")) {
						isLegitOpcode = true;
						emitWord(0xF025);
					}
					break;
				case 'j':
					if (checkRest(1, 2, "sr")) {
						isLegitOpcode = true;
						jsrOp(line);
					} else if (checkRest(1, 2, "mp")) {
						isLegitOpcode = true;
						jmpOp(line);
					}
					break;
				case 'l':
					if (current - start > 1) {
						switch (start[1]) {
						case 'd':
							if (checkRest(1, 1, "r")) {
								isLegitOpcode = true;
								RRBtypeOp(0b0110, line);
							} else if (checkRest(1, 1, "b")) {
								isLegitOpcode = true;
								RRBtypeOp(0b0010, line);
							}
							break;
						case 'e':
							if (checkRest(1, 1, "a")) {
								isLegitOpcode = true;
								leaOp(line);
							}
							break;
						case 's':
							if (checkRest(1, 2, "hf")) {
								isLegitOpcode = true;
								shiftOp(true, false, line);
							}
							break;
						}
					}
					break;
				case 'm':
					if (current - start > 1) {
						switch (start[1]) {
						case 'u':
							if (checkRest(1, 1, "l")) {
								isLegitOpcode = true;
								RRIRtypeOp(0b1010, line);
							}
							break;
						case 'o':
							if (checkRest(1, 2, "d")) {
								isLegitOpcode = true;
								divModOp(false, line);
							}
							break;
						}
					}
					break;
				case 'n':
					if (current - start > 2 && start[1] == 'o') {
						switch (start[2]) {
						case 'p':
							isLegitOpcode = true;
							emitWord(0);
							break;
						case 't':
							isLegitOpcode = true;
							notOp(line);
							break;
						}
					}
					break;
				case 's':
					if (current - start > 2 && start[1] == 't') {
						switch (start[2]) {
						case 'r':
							isLegitOpcode = true;
							RRBtypeOp(0b0111, line);
							break;
						case 'b':
							isLegitOpcode = true;
							RRBtypeOp(0b0011, line);
							break;
						}
					}
					break;
				case 'r':
					if (current - start > 1) {
						switch (start[1]) {
						case 'e':
							if (checkRest(2, 1, "t")) {
								isLegitOpcode = true;
								emitWord(0xC1C0);
							}
							break;
						case 't':
							if (checkRest(2, 1, "i")) {
								isLegitOpcode = true;
								emitWord(0x8000);
							}
							break;
						case 's':
							if (checkRest(2, 2, "hf")) {
								isLegitOpcode = true;
								shiftOp(false, false, line);
							}
							break;
						}
					}
					break;
				case 't':
					if (checkRest(1, 3, "rap")) {
						isLegitOpcode = true;
						trapOp(line);
					}
					break;
				}

				// Bonk programmer, if he wrote some shit, not the real opcode
				if (!isLegitOpcode) throw micrasm_error::generr("line %d: Unknown opcode '%s'.", line, std::string(start, current - start));
			}

			// After scanning everything needed, to the next line;
			skipComment();
		}

		// Finalize code: patch labels.
		codeFinalize();
	}

	byte* micrasm::getCode() {
		return code;
	}
}