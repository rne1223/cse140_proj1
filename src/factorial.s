####################################
# Compute the factorial of n (n!)
# The number n is initiallu loaded in 
# register $a0, while the result will be in $s0
.data
	num: .word 3
	
	
.text

main:
	addi  $a0, $0, 5
	jal   fact
	
	
	add   $s0, $v0, $zero
	j    last

fact:
	addi  $s1, $0, 12
	sub   $sp, $sp, $s1
	sw    $ra, 0($sp)
	sw    $fp, 4($sp)
	sw    $a0, 8($sp)
	add   $fp, $sp, $zero

	bne   $a0, $zero, Ric
	addi  $v0, $zero, 1
	j     end

Ric:
	addi  $s1, $0, 1
	sub   $a0, $a0, $s1

	jal   fact
	lw    $t0, 8($fp)
	mul   $v0, $v0, $t0 

end:

lw    $fp, 4($sp)
lw    $ra, 0($sp)
addi  $sp, $sp, 12
jr    $ra

last:
