#pragma once

#include "M16_Common.h"

namespace m16 {
	word subscr(word val, int start, int end);

	word setBit(word val, int n, bool set);
	bool getBit(word val, int n);

	word mult(word a, word b);
	word idiv(word a, word b);

	word signext(word val, int size);
	word zeroext(byte val_5);

	class cpu {
	private:
		byte memory[MAX_MEM_SIZE] = { 0 };

		/* GenerL purpose registers: R0 - R7, PC, PSR */
		word regs[10] = {0};
		word ctableSegment = 0x1000;

		word USP = 0;
		word SSP = 0;

		void push(word val);
		word pop();

		void setPrivileged(bool isPrivileged);
		bool isPriviledged();

		void setFlags(word result);

	public:
		bool debugHalt = false;

		void loadImage(byte* stream);

		void process();

		void setRegister(Register reg, word value);
		word getRegister(Register reg);

		void sendInterrupt(byte id, int level);

		void dumpMem();
		void printRegs();

		void writeWord(word address, word value);
		word readWord(word address);

		void writeByte(word address, byte value);
		byte readByte(word address);
	};
}