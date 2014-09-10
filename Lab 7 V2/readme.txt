 NOTES

	- use pages of 256 words (65,536 16-bit words of memory possible)
	- assume each instruction takes 1 cycle for every memory reference (including instruction fetch), but no additional time for any computation
	- the link bit is set whenever an arithmetic operation causes an overflow (add/sub/incr/decr/mul) or divide by zero
	- anything in parentheses below means contents of whatever is in the parentheses

 	 STACK
 	 	 - the stack represents an area of memory (words[]) that will be used as a stack
 	 	 - the stack grows down in memory (larger-numbered memory addresses are on top)
 	 	 - the stack pointer initially points to an empty location; the first push will set the stack pointer
 	 	 - the stack pointer represents the beginning of the stack
 	 	 - the stack pointer limit represents the end of the stack
 	 	 - pushing to the stack stores in the word that the SP points to; SP is then decremented by one
 	 	 - popping from the stack first increments the SP by one, and then uses the word that the SP points at
 	 	 - errors
 	 	 	 - stack overflow: if SP < SPL when stack push starts
 	 	 	 - stack underflow: if SP wraps around to 0 (SP == 0xffff when a stack pop starts)

 	 INSTRUCTIONS
 	 	 - 6 classes of instructions

 	 	 	 - non-register, non-memory instructions
 	 	 	 	 - high-order 6 bits are ALWAYS 0 (000000)
 	 	 	 	 - 000000.0000000000 == NOP
 	 	 	 	 - 000000.0000000001 == HLT; low-order bit of the PSW is set to 0; halt
 	 	 	 	 - 000000.0000000010 == RET; pop the stack into the PC

 	 	 	 - register memory reference instructions
 	 	 	 	 - 4-bit opcode, 2-bit register selector, D/I bit, Z/C bit, 8-bit page offset (in that order, HOB to LOB)
 	 	 	 	 - opcodes
 	 	 	 	 	 - 0001 == ADD: (register selected) + (memory operand) => (register selected)
 	 	 	 	 	 - 0010 == SUB: (register selected) - (memory operand) => (register selected)
 	 	 	 	 	 - 0011 == MUL: (register selected) * (memory operand) => (register selected)
 	 	 	 	 	 - 0100 == DIV: (register selected) / (memory operand) => (register selected)
 	 	 	 	 	 - 0101 == AND: (register selected) & (memory operand) => (register selected)
 	 	 	 	 	 - 0110 == OR:  (register selected) | (memory operand) => (register selected)
 	 	 	 	 	 - 0111 == XOR: (register selected) ^ (memory operand) => (register selected)
 	 	 	 	 	 - 1000 == LD:  (memory operand) => (register selected); overwrite contents of register selected with contents of memory operand
 	 	 	 	 	 - 1001 == ST:  (register selected) => (memory operand); overwrite contents of memory operand with contents of register selected
 	 	 	 	 - register selector
 	 	 	 	 	 - 00 == A register
 	 	 	 	 	 - 01 == B register
 	 	 	 	 	 - 10 == C register
 	 	 	 	 	 - 11 == D register

 	 	 	 - i/o transfer instructions
 	 	 	 	 - 4-bit opcode of 1010, 2-bit register selector, 7-bit device number, 3-bit function field (in that order, HOB to LOB)

 	 	 	 - non-register memory reference instructions
 	 	 	 	 - 6-bit opcode, D/I bit, Z/C bit, 8-bit page offset (in that order, HOB to LOB)
 	 	 	 	 - 6-bit opcode is actually the 4-bit opcode and 2-bit register fields of the register memory reference instructions combined
 	 	 	 	 - opcodes
 	 	 	 	 	 - 1011.00 == ISZ: (memory operand) + 1 => (memory operand); if (memory operand) == 0, PC++
 	 	 	 	 	 - 1011.01 == JMP: address of memory operand => PC
 	 	 	 	 	 - 1011.10 == CALL: push return address (== PC + 1) on stack; address of memory operand => PC
 	 	 	 	 	 - 1100.00 == PUSH: push address of memory operand to the stack
 	 	 	 	 	 - 1100.01 == POP: pop top of stack => (memory operand)

 	 	 	 - register-to-register instructions
 	 	 	 	 - operate ONLY on registers
 	 	 	 	 - 4-bit opcode of 1110, 3-bit sub-opcode, three 3-bit register specifiers (i, j, and k, respectively) (in that order, HOB to LOB)
 	 	 	 	 - instructions roughly take the format (jReg [op] kReg => iReg)
 	 	 	 	 - register specifiers
 	 	 	 	 	 - 000 == A register
 	 	 	 	 	 - 001 == B register
 	 	 	 	 	 - 010 == C register
 	 	 	 	 	 - 011 == D register
 	 	 	 	 	 - 100 == PC
 	 	 	 	 	 - 101 == PSW
 	 	 	 	 	 - 110 == SP
 	 	 	 	 	 - 111 == SPL
 	 	 	 	 - instructions
 	 	 	 	 	 - 1110.000 == MOD: (reg[j]) % (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.001 == ADD: (reg[j]) + (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.010 == SUB: (reg[j]) - (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.011 == MUL: (reg[j]) * (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.100 == DIV: (reg[j]) / (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.101 == AND: (reg[j]) & (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.110 == OR:  (reg[j]) | (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.111 == XOR: (reg[j]) ^ (reg[k]) => (reg[i])

 	 	 	 - non-memory register instructions
 	 	 	 	 - similar to microinstructions on the PDP-8
 	 	 	 	 - 4-bit opcode of 1111, 2-bit register selector, SM, SZ, SNL, RSS, CL, CLL, CM, CML, DC, IC (all one bit) (in that order, HOB to LOB)
 	 	 	 	 - instructions
 	 	 	 	 	 - SM == skip if (register) is negative (sign bit is 1)
 	 	 	 	 	 - SZ == skip if (register) is zero
 	 	 	 	 	 - SNL == skip if link bit is non-zero
 	 	 	 	 	 - RSS == reverse skip sense; flip skip flag bit
 	 	 	 	 	 - CL == clear (register)
 	 	 	 	 	 - CLL == clear link bit
 	 	 	 	 	 - CM == complement (register)
 	 	 	 	 	 - CML == complement link bit
 	 	 	 	 	 - DC == decrement (register) by one
 	 	 	 	 	 - IC == increment (register) by one
 	 	 	 	 - *** EVALUATION OF INSTRUCTIONS OCCURS IN THE ORDER LISTED ABOVE ***
 	 	 -