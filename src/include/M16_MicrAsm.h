#pragma once

#include "M16_Common.h"

namespace m16 {
	class micrasm_error : public std::runtime_error {
	public:
		micrasm_error(std::string message) : std::runtime_error(message) {}

		static micrasm_error generr(const char* fmt, ...);
	};

	class micrasm {
	private:
		struct patchedLabel {
			std::string name;
			word address;
			int offsetSize;
			int line;

			patchedLabel(std::string& name, word address, int offsetSize, int line) {
				this->name = name;
				this->address = address;
				this->offsetSize = offsetSize;
				this->line = line;
			}
		};

		std::unordered_map<std::string, word> labels;
		std::stack<patchedLabel> labelsToPatch;

		byte* code = nullptr;
		word PC = 0;
		int line = 1;
		bool lineHasLabelDecl = false;
		bool panicMode = false;

		const char* start;
		const char* current;

		void emitWord(word val);

		bool isAlpha(char c) {
			return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
		}

		bool isDigit(char c) {
			return c >= '0' && c <= '9';
		}

		bool isAlphaOrDigit(char c) { return isAlpha(c) || isDigit(c); }

		word readWord(word at);
		void writeWord(word at, word v);

		char peekChar();
		char peekNextChar();
		char nextChar();
		bool matchChar(char c);

		bool checkRest(int start, int length, const char* rest);

		void skipWhitespace();
		void skipComment();

		bool scanIdent();
		word scanUnsignedWord(int size);
		int16_t scanSignedWord(int size);

		byte scanRegister();
		bool scanRegisterOrSignedNumber(int16_t* result, int size);

		/* Pseudo-opcodes*/
		// Pur null-terminated string into memory
		// usage: .strz "A string!"
		void strzPseudoOp(int line);

		// Put single or multiple data into memory
		// usage: .dat value[, value...]
		void datPseudoOp(int line);

		// Reserve block of memory
		// usage: .blk size
		void blkPseudoOp(int line);

		// Don't know, why I need it...
		void origPseudoOp(int line);

		/* Opcodes */
		void RRIRtypeOp(byte opcode, int line);		// template for R = R <op> (Imm5|R) type of instructions
		void RRBtypeOp(byte opcode, int line);		// template for R = R + Base type of instructions
		bool brOp(int line);
		void leaOp(int line);
		void trapOp(int line);
		void jsrOp(int line);
		void jmpOp(int line);
		void notOp(int line);
		void divModOp(bool isDIV, int line);
		void shiftOp(bool isLeft, bool isArith, int line);

		// This function is called after assembly process to resolve labels
		void codeFinalize();

	public:
		void assemble(const char* source);

		byte* getCode();
	};
}