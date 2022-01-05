	add $sp, $zero, $zero, $imm2,0 , 2000
	lw $a0, $zero, $imm2, $zero, 0, 0x100		# get n from address 0x100
	lw $a1, $zero, $imm2, $zero, 0, 0x101		# get k from address 0x101
	jal $ra, $zero, $zero, $imm2, 0, binom		# jump to binom
	sw $zero, $zero, $imm2, $v0, 0, 0x102		# store fib(x) in 0x102
	halt $zero, $zero, $zero, $zero, 0, 0		# halt
binom:
    add $sp, $sp, $imm2, $zero, 0, -4           # adjust stack for 4 items
    sw $zero, $sp, $imm2, $s0, 0, 3             # save $s0
	sw $zero, $sp, $imm2, $ra, 0, 2			    # save return address
	sw $zero, $sp, $imm2, $a1, 0, 1
	sw $zero, $sp, $imm2, $a0, 0, 0
	beq $zero, $a1, $zero, $imm2, 0, ret1       # if k=0 go to ret1
	beq $zero, $a0, $a1, $imm2, 0, ret1         # if n=k go to ret1
	beq $zero, $zero, $zero, $imm2, 0, ret2
ret1:
    add $v0, $zero, $imm2, $zero, 0, 1			# $v0 = fib(x-2) + fib(x-1)
    beq $zero, $zero, $zero, $imm2, 0, rtn      # return
ret2:
    sub $a0,$a0,$zero,$imm2,0,1                 # n-1
	sub $a1,$a1,$zero,$imm2,0,1                 # k-1
    jal $ra, $zero, $zero, $imm2, 0, binom      #binom(n-1, k-1)
    add $s0,$v0,$zero,$zero,0,0                 # saving binom(n-1, k-1)
    add $a1,$a1,$zero,$imm2,0,1                 # k
    jal $ra, $zero, $zero, $imm2, 0, binom      #binom(n-1, k-1)
    add $v0,$v0,$s0,$zero,0,0                   #binom(n-1, k-1)+ binom(n-1, k)
    add $t1,$zero,$zero,$zero,0,0
    beq $zero, $zero, $zero, $imm2, 0, rtn      # return
rtn:
    lw $a0, $sp, $imm2, $zero, 0, 0			    # restore $a0
    lw $a1, $sp, $imm2, $zero, 0, 1	            # restore $a1
    lw $ra, $sp, $imm2, $zero, 0, 2			    # restore $ra
    lw $s0, $sp, $imm2, $zero, 0, 3			    # restore $s0
    add $sp, $sp, $imm2, $zero, 0, 4
    beq $zero, $zero, $zero, $ra, 0, 0          # real return

#for testing
.word 0x100 3
.word 0x101 2