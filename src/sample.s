# Short test case for your project.
#
# Note that this is by no means a comprehensive test!
#

		.text
		addi	$sp,$sp,-4
		addiu	$a1,$0,33
		sw 	$a1, 4($sp)
		lw   	$a2, 4($sp)
		