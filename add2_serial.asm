; add2: Adds two bits together to form a two bit result.
;
;   add2.operands_addr: 64 bit address of the two consecutive 1bit operands
;   add2.output_addr: 64 bit address to store the 2 bit result
; 
; The first code to successfully store 128 bits in a single move into add2.operands_addr and add2.output_addr
; starts execution of the add2 routine and stalls further writes to the address until it is re-armed.  This
; serializes execution.  If the calling function wants a callback on completion, then before writing to the
; input fields, it should arm itself on the address specified by add2.output_addr.

add2:
	set_n	64						; caller could be from anywhere, use 64 bit addrs
	move	add2.operands_addr, .fetch, 64			; patch code to fetch operands
	move	add2.output_addr, .store, 64			; patch code to store output

	move	.fetch:, .operands, 2				; fetches operands and patches table lookup addr

	set_n	16						; don't need the full range for local code/data
	move	.lookup_table(.operands:), .output, 2		; does the table lookup and stores result locally

	set_n	64						; prepare to send output back to caller

	move	add2.output, .store:, 2				; send output back to caller
	arm	add2, add2.operands_addr			; re-arm this routine now that we're done
	wait							; make pinky available for more work


.lookup_table(2->2):						; means 2 bits lookup index and 2 bit entries, automatically aligned
	table	#00, #01, #01, #10				; table lookup of results of add + carry
.output:
	bits	2						; local output buffer

add2.operands_addr:
	bits	64						; address of the input operands
add2.output_addr:
	bits	64						; address to send the output to

