
        .text
        .align 6


        @.func idct_row4_pld_neon
idct_row4_pld_neon:
        pld [r0]
        add r3, r0, r1, lsl #2
        pld [r0, r1]
        pld [r0, r1, lsl #1]
        pld [r3, -r1]
        pld [r3]
        pld [r3, r1]
        add r3, r3, r1, lsl #1
        pld [r3]
        pld [r3, r1]

        @.endfunc


        @.func idct_row4_neon
idct_row4_neon:
        vmov.i32 q15, #(1<<(11 -1))
        vld1.64 {d2-d5}, [r2,:128]!
        vmlal.s16 q15, d2, d0[3]
        vld1.64 {d6,d7}, [r2,:128]!
        vorr d10, d3, d5
        vld1.64 {d8,d9}, [r2,:128]!
        add r2, r2, #-64

        vorr d11, d7, d9
        vorr d10, d10, d11
        vmov r3, r4, d10

        vmull.s16 q7, d6, d0[1]
        vmull.s16 q8, d6, d1[1]
        vmull.s16 q9, d4, d0[0]
        vadd.i32 q11, q15, q7
        vmull.s16 q10, d4, d0[2]
        vadd.i32 q12, q15, q8
        vmull.s16 q5, d4, d1[0]
        vsub.i32 q13, q15, q8
        vmull.s16 q6, d4, d1[2]
        vsub.i32 q14, q15, q7

        vmlal.s16 q9, d8, d0[2]
        vmlsl.s16 q10, d8, d1[2]
        vmlsl.s16 q5, d8, d0[0]
        vmlsl.s16 q6, d8, d1[0]

        orrs r3, r3, r4
        beq 1f

        vmull.s16 q7, d3, d0[3]
        vmlal.s16 q9, d5, d1[0]
        vmlsl.s16 q10, d5, d0[0]
        vmull.s16 q8, d7, d0[1]
        vmlal.s16 q5, d5, d1[2]
        vadd.i32 q11, q11, q7
        vsub.i32 q12, q12, q7
        vsub.i32 q13, q13, q7
        vadd.i32 q14, q14, q7
        vmlal.s16 q6, d5, d0[2]
        vmull.s16 q7, d7, d1[1]
        vmlal.s16 q9, d9, d1[2]
        vmlsl.s16 q10, d9, d1[0]
        vmlal.s16 q5, d9, d0[2]
        vmlsl.s16 q6, d9, d0[0]
        vadd.i32 q11, q11, q7
        vsub.i32 q12, q12, q8
        vadd.i32 q13, q13, q8
        vsub.i32 q14, q14, q7

1: vadd.i32 q3, q11, q9
        vadd.i32 q4, q12, q10
        vshrn.i32 d2, q3, #11
        vshrn.i32 d4, q4, #11
        vadd.i32 q7, q13, q5
        vadd.i32 q8, q14, q6
        vtrn.16 d2, d4
        vshrn.i32 d6, q7, #11
        vshrn.i32 d8, q8, #11
        vsub.i32 q14, q14, q6
        vsub.i32 q11, q11, q9
        vtrn.16 d6, d8
        vsub.i32 q13, q13, q5
        vshrn.i32 d3, q14, #11
        vtrn.32 d2, d6
        vsub.i32 q12, q12, q10
        vtrn.32 d4, d8
        vshrn.i32 d5, q13, #11
        vshrn.i32 d7, q12, #11
        vshrn.i32 d9, q11, #11

        vtrn.16 d3, d5
        vtrn.16 d7, d9
        vtrn.32 d3, d7
        vtrn.32 d5, d9

        vst1.64 {d2-d5}, [r2,:128]!
        vst1.64 {d6-d9}, [r2,:128]!

        bx lr

        @.endfunc


        @.func idct_col4_neon
idct_col4_neon:
        mov ip, #16
        vld1.64 {d2}, [r2,:64], ip
        vdup.16 d30, d1[3]
        vld1.64 {d4}, [r2,:64], ip
        vadd.i16 d30, d30, d2
        vld1.64 {d6}, [r2,:64], ip
        vmull.s16 q15, d30, d0[3]
        vld1.64 {d8}, [r2,:64], ip

        ldrd r4, [r2]
        ldrd r6, [r2, #16]
        orrs r4, r4, r5

        vmull.s16 q7, d6, d0[1]
        vmull.s16 q8, d6, d1[1]
        vmull.s16 q9, d4, d0[0]
        vadd.i32 q11, q15, q7
        vmull.s16 q10, d4, d0[2]
        vadd.i32 q12, q15, q8
        vmull.s16 q5, d4, d1[0]
        vsub.i32 q13, q15, q8
        vmull.s16 q6, d4, d1[2]
        vsub.i32 q14, q15, q7

        vmlal.s16 q9, d8, d0[2]
        vmlsl.s16 q10, d8, d1[2]
        vmlsl.s16 q5, d8, d0[0]
        vmlsl.s16 q6, d8, d1[0]
        addeq r2, r2, #16
        beq 1f

        vld1.64 {d3}, [r2,:64], ip
        vmull.s16 q7, d3, d0[3]
        vadd.i32 q11, q11, q7
        vsub.i32 q12, q12, q7
        vsub.i32 q13, q13, q7
        vadd.i32 q14, q14, q7

1: orrs r6, r6, r7
        ldrd r4, [r2, #16]
        addeq r2, r2, #16
        beq 2f

        vld1.64 {d5}, [r2,:64], ip
        vmlal.s16 q9, d5, d1[0]
        vmlsl.s16 q10, d5, d0[0]
        vmlal.s16 q5, d5, d1[2]
        vmlal.s16 q6, d5, d0[2]

2: orrs r4, r4, r5
        ldrd r4, [r2, #16]
        addeq r2, r2, #16
        beq 3f

        vld1.64 {d7}, [r2,:64], ip
        vmull.s16 q7, d7, d1[1]
        vmull.s16 q8, d7, d0[1]
        vadd.i32 q11, q11, q7
        vsub.i32 q14, q14, q7
        vsub.i32 q12, q12, q8
        vadd.i32 q13, q13, q8

3: orrs r4, r4, r5
        addeq r2, r2, #16
        beq 4f

        vld1.64 {d9}, [r2,:64], ip
        vmlal.s16 q9, d9, d1[2]
        vmlsl.s16 q10, d9, d1[0]
        vmlal.s16 q5, d9, d0[2]
        vmlsl.s16 q6, d9, d0[0]

4: vaddhn.i32 d2, q11, q9
        vaddhn.i32 d3, q12, q10
        vaddhn.i32 d4, q13, q5
        vaddhn.i32 d5, q14, q6
        vsubhn.i32 d9, q11, q9
        vsubhn.i32 d8, q12, q10
        vsubhn.i32 d7, q13, q5
        vsubhn.i32 d6, q14, q6

        bx lr

        @.endfunc

        .align 6


        @.func idct_col4_st8_neon
idct_col4_st8_neon:
        vqshrun.s16 d2, q1, #20 -16
        vqshrun.s16 d3, q2, #20 -16
        vqshrun.s16 d4, q3, #20 -16
        vqshrun.s16 d5, q4, #20 -16
        vst1.32 {d2[0]}, [r0,:32], r1
        vst1.32 {d2[1]}, [r0,:32], r1
        vst1.32 {d3[0]}, [r0,:32], r1
        vst1.32 {d3[1]}, [r0,:32], r1
        vst1.32 {d4[0]}, [r0,:32], r1
        vst1.32 {d4[1]}, [r0,:32], r1
        vst1.32 {d5[0]}, [r0,:32], r1
        vst1.32 {d5[1]}, [r0,:32], r1

        bx lr

        @.endfunc

.const_data
        .align 4
idct_coeff_neon:
        .short 22725, 21407, 19266, 16383, 12873, 8867, 4520, ((1<<(20 -1))/16383)
        .text




        .globl _ff_simple_idct_put_neon

        @.func _ff_simple_idct_put_neon
_ff_simple_idct_put_neon:
        push {r4-r7, lr}
        pld [r2]
        pld [r2, #64]
        vpush {d8-d15}




         ldr r3, .Literal_0

        vld1.64 {d0,d1}, [r3,:128]

        bl idct_row4_pld_neon
        bl idct_row4_neon
        add r2, r2, #-128
        bl idct_col4_neon
        bl idct_col4_st8_neon
        sub r0, r0, r1, lsl #3
        add r0, r0, #4
        add r2, r2, #-120
        bl idct_col4_neon
        bl idct_col4_st8_neon

        vpop {d8-d15}
        pop {r4-r7, pc}

        @.endfunc

        .align 6


        @.func idct_col4_add8_neon
idct_col4_add8_neon:
        mov ip, r0

        vld1.32 {d10[0]}, [r0,:32], r1
        vshr.s16 q1, q1, #20 -16
        vld1.32 {d10[1]}, [r0,:32], r1
        vshr.s16 q2, q2, #20 -16
        vld1.32 {d11[0]}, [r0,:32], r1
        vshr.s16 q3, q3, #20 -16
        vld1.32 {d11[1]}, [r0,:32], r1
        vshr.s16 q4, q4, #20 -16
        vld1.32 {d12[0]}, [r0,:32], r1
        vaddw.u8 q1, q1, d10
        vld1.32 {d12[1]}, [r0,:32], r1
        vaddw.u8 q2, q2, d11
        vld1.32 {d13[0]}, [r0,:32], r1
        vqmovun.s16 d2, q1
        vld1.32 {d13[1]}, [r0,:32], r1
        vaddw.u8 q3, q3, d12
        vst1.32 {d2[0]}, [ip,:32], r1
        vqmovun.s16 d3, q2
        vst1.32 {d2[1]}, [ip,:32], r1
        vaddw.u8 q4, q4, d13
        vst1.32 {d3[0]}, [ip,:32], r1
        vqmovun.s16 d4, q3
        vst1.32 {d3[1]}, [ip,:32], r1
        vqmovun.s16 d5, q4
        vst1.32 {d4[0]}, [ip,:32], r1
        vst1.32 {d4[1]}, [ip,:32], r1
        vst1.32 {d5[0]}, [ip,:32], r1
        vst1.32 {d5[1]}, [ip,:32], r1

        bx lr

        @.endfunc


        .globl _ff_simple_idct_add_neon

        @.func _ff_simple_idct_add_neon
_ff_simple_idct_add_neon:
        push {r4-r7, lr}
        pld [r2]
        pld [r2, #64]
        vpush {d8-d15}




         ldr r3, .Literal_0

        vld1.64 {d0,d1}, [r3,:128]

        bl idct_row4_pld_neon
        bl idct_row4_neon
        add r2, r2, #-128
        bl idct_col4_neon
        bl idct_col4_add8_neon
        sub r0, r0, r1, lsl #3
        add r0, r0, #4
        add r2, r2, #-120
        bl idct_col4_neon
        bl idct_col4_add8_neon

        vpop {d8-d15}
        pop {r4-r7, pc}

        @.endfunc

        .align 6


        @.func idct_col4_st16_neon
idct_col4_st16_neon:
        mov ip, #16

        vshr.s16 q1, q1, #20 -16
        vshr.s16 q2, q2, #20 -16
        vst1.64 {d2}, [r2,:64], ip
        vshr.s16 q3, q3, #20 -16
        vst1.64 {d3}, [r2,:64], ip
        vshr.s16 q4, q4, #20 -16
        vst1.64 {d4}, [r2,:64], ip
        vst1.64 {d5}, [r2,:64], ip
        vst1.64 {d6}, [r2,:64], ip
        vst1.64 {d7}, [r2,:64], ip
        vst1.64 {d8}, [r2,:64], ip
        vst1.64 {d9}, [r2,:64], ip

        bx lr

        @.endfunc


        .globl _ff_simple_idct_neon

        @.func _ff_simple_idct_neon
_ff_simple_idct_neon:
        push {r4-r7, lr}
        pld [r0]
        pld [r0, #64]
        vpush {d8-d15}




         ldr r3, .Literal_0

        vld1.64 {d0,d1}, [r3,:128]

        mov r2, r0
        bl idct_row4_neon
        bl idct_row4_neon
        add r2, r2, #-128
        bl idct_col4_neon
        add r2, r2, #-128
        bl idct_col4_st16_neon
        add r2, r2, #-120
        bl idct_col4_neon
        add r2, r2, #-128
        bl idct_col4_st16_neon

        vpop {d8-d15}
        pop {r4-r7, pc}

        @.endfunc
.text
.Literal_0:
 .word idct_coeff_neon
