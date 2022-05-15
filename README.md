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

### Instruction set
##### Registers
Name | Usage
----|----
R0 | GPR
R1 | GPR
R2 | GPR
R3 | GPR
R4 | GPR
R5 | GPR
R6 | Stack pointer
R7 | Link register

### Opcodes
> More to come...
