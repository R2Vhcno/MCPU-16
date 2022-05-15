#pragma once

#include "M16_Common.h"


namespace m16 {
	struct symbol {
		word address;
		word length;
	};

	class ir_error : public std::runtime_error {
	public:
		ir_error(std::string message) : std::runtime_error(message) {}

		static ir_error generr(const char* fmt, ...);
	};

	class ir {
	private:
		struct labelUsage {
			std::string name;
			word patchAddr;
			int offsetSize;

			labelUsage(std::string& name, word patchAddr, int offsetSize) {
				this->name = name;
				this->patchAddr = patchAddr;
				this->offsetSize = offsetSize;
			}
		};

		word readWord(word at);
		void writeWord(word at, word v);

		byte* code;
		word PC;

		std::unordered_map<std::string, word> labels;
		std::stack<labelUsage> labelsToPatch;

	public:
		ir() {
			code = new byte[MAX_MEM_SIZE];
			memset(code, 0, MAX_MEM_SIZE);
			PC = 0;
		}

		~ir() {
			delete[] code;
		}

		word label(const char* name, int offsetSize);

		void startfrom(word address);

		void emitWord(word value);
		void emitByte(byte value);

		void emitString(const char* text, size_t size);

		/* Special emit commands */
		void emitBR(bool n, bool z, bool p, word offset9);//
		void emitBR(bool n, bool z, bool p, const char* label);//
		void emitADD(Register dest, Register src1, Register src2);//
		void emitADD(Register dest, Register src1, int imm5);//
		void emitLDB(Register dest, Register base, int offset6);//
		void emitSTB(Register src, Register base, int offset6);//
		void emitJSR(int offset11);//
		void emitJSR(const char* label);//
		void emitJSRR(Register base);//
		void emitAND(Register dest, Register src1, Register src2);//
		void emitAND(Register dest, Register src1, int imm5);//
		void emitLDR(Register dest, Register base, int offset6);//
		void emitSTR(Register src, Register base, int offset6);//
		void emitRTI();//
		void emitNOT(Register dest, Register src1);//
		void emitMUL(Register dest, Register src1, Register src2);//
		void emitMUL(Register dest, Register src1, int imm5);//
		void emitDIV(Register dest, Register src1, Register src2);//
		void emitMOD(Register dest, Register src1, Register src2);//
		void emitJMP(Register base);//
		void emitRET();//
		void emitLSHF(Register dest, Register src1, int imm4);//
		void emitRSHF(Register dest, Register src1, int imm4);//
		void emitARSHF(Register dest, Register src1, int imm4);//
		void emitLEA(Register src, int offset9);//
		void emitLEA(Register src, const char* label);//
		void emitTRAP(int trapvect8);//

		/* Macro instructions */
		void emitMOV(Register dest, Register src);
		void emitMOV(Register dest, int imm5);
		void emitMOV(Register dest, const char* label);
		void emitLabel(const char* name);

		void completeCode();

		byte* getCode();
	};
}