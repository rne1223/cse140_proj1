.text

main:
	addi $a1, $zero, 1
	addi $a2, $zero, 2
	addi $a3, $zero, 3
	jal testInstructions
	
	j end
	
testInstructions:
	addu $a3, $a1, $zero 
	addiu $a3, $a2, 0
	subu $a3, $a2, 1 
	sll $a3, $a2, 1 
	srl $a3, $a2, 1
	and $a3, $a2, $a1
	or $a3, $a2, $a1 
	ori $a3, $a2, 15 
	andi $a3, $a2, 15
	lui $a3, 15
	bne $a3, $a1, notequals 
	
continue:
	slt $a3, $a1, $a2 
 	jr $31
	
equals:
	lw $zero, 8($sp)
	beq $zero, $zero, continue 

notequals:
	addi $sp, $sp, -4
	sw $a3, 4($sp)
	j equals

end:
	li $v0, 10
	syscall
