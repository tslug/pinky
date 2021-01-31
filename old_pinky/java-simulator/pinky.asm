; This is a test file that does nothing but provide
; me with labels that reference weird places in the
; asm..  A good N calculation test. :)

main:
	arm 1, end
label1:
	mv label3+5+label6, label5 + 5 -2, 6
	mv 0, 1, 0
label3:
	data 16
label4:
	arm 1, label6
label5:
	mv.32 label3, label6, 0
end:
	arm 1, main
	mv.1 0, 1, 0
label6:
	wait
