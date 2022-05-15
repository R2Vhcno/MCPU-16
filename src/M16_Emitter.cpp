#include "include/M16_Emitter.h"

#include <cstdarg>

namespace m16 {
	ir_error ir_error::generr(const char* fmt, ...) {
		constexpr size_t BUF_SIZE = 512;

		va_list a;

		char buf[BUF_SIZE];

		va_start(a, fmt);
		vsnprintf(buf, BUF_SIZE, fmt, a);
		va_end(a);

		return ir_error(std::string(buf));
	}

	void ir::emitWord(word value) {
		code[PC++] = (value >> 8);
		code[PC++] = (value & 0xff);
	}

	void ir::emitByte(byte value) {
		code[PC++] = value;
	}

	void ir::emitString(const char* text, size_t size) {
		for (size_t i = 0; i < size; i++) {
			emitByte(text[i]);
		}
	}

	word ir::readWord(word at) {
		if (at & 1) throw ir_error("Unaligned access to memory while reading word!");

		return (code[at] << 8) | code[at + 1];
	}

	void ir::writeWord(word at, word v) {
		if (at & 1) throw ir_error("Unaligned access to memory while writing word!");

		code[at] = v >> 8;
		code[at + 1] = v & 0xff;
	}

	word ir::label(const char* name, int offsetSize) {
		std::string convName(name);

		if (labels.contains(convName)) {
			word absAddr = labels.at(convName) >> 1;
			word curAddr = PC >> 1;
			int16_t difference = absAddr - curAddr;

			if (difference < -((1 << (offsetSize - 1)) - 1) || difference >((1 << (offsetSize - 1)) - 1)) throw ir_error::generr("Label '%s' is not reachable.", convName);
			return difference;
		}

		labelsToPatch.push(labelUsage(convName, PC, offsetSize));

		return 12;
	}

	void ir::startfrom(word address) {
		PC = address;
	}

	void ir::emitBR(bool n, bool z, bool p, word offset9) {
		word op = n << 11;
		op |= z << 10;
		op |= p << 9;
		op |= offset9 & 0x1ff;

		emitWord(op);
	}

	void ir::emitBR(bool n, bool z, bool p, const char* _label) {
		emitBR(n, z, p, label(_label, 9));
	}

	void ir::emitADD(Register dest, Register src1, Register src2) {
		word op = 0b0001 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;
		op |= 0 << 5;
		op |= (byte)src2 & 0x7;

		emitWord(op);
	}

	void ir::emitADD(Register dest, Register src1, int imm5) {
		word op = 0b0001 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;
		op |= 1 << 5;
		op |= imm5 & 31;

		emitWord(op);
	}

	void ir::emitLDB(Register dest, Register base, int offset6) {
		word op = 0b0010 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)base & 0x7) << 6;
		op |= offset6 & 63;

		emitWord(op);
	}

	void ir::emitSTB(Register src, Register base, int offset6) {
		word op = 0b0011 << 12;
		op |= ((byte)src & 0x7) << 9;
		op |= ((byte)base & 0x7) << 6;
		op |= offset6 & 63;

		emitWord(op);
	}

	void ir::emitJSR(int offset11) {
		word op = 0b0100 << 12;
		op |= 1 << 11;
		op |= offset11 & 2047;

		emitWord(op);
	}

	void ir::emitJSR(const char* _label) {
		emitJSR(label(_label, 11));
	}

	void ir::emitJSRR(Register base) {
		word op = 0b0100 << 12;
		op |= 0 << 11;
		op |= ((byte)base & 0x7) << 6;

		emitWord(op);
	}

	void ir::emitAND(Register dest, Register src1, Register src2) {
		word op = 0b0101 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;
		op |= 0 << 5;
		op |= (byte)src2 & 0x7;

		emitWord(op);
	}

	void ir::emitAND(Register dest, Register src1, int imm5) {
		word op = 0b0101 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;
		op |= 1 << 5;
		op |= imm5 & 31;

		emitWord(op);
	}

	void ir::emitLDR(Register dest, Register base, int offset6) {
		word op = 0b0110 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)base & 0x7) << 6;
		op |= offset6 & 63;

		emitWord(op);
	}

	void ir::emitSTR(Register src, Register base, int offset6) {
		word op = 0b0111 << 12;
		op |= ((byte)src & 0x7) << 9;
		op |= ((byte)base & 0x7) << 6;
		op |= offset6 & 63;

		emitWord(op);
	}

	void ir::emitRTI() {
		emitWord(0x8000);
	}

	void ir::emitNOT(Register dest, Register src1) {
		word op = 0b1001 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;

		emitWord(op);
	}

	void ir::emitMUL(Register dest, Register src1, Register src2) {
		word op = 0b1010 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;
		op |= 0 << 5;
		op |= (byte)src2 & 0x7;

		emitWord(op);
	}

	void ir::emitMUL(Register dest, Register src1, int imm5) {
		word op = 0b1010 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;
		op |= 1 << 5;
		op |= imm5 & 31;

		emitWord(op);
	}

	void ir::emitDIV(Register dest, Register src1, Register src2) {
		word op = 0b1011 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;
		op |= 0 << 5;
		op |= (byte)src2 & 0x7;

		emitWord(op);
	}

	void ir::emitMOD(Register dest, Register src1, Register src2) {
		word op = 0b1011 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;
		op |= 1 << 5;
		op |= (byte)src2 & 0x7;

		emitWord(op);
	}

	void ir::emitJMP(Register base) {
		word op = 0b1100 << 12;
		op |= ((byte)base & 0x7) << 6;

		emitWord(op);
	}

	void ir::emitRET() {
		emitWord(0xC1C0);
	}

	void ir::emitLSHF(Register dest, Register src1, int imm4) {
		word op = 0b1101 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;
		op |= 0 << 5;
		op |= 1 << 4;
		op |= (byte)imm4 & 0xf;

		emitWord(op);
	}

	void ir::emitRSHF(Register dest, Register src1, int imm4) {
		word op = 0b1101 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;
		op |= 0 << 5;
		op |= 0 << 4;
		op |= (byte)imm4 & 0xf;

		emitWord(op);
	}

	void ir::emitARSHF(Register dest, Register src1, int imm4) {
		word op = 0b1101 << 12;
		op |= ((byte)dest & 0x7) << 9;
		op |= ((byte)src1 & 0x7) << 6;
		op |= 1 << 5;
		op |= 0 << 4;
		op |= (byte)imm4 & 0xf;

		emitWord(op);
	}

	void ir::emitLEA(Register src, int offset9) {
		word op = 0b1110 << 12;
		op |= ((byte)src & 0x7) << 9;
		op |= offset9 & 511;

		emitWord(op);
	}

	void ir::emitLEA(Register src, const char* _label) {
		emitLEA(src, label(_label, 9));
	}

	void ir::emitTRAP(int trapvect8) {
		word op = 0b1111 << 12;
		op |= trapvect8 & 0xff;

		emitWord(op);
	}

	void ir::emitMOV(Register dest, Register src) {
		emitAND(dest, dest, 0);
		emitADD(dest, dest, src);
	}

	void ir::emitMOV(Register dest, int imm5) {
		emitAND(dest, dest, 0);
		emitADD(dest, dest, imm5);
	}

	void ir::emitMOV(Register dest, const char* label) {
		emitLEA(dest, label);
		emitLDR(dest, dest, 0);
	}

	void ir::emitLabel(const char* name) {
		std::string convName(name);

		if (labels.contains(convName)) throw ir_error::generr("Label '%s' already exists.", convName);

		labels.emplace(convName, PC);
	}

	void ir::completeCode() {
		/* Check for earlier unfound labels */
		while (labelsToPatch.size() > 0) {
			labelUsage& label = labelsToPatch.top();

			if (!labels.contains(label.name)) throw ir_error::generr("There is no label with name '%s'", label.name);

			word absAddr = labels.at(label.name) >> 1;
			word curAddr = label.patchAddr >> 1;
			int16_t difference = absAddr - curAddr;

			if (difference < -((1 << (label.offsetSize - 1)) - 1) || difference >((1 << (label.offsetSize - 1)) - 1)) throw ir_error::generr("Label '%s' is not reachable.", label.name);

			word instr = readWord(label.patchAddr);
			instr &= ~((1 << label.offsetSize) - 1);
			instr |= difference & ((1 << label.offsetSize) - 1);
			writeWord(label.patchAddr, instr);

			labelsToPatch.pop();
		}
	}

	byte* ir::getCode() {
		return code;
	}
}