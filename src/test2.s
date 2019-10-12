.text

main:
	addi $a1, $zero, -4
	addi $a2, $a1, 2
	addi $a3, $a2, 3
	jal testInstructions

	j end

notequals:
	addi $sp, $sp, -4
	sw $a3, 4($sp)
	j equals

equals:
	lw $t5, 4($sp)
	beq $zero, $zero, continue 


	
continue:
	slt $t6, $a1, $a2 
	jr $31

testInstructions:
	addu $s0, $a1, $a2 
	addiu $s1, $a1, 5
	subu $s2, $a2, 1 
	sll $s3, $a2, 1 
	srl $s4, $a2, 1
	and $s5, $a2, $a1
	or $s6, $a2, $a1 
	ori $s7, $a2, 15 
	andi $t0, $a2, 15
	lui $t1, 15
	bne $zero, $a1, notequals 
	
	
end:
