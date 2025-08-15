/*
 * Your code here for context_switch
 *
 * It should restore the register values in the Trapframe with the 'iret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.
 */

.globl context_switch;
context_switch:
	add $4, %esp
	mov (%esp), %eax
	mov %eax, %esp

	popal
	pop %ds
	pop %es

	add $8, %esp
	iret
spin:
	jmp spin
