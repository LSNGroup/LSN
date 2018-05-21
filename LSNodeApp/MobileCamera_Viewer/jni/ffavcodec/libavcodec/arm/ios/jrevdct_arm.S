
        .text
        .align

        .globl _ff_j_rev_dct_arm

        @.func _ff_j_rev_dct_arm
_ff_j_rev_dct_arm:
        stmdb sp!, { r4 - r12, lr } 

        sub sp, sp, #4 
        str r0, [ sp ] 

        mov lr, r0 
        mov r12, #8 
        adr r11, const_array 
row_loop:
        ldrsh r0, [lr, # 0] 
        ldrsh r2, [lr, # 2] 

        
        
        ldr r5, [lr, # 0]
        ldr r6, [lr, # 4]
        ldr r3, [lr, # 8]
        ldr r4, [lr, #12]
        orr r3, r3, r4
        orr r3, r3, r6
        orrs r5, r3, r5
        beq end_of_row_loop 
        orrs r3, r3, r2
        beq empty_row

        ldrsh r1, [lr, # 8] 
        ldrsh r4, [lr, # 4] 
        ldrsh r6, [lr, # 6] 

        ldr r3, [r11, #4]
        add r7, r2, r6
        ldr r5, [r11, #36]
        mul r7, r3, r7 
        ldr r3, [r11, #8]
        mla r6, r5, r6, r7 
        add r5, r0, r4 
        mla r2, r3, r2, r7 
        sub r3, r0, r4 

        add r0, r2, r5, lsl #13 
        rsb r2, r2, r5, lsl #13 
        add r4, r6, r3, lsl #13 
        rsb r3, r6, r3, lsl #13 

        stmdb sp!, { r0, r2, r3, r4 } 

        ldrsh r3, [lr, #10] 
        ldrsh r5, [lr, #12] 
        ldrsh r7, [lr, #14] 

        add r0, r3, r5 
        add r2, r1, r7 
        add r4, r3, r7 
        add r6, r1, r5 
        ldr r9, [r11, #12]
        add r8, r4, r6 
        ldr r10, [r11, #32]
        mul r8, r9, r8 
        ldr r9, [r11, #44]
        mul r2, r10, r2 
        ldr r10, [r11, #40]
        mul r0, r9, r0 
        ldr r9, [r11, #28]
        mla r4, r10, r4, r8 
        ldr r10, [r11, #0]
        mla r6, r9, r6, r8 
        ldr r9, [r11, #20]
        mla r7, r10, r7, r2 
        ldr r10, [r11, #24]
        mla r5, r9, r5, r0 
        ldr r9, [r11, #16]
        mla r3, r10, r3, r0 
        add r7, r7, r4 
        mla r1, r9, r1, r2 
        add r5, r5, r6 
        add r3, r3, r4 
        add r1, r1, r6 

        ldmia sp!, { r0, r2, r4, r6 } 
                                      

        
        add r8, r0, r1
        add r8, r8, #(1<<10)
        mov r8, r8, asr #11
        strh r8, [lr, # 0]

        
        sub r8, r0, r1
        add r8, r8, #(1<<10)
        mov r8, r8, asr #11
        strh r8, [lr, #14]

        
        add r8, r6, r3
        add r8, r8, #(1<<10)
        mov r8, r8, asr #11
        strh r8, [lr, # 2]

        
        sub r8, r6, r3
        add r8, r8, #(1<<10)
        mov r8, r8, asr #11
        strh r8, [lr, #12]

        
        add r8, r4, r5
        add r8, r8, #(1<<10)
        mov r8, r8, asr #11
        strh r8, [lr, # 4]

        
        sub r8, r4, r5
        add r8, r8, #(1<<10)
        mov r8, r8, asr #11
        strh r8, [lr, #10]

        
        add r8, r2, r7
        add r8, r8, #(1<<10)
        mov r8, r8, asr #11
        strh r8, [lr, # 6]

        
        sub r8, r2, r7
        add r8, r8, #(1<<10)
        mov r8, r8, asr #11
        strh r8, [lr, # 8]

        
        add lr, lr, #16
        subs r12, r12, #1
        bne row_loop
        beq start_column_loop

empty_row:
        ldr r1, [r11, #48]
        mov r0, r0, lsl #2
        and r0, r0, r1
        add r0, r0, r0, lsl #16
        str r0, [lr, # 0]
        str r0, [lr, # 4]
        str r0, [lr, # 8]
        str r0, [lr, #12]

end_of_row_loop:
        
        add lr, lr, #16
        subs r12, r12, #1
        bne row_loop

start_column_loop:
        
        ldr lr, [ sp ]
        mov r12, #8
column_loop:
        ldrsh r0, [lr, #( 0*8)] 
        ldrsh r2, [lr, #( 4*8)] 
        ldrsh r4, [lr, #( 8*8)] 
        ldrsh r6, [lr, #(12*8)] 

        ldr r3, [r11, #4]
        add r1, r2, r6
        ldr r5, [r11, #36]
        mul r1, r3, r1 
        ldr r3, [r11, #8]
        mla r6, r5, r6, r1 
        add r5, r0, r4 
        mla r2, r3, r2, r1 
        sub r3, r0, r4 

        add r0, r2, r5, lsl #13 
        rsb r2, r2, r5, lsl #13 
        add r4, r6, r3, lsl #13 
        rsb r6, r6, r3, lsl #13 

        ldrsh r1, [lr, #( 2*8)] 
        ldrsh r3, [lr, #( 6*8)] 
        ldrsh r5, [lr, #(10*8)] 
        ldrsh r7, [lr, #(14*8)] 

        
        orr r9, r1, r3
        orr r10, r5, r7
        orrs r10, r9, r10
        beq empty_odd_column

        stmdb sp!, { r0, r2, r4, r6 } 

        add r0, r3, r5 
        add r2, r1, r7 
        add r4, r3, r7 
        add r6, r1, r5 
        ldr r9, [r11, #12]
        add r8, r4, r6
        ldr r10, [r11, #32]
        mul r8, r9, r8 
        ldr r9, [r11, #44]
        mul r2, r10, r2 
        ldr r10, [r11, #40]
        mul r0, r9, r0 
        ldr r9, [r11, #28]
        mla r4, r10, r4, r8 
        ldr r10, [r11, #0]
        mla r6, r9, r6, r8 
        ldr r9, [r11, #20]
        mla r7, r10, r7, r2 
        ldr r10, [r11, #24]
        mla r5, r9, r5, r0 
        ldr r9, [r11, #16]
        mla r3, r10, r3, r0 
        add r7, r7, r4 
        mla r1, r9, r1, r2 
        add r5, r5, r6 
        add r3, r3, r4 
        add r1, r1, r6 

        ldmia sp!, { r0, r2, r4, r6 } 
                                      

        
        add r8, r0, r1
        add r8, r8, #(1<<17)
        mov r8, r8, asr #18
        strh r8, [lr, #( 0*8)]

        
        sub r8, r0, r1
        add r8, r8, #(1<<17)
        mov r8, r8, asr #18
        strh r8, [lr, #(14*8)]

        
        add r8, r4, r3
        add r8, r8, #(1<<17)
        mov r8, r8, asr #18
        strh r8, [lr, #( 2*8)]

        
        sub r8, r4, r3
        add r8, r8, #(1<<17)
        mov r8, r8, asr #18
        strh r8, [lr, #(12*8)]

        
        add r8, r6, r5
        add r8, r8, #(1<<17)
        mov r8, r8, asr #18
        strh r8, [lr, #( 4*8)]

        
        sub r8, r6, r5
        add r8, r8, #(1<<17)
        mov r8, r8, asr #18
        strh r8, [lr, #(10*8)]

        
        add r8, r2, r7
        add r8, r8, #(1<<17)
        mov r8, r8, asr #18
        strh r8, [lr, #( 6*8)]

        
        sub r8, r2, r7
        add r8, r8, #(1<<17)
        mov r8, r8, asr #18
        strh r8, [lr, #( 8*8)]

        
        add lr, lr, #2
        subs r12, r12, #1
        bne column_loop
        beq the_end

empty_odd_column:
        
        
        add r0, r0, #(1<<17)
        mov r0, r0, asr #18
        strh r0, [lr, #( 0*8)]
        strh r0, [lr, #(14*8)]

        
        
        add r4, r4, #(1<<17)
        mov r4, r4, asr #18
        strh r4, [lr, #( 2*8)]
        strh r4, [lr, #(12*8)]

        
        
        add r6, r6, #(1<<17)
        mov r6, r6, asr #18
        strh r6, [lr, #( 4*8)]
        strh r6, [lr, #(10*8)]

        
        
        add r2, r2, #(1<<17)
        mov r2, r2, asr #18
        strh r2, [lr, #( 6*8)]
        strh r2, [lr, #( 8*8)]

        
        add lr, lr, #2
        subs r12, r12, #1
        bne column_loop

the_end:
        
        add sp, sp, #4
        ldmia sp!, { r4 - r12, pc } 

const_array:
        .align
        .word 2446
        .word 4433
        .word 6270
        .word 9633
        .word 12299
        .word 16819
        .word 25172
        .word -3196
        .word -7373
        .word -15137
        .word -16069
        .word -20995
        .word 0xFFFF
.text
