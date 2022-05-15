## MCPU-16
Basic 16-bit RISC CPU architecture

### Tools
- Interpreter
- Assembler

### Usage of built executable
```m16 <path to .asm file>```

### Assembler usage
The usual structure of instruction is

```[<label>:] <opcode> <operand1>, <operand2>... [; <comment>]```

> Note that every instruction must be placed at different lines
>
> Labels can't be put in series on one line, but you can have lines with only labels

### Example assembly code
> Calculate first 10 numbers in Fibonacci sequence
```
		lea r0, n
		ldr r0, r0, #0

		lea r1, n1 
		ldr r1, r1, #0

		lea r2, n2
		ldr r2, r2, #0

loop:
		add r4, r1, #0
		trap x10        ; Print contents of register R4

		add r3, r1, r2
		add r1, r2, #0
		add r2, r3, #0

		add r0, r0, #-1
		brp loop

		trap x25

n1:		.dat #1
n2:		.dat #1
n:		.dat #10		; Length of sequence
```
