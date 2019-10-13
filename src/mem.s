# Short test case for your project.
#
# Note that this is by no means a comprehensive test!
#

		.text
		addiu	$a1,$0,16
		sw 	$a1, ($sp)
		sw 	$a1, 4($sp)
		sw 	$a1, 8($sp)
		addiu	$a1,$0,32
		sw 	$a1, 12($sp)
		sw 	$a1, 16($sp)
		sw 	$a1, 20($sp)
		
		lw	$a1, ($sp)
		lw 	$a1, 4($sp)
		lw 	$a1, 8($sp)
		
		lw 	$a1, 12($sp)
		lw 	$a1, 16($sp)
		lw 	$a1, 20($sp)
		
