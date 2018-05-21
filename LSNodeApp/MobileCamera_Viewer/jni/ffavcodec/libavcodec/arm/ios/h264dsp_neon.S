
        .text
        .align

        .globl _ff_put_h264_chroma_mc8_neon

        @.func _ff_put_h264_chroma_mc8_neon
_ff_put_h264_chroma_mc8_neon:
        push {r4-r7, lr}
        ldrd r4, [sp, #20]
        pld [r1]
        pld [r1, r2]

        muls r7, r4, r5
        rsb r6, r7, r5, lsl #3
        rsb ip, r7, r4, lsl #3
        sub r4, r7, r4, lsl #3
        sub r4, r4, r5, lsl #3
        add r4, r4, #64

        beq 2f

        add r5, r1, r2

        vdup.8 d0, r4
        lsl r4, r2, #1
        vdup.8 d1, ip
        vld1.64 {d4, d5}, [r1], r4
        vdup.8 d2, r6
        vld1.64 {d6, d7}, [r5], r4
        vdup.8 d3, r7

        vext.8 d5, d4, d5, #1
        vext.8 d7, d6, d7, #1

1: pld [r5]
        vmull.u8 q8, d4, d0
        vmlal.u8 q8, d5, d1
        vld1.64 {d4, d5}, [r1], r4
        vmlal.u8 q8, d6, d2
        vext.8 d5, d4, d5, #1
        vmlal.u8 q8, d7, d3
        vmull.u8 q9, d6, d0
        subs r3, r3, #2
        vmlal.u8 q9, d7, d1
        vmlal.u8 q9, d4, d2
        vmlal.u8 q9, d5, d3
        vrshrn.u16 d16, q8, #6
        vld1.64 {d6, d7}, [r5], r4
        pld [r1]
        vrshrn.u16 d17, q9, #6
        vext.8 d7, d6, d7, #1
        vst1.64 {d16}, [r0,:64], r2
        vst1.64 {d17}, [r0,:64], r2
        bgt 1b

        pop {r4-r7, pc}

2: tst r6, r6
        add ip, ip, r6
        vdup.8 d0, r4
        vdup.8 d1, ip

        beq 4f

        add r5, r1, r2
        lsl r4, r2, #1
        vld1.64 {d4}, [r1], r4
        vld1.64 {d6}, [r5], r4

3: pld [r5]
        vmull.u8 q8, d4, d0
        vmlal.u8 q8, d6, d1
        vld1.64 {d4}, [r1], r4
        vmull.u8 q9, d6, d0
        vmlal.u8 q9, d4, d1
        vld1.64 {d6}, [r5], r4
        vrshrn.u16 d16, q8, #6
        vrshrn.u16 d17, q9, #6
        subs r3, r3, #2
        pld [r1]
        vst1.64 {d16}, [r0,:64], r2
        vst1.64 {d17}, [r0,:64], r2
        bgt 3b

        pop {r4-r7, pc}

4: vld1.64 {d4, d5}, [r1], r2
        vld1.64 {d6, d7}, [r1], r2
        vext.8 d5, d4, d5, #1
        vext.8 d7, d6, d7, #1

5: pld [r1]
        subs r3, r3, #2
        vmull.u8 q8, d4, d0
        vmlal.u8 q8, d5, d1
        vld1.64 {d4, d5}, [r1], r2
        vmull.u8 q9, d6, d0
        vmlal.u8 q9, d7, d1
        pld [r1]
        vext.8 d5, d4, d5, #1
        vrshrn.u16 d16, q8, #6
        vrshrn.u16 d17, q9, #6
        vld1.64 {d6, d7}, [r1], r2
        vext.8 d7, d6, d7, #1
        vst1.64 {d16}, [r0,:64], r2
        vst1.64 {d17}, [r0,:64], r2
        bgt 5b

        pop {r4-r7, pc}

        @.endfunc
        .globl _ff_avg_h264_chroma_mc8_neon

        @.func _ff_avg_h264_chroma_mc8_neon
_ff_avg_h264_chroma_mc8_neon:
        push {r4-r7, lr}
        ldrd r4, [sp, #20]
        mov lr, r0
        pld [r1]
        pld [r1, r2]

        muls r7, r4, r5
        rsb r6, r7, r5, lsl #3
        rsb ip, r7, r4, lsl #3
        sub r4, r7, r4, lsl #3
        sub r4, r4, r5, lsl #3
        add r4, r4, #64

        beq 2f

        add r5, r1, r2

        vdup.8 d0, r4
        lsl r4, r2, #1
        vdup.8 d1, ip
        vld1.64 {d4, d5}, [r1], r4
        vdup.8 d2, r6
        vld1.64 {d6, d7}, [r5], r4
        vdup.8 d3, r7

        vext.8 d5, d4, d5, #1
        vext.8 d7, d6, d7, #1

1: pld [r5]
        vmull.u8 q8, d4, d0
        vmlal.u8 q8, d5, d1
        vld1.64 {d4, d5}, [r1], r4
        vmlal.u8 q8, d6, d2
        vext.8 d5, d4, d5, #1
        vmlal.u8 q8, d7, d3
        vmull.u8 q9, d6, d0
        subs r3, r3, #2
        vmlal.u8 q9, d7, d1
        vmlal.u8 q9, d4, d2
        vmlal.u8 q9, d5, d3
        vrshrn.u16 d16, q8, #6
        vld1.64 {d6, d7}, [r5], r4
        pld [r1]
        vrshrn.u16 d17, q9, #6
        vld1.64 {d20}, [lr,:64], r2
        vld1.64 {d21}, [lr,:64], r2
        vrhadd.u8 q8, q8, q10
        vext.8 d7, d6, d7, #1
        vst1.64 {d16}, [r0,:64], r2
        vst1.64 {d17}, [r0,:64], r2
        bgt 1b

        pop {r4-r7, pc}

2: tst r6, r6
        add ip, ip, r6
        vdup.8 d0, r4
        vdup.8 d1, ip

        beq 4f

        add r5, r1, r2
        lsl r4, r2, #1
        vld1.64 {d4}, [r1], r4
        vld1.64 {d6}, [r5], r4

3: pld [r5]
        vmull.u8 q8, d4, d0
        vmlal.u8 q8, d6, d1
        vld1.64 {d4}, [r1], r4
        vmull.u8 q9, d6, d0
        vmlal.u8 q9, d4, d1
        vld1.64 {d6}, [r5], r4
        vrshrn.u16 d16, q8, #6
        vrshrn.u16 d17, q9, #6
        vld1.64 {d20}, [lr,:64], r2
        vld1.64 {d21}, [lr,:64], r2
        vrhadd.u8 q8, q8, q10
        subs r3, r3, #2
        pld [r1]
        vst1.64 {d16}, [r0,:64], r2
        vst1.64 {d17}, [r0,:64], r2
        bgt 3b

        pop {r4-r7, pc}

4: vld1.64 {d4, d5}, [r1], r2
        vld1.64 {d6, d7}, [r1], r2
        vext.8 d5, d4, d5, #1
        vext.8 d7, d6, d7, #1

5: pld [r1]
        subs r3, r3, #2
        vmull.u8 q8, d4, d0
        vmlal.u8 q8, d5, d1
        vld1.64 {d4, d5}, [r1], r2
        vmull.u8 q9, d6, d0
        vmlal.u8 q9, d7, d1
        pld [r1]
        vext.8 d5, d4, d5, #1
        vrshrn.u16 d16, q8, #6
        vrshrn.u16 d17, q9, #6
        vld1.64 {d20}, [lr,:64], r2
        vld1.64 {d21}, [lr,:64], r2
        vrhadd.u8 q8, q8, q10
        vld1.64 {d6, d7}, [r1], r2
        vext.8 d7, d6, d7, #1
        vst1.64 {d16}, [r0,:64], r2
        vst1.64 {d17}, [r0,:64], r2
        bgt 5b

        pop {r4-r7, pc}

        @.endfunc
        .globl _ff_put_h264_chroma_mc4_neon

        @.func _ff_put_h264_chroma_mc4_neon
_ff_put_h264_chroma_mc4_neon:
        push {r4-r7, lr}
        ldrd r4, [sp, #20]
        pld [r1]
        pld [r1, r2]

        muls r7, r4, r5
        rsb r6, r7, r5, lsl #3
        rsb ip, r7, r4, lsl #3
        sub r4, r7, r4, lsl #3
        sub r4, r4, r5, lsl #3
        add r4, r4, #64

        beq 2f

        add r5, r1, r2

        vdup.8 d0, r4
        lsl r4, r2, #1
        vdup.8 d1, ip
        vld1.64 {d4}, [r1], r4
        vdup.8 d2, r6
        vld1.64 {d6}, [r5], r4
        vdup.8 d3, r7

        vext.8 d5, d4, d5, #1
        vext.8 d7, d6, d7, #1
        vtrn.32 d4, d5
        vtrn.32 d6, d7

        vtrn.32 d0, d1
        vtrn.32 d2, d3

1: pld [r5]
        vmull.u8 q8, d4, d0
        vmlal.u8 q8, d6, d2
        vld1.64 {d4}, [r1], r4
        vext.8 d5, d4, d5, #1
        vtrn.32 d4, d5
        vmull.u8 q9, d6, d0
        vmlal.u8 q9, d4, d2
        vld1.64 {d6}, [r5], r4
        vadd.i16 d16, d16, d17
        vadd.i16 d17, d18, d19
        vrshrn.u16 d16, q8, #6
        subs r3, r3, #2
        pld [r1]
        vext.8 d7, d6, d7, #1
        vtrn.32 d6, d7
        vst1.32 {d16[0]}, [r0,:32], r2
        vst1.32 {d16[1]}, [r0,:32], r2
        bgt 1b

        pop {r4-r7, pc}

2: tst r6, r6
        add ip, ip, r6
        vdup.8 d0, r4
        vdup.8 d1, ip
        vtrn.32 d0, d1

        beq 4f

        vext.32 d1, d0, d1, #1
        add r5, r1, r2
        lsl r4, r2, #1
        vld1.32 {d4[0]}, [r1], r4
        vld1.32 {d4[1]}, [r5], r4

3: pld [r5]
        vmull.u8 q8, d4, d0
        vld1.32 {d4[0]}, [r1], r4
        vmull.u8 q9, d4, d1
        vld1.32 {d4[1]}, [r5], r4
        vadd.i16 d16, d16, d17
        vadd.i16 d17, d18, d19
        vrshrn.u16 d16, q8, #6
        subs r3, r3, #2
        pld [r1]
        vst1.32 {d16[0]}, [r0,:32], r2
        vst1.32 {d16[1]}, [r0,:32], r2
        bgt 3b

        pop {r4-r7, pc}

4: vld1.64 {d4}, [r1], r2
        vld1.64 {d6}, [r1], r2
        vext.8 d5, d4, d5, #1
        vext.8 d7, d6, d7, #1
        vtrn.32 d4, d5
        vtrn.32 d6, d7

5: vmull.u8 q8, d4, d0
        vmull.u8 q9, d6, d0
        subs r3, r3, #2
        vld1.64 {d4}, [r1], r2
        vext.8 d5, d4, d5, #1
        vtrn.32 d4, d5
        vadd.i16 d16, d16, d17
        vadd.i16 d17, d18, d19
        pld [r1]
        vrshrn.u16 d16, q8, #6
        vld1.64 {d6}, [r1], r2
        vext.8 d7, d6, d7, #1
        vtrn.32 d6, d7
        pld [r1]
        vst1.32 {d16[0]}, [r0,:32], r2
        vst1.32 {d16[1]}, [r0,:32], r2
        bgt 5b

        pop {r4-r7, pc}

        @.endfunc
        .globl _ff_avg_h264_chroma_mc4_neon

        @.func _ff_avg_h264_chroma_mc4_neon
_ff_avg_h264_chroma_mc4_neon:
        push {r4-r7, lr}
        ldrd r4, [sp, #20]
        mov lr, r0
        pld [r1]
        pld [r1, r2]

        muls r7, r4, r5
        rsb r6, r7, r5, lsl #3
        rsb ip, r7, r4, lsl #3
        sub r4, r7, r4, lsl #3
        sub r4, r4, r5, lsl #3
        add r4, r4, #64

        beq 2f

        add r5, r1, r2

        vdup.8 d0, r4
        lsl r4, r2, #1
        vdup.8 d1, ip
        vld1.64 {d4}, [r1], r4
        vdup.8 d2, r6
        vld1.64 {d6}, [r5], r4
        vdup.8 d3, r7

        vext.8 d5, d4, d5, #1
        vext.8 d7, d6, d7, #1
        vtrn.32 d4, d5
        vtrn.32 d6, d7

        vtrn.32 d0, d1
        vtrn.32 d2, d3

1: pld [r5]
        vmull.u8 q8, d4, d0
        vmlal.u8 q8, d6, d2
        vld1.64 {d4}, [r1], r4
        vext.8 d5, d4, d5, #1
        vtrn.32 d4, d5
        vmull.u8 q9, d6, d0
        vmlal.u8 q9, d4, d2
        vld1.64 {d6}, [r5], r4
        vadd.i16 d16, d16, d17
        vadd.i16 d17, d18, d19
        vrshrn.u16 d16, q8, #6
        subs r3, r3, #2
        pld [r1]
        vld1.32 {d20[0]}, [lr,:32], r2
        vld1.32 {d20[1]}, [lr,:32], r2
        vrhadd.u8 d16, d16, d20
        vext.8 d7, d6, d7, #1
        vtrn.32 d6, d7
        vst1.32 {d16[0]}, [r0,:32], r2
        vst1.32 {d16[1]}, [r0,:32], r2
        bgt 1b

        pop {r4-r7, pc}

2: tst r6, r6
        add ip, ip, r6
        vdup.8 d0, r4
        vdup.8 d1, ip
        vtrn.32 d0, d1

        beq 4f

        vext.32 d1, d0, d1, #1
        add r5, r1, r2
        lsl r4, r2, #1
        vld1.32 {d4[0]}, [r1], r4
        vld1.32 {d4[1]}, [r5], r4

3: pld [r5]
        vmull.u8 q8, d4, d0
        vld1.32 {d4[0]}, [r1], r4
        vmull.u8 q9, d4, d1
        vld1.32 {d4[1]}, [r5], r4
        vadd.i16 d16, d16, d17
        vadd.i16 d17, d18, d19
        vrshrn.u16 d16, q8, #6
        vld1.32 {d20[0]}, [lr,:32], r2
        vld1.32 {d20[1]}, [lr,:32], r2
        vrhadd.u8 d16, d16, d20
        subs r3, r3, #2
        pld [r1]
        vst1.32 {d16[0]}, [r0,:32], r2
        vst1.32 {d16[1]}, [r0,:32], r2
        bgt 3b

        pop {r4-r7, pc}

4: vld1.64 {d4}, [r1], r2
        vld1.64 {d6}, [r1], r2
        vext.8 d5, d4, d5, #1
        vext.8 d7, d6, d7, #1
        vtrn.32 d4, d5
        vtrn.32 d6, d7

5: vmull.u8 q8, d4, d0
        vmull.u8 q9, d6, d0
        subs r3, r3, #2
        vld1.64 {d4}, [r1], r2
        vext.8 d5, d4, d5, #1
        vtrn.32 d4, d5
        vadd.i16 d16, d16, d17
        vadd.i16 d17, d18, d19
        pld [r1]
        vrshrn.u16 d16, q8, #6
        vld1.32 {d20[0]}, [lr,:32], r2
        vld1.32 {d20[1]}, [lr,:32], r2
        vrhadd.u8 d16, d16, d20
        vld1.64 {d6}, [r1], r2
        vext.8 d7, d6, d7, #1
        vtrn.32 d6, d7
        pld [r1]
        vst1.32 {d16[0]}, [r0,:32], r2
        vst1.32 {d16[1]}, [r0,:32], r2
        bgt 5b

        pop {r4-r7, pc}

        @.endfunc
        .globl _ff_put_h264_chroma_mc2_neon

        @.func _ff_put_h264_chroma_mc2_neon
_ff_put_h264_chroma_mc2_neon:
        push {r4-r6, lr}
        ldr r4, [sp, #16]
        ldr lr, [sp, #20]
        pld [r1]
        pld [r1, r2]
        orrs r5, r4, lr
        beq 2f

        mul r5, r4, lr
        rsb r6, r5, lr, lsl #3
        rsb r12, r5, r4, lsl #3
        sub r4, r5, r4, lsl #3
        sub r4, r4, lr, lsl #3
        add r4, r4, #64
        vdup.8 d0, r4
        vdup.8 d2, r12
        vdup.8 d1, r6
        vdup.8 d3, r5
        vtrn.16 q0, q1
1:
        vld1.32 {d4[0]}, [r1], r2
        vld1.32 {d4[1]}, [r1], r2
        vrev64.32 d5, d4
        vld1.32 {d5[1]}, [r1]
        vext.8 q3, q2, q2, #1
        vtrn.16 q2, q3
        vmull.u8 q8, d4, d0
        vmlal.u8 q8, d5, d1
        vtrn.32 d16, d17
        vadd.i16 d16, d16, d17
        vrshrn.u16 d16, q8, #6
        vst1.16 {d16[0]}, [r0,:16], r2
        vst1.16 {d16[1]}, [r0,:16], r2
        subs r3, r3, #2
        bgt 1b
        pop {r4-r6, pc}
2:
        ldrh r5, [r1], r2
        strh r5, [r0], r2
        ldrh r6, [r1], r2
        strh r6, [r0], r2
        subs r3, r3, #2
        bgt 2b
        pop {r4-r6, pc}

        @.endfunc
        .globl _ff_avg_h264_chroma_mc2_neon

        @.func _ff_avg_h264_chroma_mc2_neon
_ff_avg_h264_chroma_mc2_neon:
        push {r4-r6, lr}
        ldr r4, [sp, #16]
        ldr lr, [sp, #20]
        pld [r1]
        pld [r1, r2]
        orrs r5, r4, lr
        beq 2f

        mul r5, r4, lr
        rsb r6, r5, lr, lsl #3
        rsb r12, r5, r4, lsl #3
        sub r4, r5, r4, lsl #3
        sub r4, r4, lr, lsl #3
        add r4, r4, #64
        vdup.8 d0, r4
        vdup.8 d2, r12
        vdup.8 d1, r6
        vdup.8 d3, r5
        vtrn.16 q0, q1
1:
        vld1.32 {d4[0]}, [r1], r2
        vld1.32 {d4[1]}, [r1], r2
        vrev64.32 d5, d4
        vld1.32 {d5[1]}, [r1]
        vext.8 q3, q2, q2, #1
        vtrn.16 q2, q3
        vmull.u8 q8, d4, d0
        vmlal.u8 q8, d5, d1
        vld1.16 {d18[0]}, [r0,:16], r2
        vld1.16 {d18[1]}, [r0,:16]
        sub r0, r0, r2
        vtrn.32 d16, d17
        vadd.i16 d16, d16, d17
        vrshrn.u16 d16, q8, #6
        vrhadd.u8 d16, d16, d18
        vst1.16 {d16[0]}, [r0,:16], r2
        vst1.16 {d16[1]}, [r0,:16], r2
        subs r3, r3, #2
        bgt 1b
        pop {r4-r6, pc}
2:
        vld1.16 {d16[0]}, [r1], r2
        vld1.16 {d16[1]}, [r1], r2
        vld1.16 {d18[0]}, [r0,:16], r2
        vld1.16 {d18[1]}, [r0,:16]
        sub r0, r0, r2
        vrhadd.u8 d16, d16, d18
        vst1.16 {d16[0]}, [r0,:16], r2
        vst1.16 {d16[1]}, [r0,:16], r2
        subs r3, r3, #2
        bgt 2b
        pop {r4-r6, pc}

        @.endfunc







        .globl _ff_h264_v_loop_filter_luma_neon

        @.func _ff_h264_v_loop_filter_luma_neon
_ff_h264_v_loop_filter_luma_neon:
        ldr ip, [sp]
        tst r2, r2
        ldr ip, [ip]
        tstne r3, r3
        vmov.32 d24[0], ip
        and ip, ip, ip, lsl #16
        bxeq lr
        ands ip, ip, ip, lsl #8
        bxlt lr

        vld1.64 {d0, d1}, [r0,:128], r1
        vld1.64 {d2, d3}, [r0,:128], r1
        vld1.64 {d4, d5}, [r0,:128], r1
        sub r0, r0, r1, lsl #2
        sub r0, r0, r1, lsl #1
        vld1.64 {d20,d21}, [r0,:128], r1
        vld1.64 {d18,d19}, [r0,:128], r1
        vld1.64 {d16,d17}, [r0,:128], r1

        and ip, sp, #15
        add ip, ip, #32
        sub sp, sp, ip
        vst1.64 {d12-d15}, [sp,:128]
        sub sp, sp, #32
        vst1.64 {d8-d11}, [sp,:128]

        vdup.8 q11, r2 
        vmovl.u8 q12, d24
        vabd.u8 q6, q8, q0 
        vmovl.u16 q12, d24
        vabd.u8 q14, q9, q8 
        vsli.16 q12, q12, #8
        vabd.u8 q15, q1, q0 
        vsli.32 q12, q12, #16
        vclt.u8 q6, q6, q11 
        vdup.8 q11, r3 
        vclt.s8 q7, q12, #0
        vclt.u8 q14, q14, q11 
        vclt.u8 q15, q15, q11 
        vbic q6, q6, q7
        vabd.u8 q4, q10, q8 
        vand q6, q6, q14
        vabd.u8 q5, q2, q0 
        vclt.u8 q4, q4, q11 
        vand q6, q6, q15
        vclt.u8 q5, q5, q11 
        vand q4, q4, q6
        vand q5, q5, q6
        vand q12, q12, q6
        vrhadd.u8 q14, q8, q0
        vsub.i8 q6, q12, q4
        vqadd.u8 q7, q9, q12
        vhadd.u8 q10, q10, q14
        vsub.i8 q6, q6, q5
        vhadd.u8 q14, q2, q14
        vmin.u8 q7, q7, q10
        vqsub.u8 q11, q9, q12
        vqadd.u8 q2, q1, q12
        vmax.u8 q7, q7, q11
        vqsub.u8 q11, q1, q12
        vmin.u8 q14, q2, q14
        vmovl.u8 q2, d0
        vmax.u8 q14, q14, q11
        vmovl.u8 q10, d1
        vsubw.u8 q2, q2, d16
        vsubw.u8 q10, q10, d17
        vshl.i16 q2, q2, #2
        vshl.i16 q10, q10, #2
        vaddw.u8 q2, q2, d18
        vaddw.u8 q10, q10, d19
        vsubw.u8 q2, q2, d2
        vsubw.u8 q10, q10, d3
        vrshrn.i16 d4, q2, #3
        vrshrn.i16 d5, q10, #3
        vbsl q4, q7, q9
        vbsl q5, q14, q1
        vneg.s8 q7, q6
        vmovl.u8 q14, d16
        vmin.s8 q2, q2, q6
        vmovl.u8 q6, d17
        vmax.s8 q2, q2, q7
        vmovl.u8 q11, d0
        vmovl.u8 q12, d1
        vaddw.s8 q14, q14, d4
        vaddw.s8 q6, q6, d5
        vsubw.s8 q11, q11, d4
        vsubw.s8 q12, q12, d5
        vqmovun.s16 d16, q14
        vqmovun.s16 d17, q6
        vqmovun.s16 d0, q11
        vqmovun.s16 d1, q12

        sub r0, r0, r1, lsl #1
        vst1.64 {d8, d9}, [r0,:128], r1
        vst1.64 {d16,d17}, [r0,:128], r1
        vst1.64 {d0, d1}, [r0,:128], r1
        vst1.64 {d10,d11}, [r0,:128]

        vld1.64 {d8-d11}, [sp,:128]!
        vld1.64 {d12-d15}, [sp,:128], ip
        bx lr

        @.endfunc

        .globl _ff_h264_h_loop_filter_luma_neon

        @.func _ff_h264_h_loop_filter_luma_neon
_ff_h264_h_loop_filter_luma_neon:
        ldr ip, [sp]
        tst r2, r2
        ldr ip, [ip]
        tstne r3, r3
        vmov.32 d24[0], ip
        and ip, ip, ip, lsl #16
        bxeq lr
        ands ip, ip, ip, lsl #8
        bxlt lr

        sub r0, r0, #4
        vld1.64 {d6}, [r0], r1
        vld1.64 {d20}, [r0], r1
        vld1.64 {d18}, [r0], r1
        vld1.64 {d16}, [r0], r1
        vld1.64 {d0}, [r0], r1
        vld1.64 {d2}, [r0], r1
        vld1.64 {d4}, [r0], r1
        vld1.64 {d26}, [r0], r1
        vld1.64 {d7}, [r0], r1
        vld1.64 {d21}, [r0], r1
        vld1.64 {d19}, [r0], r1
        vld1.64 {d17}, [r0], r1
        vld1.64 {d1}, [r0], r1
        vld1.64 {d3}, [r0], r1
        vld1.64 {d5}, [r0], r1
        vld1.64 {d27}, [r0], r1

        vtrn.32 q3, q0
        vtrn.32 q10, q1
        vtrn.32 q9, q2
        vtrn.32 q8, q13
        vtrn.16 q3, q9
        vtrn.16 q10, q8
        vtrn.16 q0, q2
        vtrn.16 q1, q13
        vtrn.8 q3, q10
        vtrn.8 q9, q8
        vtrn.8 q0, q1
        vtrn.8 q2, q13

        and ip, sp, #15
        add ip, ip, #32
        sub sp, sp, ip
        vst1.64 {d12-d15}, [sp,:128]
        sub sp, sp, #32
        vst1.64 {d8-d11}, [sp,:128]

        vdup.8 q11, r2 
        vmovl.u8 q12, d24
        vabd.u8 q6, q8, q0 
        vmovl.u16 q12, d24
        vabd.u8 q14, q9, q8 
        vsli.16 q12, q12, #8
        vabd.u8 q15, q1, q0 
        vsli.32 q12, q12, #16
        vclt.u8 q6, q6, q11 
        vdup.8 q11, r3 
        vclt.s8 q7, q12, #0
        vclt.u8 q14, q14, q11 
        vclt.u8 q15, q15, q11 
        vbic q6, q6, q7
        vabd.u8 q4, q10, q8 
        vand q6, q6, q14
        vabd.u8 q5, q2, q0 
        vclt.u8 q4, q4, q11 
        vand q6, q6, q15
        vclt.u8 q5, q5, q11 
        vand q4, q4, q6
        vand q5, q5, q6
        vand q12, q12, q6
        vrhadd.u8 q14, q8, q0
        vsub.i8 q6, q12, q4
        vqadd.u8 q7, q9, q12
        vhadd.u8 q10, q10, q14
        vsub.i8 q6, q6, q5
        vhadd.u8 q14, q2, q14
        vmin.u8 q7, q7, q10
        vqsub.u8 q11, q9, q12
        vqadd.u8 q2, q1, q12
        vmax.u8 q7, q7, q11
        vqsub.u8 q11, q1, q12
        vmin.u8 q14, q2, q14
        vmovl.u8 q2, d0
        vmax.u8 q14, q14, q11
        vmovl.u8 q10, d1
        vsubw.u8 q2, q2, d16
        vsubw.u8 q10, q10, d17
        vshl.i16 q2, q2, #2
        vshl.i16 q10, q10, #2
        vaddw.u8 q2, q2, d18
        vaddw.u8 q10, q10, d19
        vsubw.u8 q2, q2, d2
        vsubw.u8 q10, q10, d3
        vrshrn.i16 d4, q2, #3
        vrshrn.i16 d5, q10, #3
        vbsl q4, q7, q9
        vbsl q5, q14, q1
        vneg.s8 q7, q6
        vmovl.u8 q14, d16
        vmin.s8 q2, q2, q6
        vmovl.u8 q6, d17
        vmax.s8 q2, q2, q7
        vmovl.u8 q11, d0
        vmovl.u8 q12, d1
        vaddw.s8 q14, q14, d4
        vaddw.s8 q6, q6, d5
        vsubw.s8 q11, q11, d4
        vsubw.s8 q12, q12, d5
        vqmovun.s16 d16, q14
        vqmovun.s16 d17, q6
        vqmovun.s16 d0, q11
        vqmovun.s16 d1, q12

        vtrn.16 q4, q0
        vtrn.16 q8, q5
        vtrn.8 q4, q8
        vtrn.8 q0, q5

        sub r0, r0, r1, lsl #4
        add r0, r0, #2
        vst1.32 {d8[0]}, [r0], r1
        vst1.32 {d16[0]}, [r0], r1
        vst1.32 {d0[0]}, [r0], r1
        vst1.32 {d10[0]}, [r0], r1
        vst1.32 {d8[1]}, [r0], r1
        vst1.32 {d16[1]}, [r0], r1
        vst1.32 {d0[1]}, [r0], r1
        vst1.32 {d10[1]}, [r0], r1
        vst1.32 {d9[0]}, [r0], r1
        vst1.32 {d17[0]}, [r0], r1
        vst1.32 {d1[0]}, [r0], r1
        vst1.32 {d11[0]}, [r0], r1
        vst1.32 {d9[1]}, [r0], r1
        vst1.32 {d17[1]}, [r0], r1
        vst1.32 {d1[1]}, [r0], r1
        vst1.32 {d11[1]}, [r0], r1

        vld1.64 {d8-d11}, [sp,:128]!
        vld1.64 {d12-d15}, [sp,:128], ip
        bx lr

        @.endfunc


        .globl _ff_h264_v_loop_filter_chroma_neon

        @.func _ff_h264_v_loop_filter_chroma_neon
_ff_h264_v_loop_filter_chroma_neon:
        ldr ip, [sp]
        tst r2, r2
        ldr ip, [ip]
        tstne r3, r3
        vmov.32 d24[0], ip
        and ip, ip, ip, lsl #16
        bxeq lr
        ands ip, ip, ip, lsl #8
        bxlt lr

        sub r0, r0, r1, lsl #1
        vld1.64 {d18}, [r0,:64], r1
        vld1.64 {d16}, [r0,:64], r1
        vld1.64 {d0}, [r0,:64], r1
        vld1.64 {d2}, [r0,:64]

        vdup.8 d22, r2 
        vmovl.u8 q12, d24
        vabd.u8 d26, d16, d0 
        vmovl.u8 q2, d0
        vabd.u8 d28, d18, d16 
        vsubw.u8 q2, q2, d16
        vsli.16 d24, d24, #8
        vshl.i16 q2, q2, #2
        vabd.u8 d30, d2, d0 
        vaddw.u8 q2, q2, d18
        vclt.u8 d26, d26, d22 
        vsubw.u8 q2, q2, d2
        vdup.8 d22, r3 
        vclt.s8 d25, d24, #0
        vrshrn.i16 d4, q2, #3
        vclt.u8 d28, d28, d22 
        vbic d26, d26, d25
        vclt.u8 d30, d30, d22 
        vand d26, d26, d28
        vneg.s8 d25, d24
        vand d26, d26, d30
        vmin.s8 d4, d4, d24
        vmovl.u8 q14, d16
        vand d4, d4, d26
        vmax.s8 d4, d4, d25
        vmovl.u8 q11, d0
        vaddw.s8 q14, q14, d4
        vsubw.s8 q11, q11, d4
        vqmovun.s16 d16, q14
        vqmovun.s16 d0, q11

        sub r0, r0, r1, lsl #1
        vst1.64 {d16}, [r0,:64], r1
        vst1.64 {d0}, [r0,:64], r1

        bx lr

        @.endfunc

        .globl _ff_h264_h_loop_filter_chroma_neon

        @.func _ff_h264_h_loop_filter_chroma_neon
_ff_h264_h_loop_filter_chroma_neon:
        ldr ip, [sp]
        tst r2, r2
        ldr ip, [ip]
        tstne r3, r3
        vmov.32 d24[0], ip
        and ip, ip, ip, lsl #16
        bxeq lr
        ands ip, ip, ip, lsl #8
        bxlt lr

        sub r0, r0, #2
        vld1.32 {d18[0]}, [r0], r1
        vld1.32 {d16[0]}, [r0], r1
        vld1.32 {d0[0]}, [r0], r1
        vld1.32 {d2[0]}, [r0], r1
        vld1.32 {d18[1]}, [r0], r1
        vld1.32 {d16[1]}, [r0], r1
        vld1.32 {d0[1]}, [r0], r1
        vld1.32 {d2[1]}, [r0], r1

        vtrn.16 d18, d0
        vtrn.16 d16, d2
        vtrn.8 d18, d16
        vtrn.8 d0, d2

        vdup.8 d22, r2 
        vmovl.u8 q12, d24
        vabd.u8 d26, d16, d0 
        vmovl.u8 q2, d0
        vabd.u8 d28, d18, d16 
        vsubw.u8 q2, q2, d16
        vsli.16 d24, d24, #8
        vshl.i16 q2, q2, #2
        vabd.u8 d30, d2, d0 
        vaddw.u8 q2, q2, d18
        vclt.u8 d26, d26, d22 
        vsubw.u8 q2, q2, d2
        vdup.8 d22, r3 
        vclt.s8 d25, d24, #0
        vrshrn.i16 d4, q2, #3
        vclt.u8 d28, d28, d22 
        vbic d26, d26, d25
        vclt.u8 d30, d30, d22 
        vand d26, d26, d28
        vneg.s8 d25, d24
        vand d26, d26, d30
        vmin.s8 d4, d4, d24
        vmovl.u8 q14, d16
        vand d4, d4, d26
        vmax.s8 d4, d4, d25
        vmovl.u8 q11, d0
        vaddw.s8 q14, q14, d4
        vsubw.s8 q11, q11, d4
        vqmovun.s16 d16, q14
        vqmovun.s16 d0, q11

        vtrn.16 d18, d0
        vtrn.16 d16, d2
        vtrn.8 d18, d16
        vtrn.8 d0, d2

        sub r0, r0, r1, lsl #3
        vst1.32 {d18[0]}, [r0], r1
        vst1.32 {d16[0]}, [r0], r1
        vst1.32 {d0[0]}, [r0], r1
        vst1.32 {d2[0]}, [r0], r1
        vst1.32 {d18[1]}, [r0], r1
        vst1.32 {d16[1]}, [r0], r1
        vst1.32 {d0[1]}, [r0], r1
        vst1.32 {d2[1]}, [r0], r1

        bx lr

        @.endfunc








        @.func put_h264_qpel16_h_lowpass_neon_packed
put_h264_qpel16_h_lowpass_neon_packed:
        mov r4, lr
        mov ip, #16
        mov r3, #8
        bl put_h264_qpel8_h_lowpass_neon
        sub r1, r1, r2, lsl #4
        add r1, r1, #8
        mov ip, #16
        mov lr, r4
        b put_h264_qpel8_h_lowpass_neon

        @.endfunc



        @.func put_h264_qpel16_h_lowpass_neon
put_h264_qpel16_h_lowpass_neon:
        push {lr}
        mov ip, #16
        bl put_h264_qpel8_h_lowpass_neon
        sub r0, r0, r3, lsl #4
        sub r1, r1, r2, lsl #4
        add r0, r0, #8
        add r1, r1, #8
        mov ip, #16
        pop {lr}

        @.endfunc


        @.func put_h264_qpel8_h_lowpass_neon
put_h264_qpel8_h_lowpass_neon:
1: vld1.64 {d0, d1}, [r1], r2
        vld1.64 {d16,d17}, [r1], r2
        subs ip, ip, #2
        t0 .req q0
        t1 .req q8
        vext.8 d2, d0, d1, #2
        vext.8 d3, d0, d1, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d0, d1, #1
        vext.8 d5, d0, d1, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d0, d1, #5
        vaddl.u8 t0, d0, d30
        vext.8 d18, d16, d17, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d16, d17, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d16, d17, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d16, d17, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d16, d17, #5
        vaddl.u8 t1, d16, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d0, t0, #5
        vqrshrun.s16 d16, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        vst1.64 {d0}, [r0,:64], r3
        vst1.64 {d16}, [r0,:64], r3
        bne 1b
        bx lr

        @.endfunc

        @.func avg_h264_qpel16_h_lowpass_neon
avg_h264_qpel16_h_lowpass_neon:
        push {lr}
        mov ip, #16
        bl avg_h264_qpel8_h_lowpass_neon
        sub r0, r0, r3, lsl #4
        sub r1, r1, r2, lsl #4
        add r0, r0, #8
        add r1, r1, #8
        mov ip, #16
        pop {lr}

        @.endfunc


        @.func avg_h264_qpel8_h_lowpass_neon
avg_h264_qpel8_h_lowpass_neon:
1: vld1.64 {d0, d1}, [r1], r2
        vld1.64 {d16,d17}, [r1], r2
        subs ip, ip, #2
        t0 .req q0
        t1 .req q8
        vext.8 d2, d0, d1, #2
        vext.8 d3, d0, d1, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d0, d1, #1
        vext.8 d5, d0, d1, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d0, d1, #5
        vaddl.u8 t0, d0, d30
        vext.8 d18, d16, d17, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d16, d17, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d16, d17, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d16, d17, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d16, d17, #5
        vaddl.u8 t1, d16, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d0, t0, #5
        vqrshrun.s16 d16, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        vld1.8 {d2}, [r0,:64], r3
        vrhadd.u8 d0, d0, d2
        vld1.8 {d3}, [r0,:64]
        vrhadd.u8 d16, d16, d3
        sub r0, r0, r3
        vst1.64 {d0}, [r0,:64], r3
        vst1.64 {d16}, [r0,:64], r3
        bne 1b
        bx lr

        @.endfunc



        @.func put_h264_qpel16_h_lowpass_l2_neon
put_h264_qpel16_h_lowpass_l2_neon:
        push {lr}
        mov ip, #16
        bl put_h264_qpel8_h_lowpass_l2_neon
        sub r0, r0, r2, lsl #4
        sub r1, r1, r2, lsl #4
        sub r3, r3, r2, lsl #4
        add r0, r0, #8
        add r1, r1, #8
        add r3, r3, #8
        mov ip, #16
        pop {lr}

        @.endfunc


        @.func put_h264_qpel8_h_lowpass_l2_neon
put_h264_qpel8_h_lowpass_l2_neon:
1: vld1.64 {d0, d1}, [r1], r2
        vld1.64 {d16,d17}, [r1], r2
        vld1.64 {d28}, [r3], r2
        vld1.64 {d29}, [r3], r2
        subs ip, ip, #2
        t0 .req q0
        t1 .req q8
        vext.8 d2, d0, d1, #2
        vext.8 d3, d0, d1, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d0, d1, #1
        vext.8 d5, d0, d1, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d0, d1, #5
        vaddl.u8 t0, d0, d30
        vext.8 d18, d16, d17, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d16, d17, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d16, d17, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d16, d17, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d16, d17, #5
        vaddl.u8 t1, d16, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d0, t0, #5
        vqrshrun.s16 d1, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        vrhadd.u8 q0, q0, q14
        vst1.64 {d0}, [r0,:64], r2
        vst1.64 {d1}, [r0,:64], r2
        bne 1b
        bx lr

        @.endfunc

        @.func avg_h264_qpel16_h_lowpass_l2_neon
avg_h264_qpel16_h_lowpass_l2_neon:
        push {lr}
        mov ip, #16
        bl avg_h264_qpel8_h_lowpass_l2_neon
        sub r0, r0, r2, lsl #4
        sub r1, r1, r2, lsl #4
        sub r3, r3, r2, lsl #4
        add r0, r0, #8
        add r1, r1, #8
        add r3, r3, #8
        mov ip, #16
        pop {lr}

        @.endfunc


        @.func avg_h264_qpel8_h_lowpass_l2_neon
avg_h264_qpel8_h_lowpass_l2_neon:
1: vld1.64 {d0, d1}, [r1], r2
        vld1.64 {d16,d17}, [r1], r2
        vld1.64 {d28}, [r3], r2
        vld1.64 {d29}, [r3], r2
        subs ip, ip, #2
        t0 .req q0
        t1 .req q8
        vext.8 d2, d0, d1, #2
        vext.8 d3, d0, d1, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d0, d1, #1
        vext.8 d5, d0, d1, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d0, d1, #5
        vaddl.u8 t0, d0, d30
        vext.8 d18, d16, d17, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d16, d17, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d16, d17, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d16, d17, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d16, d17, #5
        vaddl.u8 t1, d16, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d0, t0, #5
        vqrshrun.s16 d1, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        vrhadd.u8 q0, q0, q14
        vld1.8 {d2}, [r0,:64], r2
        vrhadd.u8 d0, d0, d2
        vld1.8 {d3}, [r0,:64]
        vrhadd.u8 d1, d1, d3
        sub r0, r0, r2
        vst1.64 {d0}, [r0,:64], r2
        vst1.64 {d1}, [r0,:64], r2
        bne 1b
        bx lr

        @.endfunc


        @.func put_h264_qpel16_v_lowpass_neon_packed
put_h264_qpel16_v_lowpass_neon_packed:
        mov r4, lr
        mov r2, #8
        bl put_h264_qpel8_v_lowpass_neon
        sub r1, r1, r3, lsl #2
        bl put_h264_qpel8_v_lowpass_neon
        sub r1, r1, r3, lsl #4
        sub r1, r1, r3, lsl #2
        add r1, r1, #8
        bl put_h264_qpel8_v_lowpass_neon
        sub r1, r1, r3, lsl #2
        mov lr, r4
        b put_h264_qpel8_v_lowpass_neon

        @.endfunc



        @.func put_h264_qpel16_v_lowpass_neon
put_h264_qpel16_v_lowpass_neon:
        mov r4, lr
        bl put_h264_qpel8_v_lowpass_neon
        sub r1, r1, r3, lsl #2
        bl put_h264_qpel8_v_lowpass_neon
        sub r0, r0, r2, lsl #4
        add r0, r0, #8
        sub r1, r1, r3, lsl #4
        sub r1, r1, r3, lsl #2
        add r1, r1, #8
        bl put_h264_qpel8_v_lowpass_neon
        sub r1, r1, r3, lsl #2
        mov lr, r4

        @.endfunc


        @.func put_h264_qpel8_v_lowpass_neon
put_h264_qpel8_v_lowpass_neon:
        vld1.64 {d8}, [r1], r3
        vld1.64 {d10}, [r1], r3
        vld1.64 {d12}, [r1], r3
        vld1.64 {d14}, [r1], r3
        vld1.64 {d22}, [r1], r3
        vld1.64 {d24}, [r1], r3
        vld1.64 {d26}, [r1], r3
        vld1.64 {d28}, [r1], r3
        vld1.64 {d9}, [r1], r3
        vld1.64 {d11}, [r1], r3
        vld1.64 {d13}, [r1], r3
        vld1.64 {d15}, [r1], r3
        vld1.64 {d23}, [r1]

        vtrn.32 q4, q11
        vtrn.32 q5, q12
        vtrn.32 q6, q13
        vtrn.32 q7, q14
        vtrn.16 q4, q6
        vtrn.16 q5, q7
        vtrn.16 q11, q13
        vtrn.16 q12, q14
        vtrn.8 q4, q5
        vtrn.8 q6, q7
        vtrn.8 q11, q12
        vtrn.8 q13, q14
        t0 .req q0
        t1 .req q8
        vext.8 d2, d8, d9, #2
        vext.8 d3, d8, d9, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d8, d9, #1
        vext.8 d5, d8, d9, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d8, d9, #5
        vaddl.u8 t0, d8, d30
        vext.8 d18, d10, d11, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d10, d11, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d10, d11, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d10, d11, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d10, d11, #5
        vaddl.u8 t1, d10, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d8, t0, #5
        vqrshrun.s16 d10, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        t0 .req q0
        t1 .req q8
        vext.8 d2, d12, d13, #2
        vext.8 d3, d12, d13, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d12, d13, #1
        vext.8 d5, d12, d13, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d12, d13, #5
        vaddl.u8 t0, d12, d30
        vext.8 d18, d14, d15, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d14, d15, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d14, d15, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d14, d15, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d14, d15, #5
        vaddl.u8 t1, d14, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d12, t0, #5
        vqrshrun.s16 d14, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        t0 .req q0
        t1 .req q8
        vext.8 d2, d22, d23, #2
        vext.8 d3, d22, d23, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d22, d23, #1
        vext.8 d5, d22, d23, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d22, d23, #5
        vaddl.u8 t0, d22, d30
        vext.8 d18, d24, d25, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d24, d25, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d24, d25, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d24, d25, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d24, d25, #5
        vaddl.u8 t1, d24, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d22, t0, #5
        vqrshrun.s16 d24, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        t0 .req q0
        t1 .req q8
        vext.8 d2, d26, d27, #2
        vext.8 d3, d26, d27, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d26, d27, #1
        vext.8 d5, d26, d27, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d26, d27, #5
        vaddl.u8 t0, d26, d30
        vext.8 d18, d28, d29, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d28, d29, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d28, d29, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d28, d29, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d28, d29, #5
        vaddl.u8 t1, d28, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d26, t0, #5
        vqrshrun.s16 d28, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        vtrn.32 d8, d22
        vtrn.32 d10, d24
        vtrn.32 d12, d26
        vtrn.32 d14, d28
        vtrn.16 d8, d12
        vtrn.16 d10, d14
        vtrn.16 d22, d26
        vtrn.16 d24, d28
        vtrn.8 d8, d10
        vtrn.8 d12, d14
        vtrn.8 d22, d24
        vtrn.8 d26, d28


        vst1.64 {d8}, [r0,:64], r2
        vst1.64 {d10}, [r0,:64], r2
        vst1.64 {d12}, [r0,:64], r2
        vst1.64 {d14}, [r0,:64], r2
        vst1.64 {d22}, [r0,:64], r2
        vst1.64 {d24}, [r0,:64], r2
        vst1.64 {d26}, [r0,:64], r2
        vst1.64 {d28}, [r0,:64], r2

        bx lr

        @.endfunc

        @.func avg_h264_qpel16_v_lowpass_neon
avg_h264_qpel16_v_lowpass_neon:
        mov r4, lr
        bl avg_h264_qpel8_v_lowpass_neon
        sub r1, r1, r3, lsl #2
        bl avg_h264_qpel8_v_lowpass_neon
        sub r0, r0, r2, lsl #4
        add r0, r0, #8
        sub r1, r1, r3, lsl #4
        sub r1, r1, r3, lsl #2
        add r1, r1, #8
        bl avg_h264_qpel8_v_lowpass_neon
        sub r1, r1, r3, lsl #2
        mov lr, r4

        @.endfunc


        @.func avg_h264_qpel8_v_lowpass_neon
avg_h264_qpel8_v_lowpass_neon:
        vld1.64 {d8}, [r1], r3
        vld1.64 {d10}, [r1], r3
        vld1.64 {d12}, [r1], r3
        vld1.64 {d14}, [r1], r3
        vld1.64 {d22}, [r1], r3
        vld1.64 {d24}, [r1], r3
        vld1.64 {d26}, [r1], r3
        vld1.64 {d28}, [r1], r3
        vld1.64 {d9}, [r1], r3
        vld1.64 {d11}, [r1], r3
        vld1.64 {d13}, [r1], r3
        vld1.64 {d15}, [r1], r3
        vld1.64 {d23}, [r1]

        vtrn.32 q4, q11
        vtrn.32 q5, q12
        vtrn.32 q6, q13
        vtrn.32 q7, q14
        vtrn.16 q4, q6
        vtrn.16 q5, q7
        vtrn.16 q11, q13
        vtrn.16 q12, q14
        vtrn.8 q4, q5
        vtrn.8 q6, q7
        vtrn.8 q11, q12
        vtrn.8 q13, q14
        t0 .req q0
        t1 .req q8
        vext.8 d2, d8, d9, #2
        vext.8 d3, d8, d9, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d8, d9, #1
        vext.8 d5, d8, d9, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d8, d9, #5
        vaddl.u8 t0, d8, d30
        vext.8 d18, d10, d11, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d10, d11, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d10, d11, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d10, d11, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d10, d11, #5
        vaddl.u8 t1, d10, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d8, t0, #5
        vqrshrun.s16 d10, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        t0 .req q0
        t1 .req q8
        vext.8 d2, d12, d13, #2
        vext.8 d3, d12, d13, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d12, d13, #1
        vext.8 d5, d12, d13, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d12, d13, #5
        vaddl.u8 t0, d12, d30
        vext.8 d18, d14, d15, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d14, d15, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d14, d15, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d14, d15, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d14, d15, #5
        vaddl.u8 t1, d14, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d12, t0, #5
        vqrshrun.s16 d14, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        t0 .req q0
        t1 .req q8
        vext.8 d2, d22, d23, #2
        vext.8 d3, d22, d23, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d22, d23, #1
        vext.8 d5, d22, d23, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d22, d23, #5
        vaddl.u8 t0, d22, d30
        vext.8 d18, d24, d25, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d24, d25, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d24, d25, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d24, d25, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d24, d25, #5
        vaddl.u8 t1, d24, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d22, t0, #5
        vqrshrun.s16 d24, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        t0 .req q0
        t1 .req q8
        vext.8 d2, d26, d27, #2
        vext.8 d3, d26, d27, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d26, d27, #1
        vext.8 d5, d26, d27, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d26, d27, #5
        vaddl.u8 t0, d26, d30
        vext.8 d18, d28, d29, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d28, d29, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d28, d29, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d28, d29, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d28, d29, #5
        vaddl.u8 t1, d28, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d26, t0, #5
        vqrshrun.s16 d28, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        vtrn.32 d8, d22
        vtrn.32 d10, d24
        vtrn.32 d12, d26
        vtrn.32 d14, d28
        vtrn.16 d8, d12
        vtrn.16 d10, d14
        vtrn.16 d22, d26
        vtrn.16 d24, d28
        vtrn.8 d8, d10
        vtrn.8 d12, d14
        vtrn.8 d22, d24
        vtrn.8 d26, d28

        vld1.8 {d9}, [r0,:64], r2
        vrhadd.u8 d8, d8, d9
        vld1.8 {d11}, [r0,:64], r2
        vrhadd.u8 d10, d10, d11
        vld1.8 {d13}, [r0,:64], r2
        vrhadd.u8 d12, d12, d13
        vld1.8 {d15}, [r0,:64], r2
        vrhadd.u8 d14, d14, d15
        vld1.8 {d23}, [r0,:64], r2
        vrhadd.u8 d22, d22, d23
        vld1.8 {d25}, [r0,:64], r2
        vrhadd.u8 d24, d24, d25
        vld1.8 {d27}, [r0,:64], r2
        vrhadd.u8 d26, d26, d27
        vld1.8 {d29}, [r0,:64], r2
        vrhadd.u8 d28, d28, d29
        sub r0, r0, r2, lsl #3

        vst1.64 {d8}, [r0,:64], r2
        vst1.64 {d10}, [r0,:64], r2
        vst1.64 {d12}, [r0,:64], r2
        vst1.64 {d14}, [r0,:64], r2
        vst1.64 {d22}, [r0,:64], r2
        vst1.64 {d24}, [r0,:64], r2
        vst1.64 {d26}, [r0,:64], r2
        vst1.64 {d28}, [r0,:64], r2

        bx lr

        @.endfunc



        @.func put_h264_qpel16_v_lowpass_l2_neon
put_h264_qpel16_v_lowpass_l2_neon:
        mov r4, lr
        bl put_h264_qpel8_v_lowpass_l2_neon
        sub r1, r1, r3, lsl #2
        bl put_h264_qpel8_v_lowpass_l2_neon
        sub r0, r0, r3, lsl #4
        sub ip, ip, r2, lsl #4
        add r0, r0, #8
        add ip, ip, #8
        sub r1, r1, r3, lsl #4
        sub r1, r1, r3, lsl #2
        add r1, r1, #8
        bl put_h264_qpel8_v_lowpass_l2_neon
        sub r1, r1, r3, lsl #2
        mov lr, r4

        @.endfunc


        @.func put_h264_qpel8_v_lowpass_l2_neon
put_h264_qpel8_v_lowpass_l2_neon:
        vld1.64 {d8}, [r1], r3
        vld1.64 {d10}, [r1], r3
        vld1.64 {d12}, [r1], r3
        vld1.64 {d14}, [r1], r3
        vld1.64 {d22}, [r1], r3
        vld1.64 {d24}, [r1], r3
        vld1.64 {d26}, [r1], r3
        vld1.64 {d28}, [r1], r3
        vld1.64 {d9}, [r1], r3
        vld1.64 {d11}, [r1], r3
        vld1.64 {d13}, [r1], r3
        vld1.64 {d15}, [r1], r3
        vld1.64 {d23}, [r1]

        vtrn.32 q4, q11
        vtrn.32 q5, q12
        vtrn.32 q6, q13
        vtrn.32 q7, q14
        vtrn.16 q4, q6
        vtrn.16 q5, q7
        vtrn.16 q11, q13
        vtrn.16 q12, q14
        vtrn.8 q4, q5
        vtrn.8 q6, q7
        vtrn.8 q11, q12
        vtrn.8 q13, q14
        t0 .req q0
        t1 .req q8
        vext.8 d2, d8, d9, #2
        vext.8 d3, d8, d9, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d8, d9, #1
        vext.8 d5, d8, d9, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d8, d9, #5
        vaddl.u8 t0, d8, d30
        vext.8 d18, d10, d11, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d10, d11, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d10, d11, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d10, d11, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d10, d11, #5
        vaddl.u8 t1, d10, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d8, t0, #5
        vqrshrun.s16 d9, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        t0 .req q0
        t1 .req q8
        vext.8 d2, d12, d13, #2
        vext.8 d3, d12, d13, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d12, d13, #1
        vext.8 d5, d12, d13, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d12, d13, #5
        vaddl.u8 t0, d12, d30
        vext.8 d18, d14, d15, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d14, d15, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d14, d15, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d14, d15, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d14, d15, #5
        vaddl.u8 t1, d14, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d12, t0, #5
        vqrshrun.s16 d13, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        t0 .req q0
        t1 .req q8
        vext.8 d2, d22, d23, #2
        vext.8 d3, d22, d23, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d22, d23, #1
        vext.8 d5, d22, d23, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d22, d23, #5
        vaddl.u8 t0, d22, d30
        vext.8 d18, d24, d25, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d24, d25, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d24, d25, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d24, d25, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d24, d25, #5
        vaddl.u8 t1, d24, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d22, t0, #5
        vqrshrun.s16 d23, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        t0 .req q0
        t1 .req q8
        vext.8 d2, d26, d27, #2
        vext.8 d3, d26, d27, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d26, d27, #1
        vext.8 d5, d26, d27, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d26, d27, #5
        vaddl.u8 t0, d26, d30
        vext.8 d18, d28, d29, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d28, d29, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d28, d29, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d28, d29, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d28, d29, #5
        vaddl.u8 t1, d28, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d26, t0, #5
        vqrshrun.s16 d27, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        vtrn.32 d8, d22
        vtrn.32 d9, d23
        vtrn.32 d12, d26
        vtrn.32 d13, d27
        vtrn.16 d8, d12
        vtrn.16 d9, d13
        vtrn.16 d22, d26
        vtrn.16 d23, d27
        vtrn.8 d8, d9
        vtrn.8 d12, d13
        vtrn.8 d22, d23
        vtrn.8 d26, d27

        vld1.64 {d0}, [ip], r2
        vld1.64 {d1}, [ip], r2
        vld1.64 {d2}, [ip], r2
        vld1.64 {d3}, [ip], r2
        vld1.64 {d4}, [ip], r2
        vrhadd.u8 q0, q0, q4
        vld1.64 {d5}, [ip], r2
        vrhadd.u8 q1, q1, q6
        vld1.64 {d10}, [ip], r2
        vrhadd.u8 q2, q2, q11
        vld1.64 {d11}, [ip], r2
        vrhadd.u8 q5, q5, q13


        vst1.64 {d0}, [r0,:64], r3
        vst1.64 {d1}, [r0,:64], r3
        vst1.64 {d2}, [r0,:64], r3
        vst1.64 {d3}, [r0,:64], r3
        vst1.64 {d4}, [r0,:64], r3
        vst1.64 {d5}, [r0,:64], r3
        vst1.64 {d10}, [r0,:64], r3
        vst1.64 {d11}, [r0,:64], r3

        bx lr

        @.endfunc

        @.func avg_h264_qpel16_v_lowpass_l2_neon
avg_h264_qpel16_v_lowpass_l2_neon:
        mov r4, lr
        bl avg_h264_qpel8_v_lowpass_l2_neon
        sub r1, r1, r3, lsl #2
        bl avg_h264_qpel8_v_lowpass_l2_neon
        sub r0, r0, r3, lsl #4
        sub ip, ip, r2, lsl #4
        add r0, r0, #8
        add ip, ip, #8
        sub r1, r1, r3, lsl #4
        sub r1, r1, r3, lsl #2
        add r1, r1, #8
        bl avg_h264_qpel8_v_lowpass_l2_neon
        sub r1, r1, r3, lsl #2
        mov lr, r4

        @.endfunc


        @.func avg_h264_qpel8_v_lowpass_l2_neon
avg_h264_qpel8_v_lowpass_l2_neon:
        vld1.64 {d8}, [r1], r3
        vld1.64 {d10}, [r1], r3
        vld1.64 {d12}, [r1], r3
        vld1.64 {d14}, [r1], r3
        vld1.64 {d22}, [r1], r3
        vld1.64 {d24}, [r1], r3
        vld1.64 {d26}, [r1], r3
        vld1.64 {d28}, [r1], r3
        vld1.64 {d9}, [r1], r3
        vld1.64 {d11}, [r1], r3
        vld1.64 {d13}, [r1], r3
        vld1.64 {d15}, [r1], r3
        vld1.64 {d23}, [r1]

        vtrn.32 q4, q11
        vtrn.32 q5, q12
        vtrn.32 q6, q13
        vtrn.32 q7, q14
        vtrn.16 q4, q6
        vtrn.16 q5, q7
        vtrn.16 q11, q13
        vtrn.16 q12, q14
        vtrn.8 q4, q5
        vtrn.8 q6, q7
        vtrn.8 q11, q12
        vtrn.8 q13, q14
        t0 .req q0
        t1 .req q8
        vext.8 d2, d8, d9, #2
        vext.8 d3, d8, d9, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d8, d9, #1
        vext.8 d5, d8, d9, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d8, d9, #5
        vaddl.u8 t0, d8, d30
        vext.8 d18, d10, d11, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d10, d11, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d10, d11, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d10, d11, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d10, d11, #5
        vaddl.u8 t1, d10, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d8, t0, #5
        vqrshrun.s16 d9, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        t0 .req q0
        t1 .req q8
        vext.8 d2, d12, d13, #2
        vext.8 d3, d12, d13, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d12, d13, #1
        vext.8 d5, d12, d13, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d12, d13, #5
        vaddl.u8 t0, d12, d30
        vext.8 d18, d14, d15, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d14, d15, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d14, d15, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d14, d15, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d14, d15, #5
        vaddl.u8 t1, d14, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d12, t0, #5
        vqrshrun.s16 d13, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        t0 .req q0
        t1 .req q8
        vext.8 d2, d22, d23, #2
        vext.8 d3, d22, d23, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d22, d23, #1
        vext.8 d5, d22, d23, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d22, d23, #5
        vaddl.u8 t0, d22, d30
        vext.8 d18, d24, d25, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d24, d25, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d24, d25, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d24, d25, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d24, d25, #5
        vaddl.u8 t1, d24, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d22, t0, #5
        vqrshrun.s16 d23, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        t0 .req q0
        t1 .req q8
        vext.8 d2, d26, d27, #2
        vext.8 d3, d26, d27, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d26, d27, #1
        vext.8 d5, d26, d27, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d26, d27, #5
        vaddl.u8 t0, d26, d30
        vext.8 d18, d28, d29, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d28, d29, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d28, d29, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d28, d29, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d28, d29, #5
        vaddl.u8 t1, d28, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
        vqrshrun.s16 d26, t0, #5
        vqrshrun.s16 d27, t1, #5
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        vtrn.32 d8, d22
        vtrn.32 d9, d23
        vtrn.32 d12, d26
        vtrn.32 d13, d27
        vtrn.16 d8, d12
        vtrn.16 d9, d13
        vtrn.16 d22, d26
        vtrn.16 d23, d27
        vtrn.8 d8, d9
        vtrn.8 d12, d13
        vtrn.8 d22, d23
        vtrn.8 d26, d27

        vld1.64 {d0}, [ip], r2
        vld1.64 {d1}, [ip], r2
        vld1.64 {d2}, [ip], r2
        vld1.64 {d3}, [ip], r2
        vld1.64 {d4}, [ip], r2
        vrhadd.u8 q0, q0, q4
        vld1.64 {d5}, [ip], r2
        vrhadd.u8 q1, q1, q6
        vld1.64 {d10}, [ip], r2
        vrhadd.u8 q2, q2, q11
        vld1.64 {d11}, [ip], r2
        vrhadd.u8 q5, q5, q13

        vld1.8 {d16}, [r0,:64], r3
        vrhadd.u8 d0, d0, d16
        vld1.8 {d17}, [r0,:64], r3
        vrhadd.u8 d1, d1, d17
        vld1.8 {d16}, [r0,:64], r3
        vrhadd.u8 d2, d2, d16
        vld1.8 {d17}, [r0,:64], r3
        vrhadd.u8 d3, d3, d17
        vld1.8 {d16}, [r0,:64], r3
        vrhadd.u8 d4, d4, d16
        vld1.8 {d17}, [r0,:64], r3
        vrhadd.u8 d5, d5, d17
        vld1.8 {d16}, [r0,:64], r3
        vrhadd.u8 d10, d10, d16
        vld1.8 {d17}, [r0,:64], r3
        vrhadd.u8 d11, d11, d17
        sub r0, r0, r3, lsl #3

        vst1.64 {d0}, [r0,:64], r3
        vst1.64 {d1}, [r0,:64], r3
        vst1.64 {d2}, [r0,:64], r3
        vst1.64 {d3}, [r0,:64], r3
        vst1.64 {d4}, [r0,:64], r3
        vst1.64 {d5}, [r0,:64], r3
        vst1.64 {d10}, [r0,:64], r3
        vst1.64 {d11}, [r0,:64], r3

        bx lr

        @.endfunc


        @.func put_h264_qpel8_hv_lowpass_neon_top
put_h264_qpel8_hv_lowpass_neon_top:
        movw ip, #5
        movt ip, #20
        vmov.32 d6[0], ip
        mov ip, #12
1: vld1.64 {d0, d1}, [r1], r3
        vld1.64 {d16,d17}, [r1], r3
        subs ip, ip, #2
        t0 .req q11
        t1 .req q12
        vext.8 d2, d0, d1, #2
        vext.8 d3, d0, d1, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d0, d1, #1
        vext.8 d5, d0, d1, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d0, d1, #5
        vaddl.u8 t0, d0, d30
        vext.8 d18, d16, d17, #2
        vmla.i16 t0, q1, d6[1]
        vext.8 d19, d16, d17, #3
        vaddl.u8 q9, d18, d19
        vext.8 d20, d16, d17, #1
        vmls.i16 t0, q2, d6[0]
        vext.8 d21, d16, d17, #4
        vaddl.u8 q10, d20, d21
        vext.8 d31, d16, d17, #5
        vaddl.u8 t1, d16, d31
        vmla.i16 t1, q9, d6[1]
        vmls.i16 t1, q10, d6[0]
.unreq T0
.unreq t0
.unreq T1
.unreq t1
        vst1.64 {d22-d25}, [r4,:128]!
        bne 1b

        vld1.64 {d0, d1}, [r1]
        t0 .req q12
        vext.8 d2, d0, d1, #2
        vext.8 d3, d0, d1, #3
        vaddl.u8 q1, d2, d3
        vext.8 d4, d0, d1, #1
        vext.8 d5, d0, d1, #4
        vaddl.u8 q2, d4, d5
        vext.8 d30, d0, d1, #5
        vaddl.u8 t0, d0, d30
        vmla.i16 t0, q1, d6[1]
        vmls.i16 t0, q2, d6[0]
.unreq T0
.unreq t0

        mov ip, #-16
        add r4, r4, ip
        vld1.64 {d30,d31}, [r4,:128], ip
        vld1.64 {d20,d21}, [r4,:128], ip
        vld1.64 {d18,d19}, [r4,:128], ip
        vld1.64 {d16,d17}, [r4,:128], ip
        vld1.64 {d14,d15}, [r4,:128], ip
        vld1.64 {d12,d13}, [r4,:128], ip
        vld1.64 {d10,d11}, [r4,:128], ip
        vld1.64 {d8, d9}, [r4,:128], ip
        vld1.64 {d6, d7}, [r4,:128], ip
        vld1.64 {d4, d5}, [r4,:128], ip
        vld1.64 {d2, d3}, [r4,:128], ip
        vld1.64 {d0, d1}, [r4,:128]

        vswp d1, d8
        vswp d3, d10
        vswp d5, d12
        vswp d7, d14
        vtrn.32 q0, q2
        vtrn.32 q1, q3
        vtrn.32 q4, q6
        vtrn.32 q5, q7
        vtrn.16 q0, q1
        vtrn.16 q2, q3
        vtrn.16 q4, q5
        vtrn.16 q6, q7

        vswp d17, d24
        vswp d19, d26
        vswp d21, d28
        vswp d31, d22
        vtrn.32 q8, q10
        vtrn.32 q9, q15
        vtrn.32 q12, q14
        vtrn.32 q13, q11
        vtrn.16 q8, q9
        vtrn.16 q10, q15
        vtrn.16 q12, q13
        vtrn.16 q14, q11

        vst1.64 {d30,d31}, [r4,:128]!
        vst1.64 {d6, d7}, [r4,:128]!
        vst1.64 {d20,d21}, [r4,:128]!
        vst1.64 {d4, d5}, [r4,:128]!
        vst1.64 {d18,d19}, [r4,:128]!
        vst1.64 {d2, d3}, [r4,:128]!
        vst1.64 {d16,d17}, [r4,:128]!
        vst1.64 {d0, d1}, [r4,:128]

        vext.16 q1, q4, q12, #2
        vext.16 q0, q4, q12, #3
        vaddl.s16 q9, d2, d0
        vext.16 q2, q4, q12, #1
        vaddl.s16 q1, d3, d1
        vext.16 q3, q4, q12, #4
        vaddl.s16 q10, d4, d6
        vext.16 q12, q4, q12, #5
        vaddl.s16 q2, d5, d7
        vaddl.s16 q0, d9, d25
        vaddl.s16 q8, d8, d24

        vshl.i32 q3, q9, #4
        vshl.i32 q9, q9, #2
        vshl.i32 q15, q10, #2
        vadd.i32 q9, q9, q3
        vadd.i32 q10, q10, q15

        vshl.i32 q3, q1, #4
        vshl.i32 q1, q1, #2
        vshl.i32 q15, q2, #2
        vadd.i32 q1, q1, q3
        vadd.i32 q2, q2, q15

        vadd.i32 q9, q9, q8
        vsub.i32 q9, q9, q10

        vadd.i32 q1, q1, q0
        vsub.i32 q1, q1, q2

        vrshrn.s32 d18, q9, #10
        vrshrn.s32 d19, q1, #10

        vqmovun.s16 d8, q9
        vext.16 q1, q5, q13, #2
        vext.16 q0, q5, q13, #3
        vaddl.s16 q9, d2, d0
        vext.16 q2, q5, q13, #1
        vaddl.s16 q1, d3, d1
        vext.16 q3, q5, q13, #4
        vaddl.s16 q10, d4, d6
        vext.16 q13, q5, q13, #5
        vaddl.s16 q2, d5, d7
        vaddl.s16 q0, d11, d27
        vaddl.s16 q8, d10, d26

        vshl.i32 q3, q9, #4
        vshl.i32 q9, q9, #2
        vshl.i32 q15, q10, #2
        vadd.i32 q9, q9, q3
        vadd.i32 q10, q10, q15

        vshl.i32 q3, q1, #4
        vshl.i32 q1, q1, #2
        vshl.i32 q15, q2, #2
        vadd.i32 q1, q1, q3
        vadd.i32 q2, q2, q15

        vadd.i32 q9, q9, q8
        vsub.i32 q9, q9, q10

        vadd.i32 q1, q1, q0
        vsub.i32 q1, q1, q2

        vrshrn.s32 d18, q9, #10
        vrshrn.s32 d19, q1, #10

        vqmovun.s16 d9, q9
        vext.16 q1, q6, q14, #2
        vext.16 q0, q6, q14, #3
        vaddl.s16 q9, d2, d0
        vext.16 q2, q6, q14, #1
        vaddl.s16 q1, d3, d1
        vext.16 q3, q6, q14, #4
        vaddl.s16 q10, d4, d6
        vext.16 q14, q6, q14, #5
        vaddl.s16 q2, d5, d7
        vaddl.s16 q0, d13, d29
        vaddl.s16 q8, d12, d28

        vshl.i32 q3, q9, #4
        vshl.i32 q9, q9, #2
        vshl.i32 q15, q10, #2
        vadd.i32 q9, q9, q3
        vadd.i32 q10, q10, q15

        vshl.i32 q3, q1, #4
        vshl.i32 q1, q1, #2
        vshl.i32 q15, q2, #2
        vadd.i32 q1, q1, q3
        vadd.i32 q2, q2, q15

        vadd.i32 q9, q9, q8
        vsub.i32 q9, q9, q10

        vadd.i32 q1, q1, q0
        vsub.i32 q1, q1, q2

        vrshrn.s32 d18, q9, #10
        vrshrn.s32 d19, q1, #10

        vqmovun.s16 d10, q9
        vext.16 q1, q7, q11, #2
        vext.16 q0, q7, q11, #3
        vaddl.s16 q9, d2, d0
        vext.16 q2, q7, q11, #1
        vaddl.s16 q1, d3, d1
        vext.16 q3, q7, q11, #4
        vaddl.s16 q10, d4, d6
        vext.16 q11, q7, q11, #5
        vaddl.s16 q2, d5, d7
        vaddl.s16 q0, d15, d23
        vaddl.s16 q8, d14, d22

        vshl.i32 q3, q9, #4
        vshl.i32 q9, q9, #2
        vshl.i32 q15, q10, #2
        vadd.i32 q9, q9, q3
        vadd.i32 q10, q10, q15

        vshl.i32 q3, q1, #4
        vshl.i32 q1, q1, #2
        vshl.i32 q15, q2, #2
        vadd.i32 q1, q1, q3
        vadd.i32 q2, q2, q15

        vadd.i32 q9, q9, q8
        vsub.i32 q9, q9, q10

        vadd.i32 q1, q1, q0
        vsub.i32 q1, q1, q2

        vrshrn.s32 d18, q9, #10
        vrshrn.s32 d19, q1, #10

        vqmovun.s16 d11, q9

        vld1.64 {d16,d17}, [r4,:128], ip
        vld1.64 {d30,d31}, [r4,:128], ip
        vext.16 q1, q8, q15, #2
        vext.16 q0, q8, q15, #3
        vaddl.s16 q9, d2, d0
        vext.16 q2, q8, q15, #1
        vaddl.s16 q1, d3, d1
        vext.16 q3, q8, q15, #4
        vaddl.s16 q10, d4, d6
        vext.16 q15, q8, q15, #5
        vaddl.s16 q2, d5, d7
        vaddl.s16 q0, d17, d31
        vaddl.s16 q8, d16, d30

        vshl.i32 q3, q9, #4
        vshl.i32 q9, q9, #2
        vshl.i32 q15, q10, #2
        vadd.i32 q9, q9, q3
        vadd.i32 q10, q10, q15

        vshl.i32 q3, q1, #4
        vshl.i32 q1, q1, #2
        vshl.i32 q15, q2, #2
        vadd.i32 q1, q1, q3
        vadd.i32 q2, q2, q15

        vadd.i32 q9, q9, q8
        vsub.i32 q9, q9, q10

        vadd.i32 q1, q1, q0
        vsub.i32 q1, q1, q2

        vrshrn.s32 d18, q9, #10
        vrshrn.s32 d19, q1, #10

        vqmovun.s16 d12, q9
        vld1.64 {d16,d17}, [r4,:128], ip
        vld1.64 {d30,d31}, [r4,:128], ip
        vext.16 q1, q8, q15, #2
        vext.16 q0, q8, q15, #3
        vaddl.s16 q9, d2, d0
        vext.16 q2, q8, q15, #1
        vaddl.s16 q1, d3, d1
        vext.16 q3, q8, q15, #4
        vaddl.s16 q10, d4, d6
        vext.16 q15, q8, q15, #5
        vaddl.s16 q2, d5, d7
        vaddl.s16 q0, d17, d31
        vaddl.s16 q8, d16, d30

        vshl.i32 q3, q9, #4
        vshl.i32 q9, q9, #2
        vshl.i32 q15, q10, #2
        vadd.i32 q9, q9, q3
        vadd.i32 q10, q10, q15

        vshl.i32 q3, q1, #4
        vshl.i32 q1, q1, #2
        vshl.i32 q15, q2, #2
        vadd.i32 q1, q1, q3
        vadd.i32 q2, q2, q15

        vadd.i32 q9, q9, q8
        vsub.i32 q9, q9, q10

        vadd.i32 q1, q1, q0
        vsub.i32 q1, q1, q2

        vrshrn.s32 d18, q9, #10
        vrshrn.s32 d19, q1, #10

        vqmovun.s16 d13, q9
        vld1.64 {d16,d17}, [r4,:128], ip
        vld1.64 {d30,d31}, [r4,:128], ip
        vext.16 q1, q8, q15, #2
        vext.16 q0, q8, q15, #3
        vaddl.s16 q9, d2, d0
        vext.16 q2, q8, q15, #1
        vaddl.s16 q1, d3, d1
        vext.16 q3, q8, q15, #4
        vaddl.s16 q10, d4, d6
        vext.16 q15, q8, q15, #5
        vaddl.s16 q2, d5, d7
        vaddl.s16 q0, d17, d31
        vaddl.s16 q8, d16, d30

        vshl.i32 q3, q9, #4
        vshl.i32 q9, q9, #2
        vshl.i32 q15, q10, #2
        vadd.i32 q9, q9, q3
        vadd.i32 q10, q10, q15

        vshl.i32 q3, q1, #4
        vshl.i32 q1, q1, #2
        vshl.i32 q15, q2, #2
        vadd.i32 q1, q1, q3
        vadd.i32 q2, q2, q15

        vadd.i32 q9, q9, q8
        vsub.i32 q9, q9, q10

        vadd.i32 q1, q1, q0
        vsub.i32 q1, q1, q2

        vrshrn.s32 d18, q9, #10
        vrshrn.s32 d19, q1, #10

        vqmovun.s16 d14, q9
        vld1.64 {d16,d17}, [r4,:128], ip
        vld1.64 {d30,d31}, [r4,:128]
        vext.16 q1, q8, q15, #2
        vext.16 q0, q8, q15, #3
        vaddl.s16 q9, d2, d0
        vext.16 q2, q8, q15, #1
        vaddl.s16 q1, d3, d1
        vext.16 q3, q8, q15, #4
        vaddl.s16 q10, d4, d6
        vext.16 q15, q8, q15, #5
        vaddl.s16 q2, d5, d7
        vaddl.s16 q0, d17, d31
        vaddl.s16 q8, d16, d30

        vshl.i32 q3, q9, #4
        vshl.i32 q9, q9, #2
        vshl.i32 q15, q10, #2
        vadd.i32 q9, q9, q3
        vadd.i32 q10, q10, q15

        vshl.i32 q3, q1, #4
        vshl.i32 q1, q1, #2
        vshl.i32 q15, q2, #2
        vadd.i32 q1, q1, q3
        vadd.i32 q2, q2, q15

        vadd.i32 q9, q9, q8
        vsub.i32 q9, q9, q10

        vadd.i32 q1, q1, q0
        vsub.i32 q1, q1, q2

        vrshrn.s32 d18, q9, #10
        vrshrn.s32 d19, q1, #10

        vqmovun.s16 d15, q9

        vtrn.32 d12, d8
        vtrn.32 d13, d9
        vtrn.32 d14, d10
        vtrn.32 d15, d11
        vtrn.16 d12, d14
        vtrn.16 d13, d15
        vtrn.16 d8, d10
        vtrn.16 d9, d11
        vtrn.8 d12, d13
        vtrn.8 d14, d15
        vtrn.8 d8, d9
        vtrn.8 d10, d11

        bx lr

        @.endfunc



        @.func put_h264_qpel8_hv_lowpass_neon
put_h264_qpel8_hv_lowpass_neon:
        mov r10, lr
        bl put_h264_qpel8_hv_lowpass_neon_top
        vst1.64 {d12}, [r0,:64], r2
        vst1.64 {d13}, [r0,:64], r2
        vst1.64 {d14}, [r0,:64], r2
        vst1.64 {d15}, [r0,:64], r2
        vst1.64 {d8}, [r0,:64], r2
        vst1.64 {d9}, [r0,:64], r2
        vst1.64 {d10}, [r0,:64], r2
        vst1.64 {d11}, [r0,:64], r2

        mov lr, r10
        bx lr

        @.endfunc

        @.func avg_h264_qpel8_hv_lowpass_neon
avg_h264_qpel8_hv_lowpass_neon:
        mov r10, lr
        bl put_h264_qpel8_hv_lowpass_neon_top
        vld1.8 {d0}, [r0,:64], r2
        vrhadd.u8 d12, d12, d0
        vld1.8 {d1}, [r0,:64], r2
        vrhadd.u8 d13, d13, d1
        vld1.8 {d2}, [r0,:64], r2
        vrhadd.u8 d14, d14, d2
        vld1.8 {d3}, [r0,:64], r2
        vrhadd.u8 d15, d15, d3
        vld1.8 {d4}, [r0,:64], r2
        vrhadd.u8 d8, d8, d4
        vld1.8 {d5}, [r0,:64], r2
        vrhadd.u8 d9, d9, d5
        vld1.8 {d6}, [r0,:64], r2
        vrhadd.u8 d10, d10, d6
        vld1.8 {d7}, [r0,:64], r2
        vrhadd.u8 d11, d11, d7
        sub r0, r0, r2, lsl #3
        vst1.64 {d12}, [r0,:64], r2
        vst1.64 {d13}, [r0,:64], r2
        vst1.64 {d14}, [r0,:64], r2
        vst1.64 {d15}, [r0,:64], r2
        vst1.64 {d8}, [r0,:64], r2
        vst1.64 {d9}, [r0,:64], r2
        vst1.64 {d10}, [r0,:64], r2
        vst1.64 {d11}, [r0,:64], r2

        mov lr, r10
        bx lr

        @.endfunc



        @.func put_h264_qpel8_hv_lowpass_l2_neon
put_h264_qpel8_hv_lowpass_l2_neon:
        mov r10, lr
        bl put_h264_qpel8_hv_lowpass_neon_top

        vld1.64 {d0, d1}, [r2,:128]!
        vld1.64 {d2, d3}, [r2,:128]!
        vrhadd.u8 q0, q0, q6
        vld1.64 {d4, d5}, [r2,:128]!
        vrhadd.u8 q1, q1, q7
        vld1.64 {d6, d7}, [r2,:128]!
        vrhadd.u8 q2, q2, q4
        vrhadd.u8 q3, q3, q5
        vst1.64 {d0}, [r0,:64], r3
        vst1.64 {d1}, [r0,:64], r3
        vst1.64 {d2}, [r0,:64], r3
        vst1.64 {d3}, [r0,:64], r3
        vst1.64 {d4}, [r0,:64], r3
        vst1.64 {d5}, [r0,:64], r3
        vst1.64 {d6}, [r0,:64], r3
        vst1.64 {d7}, [r0,:64], r3

        mov lr, r10
        bx lr

        @.endfunc

        @.func avg_h264_qpel8_hv_lowpass_l2_neon
avg_h264_qpel8_hv_lowpass_l2_neon:
        mov r10, lr
        bl put_h264_qpel8_hv_lowpass_neon_top

        vld1.64 {d0, d1}, [r2,:128]!
        vld1.64 {d2, d3}, [r2,:128]!
        vrhadd.u8 q0, q0, q6
        vld1.64 {d4, d5}, [r2,:128]!
        vrhadd.u8 q1, q1, q7
        vld1.64 {d6, d7}, [r2,:128]!
        vrhadd.u8 q2, q2, q4
        vrhadd.u8 q3, q3, q5
        vld1.8 {d16}, [r0,:64], r3
        vrhadd.u8 d0, d0, d16
        vld1.8 {d17}, [r0,:64], r3
        vrhadd.u8 d1, d1, d17
        vld1.8 {d18}, [r0,:64], r3
        vrhadd.u8 d2, d2, d18
        vld1.8 {d19}, [r0,:64], r3
        vrhadd.u8 d3, d3, d19
        vld1.8 {d20}, [r0,:64], r3
        vrhadd.u8 d4, d4, d20
        vld1.8 {d21}, [r0,:64], r3
        vrhadd.u8 d5, d5, d21
        vld1.8 {d22}, [r0,:64], r3
        vrhadd.u8 d6, d6, d22
        vld1.8 {d23}, [r0,:64], r3
        vrhadd.u8 d7, d7, d23
        sub r0, r0, r3, lsl #3
        vst1.64 {d0}, [r0,:64], r3
        vst1.64 {d1}, [r0,:64], r3
        vst1.64 {d2}, [r0,:64], r3
        vst1.64 {d3}, [r0,:64], r3
        vst1.64 {d4}, [r0,:64], r3
        vst1.64 {d5}, [r0,:64], r3
        vst1.64 {d6}, [r0,:64], r3
        vst1.64 {d7}, [r0,:64], r3

        mov lr, r10
        bx lr

        @.endfunc



        @.func put_h264_qpel16_hv_lowpass_neon
put_h264_qpel16_hv_lowpass_neon:
        mov r9, lr
        bl put_h264_qpel8_hv_lowpass_neon
        sub r1, r1, r3, lsl #2
        bl put_h264_qpel8_hv_lowpass_neon
        sub r1, r1, r3, lsl #4
        sub r1, r1, r3, lsl #2
        add r1, r1, #8
        sub r0, r0, r2, lsl #4
        add r0, r0, #8
        bl put_h264_qpel8_hv_lowpass_neon
        sub r1, r1, r3, lsl #2
        mov lr, r9
        b put_h264_qpel8_hv_lowpass_neon

        @.endfunc


        @.func put_h264_qpel16_hv_lowpass_l2_neon
put_h264_qpel16_hv_lowpass_l2_neon:
        mov r9, lr
        sub r2, r4, #256
        bl put_h264_qpel8_hv_lowpass_l2_neon
        sub r1, r1, r3, lsl #2
        bl put_h264_qpel8_hv_lowpass_l2_neon
        sub r1, r1, r3, lsl #4
        sub r1, r1, r3, lsl #2
        add r1, r1, #8
        sub r0, r0, r3, lsl #4
        add r0, r0, #8
        bl put_h264_qpel8_hv_lowpass_l2_neon
        sub r1, r1, r3, lsl #2
        mov lr, r9
        b put_h264_qpel8_hv_lowpass_l2_neon

        @.endfunc

        @.func avg_h264_qpel16_hv_lowpass_neon
avg_h264_qpel16_hv_lowpass_neon:
        mov r9, lr
        bl avg_h264_qpel8_hv_lowpass_neon
        sub r1, r1, r3, lsl #2
        bl avg_h264_qpel8_hv_lowpass_neon
        sub r1, r1, r3, lsl #4
        sub r1, r1, r3, lsl #2
        add r1, r1, #8
        sub r0, r0, r2, lsl #4
        add r0, r0, #8
        bl avg_h264_qpel8_hv_lowpass_neon
        sub r1, r1, r3, lsl #2
        mov lr, r9
        b avg_h264_qpel8_hv_lowpass_neon

        @.endfunc


        @.func avg_h264_qpel16_hv_lowpass_l2_neon
avg_h264_qpel16_hv_lowpass_l2_neon:
        mov r9, lr
        sub r2, r4, #256
        bl avg_h264_qpel8_hv_lowpass_l2_neon
        sub r1, r1, r3, lsl #2
        bl avg_h264_qpel8_hv_lowpass_l2_neon
        sub r1, r1, r3, lsl #4
        sub r1, r1, r3, lsl #2
        add r1, r1, #8
        sub r0, r0, r3, lsl #4
        add r0, r0, #8
        bl avg_h264_qpel8_hv_lowpass_l2_neon
        sub r1, r1, r3, lsl #2
        mov lr, r9
        b avg_h264_qpel8_hv_lowpass_l2_neon

        @.endfunc


        .globl _ff_put_h264_qpel8_mc10_neon

        @.func _ff_put_h264_qpel8_mc10_neon
_ff_put_h264_qpel8_mc10_neon:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r3, r1
        sub r1, r1, #2
        mov ip, #8
        b put_h264_qpel8_h_lowpass_l2_neon

        @.endfunc

        .globl _ff_put_h264_qpel8_mc20_neon

        @.func _ff_put_h264_qpel8_mc20_neon
_ff_put_h264_qpel8_mc20_neon:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        sub r1, r1, #2
        mov r3, r2
        mov ip, #8
        b put_h264_qpel8_h_lowpass_neon

        @.endfunc

        .globl _ff_put_h264_qpel8_mc30_neon

        @.func _ff_put_h264_qpel8_mc30_neon
_ff_put_h264_qpel8_mc30_neon:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        add r3, r1, #1
        sub r1, r1, #2
        mov ip, #8
        b put_h264_qpel8_h_lowpass_l2_neon

        @.endfunc

        .globl _ff_put_h264_qpel8_mc01_neon

        @.func _ff_put_h264_qpel8_mc01_neon
_ff_put_h264_qpel8_mc01_neon:
        push {lr}
        mov ip, r1
put_h264_qpel8_mc01:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r3, r2
        sub r1, r1, r2, lsl #1
        vpush {d8-d15}
        bl put_h264_qpel8_v_lowpass_l2_neon
        vpop {d8-d15}
        pop {pc}

        @.endfunc

        .globl _ff_put_h264_qpel8_mc11_neon

        @.func _ff_put_h264_qpel8_mc11_neon
_ff_put_h264_qpel8_mc11_neon:
        push {r0, r1, r11, lr}
put_h264_qpel8_mc11:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub sp, sp, #64
        mov r0, sp
        sub r1, r1, #2
        mov r3, #8
        mov ip, #8
        vpush {d8-d15}
        bl put_h264_qpel8_h_lowpass_neon
        ldrd r0, [r11]
        mov r3, r2
        add ip, sp, #64
        sub r1, r1, r2, lsl #1
        mov r2, #8
        bl put_h264_qpel8_v_lowpass_l2_neon
        vpop {d8-d15}
        add sp, r11, #8
        pop {r11, pc}

        @.endfunc

        .globl _ff_put_h264_qpel8_mc21_neon

        @.func _ff_put_h264_qpel8_mc21_neon
_ff_put_h264_qpel8_mc21_neon:
        push {r0, r1, r4, r10, r11, lr}
put_h264_qpel8_mc21:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub sp, sp, #(8*8+16*12)
        sub r1, r1, #2
        mov r3, #8
        mov r0, sp
        mov ip, #8
        vpush {d8-d15}
        bl put_h264_qpel8_h_lowpass_neon
        mov r4, r0
        ldrd r0, [r11]
        sub r1, r1, r2, lsl #1
        sub r1, r1, #2
        mov r3, r2
        sub r2, r4, #64
        bl put_h264_qpel8_hv_lowpass_l2_neon
        vpop {d8-d15}
        add sp, r11, #8
        pop {r4, r10, r11, pc}

        @.endfunc

        .globl _ff_put_h264_qpel8_mc31_neon

        @.func _ff_put_h264_qpel8_mc31_neon
_ff_put_h264_qpel8_mc31_neon:
        add r1, r1, #1
        push {r0, r1, r11, lr}
        sub r1, r1, #1
        b put_h264_qpel8_mc11

        @.endfunc

        .globl _ff_put_h264_qpel8_mc02_neon

        @.func _ff_put_h264_qpel8_mc02_neon
_ff_put_h264_qpel8_mc02_neon:
        push {lr}
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        sub r1, r1, r2, lsl #1
        mov r3, r2
        vpush {d8-d15}
        bl put_h264_qpel8_v_lowpass_neon
        vpop {d8-d15}
        pop {pc}

        @.endfunc

        .globl _ff_put_h264_qpel8_mc12_neon

        @.func _ff_put_h264_qpel8_mc12_neon
_ff_put_h264_qpel8_mc12_neon:
        push {r0, r1, r4, r10, r11, lr}
put_h264_qpel8_mc12:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub sp, sp, #(8*8+16*12)
        sub r1, r1, r2, lsl #1
        mov r3, r2
        mov r2, #8
        mov r0, sp
        vpush {d8-d15}
        bl put_h264_qpel8_v_lowpass_neon
        mov r4, r0
        ldrd r0, [r11]
        sub r1, r1, r3, lsl #1
        sub r1, r1, #2
        sub r2, r4, #64
        bl put_h264_qpel8_hv_lowpass_l2_neon
        vpop {d8-d15}
        add sp, r11, #8
        pop {r4, r10, r11, pc}

        @.endfunc

        .globl _ff_put_h264_qpel8_mc22_neon

        @.func _ff_put_h264_qpel8_mc22_neon
_ff_put_h264_qpel8_mc22_neon:
        push {r4, r10, r11, lr}
        mov r11, sp
        bic sp, sp, #15
        sub r1, r1, r2, lsl #1
        sub r1, r1, #2
        mov r3, r2
        sub sp, sp, #(16*12)
        mov r4, sp
        vpush {d8-d15}
        bl put_h264_qpel8_hv_lowpass_neon
        vpop {d8-d15}
        mov sp, r11
        pop {r4, r10, r11, pc}

        @.endfunc

        .globl _ff_put_h264_qpel8_mc32_neon

        @.func _ff_put_h264_qpel8_mc32_neon
_ff_put_h264_qpel8_mc32_neon:
        push {r0, r1, r4, r10, r11, lr}
        add r1, r1, #1
        b put_h264_qpel8_mc12

        @.endfunc

        .globl _ff_put_h264_qpel8_mc03_neon

        @.func _ff_put_h264_qpel8_mc03_neon
_ff_put_h264_qpel8_mc03_neon:
        push {lr}
        add ip, r1, r2
        b put_h264_qpel8_mc01

        @.endfunc

        .globl _ff_put_h264_qpel8_mc13_neon

        @.func _ff_put_h264_qpel8_mc13_neon
_ff_put_h264_qpel8_mc13_neon:
        push {r0, r1, r11, lr}
        add r1, r1, r2
        b put_h264_qpel8_mc11

        @.endfunc

        .globl _ff_put_h264_qpel8_mc23_neon

        @.func _ff_put_h264_qpel8_mc23_neon
_ff_put_h264_qpel8_mc23_neon:
        push {r0, r1, r4, r10, r11, lr}
        add r1, r1, r2
        b put_h264_qpel8_mc21

        @.endfunc

        .globl _ff_put_h264_qpel8_mc33_neon

        @.func _ff_put_h264_qpel8_mc33_neon
_ff_put_h264_qpel8_mc33_neon:
        add r1, r1, #1
        push {r0, r1, r11, lr}
        add r1, r1, r2
        sub r1, r1, #1
        b put_h264_qpel8_mc11

        @.endfunc
        .globl _ff_avg_h264_qpel8_mc10_neon

        @.func _ff_avg_h264_qpel8_mc10_neon
_ff_avg_h264_qpel8_mc10_neon:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r3, r1
        sub r1, r1, #2
        mov ip, #8
        b avg_h264_qpel8_h_lowpass_l2_neon

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc20_neon

        @.func _ff_avg_h264_qpel8_mc20_neon
_ff_avg_h264_qpel8_mc20_neon:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        sub r1, r1, #2
        mov r3, r2
        mov ip, #8
        b avg_h264_qpel8_h_lowpass_neon

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc30_neon

        @.func _ff_avg_h264_qpel8_mc30_neon
_ff_avg_h264_qpel8_mc30_neon:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        add r3, r1, #1
        sub r1, r1, #2
        mov ip, #8
        b avg_h264_qpel8_h_lowpass_l2_neon

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc01_neon

        @.func _ff_avg_h264_qpel8_mc01_neon
_ff_avg_h264_qpel8_mc01_neon:
        push {lr}
        mov ip, r1
avg_h264_qpel8_mc01:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r3, r2
        sub r1, r1, r2, lsl #1
        vpush {d8-d15}
        bl avg_h264_qpel8_v_lowpass_l2_neon
        vpop {d8-d15}
        pop {pc}

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc11_neon

        @.func _ff_avg_h264_qpel8_mc11_neon
_ff_avg_h264_qpel8_mc11_neon:
        push {r0, r1, r11, lr}
avg_h264_qpel8_mc11:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub sp, sp, #64
        mov r0, sp
        sub r1, r1, #2
        mov r3, #8
        mov ip, #8
        vpush {d8-d15}
        bl put_h264_qpel8_h_lowpass_neon
        ldrd r0, [r11]
        mov r3, r2
        add ip, sp, #64
        sub r1, r1, r2, lsl #1
        mov r2, #8
        bl avg_h264_qpel8_v_lowpass_l2_neon
        vpop {d8-d15}
        add sp, r11, #8
        pop {r11, pc}

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc21_neon

        @.func _ff_avg_h264_qpel8_mc21_neon
_ff_avg_h264_qpel8_mc21_neon:
        push {r0, r1, r4, r10, r11, lr}
avg_h264_qpel8_mc21:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub sp, sp, #(8*8+16*12)
        sub r1, r1, #2
        mov r3, #8
        mov r0, sp
        mov ip, #8
        vpush {d8-d15}
        bl put_h264_qpel8_h_lowpass_neon
        mov r4, r0
        ldrd r0, [r11]
        sub r1, r1, r2, lsl #1
        sub r1, r1, #2
        mov r3, r2
        sub r2, r4, #64
        bl avg_h264_qpel8_hv_lowpass_l2_neon
        vpop {d8-d15}
        add sp, r11, #8
        pop {r4, r10, r11, pc}

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc31_neon

        @.func _ff_avg_h264_qpel8_mc31_neon
_ff_avg_h264_qpel8_mc31_neon:
        add r1, r1, #1
        push {r0, r1, r11, lr}
        sub r1, r1, #1
        b avg_h264_qpel8_mc11

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc02_neon

        @.func _ff_avg_h264_qpel8_mc02_neon
_ff_avg_h264_qpel8_mc02_neon:
        push {lr}
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        sub r1, r1, r2, lsl #1
        mov r3, r2
        vpush {d8-d15}
        bl avg_h264_qpel8_v_lowpass_neon
        vpop {d8-d15}
        pop {pc}

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc12_neon

        @.func _ff_avg_h264_qpel8_mc12_neon
_ff_avg_h264_qpel8_mc12_neon:
        push {r0, r1, r4, r10, r11, lr}
avg_h264_qpel8_mc12:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub sp, sp, #(8*8+16*12)
        sub r1, r1, r2, lsl #1
        mov r3, r2
        mov r2, #8
        mov r0, sp
        vpush {d8-d15}
        bl put_h264_qpel8_v_lowpass_neon
        mov r4, r0
        ldrd r0, [r11]
        sub r1, r1, r3, lsl #1
        sub r1, r1, #2
        sub r2, r4, #64
        bl avg_h264_qpel8_hv_lowpass_l2_neon
        vpop {d8-d15}
        add sp, r11, #8
        pop {r4, r10, r11, pc}

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc22_neon

        @.func _ff_avg_h264_qpel8_mc22_neon
_ff_avg_h264_qpel8_mc22_neon:
        push {r4, r10, r11, lr}
        mov r11, sp
        bic sp, sp, #15
        sub r1, r1, r2, lsl #1
        sub r1, r1, #2
        mov r3, r2
        sub sp, sp, #(16*12)
        mov r4, sp
        vpush {d8-d15}
        bl avg_h264_qpel8_hv_lowpass_neon
        vpop {d8-d15}
        mov sp, r11
        pop {r4, r10, r11, pc}

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc32_neon

        @.func _ff_avg_h264_qpel8_mc32_neon
_ff_avg_h264_qpel8_mc32_neon:
        push {r0, r1, r4, r10, r11, lr}
        add r1, r1, #1
        b avg_h264_qpel8_mc12

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc03_neon

        @.func _ff_avg_h264_qpel8_mc03_neon
_ff_avg_h264_qpel8_mc03_neon:
        push {lr}
        add ip, r1, r2
        b avg_h264_qpel8_mc01

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc13_neon

        @.func _ff_avg_h264_qpel8_mc13_neon
_ff_avg_h264_qpel8_mc13_neon:
        push {r0, r1, r11, lr}
        add r1, r1, r2
        b avg_h264_qpel8_mc11

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc23_neon

        @.func _ff_avg_h264_qpel8_mc23_neon
_ff_avg_h264_qpel8_mc23_neon:
        push {r0, r1, r4, r10, r11, lr}
        add r1, r1, r2
        b avg_h264_qpel8_mc21

        @.endfunc

        .globl _ff_avg_h264_qpel8_mc33_neon

        @.func _ff_avg_h264_qpel8_mc33_neon
_ff_avg_h264_qpel8_mc33_neon:
        add r1, r1, #1
        push {r0, r1, r11, lr}
        add r1, r1, r2
        sub r1, r1, #1
        b avg_h264_qpel8_mc11

        @.endfunc


        .globl _ff_put_h264_qpel16_mc10_neon

        @.func _ff_put_h264_qpel16_mc10_neon
_ff_put_h264_qpel16_mc10_neon:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r3, r1
        sub r1, r1, #2
        b put_h264_qpel16_h_lowpass_l2_neon

        @.endfunc

        .globl _ff_put_h264_qpel16_mc20_neon

        @.func _ff_put_h264_qpel16_mc20_neon
_ff_put_h264_qpel16_mc20_neon:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        sub r1, r1, #2
        mov r3, r2
        b put_h264_qpel16_h_lowpass_neon

        @.endfunc

        .globl _ff_put_h264_qpel16_mc30_neon

        @.func _ff_put_h264_qpel16_mc30_neon
_ff_put_h264_qpel16_mc30_neon:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        add r3, r1, #1
        sub r1, r1, #2
        b put_h264_qpel16_h_lowpass_l2_neon

        @.endfunc

        .globl _ff_put_h264_qpel16_mc01_neon

        @.func _ff_put_h264_qpel16_mc01_neon
_ff_put_h264_qpel16_mc01_neon:
        push {r4, lr}
        mov ip, r1
put_h264_qpel16_mc01:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r3, r2
        sub r1, r1, r2, lsl #1
        vpush {d8-d15}
        bl put_h264_qpel16_v_lowpass_l2_neon
        vpop {d8-d15}
        pop {r4, pc}

        @.endfunc

        .globl _ff_put_h264_qpel16_mc11_neon

        @.func _ff_put_h264_qpel16_mc11_neon
_ff_put_h264_qpel16_mc11_neon:
        push {r0, r1, r4, r11, lr}
put_h264_qpel16_mc11:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub sp, sp, #256
        mov r0, sp
        sub r1, r1, #2
        mov r3, #16
        vpush {d8-d15}
        bl put_h264_qpel16_h_lowpass_neon
        ldrd r0, [r11]
        mov r3, r2
        add ip, sp, #64
        sub r1, r1, r2, lsl #1
        mov r2, #16
        bl put_h264_qpel16_v_lowpass_l2_neon
        vpop {d8-d15}
        add sp, r11, #8
        pop {r4, r11, pc}

        @.endfunc

        .globl _ff_put_h264_qpel16_mc21_neon

        @.func _ff_put_h264_qpel16_mc21_neon
_ff_put_h264_qpel16_mc21_neon:
        push {r0, r1, r4-r5, r9-r11, lr}
put_h264_qpel16_mc21:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub sp, sp, #(16*16+16*12)
        sub r1, r1, #2
        mov r0, sp
        vpush {d8-d15}
        bl put_h264_qpel16_h_lowpass_neon_packed
        mov r4, r0
        ldrd r0, [r11]
        sub r1, r1, r2, lsl #1
        sub r1, r1, #2
        mov r3, r2
        bl put_h264_qpel16_hv_lowpass_l2_neon
        vpop {d8-d15}
        add sp, r11, #8
        pop {r4-r5, r9-r11, pc}

        @.endfunc

        .globl _ff_put_h264_qpel16_mc31_neon

        @.func _ff_put_h264_qpel16_mc31_neon
_ff_put_h264_qpel16_mc31_neon:
        add r1, r1, #1
        push {r0, r1, r4, r11, lr}
        sub r1, r1, #1
        b put_h264_qpel16_mc11

        @.endfunc

        .globl _ff_put_h264_qpel16_mc02_neon

        @.func _ff_put_h264_qpel16_mc02_neon
_ff_put_h264_qpel16_mc02_neon:
        push {r4, lr}
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        sub r1, r1, r2, lsl #1
        mov r3, r2
        vpush {d8-d15}
        bl put_h264_qpel16_v_lowpass_neon
        vpop {d8-d15}
        pop {r4, pc}

        @.endfunc

        .globl _ff_put_h264_qpel16_mc12_neon

        @.func _ff_put_h264_qpel16_mc12_neon
_ff_put_h264_qpel16_mc12_neon:
        push {r0, r1, r4-r5, r9-r11, lr}
put_h264_qpel16_mc12:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub sp, sp, #(16*16+16*12)
        sub r1, r1, r2, lsl #1
        mov r0, sp
        mov r3, r2
        vpush {d8-d15}
        bl put_h264_qpel16_v_lowpass_neon_packed
        mov r4, r0
        ldrd r0, [r11]
        sub r1, r1, r3, lsl #1
        sub r1, r1, #2
        mov r2, r3
        bl put_h264_qpel16_hv_lowpass_l2_neon
        vpop {d8-d15}
        add sp, r11, #8
        pop {r4-r5, r9-r11, pc}

        @.endfunc

        .globl _ff_put_h264_qpel16_mc22_neon

        @.func _ff_put_h264_qpel16_mc22_neon
_ff_put_h264_qpel16_mc22_neon:
        push {r4, r9-r11, lr}
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub r1, r1, r2, lsl #1
        sub r1, r1, #2
        mov r3, r2
        sub sp, sp, #(16*12)
        mov r4, sp
        vpush {d8-d15}
        bl put_h264_qpel16_hv_lowpass_neon
        vpop {d8-d15}
        mov sp, r11
        pop {r4, r9-r11, pc}

        @.endfunc

        .globl _ff_put_h264_qpel16_mc32_neon

        @.func _ff_put_h264_qpel16_mc32_neon
_ff_put_h264_qpel16_mc32_neon:
        push {r0, r1, r4-r5, r9-r11, lr}
        add r1, r1, #1
        b put_h264_qpel16_mc12

        @.endfunc

        .globl _ff_put_h264_qpel16_mc03_neon

        @.func _ff_put_h264_qpel16_mc03_neon
_ff_put_h264_qpel16_mc03_neon:
        push {r4, lr}
        add ip, r1, r2
        b put_h264_qpel16_mc01

        @.endfunc

        .globl _ff_put_h264_qpel16_mc13_neon

        @.func _ff_put_h264_qpel16_mc13_neon
_ff_put_h264_qpel16_mc13_neon:
        push {r0, r1, r4, r11, lr}
        add r1, r1, r2
        b put_h264_qpel16_mc11

        @.endfunc

        .globl _ff_put_h264_qpel16_mc23_neon

        @.func _ff_put_h264_qpel16_mc23_neon
_ff_put_h264_qpel16_mc23_neon:
        push {r0, r1, r4-r5, r9-r11, lr}
        add r1, r1, r2
        b put_h264_qpel16_mc21

        @.endfunc

        .globl _ff_put_h264_qpel16_mc33_neon

        @.func _ff_put_h264_qpel16_mc33_neon
_ff_put_h264_qpel16_mc33_neon:
        add r1, r1, #1
        push {r0, r1, r4, r11, lr}
        add r1, r1, r2
        sub r1, r1, #1
        b put_h264_qpel16_mc11

        @.endfunc
        .globl _ff_avg_h264_qpel16_mc10_neon

        @.func _ff_avg_h264_qpel16_mc10_neon
_ff_avg_h264_qpel16_mc10_neon:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r3, r1
        sub r1, r1, #2
        b avg_h264_qpel16_h_lowpass_l2_neon

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc20_neon

        @.func _ff_avg_h264_qpel16_mc20_neon
_ff_avg_h264_qpel16_mc20_neon:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        sub r1, r1, #2
        mov r3, r2
        b avg_h264_qpel16_h_lowpass_neon

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc30_neon

        @.func _ff_avg_h264_qpel16_mc30_neon
_ff_avg_h264_qpel16_mc30_neon:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        add r3, r1, #1
        sub r1, r1, #2
        b avg_h264_qpel16_h_lowpass_l2_neon

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc01_neon

        @.func _ff_avg_h264_qpel16_mc01_neon
_ff_avg_h264_qpel16_mc01_neon:
        push {r4, lr}
        mov ip, r1
avg_h264_qpel16_mc01:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r3, r2
        sub r1, r1, r2, lsl #1
        vpush {d8-d15}
        bl avg_h264_qpel16_v_lowpass_l2_neon
        vpop {d8-d15}
        pop {r4, pc}

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc11_neon

        @.func _ff_avg_h264_qpel16_mc11_neon
_ff_avg_h264_qpel16_mc11_neon:
        push {r0, r1, r4, r11, lr}
avg_h264_qpel16_mc11:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub sp, sp, #256
        mov r0, sp
        sub r1, r1, #2
        mov r3, #16
        vpush {d8-d15}
        bl put_h264_qpel16_h_lowpass_neon
        ldrd r0, [r11]
        mov r3, r2
        add ip, sp, #64
        sub r1, r1, r2, lsl #1
        mov r2, #16
        bl avg_h264_qpel16_v_lowpass_l2_neon
        vpop {d8-d15}
        add sp, r11, #8
        pop {r4, r11, pc}

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc21_neon

        @.func _ff_avg_h264_qpel16_mc21_neon
_ff_avg_h264_qpel16_mc21_neon:
        push {r0, r1, r4-r5, r9-r11, lr}
avg_h264_qpel16_mc21:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub sp, sp, #(16*16+16*12)
        sub r1, r1, #2
        mov r0, sp
        vpush {d8-d15}
        bl put_h264_qpel16_h_lowpass_neon_packed
        mov r4, r0
        ldrd r0, [r11]
        sub r1, r1, r2, lsl #1
        sub r1, r1, #2
        mov r3, r2
        bl avg_h264_qpel16_hv_lowpass_l2_neon
        vpop {d8-d15}
        add sp, r11, #8
        pop {r4-r5, r9-r11, pc}

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc31_neon

        @.func _ff_avg_h264_qpel16_mc31_neon
_ff_avg_h264_qpel16_mc31_neon:
        add r1, r1, #1
        push {r0, r1, r4, r11, lr}
        sub r1, r1, #1
        b avg_h264_qpel16_mc11

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc02_neon

        @.func _ff_avg_h264_qpel16_mc02_neon
_ff_avg_h264_qpel16_mc02_neon:
        push {r4, lr}
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        sub r1, r1, r2, lsl #1
        mov r3, r2
        vpush {d8-d15}
        bl avg_h264_qpel16_v_lowpass_neon
        vpop {d8-d15}
        pop {r4, pc}

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc12_neon

        @.func _ff_avg_h264_qpel16_mc12_neon
_ff_avg_h264_qpel16_mc12_neon:
        push {r0, r1, r4-r5, r9-r11, lr}
avg_h264_qpel16_mc12:
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub sp, sp, #(16*16+16*12)
        sub r1, r1, r2, lsl #1
        mov r0, sp
        mov r3, r2
        vpush {d8-d15}
        bl put_h264_qpel16_v_lowpass_neon_packed
        mov r4, r0
        ldrd r0, [r11]
        sub r1, r1, r3, lsl #1
        sub r1, r1, #2
        mov r2, r3
        bl avg_h264_qpel16_hv_lowpass_l2_neon
        vpop {d8-d15}
        add sp, r11, #8
        pop {r4-r5, r9-r11, pc}

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc22_neon

        @.func _ff_avg_h264_qpel16_mc22_neon
_ff_avg_h264_qpel16_mc22_neon:
        push {r4, r9-r11, lr}
        movw r3, #5
        movt r3, #20
        vmov.32 d6[0], r3
        mov r11, sp
        bic sp, sp, #15
        sub r1, r1, r2, lsl #1
        sub r1, r1, #2
        mov r3, r2
        sub sp, sp, #(16*12)
        mov r4, sp
        vpush {d8-d15}
        bl avg_h264_qpel16_hv_lowpass_neon
        vpop {d8-d15}
        mov sp, r11
        pop {r4, r9-r11, pc}

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc32_neon

        @.func _ff_avg_h264_qpel16_mc32_neon
_ff_avg_h264_qpel16_mc32_neon:
        push {r0, r1, r4-r5, r9-r11, lr}
        add r1, r1, #1
        b avg_h264_qpel16_mc12

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc03_neon

        @.func _ff_avg_h264_qpel16_mc03_neon
_ff_avg_h264_qpel16_mc03_neon:
        push {r4, lr}
        add ip, r1, r2
        b avg_h264_qpel16_mc01

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc13_neon

        @.func _ff_avg_h264_qpel16_mc13_neon
_ff_avg_h264_qpel16_mc13_neon:
        push {r0, r1, r4, r11, lr}
        add r1, r1, r2
        b avg_h264_qpel16_mc11

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc23_neon

        @.func _ff_avg_h264_qpel16_mc23_neon
_ff_avg_h264_qpel16_mc23_neon:
        push {r0, r1, r4-r5, r9-r11, lr}
        add r1, r1, r2
        b avg_h264_qpel16_mc21

        @.endfunc

        .globl _ff_avg_h264_qpel16_mc33_neon

        @.func _ff_avg_h264_qpel16_mc33_neon
_ff_avg_h264_qpel16_mc33_neon:
        add r1, r1, #1
        push {r0, r1, r4, r11, lr}
        add r1, r1, r2
        sub r1, r1, #1
        b avg_h264_qpel16_mc11

        @.endfunc








        .globl _ff_biweight_h264_pixels_16x8_neon

        @.func _ff_biweight_h264_pixels_16x8_neon
_ff_biweight_h264_pixels_16x8_neon:
        mov ip, #8
        b biweight_h264_pixels_16_neon

        @.endfunc
        .globl _ff_biweight_h264_pixels_16x16_neon

        @.func _ff_biweight_h264_pixels_16x16_neon
_ff_biweight_h264_pixels_16x16_neon:
        mov ip, #16

        @.endfunc

        @.func biweight_h264_pixels_16_neon
biweight_h264_pixels_16_neon:
        push {r4-r6, lr}
        add r4, sp, #16
        ldm r4, {r4-r6}
        lsr lr, r4, #31
        add r6, r6, #1
        eors lr, lr, r5, lsr #30
        orr r6, r6, #1
        vdup.16 q9, r3
        lsl r6, r6, r3
        vmvn q9, q9
        vdup.16 q8, r6
        mov r6, r0
        beq 10f
        subs lr, lr, #1
        beq 20f
        subs lr, lr, #1
        beq 30f
        b 40f
10:        vdup.8 d0, r4
        vdup.8 d1, r5
        vmov q2, q8
        vmov q3, q8
1: subs ip, ip, #2
        vld1.8 {d20-d21},[r0,:128], r2
        vmlal.u8 q2, d0, d20
        pld [r0]
        vmlal.u8 q3, d0, d21
        vld1.8 {d22-d23},[r1,:128], r2
        vmlal.u8 q2, d1, d22
        pld [r1]
        vmlal.u8 q3, d1, d23
        vmov q12, q8
        vld1.8 {d28-d29},[r0,:128], r2
        vmov q13, q8
        vmlal.u8 q12, d0, d28
        pld [r0]
        vmlal.u8 q13, d0, d29
        vld1.8 {d30-d31},[r1,:128], r2
        vmlal.u8 q12, d1, d30
        pld [r1]
        vmlal.u8 q13, d1, d31
        vshl.s16 q2, q2, q9
        vshl.s16 q3, q3, q9
        vqmovun.s16 d4, q2
        vqmovun.s16 d5, q3
        vshl.s16 q12, q12, q9
        vshl.s16 q13, q13, q9
        vqmovun.s16 d24, q12
        vqmovun.s16 d25, q13
        vmov q3, q8
        vst1.8 {d4- d5}, [r6,:128], r2
        vmov q2, q8
        vst1.8 {d24-d25},[r6,:128], r2
        bne 1b
        pop {r4-r6, pc}
20: rsb r4, r4, #0
        vdup.8 d0, r4
        vdup.8 d1, r5
        vmov q2, q8
        vmov q3, q8
1: subs ip, ip, #2
        vld1.8 {d20-d21},[r0,:128], r2
        vmlsl.u8 q2, d0, d20
        pld [r0]
        vmlsl.u8 q3, d0, d21
        vld1.8 {d22-d23},[r1,:128], r2
        vmlal.u8 q2, d1, d22
        pld [r1]
        vmlal.u8 q3, d1, d23
        vmov q12, q8
        vld1.8 {d28-d29},[r0,:128], r2
        vmov q13, q8
        vmlsl.u8 q12, d0, d28
        pld [r0]
        vmlsl.u8 q13, d0, d29
        vld1.8 {d30-d31},[r1,:128], r2
        vmlal.u8 q12, d1, d30
        pld [r1]
        vmlal.u8 q13, d1, d31
        vshl.s16 q2, q2, q9
        vshl.s16 q3, q3, q9
        vqmovun.s16 d4, q2
        vqmovun.s16 d5, q3
        vshl.s16 q12, q12, q9
        vshl.s16 q13, q13, q9
        vqmovun.s16 d24, q12
        vqmovun.s16 d25, q13
        vmov q3, q8
        vst1.8 {d4- d5}, [r6,:128], r2
        vmov q2, q8
        vst1.8 {d24-d25},[r6,:128], r2
        bne 1b
        pop {r4-r6, pc}
30: rsb r4, r4, #0
        rsb r5, r5, #0
        vdup.8 d0, r4
        vdup.8 d1, r5
        vmov q2, q8
        vmov q3, q8
1: subs ip, ip, #2
        vld1.8 {d20-d21},[r0,:128], r2
        vmlsl.u8 q2, d0, d20
        pld [r0]
        vmlsl.u8 q3, d0, d21
        vld1.8 {d22-d23},[r1,:128], r2
        vmlsl.u8 q2, d1, d22
        pld [r1]
        vmlsl.u8 q3, d1, d23
        vmov q12, q8
        vld1.8 {d28-d29},[r0,:128], r2
        vmov q13, q8
        vmlsl.u8 q12, d0, d28
        pld [r0]
        vmlsl.u8 q13, d0, d29
        vld1.8 {d30-d31},[r1,:128], r2
        vmlsl.u8 q12, d1, d30
        pld [r1]
        vmlsl.u8 q13, d1, d31
        vshl.s16 q2, q2, q9
        vshl.s16 q3, q3, q9
        vqmovun.s16 d4, q2
        vqmovun.s16 d5, q3
        vshl.s16 q12, q12, q9
        vshl.s16 q13, q13, q9
        vqmovun.s16 d24, q12
        vqmovun.s16 d25, q13
        vmov q3, q8
        vst1.8 {d4- d5}, [r6,:128], r2
        vmov q2, q8
        vst1.8 {d24-d25},[r6,:128], r2
        bne 1b
        pop {r4-r6, pc}
40: rsb r5, r5, #0
        vdup.8 d0, r4
        vdup.8 d1, r5
        vmov q2, q8
        vmov q3, q8
1: subs ip, ip, #2
        vld1.8 {d20-d21},[r0,:128], r2
        vmlal.u8 q2, d0, d20
        pld [r0]
        vmlal.u8 q3, d0, d21
        vld1.8 {d22-d23},[r1,:128], r2
        vmlsl.u8 q2, d1, d22
        pld [r1]
        vmlsl.u8 q3, d1, d23
        vmov q12, q8
        vld1.8 {d28-d29},[r0,:128], r2
        vmov q13, q8
        vmlal.u8 q12, d0, d28
        pld [r0]
        vmlal.u8 q13, d0, d29
        vld1.8 {d30-d31},[r1,:128], r2
        vmlsl.u8 q12, d1, d30
        pld [r1]
        vmlsl.u8 q13, d1, d31
        vshl.s16 q2, q2, q9
        vshl.s16 q3, q3, q9
        vqmovun.s16 d4, q2
        vqmovun.s16 d5, q3
        vshl.s16 q12, q12, q9
        vshl.s16 q13, q13, q9
        vqmovun.s16 d24, q12
        vqmovun.s16 d25, q13
        vmov q3, q8
        vst1.8 {d4- d5}, [r6,:128], r2
        vmov q2, q8
        vst1.8 {d24-d25},[r6,:128], r2
        bne 1b
        pop {r4-r6, pc}

        @.endfunc

        .globl _ff_biweight_h264_pixels_8x16_neon

        @.func _ff_biweight_h264_pixels_8x16_neon
_ff_biweight_h264_pixels_8x16_neon:
        mov ip, #16
        b biweight_h264_pixels_8_neon

        @.endfunc
        .globl _ff_biweight_h264_pixels_8x4_neon

        @.func _ff_biweight_h264_pixels_8x4_neon
_ff_biweight_h264_pixels_8x4_neon:
        mov ip, #4
        b biweight_h264_pixels_8_neon

        @.endfunc
        .globl _ff_biweight_h264_pixels_8x8_neon

        @.func _ff_biweight_h264_pixels_8x8_neon
_ff_biweight_h264_pixels_8x8_neon:
        mov ip, #8

        @.endfunc

        @.func biweight_h264_pixels_8_neon
biweight_h264_pixels_8_neon:
        push {r4-r6, lr}
        add r4, sp, #16
        ldm r4, {r4-r6}
        lsr lr, r4, #31
        add r6, r6, #1
        eors lr, lr, r5, lsr #30
        orr r6, r6, #1
        vdup.16 q9, r3
        lsl r6, r6, r3
        vmvn q9, q9
        vdup.16 q8, r6
        mov r6, r0
        beq 10f
        subs lr, lr, #1
        beq 20f
        subs lr, lr, #1
        beq 30f
        b 40f
10:        vdup.8 d0, r4
        vdup.8 d1, r5
        vmov q1, q8
        vmov q10, q8
1: subs ip, ip, #2
        vld1.8 {d4},[r0,:64], r2
        vmlal.u8 q1, d0, d4
        pld [r0]
        vld1.8 {d5},[r1,:64], r2
        vmlal.u8 q1, d1, d5
        pld [r1]
        vld1.8 {d6},[r0,:64], r2
        vmlal.u8 q10, d0, d6
        pld [r0]
        vld1.8 {d7},[r1,:64], r2
        vmlal.u8 q10, d1, d7
        pld [r1]
        vshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vmov q10, q8
        vst1.8 {d2},[r6,:64], r2
        vmov q1, q8
        vst1.8 {d4},[r6,:64], r2
        bne 1b
        pop {r4-r6, pc}
20: rsb r4, r4, #0
        vdup.8 d0, r4
        vdup.8 d1, r5
        vmov q1, q8
        vmov q10, q8
1: subs ip, ip, #2
        vld1.8 {d4},[r0,:64], r2
        vmlsl.u8 q1, d0, d4
        pld [r0]
        vld1.8 {d5},[r1,:64], r2
        vmlal.u8 q1, d1, d5
        pld [r1]
        vld1.8 {d6},[r0,:64], r2
        vmlsl.u8 q10, d0, d6
        pld [r0]
        vld1.8 {d7},[r1,:64], r2
        vmlal.u8 q10, d1, d7
        pld [r1]
        vshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vmov q10, q8
        vst1.8 {d2},[r6,:64], r2
        vmov q1, q8
        vst1.8 {d4},[r6,:64], r2
        bne 1b
        pop {r4-r6, pc}
30: rsb r4, r4, #0
        rsb r5, r5, #0
        vdup.8 d0, r4
        vdup.8 d1, r5
        vmov q1, q8
        vmov q10, q8
1: subs ip, ip, #2
        vld1.8 {d4},[r0,:64], r2
        vmlsl.u8 q1, d0, d4
        pld [r0]
        vld1.8 {d5},[r1,:64], r2
        vmlsl.u8 q1, d1, d5
        pld [r1]
        vld1.8 {d6},[r0,:64], r2
        vmlsl.u8 q10, d0, d6
        pld [r0]
        vld1.8 {d7},[r1,:64], r2
        vmlsl.u8 q10, d1, d7
        pld [r1]
        vshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vmov q10, q8
        vst1.8 {d2},[r6,:64], r2
        vmov q1, q8
        vst1.8 {d4},[r6,:64], r2
        bne 1b
        pop {r4-r6, pc}
40: rsb r5, r5, #0
        vdup.8 d0, r4
        vdup.8 d1, r5
        vmov q1, q8
        vmov q10, q8
1: subs ip, ip, #2
        vld1.8 {d4},[r0,:64], r2
        vmlal.u8 q1, d0, d4
        pld [r0]
        vld1.8 {d5},[r1,:64], r2
        vmlsl.u8 q1, d1, d5
        pld [r1]
        vld1.8 {d6},[r0,:64], r2
        vmlal.u8 q10, d0, d6
        pld [r0]
        vld1.8 {d7},[r1,:64], r2
        vmlsl.u8 q10, d1, d7
        pld [r1]
        vshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vmov q10, q8
        vst1.8 {d2},[r6,:64], r2
        vmov q1, q8
        vst1.8 {d4},[r6,:64], r2
        bne 1b
        pop {r4-r6, pc}

        @.endfunc

        .globl _ff_biweight_h264_pixels_4x8_neon

        @.func _ff_biweight_h264_pixels_4x8_neon
_ff_biweight_h264_pixels_4x8_neon:
        mov ip, #8
        b biweight_h264_pixels_4_neon

        @.endfunc
        .globl _ff_biweight_h264_pixels_4x2_neon

        @.func _ff_biweight_h264_pixels_4x2_neon
_ff_biweight_h264_pixels_4x2_neon:
        mov ip, #2
        b biweight_h264_pixels_4_neon

        @.endfunc
        .globl _ff_biweight_h264_pixels_4x4_neon

        @.func _ff_biweight_h264_pixels_4x4_neon
_ff_biweight_h264_pixels_4x4_neon:
        mov ip, #4

        @.endfunc

        @.func biweight_h264_pixels_4_neon
biweight_h264_pixels_4_neon:
        push {r4-r6, lr}
        add r4, sp, #16
        ldm r4, {r4-r6}
        lsr lr, r4, #31
        add r6, r6, #1
        eors lr, lr, r5, lsr #30
        orr r6, r6, #1
        vdup.16 q9, r3
        lsl r6, r6, r3
        vmvn q9, q9
        vdup.16 q8, r6
        mov r6, r0
        beq 10f
        subs lr, lr, #1
        beq 20f
        subs lr, lr, #1
        beq 30f
        b 40f
10:        vdup.8 d0, r4
        vdup.8 d1, r5
        vmov q1, q8
        vmov q10, q8
1: subs ip, ip, #4
        vld1.32 {d4[0]},[r0,:32], r2
        vld1.32 {d4[1]},[r0,:32], r2
        vmlal.u8 q1, d0, d4
        pld [r0]
        vld1.32 {d5[0]},[r1,:32], r2
        vld1.32 {d5[1]},[r1,:32], r2
        vmlal.u8 q1, d1, d5
        pld [r1]
        blt 2f
        vld1.32 {d6[0]},[r0,:32], r2
        vld1.32 {d6[1]},[r0,:32], r2
        vmlal.u8 q10, d0, d6
        pld [r0]
        vld1.32 {d7[0]},[r1,:32], r2
        vld1.32 {d7[1]},[r1,:32], r2
        vmlal.u8 q10, d1, d7
        pld [r1]
        vshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vmov q10, q8
        vst1.32 {d2[0]},[r6,:32], r2
        vst1.32 {d2[1]},[r6,:32], r2
        vmov q1, q8
        vst1.32 {d4[0]},[r6,:32], r2
        vst1.32 {d4[1]},[r6,:32], r2
        bne 1b
        pop {r4-r6, pc}
2: vshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vst1.32 {d2[0]},[r6,:32], r2
        vst1.32 {d2[1]},[r6,:32], r2
        pop {r4-r6, pc}
20: rsb r4, r4, #0
        vdup.8 d0, r4
        vdup.8 d1, r5
        vmov q1, q8
        vmov q10, q8
1: subs ip, ip, #4
        vld1.32 {d4[0]},[r0,:32], r2
        vld1.32 {d4[1]},[r0,:32], r2
        vmlsl.u8 q1, d0, d4
        pld [r0]
        vld1.32 {d5[0]},[r1,:32], r2
        vld1.32 {d5[1]},[r1,:32], r2
        vmlal.u8 q1, d1, d5
        pld [r1]
        blt 2f
        vld1.32 {d6[0]},[r0,:32], r2
        vld1.32 {d6[1]},[r0,:32], r2
        vmlsl.u8 q10, d0, d6
        pld [r0]
        vld1.32 {d7[0]},[r1,:32], r2
        vld1.32 {d7[1]},[r1,:32], r2
        vmlal.u8 q10, d1, d7
        pld [r1]
        vshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vmov q10, q8
        vst1.32 {d2[0]},[r6,:32], r2
        vst1.32 {d2[1]},[r6,:32], r2
        vmov q1, q8
        vst1.32 {d4[0]},[r6,:32], r2
        vst1.32 {d4[1]},[r6,:32], r2
        bne 1b
        pop {r4-r6, pc}
2: vshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vst1.32 {d2[0]},[r6,:32], r2
        vst1.32 {d2[1]},[r6,:32], r2
        pop {r4-r6, pc}
30: rsb r4, r4, #0
        rsb r5, r5, #0
        vdup.8 d0, r4
        vdup.8 d1, r5
        vmov q1, q8
        vmov q10, q8
1: subs ip, ip, #4
        vld1.32 {d4[0]},[r0,:32], r2
        vld1.32 {d4[1]},[r0,:32], r2
        vmlsl.u8 q1, d0, d4
        pld [r0]
        vld1.32 {d5[0]},[r1,:32], r2
        vld1.32 {d5[1]},[r1,:32], r2
        vmlsl.u8 q1, d1, d5
        pld [r1]
        blt 2f
        vld1.32 {d6[0]},[r0,:32], r2
        vld1.32 {d6[1]},[r0,:32], r2
        vmlsl.u8 q10, d0, d6
        pld [r0]
        vld1.32 {d7[0]},[r1,:32], r2
        vld1.32 {d7[1]},[r1,:32], r2
        vmlsl.u8 q10, d1, d7
        pld [r1]
        vshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vmov q10, q8
        vst1.32 {d2[0]},[r6,:32], r2
        vst1.32 {d2[1]},[r6,:32], r2
        vmov q1, q8
        vst1.32 {d4[0]},[r6,:32], r2
        vst1.32 {d4[1]},[r6,:32], r2
        bne 1b
        pop {r4-r6, pc}
2: vshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vst1.32 {d2[0]},[r6,:32], r2
        vst1.32 {d2[1]},[r6,:32], r2
        pop {r4-r6, pc}
40: rsb r5, r5, #0
        vdup.8 d0, r4
        vdup.8 d1, r5
        vmov q1, q8
        vmov q10, q8
1: subs ip, ip, #4
        vld1.32 {d4[0]},[r0,:32], r2
        vld1.32 {d4[1]},[r0,:32], r2
        vmlal.u8 q1, d0, d4
        pld [r0]
        vld1.32 {d5[0]},[r1,:32], r2
        vld1.32 {d5[1]},[r1,:32], r2
        vmlsl.u8 q1, d1, d5
        pld [r1]
        blt 2f
        vld1.32 {d6[0]},[r0,:32], r2
        vld1.32 {d6[1]},[r0,:32], r2
        vmlal.u8 q10, d0, d6
        pld [r0]
        vld1.32 {d7[0]},[r1,:32], r2
        vld1.32 {d7[1]},[r1,:32], r2
        vmlsl.u8 q10, d1, d7
        pld [r1]
        vshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vmov q10, q8
        vst1.32 {d2[0]},[r6,:32], r2
        vst1.32 {d2[1]},[r6,:32], r2
        vmov q1, q8
        vst1.32 {d4[0]},[r6,:32], r2
        vst1.32 {d4[1]},[r6,:32], r2
        bne 1b
        pop {r4-r6, pc}
2: vshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vst1.32 {d2[0]},[r6,:32], r2
        vst1.32 {d2[1]},[r6,:32], r2
        pop {r4-r6, pc}

        @.endfunc








        .globl _ff_weight_h264_pixels_16x8_neon

        @.func _ff_weight_h264_pixels_16x8_neon
_ff_weight_h264_pixels_16x8_neon:
        mov ip, #8
        b weight_h264_pixels_16_neon

        @.endfunc
        .globl _ff_weight_h264_pixels_16x16_neon

        @.func _ff_weight_h264_pixels_16x16_neon
_ff_weight_h264_pixels_16x16_neon:
        mov ip, #16

        @.endfunc

        @.func weight_h264_pixels_16_neon
weight_h264_pixels_16_neon:
        push {r4, lr}
        ldr r4, [sp, #8]
        cmp r2, #1
        lsl r4, r4, r2
        vdup.16 q8, r4
        mov r4, r0
        ble 20f
        rsb lr, r2, #1
        vdup.16 q9, lr
        cmp r3, #0
        blt 10f
        vdup.8 d0, r3
1: subs ip, ip, #2
        vld1.8 {d20-d21},[r0,:128], r1
        vmull.u8 q2, d0, d20
        pld [r0]
        vmull.u8 q3, d0, d21
        vld1.8 {d28-d29},[r0,:128], r1
        vmull.u8 q12, d0, d28
        pld [r0]
        vmull.u8 q13, d0, d29
        vhadd.s16 q2, q8, q2
        vrshl.s16 q2, q2, q9
        vhadd.s16 q3, q8, q3
        vrshl.s16 q3, q3, q9
        vqmovun.s16 d4, q2
        vqmovun.s16 d5, q3
        vhadd.s16 q12, q8, q12
        vrshl.s16 q12, q12, q9
        vhadd.s16 q13, q8, q13
        vrshl.s16 q13, q13, q9
        vqmovun.s16 d24, q12
        vqmovun.s16 d25, q13
        vst1.8 {d4- d5}, [r4,:128], r1
        vst1.8 {d24-d25},[r4,:128], r1
        bne 1b
        pop {r4, pc}
10: rsb r3, r3, #0
        vdup.8 d0, r3
1: subs ip, ip, #2
        vld1.8 {d20-d21},[r0,:128], r1
        vmull.u8 q2, d0, d20
        pld [r0]
        vmull.u8 q3, d0, d21
        vld1.8 {d28-d29},[r0,:128], r1
        vmull.u8 q12, d0, d28
        pld [r0]
        vmull.u8 q13, d0, d29
        vhsub.s16 q2, q8, q2
        vrshl.s16 q2, q2, q9
        vhsub.s16 q3, q8, q3
        vrshl.s16 q3, q3, q9
        vqmovun.s16 d4, q2
        vqmovun.s16 d5, q3
        vhsub.s16 q12, q8, q12
        vrshl.s16 q12, q12, q9
        vhsub.s16 q13, q8, q13
        vrshl.s16 q13, q13, q9
        vqmovun.s16 d24, q12
        vqmovun.s16 d25, q13
        vst1.8 {d4- d5}, [r4,:128], r1
        vst1.8 {d24-d25},[r4,:128], r1
        bne 1b
        pop {r4, pc}
20: rsb lr, r2, #0
        vdup.16 q9, lr
        cmp r3, #0
        blt 10f
        vdup.8 d0, r3
1: subs ip, ip, #2
        vld1.8 {d20-d21},[r0,:128], r1
        vmull.u8 q2, d0, d20
        pld [r0]
        vmull.u8 q3, d0, d21
        vld1.8 {d28-d29},[r0,:128], r1
        vmull.u8 q12, d0, d28
        pld [r0]
        vmull.u8 q13, d0, d29
        vadd.s16 q2, q8, q2
        vrshl.s16 q2, q2, q9
        vadd.s16 q3, q8, q3
        vrshl.s16 q3, q3, q9
        vqmovun.s16 d4, q2
        vqmovun.s16 d5, q3
        vadd.s16 q12, q8, q12
        vrshl.s16 q12, q12, q9
        vadd.s16 q13, q8, q13
        vrshl.s16 q13, q13, q9
        vqmovun.s16 d24, q12
        vqmovun.s16 d25, q13
        vst1.8 {d4- d5}, [r4,:128], r1
        vst1.8 {d24-d25},[r4,:128], r1
        bne 1b
        pop {r4, pc}
10: rsb r3, r3, #0
        vdup.8 d0, r3
1: subs ip, ip, #2
        vld1.8 {d20-d21},[r0,:128], r1
        vmull.u8 q2, d0, d20
        pld [r0]
        vmull.u8 q3, d0, d21
        vld1.8 {d28-d29},[r0,:128], r1
        vmull.u8 q12, d0, d28
        pld [r0]
        vmull.u8 q13, d0, d29
        vsub.s16 q2, q8, q2
        vrshl.s16 q2, q2, q9
        vsub.s16 q3, q8, q3
        vrshl.s16 q3, q3, q9
        vqmovun.s16 d4, q2
        vqmovun.s16 d5, q3
        vsub.s16 q12, q8, q12
        vrshl.s16 q12, q12, q9
        vsub.s16 q13, q8, q13
        vrshl.s16 q13, q13, q9
        vqmovun.s16 d24, q12
        vqmovun.s16 d25, q13
        vst1.8 {d4- d5}, [r4,:128], r1
        vst1.8 {d24-d25},[r4,:128], r1
        bne 1b
        pop {r4, pc}

        @.endfunc

        .globl _ff_weight_h264_pixels_8x16_neon

        @.func _ff_weight_h264_pixels_8x16_neon
_ff_weight_h264_pixels_8x16_neon:
        mov ip, #16
        b weight_h264_pixels_8_neon

        @.endfunc
        .globl _ff_weight_h264_pixels_8x4_neon

        @.func _ff_weight_h264_pixels_8x4_neon
_ff_weight_h264_pixels_8x4_neon:
        mov ip, #4
        b weight_h264_pixels_8_neon

        @.endfunc
        .globl _ff_weight_h264_pixels_8x8_neon

        @.func _ff_weight_h264_pixels_8x8_neon
_ff_weight_h264_pixels_8x8_neon:
        mov ip, #8

        @.endfunc

        @.func weight_h264_pixels_8_neon
weight_h264_pixels_8_neon:
        push {r4, lr}
        ldr r4, [sp, #8]
        cmp r2, #1
        lsl r4, r4, r2
        vdup.16 q8, r4
        mov r4, r0
        ble 20f
        rsb lr, r2, #1
        vdup.16 q9, lr
        cmp r3, #0
        blt 10f
        vdup.8 d0, r3
1: subs ip, ip, #2
        vld1.8 {d4},[r0,:64], r1
        vmull.u8 q1, d0, d4
        pld [r0]
        vld1.8 {d6},[r0,:64], r1
        vmull.u8 q10, d0, d6
        vhadd.s16 q1, q8, q1
        pld [r0]
        vrshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vhadd.s16 q10, q8, q10
        vrshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vst1.8 {d2},[r4,:64], r1
        vst1.8 {d4},[r4,:64], r1
        bne 1b
        pop {r4, pc}
10: rsb r3, r3, #0
        vdup.8 d0, r3
1: subs ip, ip, #2
        vld1.8 {d4},[r0,:64], r1
        vmull.u8 q1, d0, d4
        pld [r0]
        vld1.8 {d6},[r0,:64], r1
        vmull.u8 q10, d0, d6
        vhsub.s16 q1, q8, q1
        pld [r0]
        vrshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vhsub.s16 q10, q8, q10
        vrshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vst1.8 {d2},[r4,:64], r1
        vst1.8 {d4},[r4,:64], r1
        bne 1b
        pop {r4, pc}
20: rsb lr, r2, #0
        vdup.16 q9, lr
        cmp r3, #0
        blt 10f
        vdup.8 d0, r3
1: subs ip, ip, #2
        vld1.8 {d4},[r0,:64], r1
        vmull.u8 q1, d0, d4
        pld [r0]
        vld1.8 {d6},[r0,:64], r1
        vmull.u8 q10, d0, d6
        vadd.s16 q1, q8, q1
        pld [r0]
        vrshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vadd.s16 q10, q8, q10
        vrshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vst1.8 {d2},[r4,:64], r1
        vst1.8 {d4},[r4,:64], r1
        bne 1b
        pop {r4, pc}
10: rsb r3, r3, #0
        vdup.8 d0, r3
1: subs ip, ip, #2
        vld1.8 {d4},[r0,:64], r1
        vmull.u8 q1, d0, d4
        pld [r0]
        vld1.8 {d6},[r0,:64], r1
        vmull.u8 q10, d0, d6
        vsub.s16 q1, q8, q1
        pld [r0]
        vrshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vsub.s16 q10, q8, q10
        vrshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vst1.8 {d2},[r4,:64], r1
        vst1.8 {d4},[r4,:64], r1
        bne 1b
        pop {r4, pc}

        @.endfunc

        .globl _ff_weight_h264_pixels_4x8_neon

        @.func _ff_weight_h264_pixels_4x8_neon
_ff_weight_h264_pixels_4x8_neon:
        mov ip, #8
        b weight_h264_pixels_4_neon

        @.endfunc
        .globl _ff_weight_h264_pixels_4x2_neon

        @.func _ff_weight_h264_pixels_4x2_neon
_ff_weight_h264_pixels_4x2_neon:
        mov ip, #2
        b weight_h264_pixels_4_neon

        @.endfunc
        .globl _ff_weight_h264_pixels_4x4_neon

        @.func _ff_weight_h264_pixels_4x4_neon
_ff_weight_h264_pixels_4x4_neon:
        mov ip, #4

        @.endfunc

        @.func weight_h264_pixels_4_neon
weight_h264_pixels_4_neon:
        push {r4, lr}
        ldr r4, [sp, #8]
        cmp r2, #1
        lsl r4, r4, r2
        vdup.16 q8, r4
        mov r4, r0
        ble 20f
        rsb lr, r2, #1
        vdup.16 q9, lr
        cmp r3, #0
        blt 10f
        vdup.8 d0, r3
        vmov q1, q8
        vmov q10, q8
1: subs ip, ip, #4
        vld1.32 {d4[0]},[r0,:32], r1
        vld1.32 {d4[1]},[r0,:32], r1
        vmull.u8 q1, d0, d4
        pld [r0]
        blt 2f
        vld1.32 {d6[0]},[r0,:32], r1
        vld1.32 {d6[1]},[r0,:32], r1
        vmull.u8 q10, d0, d6
        pld [r0]
        vhadd.s16 q1, q8, q1
        vrshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vhadd.s16 q10, q8, q10
        vrshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vmov q10, q8
        vst1.32 {d2[0]},[r4,:32], r1
        vst1.32 {d2[1]},[r4,:32], r1
        vmov q1, q8
        vst1.32 {d4[0]},[r4,:32], r1
        vst1.32 {d4[1]},[r4,:32], r1
        bne 1b
        pop {r4, pc}
2: vhadd.s16 q1, q8, q1
        vrshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vst1.32 {d2[0]},[r4,:32], r1
        vst1.32 {d2[1]},[r4,:32], r1
        pop {r4, pc}
10: rsb r3, r3, #0
        vdup.8 d0, r3
        vmov q1, q8
        vmov q10, q8
1: subs ip, ip, #4
        vld1.32 {d4[0]},[r0,:32], r1
        vld1.32 {d4[1]},[r0,:32], r1
        vmull.u8 q1, d0, d4
        pld [r0]
        blt 2f
        vld1.32 {d6[0]},[r0,:32], r1
        vld1.32 {d6[1]},[r0,:32], r1
        vmull.u8 q10, d0, d6
        pld [r0]
        vhsub.s16 q1, q8, q1
        vrshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vhsub.s16 q10, q8, q10
        vrshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vmov q10, q8
        vst1.32 {d2[0]},[r4,:32], r1
        vst1.32 {d2[1]},[r4,:32], r1
        vmov q1, q8
        vst1.32 {d4[0]},[r4,:32], r1
        vst1.32 {d4[1]},[r4,:32], r1
        bne 1b
        pop {r4, pc}
2: vhsub.s16 q1, q8, q1
        vrshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vst1.32 {d2[0]},[r4,:32], r1
        vst1.32 {d2[1]},[r4,:32], r1
        pop {r4, pc}
20: rsb lr, r2, #0
        vdup.16 q9, lr
        cmp r3, #0
        blt 10f
        vdup.8 d0, r3
        vmov q1, q8
        vmov q10, q8
1: subs ip, ip, #4
        vld1.32 {d4[0]},[r0,:32], r1
        vld1.32 {d4[1]},[r0,:32], r1
        vmull.u8 q1, d0, d4
        pld [r0]
        blt 2f
        vld1.32 {d6[0]},[r0,:32], r1
        vld1.32 {d6[1]},[r0,:32], r1
        vmull.u8 q10, d0, d6
        pld [r0]
        vadd.s16 q1, q8, q1
        vrshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vadd.s16 q10, q8, q10
        vrshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vmov q10, q8
        vst1.32 {d2[0]},[r4,:32], r1
        vst1.32 {d2[1]},[r4,:32], r1
        vmov q1, q8
        vst1.32 {d4[0]},[r4,:32], r1
        vst1.32 {d4[1]},[r4,:32], r1
        bne 1b
        pop {r4, pc}
2: vadd.s16 q1, q8, q1
        vrshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vst1.32 {d2[0]},[r4,:32], r1
        vst1.32 {d2[1]},[r4,:32], r1
        pop {r4, pc}
10: rsb r3, r3, #0
        vdup.8 d0, r3
        vmov q1, q8
        vmov q10, q8
1: subs ip, ip, #4
        vld1.32 {d4[0]},[r0,:32], r1
        vld1.32 {d4[1]},[r0,:32], r1
        vmull.u8 q1, d0, d4
        pld [r0]
        blt 2f
        vld1.32 {d6[0]},[r0,:32], r1
        vld1.32 {d6[1]},[r0,:32], r1
        vmull.u8 q10, d0, d6
        pld [r0]
        vsub.s16 q1, q8, q1
        vrshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vsub.s16 q10, q8, q10
        vrshl.s16 q10, q10, q9
        vqmovun.s16 d4, q10
        vmov q10, q8
        vst1.32 {d2[0]},[r4,:32], r1
        vst1.32 {d2[1]},[r4,:32], r1
        vmov q1, q8
        vst1.32 {d4[0]},[r4,:32], r1
        vst1.32 {d4[1]},[r4,:32], r1
        bne 1b
        pop {r4, pc}
2: vsub.s16 q1, q8, q1
        vrshl.s16 q1, q1, q9
        vqmovun.s16 d2, q1
        vst1.32 {d2[0]},[r4,:32], r1
        vst1.32 {d2[1]},[r4,:32], r1
        pop {r4, pc}

        @.endfunc
.text
