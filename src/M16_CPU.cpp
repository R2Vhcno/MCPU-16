#include "include/M16_CPU.h"

namespace m16 {
	word subscr(word val, int start, int end) {
		return (~(0xffff << end) & val) >> start;
	}

	word setBit(word val, int n, bool set) {
		return (val & ~(1 << n)) | (set << n);
	}

	bool getBit(word val, int n) {
		return (val >> n) & 1;
	}

	word mult(word a, word b) {
		word c = 0;

		while (b & 0xffff) {
			if (b & 1) c += a;

			a <<= 1;
			b >>= 1;
		}

		return c;
	}

	word idiv(word a, word b) {
		word q = 0, r = 0;

		for (int i = 15; i > 0; i--) {
			r <<= 1;
			r |= (a >> i) & 1;
			if (r >= b) {
				r -= b;
				q |= 1 << i;
			}
		}

		return q;
	}

	word signext(word val, int size) {
		word mask = (1 << size) - 1;
		if (val & (1 << (size - 1))) {
			return  (0xffff ^ mask) | (val & mask);
		}

		return val & mask;
	}

	word zeroext(byte val_5) {
		return val_5;
	}

	void cpu::push(word val) {
		writeWord(--regs[6], val);
	}

	word cpu::pop() {
		return readWord(regs[6]++);
	}

	void cpu::setPrivileged(bool isPrivileged) {
		regs[9] = setBit(regs[9], 15, !isPrivileged);
	}

	bool cpu::isPriviledged() {
		return !getBit(regs[9], 15);
	}

	void cpu::setFlags(word result) {
		if (result == 0) {
			regs[9] = setBit(regs[9], 2, false);	// NF
			regs[9] = setBit(regs[9], 1, true);		// ZF
			regs[9] = setBit(regs[9], 0, false);	// PF
			return;
		}

		regs[9] = setBit(regs[9], 1, false);		// ZF
		if (result & 0x8000) {
			regs[9] = setBit(regs[9], 2, true);		// NF
			regs[9] = setBit(regs[9], 0, false);	// PF
		} else {
			regs[9] = setBit(regs[9], 2, false);	// NF
			regs[9] = setBit(regs[9], 0, true);		// PF
		}
	}

	void cpu::loadImage(byte* code) {
		int i = 0;

		while (i < MAX_MEM_SIZE) {
			memory[i] = code[i];
			i++;
		}
	}

	void cpu::process() {
		word inst = readWord(regs[8]);
		byte opcode = inst >> 12;

		byte reg1 = (inst >> 9) & 0x7;
		byte reg2 = (inst >> 6) & 0x7;
		byte imm6 = inst & 0x3f;

		regs[8] += 2;

		switch (opcode) {
		case 0b0000: { /* BR */
			if (((inst & 0x800) && (regs[9] & 0x4)) ||
				((inst & 0x400) && (regs[9] & 0x2)) ||
				((inst & 0x200) && (regs[9] & 0x1))) {
				regs[8] += (signext(inst & 0x1ff, 9) << 1);
			}
			break;
		}
		case 0b0001: { /* ADD */
			if (!(inst & 0x20)) {
				regs[reg1] = regs[reg2] + regs[imm6 & 0x7];
			} else {
				regs[reg1] = regs[reg2] + signext(imm6 & 0x1f, 5);
			}

			setFlags(regs[reg1]);
			break;
		}
		case 0b0010: { /* LDB */
			regs[reg1] = zeroext(readByte(regs[reg2] + signext(imm6, 6)));

			setFlags(regs[reg1]);
			break;
		}
		case 0b0011: { /* STB */
			writeByte(regs[reg2] + signext(imm6, 6), regs[reg1]);
			break;
		}
		case 0b0100: { /* JSR */
			regs[7] = regs[8];

			if ((inst >> 11) & 1) {
				regs[8] = regs[8] + (signext(inst & 0x3ff, 11) << 1);
			} else {
				regs[8] = regs[reg2];
			}
			break;
		}
		case 0b0101: { /* AND */
			if (!(inst & 0x20)) {
				regs[reg1] = regs[reg2] & regs[imm6 & 0x7];
			} else {
				regs[reg1] = regs[reg2] & signext(imm6 & 0x1f, 5);
			}

			setFlags(regs[reg1]);
			break;
		}
		case 0b0110: { /* LDR */
			regs[reg1] = readWord(regs[reg2] + (signext(imm6, 6) << 1));

			setFlags(regs[reg1]);
			break;
		}
		case 0b0111: { /* STR */
			writeWord(regs[reg2] + (signext(imm6, 6) << 1), regs[reg1]);
			break;
		}
		case 0b1000: { /* RTI */
			if (isPriviledged()) {
				regs[8] = readWord(regs[6]);
				regs[6] += 2;

				regs[9] = readWord(regs[6]);
				regs[6] += 2;
			}

			break;
		}
		case 0b1001: { /* NOT */
			regs[reg1] = ~regs[reg2];

			setFlags(regs[reg1]);
			break;
		}
		case 0b1010: { /* MUL */
			if (!(inst & 0x20)) {
				regs[reg1] = regs[reg2] * regs[imm6 & 0x7];
			} else {
				regs[reg1] = regs[reg2] * signext(imm6 & 0x1f, 5);
			}

			setFlags(regs[reg1]);
			break;
		}
		case 0b1011: { /* DIV, MOD */
			if (!(inst & 0x20)) {
				regs[reg1] = regs[reg2] / regs[imm6 & 0x7];
			} else {
				regs[reg1] = regs[reg2] % regs[imm6 & 0x7];
			}

			setFlags(regs[reg1]);
			break;
		}
		case 0b1100: { /* RET, JMP */
			regs[8] = regs[reg2] & 0xfffE;
			break;
		}
		case 0b1101: { /* SHF */
			if (imm6 & 16) {
				regs[reg1] = regs[reg2] << (imm6 & 0xf);
			} else {
				if (imm6 & 32) {
					word sign = regs[reg2] & 0x8000;
					word signMask = 0x0;

					if (sign) {
						signMask = 0xffff;
						signMask ^= (1 << (16 - (imm6 & 0xf))) - 1;
					}

					regs[reg1] = (regs[reg2] >> (imm6 & 0xf)) | signMask;
				} else {
					regs[reg1] = regs[reg2] >> (imm6 & 0xf);
				}
			}

			setFlags(regs[reg1]);
			break;
		}
		case 0b1110: { /* LEA */
			regs[reg1] = regs[8] + (signext(inst & 0x1ff, 9) << 1);

			setFlags(regs[reg1]);
			break;
		}
		case 0b1111: { /* TRAP */
			regs[7] = regs[8] + 1;

			if (IS_DEBUG) {
				switch (inst & 0xff) {
				case 0x25:
					debugHalt = true;
					break;
				case 0x10:
					printf("%d\n", regs[4]);
					break;
				default:
					break;
				}
			} else {
				regs[8] = readWord(zeroext(inst & 0xff) << 1);

				if ((inst & 0xff) == 0x25) debugHalt = true;
			}	
			break;
		}
		}
	}

	void cpu::setRegister(Register reg, word value) {
		regs[(int)reg] = value;
	}

	word cpu::getRegister(Register reg) {
		return regs[(int)reg];
	}

	void cpu::sendInterrupt(byte id, int level) {
		if (level < subscr(regs[9], 8, 10)) return;

		/* Start interrupt routine! */
		/* First set mode to privileged */
		setPrivileged(true);

		/* Save User SP and set SP to Supervisor SP*/
		USP = regs[6];
		regs[6] = SSP;

		/* Push PC and PSR */
		push(regs[8]);
		push(regs[9]);

		/* Jump to Interrupt routine */
		regs[8] = memory[id];
	}

	void cpu::dumpMem() {
		printf("*** <Memory dump>\n");
		bool previousIsEmpty = false;
		for (int line = 0; line < MAX_MEM_SIZE; line += 16) {
			/* Chec line for emptiness */
			bool isEmpty = true;
			for (int i = 0; i < 16; i++) {
				if (memory[line + i] != 0) {
					isEmpty = false;
					break;
				}
			}

			/* Decide whether to print line or not */
			if (previousIsEmpty && !isEmpty) {
				printf("...\n");
			}

			previousIsEmpty = isEmpty;
			if (isEmpty) continue;

			printf("%04x: ", line);

			for (int i = 0; i < 16; i++) {
				printf("%02x ", memory[line + i]);
			}

			printf("| ");

			for (int i = 0; i < 16; i++) {
				printf("%c", memory[line + i] >= 32 ? memory[line + i] : '.');
			}

			printf("\n");
		}

		printf("[END OF MEMORY]\n***\n");
	}

	void cpu::printRegs() {
		printf("*** <Registers dump>\n");

		printf("General purpose registers:\n");
		printf("R0 = 0x%04x : %-6d | R4 = 0x%04x : %d\n", regs[0], (int16_t)regs[0], regs[4], (int16_t)regs[4]);
		printf("R1 = 0x%04x : %-6d | R5 = 0x%04x : %d\n", regs[1], (int16_t)regs[1], regs[5], (int16_t)regs[5]);
		printf("R2 = 0x%04x : %-6d | R6 = 0x%04x : %d\n", regs[2], (int16_t)regs[2], regs[6], (int16_t)regs[6]);
		printf("R3 = 0x%04x : %-6d | R7 = 0x%04x : %d\n", regs[3], (int16_t)regs[3], regs[7], (int16_t)regs[7]);

		printf("\nControl registers:\n");
		printf("PC  = 0x%04x\n", regs[8]);
		printf("PSR = 0x%04x(n = %s, ", regs[9], regs[9] & 0x4 ? "true" : "false");
		printf("z = %s, ", regs[9] & 0x2 ? "true" : "false");
		printf("p = %s)\n***\n", regs[9] & 0x1 ? "true" : "false");
	}

	void cpu::writeWord(word address, word value) {
		if (address & 1) throw std::runtime_error("Unaligned access to memory while writing word!");

		memory[address] = value >> 8;
		memory[address + 1] = value & 0xff;
	}

	word cpu::readWord(word address) {
		if (address & 1) throw std::runtime_error("Unaligned access to memory while reading word!");

		return (memory[address] << 8) | memory[address + 1];
	}

	void cpu::writeByte(word address, byte value) {
		memory[address] = value;
	}

	byte cpu::readByte(word address) {
		return memory[address];
	}
}