; add2: Adds two bits together to form a two bit result.
;
;   add2.operands_addr: 64 bit address of the two consecutive 1bit operands
;   add2.output_addr: 64 bit address to store the 2 bit result
; 

l1:	def	addr_bits, 10
l2:	org	0

add2:
	set_n	addr_bits

l3:	mov	add2.operands_addr, .fetch, addr_bits		; patch code to fetch operands
l4:	mov	add2.output_addr, .store, addr_bits		; patch code to store output

l5:	mov	.fetch:, .operands, 2				; fetches operands and patches lookup addr

l6:	mov	.lookup_table[.operands:], .output, 2		; does lookup, stores result

l7:	mov	add2.output, .store:, 2	    			; send output back to caller
l8:	arm	add2, add2.operands_addr    			; re-arm this routine now that we're done
l9:	wait				             		; make pinky available for more work


.lookup_table[2->2]:						; means 2 bits lookup index and 2 bit entries, automatically aligned
	data	#00, #01, #01, #10				; table lookup of results of add + carry
.output:
	bits	2						; local output buffer

add2.operands_addr:
	bits	addr_bits					; address of the input operands
add2.output_addr:
	bits	addr_bits					; address to send the output to

