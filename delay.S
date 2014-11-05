//#include <avr/io.h>

; defines the # of cpu cycles of overhead
; (includes the ldi r16,byte0; ldi r17,byte1; ldi r18, byte2, 
; ldi r19, byte3, and the call _delay_cycles)
OVERHEAD = 24

; some register aliases
cycles0 = 22
cycles1 = 23
cycles2 = 24
cycles3 = 25
temp = 19

    .text
    .global delay_cycles

delay_cycles:
;
;       subtract the overhead
        subi    cycles0,OVERHEAD        ; subtract the overhead
        sbc     cycles1,r1              ; ..
        sbc     cycles2,r1              ; ..
        sbc     cycles3,r1              ; ..
        brcs    dcx                     ; return if delay was less than the minimum
;
;       delay the lsb
        mov     r30,cycles0             ; Z = jtable offset to delay 0-7 cycles
        com     r30                     ; ..
        andi    r30,7                   ; ..
        clr     r31                     ; ..
        subi    r30,lo8 (-(gs(jtable))) ; add the table offset
        sbci    r31,hi8 (-(gs(jtable))) ; ..
        ijmp                            ; vector into our table to delay the cycles
jtable: nop
        nop
        nop
        nop
        nop
        nop
        nop
;
;       delay the remaining delay
loop:   subi    cycles0,8               ; decrement the count (8 cycles per loop)
        sbc     cycles1,r1              ; ..
        sbc     cycles2,r1              ; ..
        sbc     cycles3,r1              ; ..
        brcs    dcx                     ; exit if done
        nop                             ; .. add delay to make 8 cycles per loop
        rjmp    loop                    ; ..
dcx:    ret
        .end


