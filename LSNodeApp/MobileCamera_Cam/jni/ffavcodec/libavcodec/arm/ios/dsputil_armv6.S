
        .text


        .globl _ff_avg_pixels16_armv6

        @.func _ff_avg_pixels16_armv6
_ff_avg_pixels16_armv6:
        push {r0-r3, lr}
        bl _ff_avg_pixels8_armv6
        pop {r0-r3, lr}
        add r0, r0, #8
        add r1, r1, #8
        b _ff_avg_pixels8_armv6

        @.endfunc
        .globl _ff_put_pixels16_x2_armv6

        @.func _ff_put_pixels16_x2_armv6
_ff_put_pixels16_x2_armv6:
        push {r0-r3, lr}
        bl _ff_put_pixels8_x2_armv6
        pop {r0-r3, lr}
        add r0, r0, #8
        add r1, r1, #8
        b _ff_put_pixels8_x2_armv6

        @.endfunc
        .globl _ff_put_pixels16_y2_armv6

        @.func _ff_put_pixels16_y2_armv6
_ff_put_pixels16_y2_armv6:
        push {r0-r3, lr}
        bl _ff_put_pixels8_y2_armv6
        pop {r0-r3, lr}
        add r0, r0, #8
        add r1, r1, #8
        b _ff_put_pixels8_y2_armv6

        @.endfunc
        .globl _ff_put_pixels16_x2_no_rnd_armv6

        @.func _ff_put_pixels16_x2_no_rnd_armv6
_ff_put_pixels16_x2_no_rnd_armv6:
        push {r0-r3, lr}
        bl _ff_put_pixels8_x2_no_rnd_armv6
        pop {r0-r3, lr}
        add r0, r0, #8
        add r1, r1, #8
        b _ff_put_pixels8_x2_no_rnd_armv6

        @.endfunc
        .globl _ff_put_pixels16_y2_no_rnd_armv6

        @.func _ff_put_pixels16_y2_no_rnd_armv6
_ff_put_pixels16_y2_no_rnd_armv6:
        push {r0-r3, lr}
        bl _ff_put_pixels8_y2_no_rnd_armv6
        pop {r0-r3, lr}
        add r0, r0, #8
        add r1, r1, #8
        b _ff_put_pixels8_y2_no_rnd_armv6

        @.endfunc

        .globl _ff_put_pixels16_armv6

        @.func _ff_put_pixels16_armv6
_ff_put_pixels16_armv6:
        push {r4-r11}
1:
        ldr r5, [r1, #4]
        ldr r6, [r1, #8]
        ldr r7, [r1, #12]
        ldr r4, [r1], r2
        strd r6, r7, [r0, #8]
        ldr r9, [r1, #4]
        strd r4, r5, [r0], r2
        ldr r10, [r1, #8]
        ldr r11, [r1, #12]
        ldr r8, [r1], r2
        strd r10, r11, [r0, #8]
        subs r3, r3, #2
        strd r8, r9, [r0], r2
        bne 1b

        pop {r4-r11}
        bx lr

        @.endfunc

        .globl _ff_put_pixels8_armv6

        @.func _ff_put_pixels8_armv6
_ff_put_pixels8_armv6:
        push {r4-r7}
1:
        ldr r5, [r1, #4]
        ldr r4, [r1], r2
        ldr r7, [r1, #4]
        strd r4, r5, [r0], r2
        ldr r6, [r1], r2
        subs r3, r3, #2
        strd r6, r7, [r0], r2
        bne 1b

        pop {r4-r7}
        bx lr

        @.endfunc

        .globl _ff_put_pixels8_x2_armv6

        @.func _ff_put_pixels8_x2_armv6
_ff_put_pixels8_x2_armv6:
        push {r4-r11, lr}
        mov r12, #1
        orr r12, r12, r12, lsl #8
        orr r12, r12, r12, lsl #16
1:
        ldr r4, [r1]
        subs r3, r3, #2
        ldr r5, [r1, #4]
        ldr r7, [r1, #5]
        lsr r6, r4, #8
        ldr r8, [r1, r2]!
        orr r6, r6, r5, lsl #24
        ldr r9, [r1, #4]
        ldr r11, [r1, #5]
        lsr r10, r8, #8
        add r1, r1, r2
        orr r10, r10, r9, lsl #24
        eor r14, r4, r6
        uhadd8 r4, r4, r6
        eor r6, r5, r7
        uhadd8 r5, r5, r7
        and r14, r14, r12
        and r6, r6, r12
        uadd8 r4, r4, r14
        eor r14, r8, r10
        uadd8 r5, r5, r6
        eor r6, r9, r11
        uhadd8 r8, r8, r10
        and r14, r14, r12
        uhadd8 r9, r9, r11
        and r6, r6, r12
        uadd8 r8, r8, r14
        strd r4, r5, [r0], r2
        uadd8 r9, r9, r6
        strd r8, r9, [r0], r2
        bne 1b

        pop {r4-r11, pc}

        @.endfunc

        .globl _ff_put_pixels8_y2_armv6

        @.func _ff_put_pixels8_y2_armv6
_ff_put_pixels8_y2_armv6:
        push {r4-r11}
        mov r12, #1
        orr r12, r12, r12, lsl #8
        orr r12, r12, r12, lsl #16
        ldr r4, [r1]
        ldr r5, [r1, #4]
        ldr r6, [r1, r2]!
        ldr r7, [r1, #4]
1:
        subs r3, r3, #2
        uhadd8 r8, r4, r6
        eor r10, r4, r6
        uhadd8 r9, r5, r7
        eor r11, r5, r7
        and r10, r10, r12
        ldr r4, [r1, r2]!
        uadd8 r8, r8, r10
        and r11, r11, r12
        uadd8 r9, r9, r11
        ldr r5, [r1, #4]
        uhadd8 r10, r4, r6
        eor r6, r4, r6
        uhadd8 r11, r5, r7
        and r6, r6, r12
        eor r7, r5, r7
        uadd8 r10, r10, r6
        and r7, r7, r12
        ldr r6, [r1, r2]!
        uadd8 r11, r11, r7
        strd r8, r9, [r0], r2
        ldr r7, [r1, #4]
        strd r10, r11, [r0], r2
        bne 1b

        pop {r4-r11}
        bx lr

        @.endfunc

        .globl _ff_put_pixels8_x2_no_rnd_armv6

        @.func _ff_put_pixels8_x2_no_rnd_armv6
_ff_put_pixels8_x2_no_rnd_armv6:
        push {r4-r9, lr}
1:
        subs r3, r3, #2
        ldr r4, [r1]
        ldr r5, [r1, #4]
        ldr r7, [r1, #5]
        ldr r8, [r1, r2]!
        ldr r9, [r1, #4]
        ldr r14, [r1, #5]
        add r1, r1, r2
        lsr r6, r4, #8
        orr r6, r6, r5, lsl #24
        lsr r12, r8, #8
        orr r12, r12, r9, lsl #24
        uhadd8 r4, r4, r6
        uhadd8 r5, r5, r7
        uhadd8 r8, r8, r12
        uhadd8 r9, r9, r14
        stm r0, {r4,r5}
        add r0, r0, r2
        stm r0, {r8,r9}
        add r0, r0, r2
        bne 1b

        pop {r4-r9, pc}

        @.endfunc

        .globl _ff_put_pixels8_y2_no_rnd_armv6

        @.func _ff_put_pixels8_y2_no_rnd_armv6
_ff_put_pixels8_y2_no_rnd_armv6:
        push {r4-r9, lr}
        ldr r4, [r1]
        ldr r5, [r1, #4]
        ldr r6, [r1, r2]!
        ldr r7, [r1, #4]
1:
        subs r3, r3, #2
        uhadd8 r8, r4, r6
        ldr r4, [r1, r2]!
        uhadd8 r9, r5, r7
        ldr r5, [r1, #4]
        uhadd8 r12, r4, r6
        ldr r6, [r1, r2]!
        uhadd8 r14, r5, r7
        ldr r7, [r1, #4]
        stm r0, {r8,r9}
        add r0, r0, r2
        stm r0, {r12,r14}
        add r0, r0, r2
        bne 1b

        pop {r4-r9, pc}

        @.endfunc

        .globl _ff_avg_pixels8_armv6

        @.func _ff_avg_pixels8_armv6
_ff_avg_pixels8_armv6:
        pld [r1, r2]
        push {r4-r10, lr}
        mov lr, #1
        orr lr, lr, lr, lsl #8
        orr lr, lr, lr, lsl #16
        ldrd r4, r5, [r0]
        ldr r10, [r1, #4]
        ldr r9, [r1], r2
        subs r3, r3, #2
1:
        pld [r1, r2]
        eor r8, r4, r9
        uhadd8 r4, r4, r9
        eor r12, r5, r10
        ldrd r6, r7, [r0, r2]
        uhadd8 r5, r5, r10
        and r8, r8, lr
        ldr r10, [r1, #4]
        and r12, r12, lr
        uadd8 r4, r4, r8
        ldr r9, [r1], r2
        eor r8, r6, r9
        uadd8 r5, r5, r12
        pld [r1, r2, lsl #1]
        eor r12, r7, r10
        uhadd8 r6, r6, r9
        strd r4, r5, [r0], r2
        uhadd8 r7, r7, r10
        beq 2f
        and r8, r8, lr
        ldrd r4, r5, [r0, r2]
        uadd8 r6, r6, r8
        ldr r10, [r1, #4]
        and r12, r12, lr
        subs r3, r3, #2
        uadd8 r7, r7, r12
        ldr r9, [r1], r2
        strd r6, r7, [r0], r2
        b 1b
2:
        and r8, r8, lr
        and r12, r12, lr
        uadd8 r6, r6, r8
        uadd8 r7, r7, r12
        strd r6, r7, [r0], r2

        pop {r4-r10, pc}

        @.endfunc

        .globl _ff_add_pixels_clamped_armv6

        @.func _ff_add_pixels_clamped_armv6
_ff_add_pixels_clamped_armv6:
        push {r4-r8,lr}
        mov r3, #8
1:
        ldm r0!, {r4,r5,r12,lr}
        ldrd r6, r7, [r1]
        pkhbt r8, r4, r5, lsl #16
        pkhtb r5, r5, r4, asr #16
        pkhbt r4, r12, lr, lsl #16
        pkhtb lr, lr, r12, asr #16
        pld [r1, r2]
        uxtab16 r8, r8, r6
        uxtab16 r5, r5, r6, ror #8
        uxtab16 r4, r4, r7
        uxtab16 lr, lr, r7, ror #8
        usat16 r8, #8, r8
        usat16 r5, #8, r5
        usat16 r4, #8, r4
        usat16 lr, #8, lr
        orr r6, r8, r5, lsl #8
        orr r7, r4, lr, lsl #8
        subs r3, r3, #1
        strd r6, r7, [r1], r2
        bgt 1b
        pop {r4-r8,pc}

        @.endfunc

        .globl _ff_get_pixels_armv6

        @.func _ff_get_pixels_armv6
_ff_get_pixels_armv6:
        pld [r1, r2]
        push {r4-r8, lr}
        mov lr, #8
1:
        ldrd r4, r5, [r1], r2
        subs lr, lr, #1
        uxtb16 r6, r4
        uxtb16 r4, r4, ror #8
        uxtb16 r12, r5
        uxtb16 r8, r5, ror #8
        pld [r1, r2]
        pkhbt r5, r6, r4, lsl #16
        pkhtb r6, r4, r6, asr #16
        pkhbt r7, r12, r8, lsl #16
        pkhtb r12, r8, r12, asr #16
        stm r0!, {r5,r6,r7,r12}
        bgt 1b

        pop {r4-r8, pc}

        @.endfunc

        .globl _ff_diff_pixels_armv6

        @.func _ff_diff_pixels_armv6
_ff_diff_pixels_armv6:
        pld [r1, r3]
        pld [r2, r3]
        push {r4-r9, lr}
        mov lr, #8
1:
        ldrd r4, r5, [r1], r3
        ldrd r6, r7, [r2], r3
        uxtb16 r8, r4
        uxtb16 r4, r4, ror #8
        uxtb16 r9, r6
        uxtb16 r6, r6, ror #8
        pld [r1, r3]
        ssub16 r9, r8, r9
        ssub16 r6, r4, r6
        uxtb16 r8, r5
        uxtb16 r5, r5, ror #8
        pld [r2, r3]
        pkhbt r4, r9, r6, lsl #16
        pkhtb r6, r6, r9, asr #16
        uxtb16 r9, r7
        uxtb16 r7, r7, ror #8
        ssub16 r9, r8, r9
        ssub16 r5, r5, r7
        subs lr, lr, #1
        pkhbt r8, r9, r5, lsl #16
        pkhtb r9, r5, r9, asr #16
        stm r0!, {r4,r6,r8,r9}
        bgt 1b

        pop {r4-r9, pc}

        @.endfunc

        .globl _ff_pix_abs16_armv6

        @.func _ff_pix_abs16_armv6
_ff_pix_abs16_armv6:
        ldr r0, [sp]
        push {r4-r9, lr}
        mov r12, #0
        mov lr, #0
        ldm r1, {r4-r7}
        ldr r8, [r2]
1:
        ldr r9, [r2, #4]
        pld [r1, r3]
        usada8 r12, r4, r8, r12
        ldr r8, [r2, #8]
        pld [r2, r3]
        usada8 lr, r5, r9, lr
        ldr r9, [r2, #12]
        usada8 r12, r6, r8, r12
        subs r0, r0, #1
        usada8 lr, r7, r9, lr
        beq 2f
        add r1, r1, r3
        ldm r1, {r4-r7}
        add r2, r2, r3
        ldr r8, [r2]
        b 1b
2:
        add r0, r12, lr
        pop {r4-r9, pc}

        @.endfunc

        .globl _ff_pix_abs16_x2_armv6

        @.func _ff_pix_abs16_x2_armv6
_ff_pix_abs16_x2_armv6:
        ldr r12, [sp]
        push {r4-r11, lr}
        mov r0, #0
        mov lr, #1
        orr lr, lr, lr, lsl #8
        orr lr, lr, lr, lsl #16
1:
        ldr r8, [r2]
        ldr r9, [r2, #4]
        lsr r10, r8, #8
        ldr r4, [r1]
        lsr r6, r9, #8
        orr r10, r10, r9, lsl #24
        ldr r5, [r2, #8]
        eor r11, r8, r10
        uhadd8 r7, r8, r10
        orr r6, r6, r5, lsl #24
        and r11, r11, lr
        uadd8 r7, r7, r11
        ldr r8, [r1, #4]
        usada8 r0, r4, r7, r0
        eor r7, r9, r6
        lsr r10, r5, #8
        and r7, r7, lr
        uhadd8 r4, r9, r6
        ldr r6, [r2, #12]
        uadd8 r4, r4, r7
        pld [r1, r3]
        orr r10, r10, r6, lsl #24
        usada8 r0, r8, r4, r0
        ldr r4, [r1, #8]
        eor r11, r5, r10
        ldrb r7, [r2, #16]
        and r11, r11, lr
        uhadd8 r8, r5, r10
        ldr r5, [r1, #12]
        uadd8 r8, r8, r11
        pld [r2, r3]
        lsr r10, r6, #8
        usada8 r0, r4, r8, r0
        orr r10, r10, r7, lsl #24
        subs r12, r12, #1
        eor r11, r6, r10
        add r1, r1, r3
        uhadd8 r9, r6, r10
        and r11, r11, lr
        uadd8 r9, r9, r11
        add r2, r2, r3
        usada8 r0, r5, r9, r0
        bgt 1b

        pop {r4-r11, pc}

        @.endfunc


        .globl _ff_pix_abs16_y2_armv6

        @.func _ff_pix_abs16_y2_armv6
_ff_pix_abs16_y2_armv6:
        pld [r1]
        pld [r2]
        ldr r12, [sp]
        push {r4-r11, lr}
        mov r0, #0
        mov lr, #1
        orr lr, lr, lr, lsl #8
        orr lr, lr, lr, lsl #16
        ldr r4, [r2]
        ldr r5, [r2, #4]
        ldr r6, [r2, #8]
        ldr r7, [r2, #12]
        add r2, r2, r3
1:
        ldr r8, [r2]
        eor r9, r4, r8
        uhadd8 r4, r4, r8
        and r9, r9, lr
        ldr r10, [r1]
        uadd8 r4, r4, r9
        ldr r9, [r2, #4]
        usada8 r0, r4, r10, r0
        pld [r1, r3]
        eor r11, r5, r9
        uhadd8 r5, r5, r9
        and r11, r11, lr
        ldr r4, [r1, #4]
        uadd8 r5, r5, r11
        ldr r10, [r2, #8]
        usada8 r0, r5, r4, r0
        pld [r2, r3]
        eor r4, r6, r10
        uhadd8 r6, r6, r10
        and r4, r4, lr
        ldr r5, [r1, #8]
        uadd8 r6, r6, r4
        ldr r11, [r2, #12]
        usada8 r0, r6, r5, r0
        eor r5, r7, r11
        uhadd8 r7, r7, r11
        and r5, r5, lr
        ldr r4, [r1, #12]
        uadd8 r7, r7, r5
        add r1, r1, r3
        usada8 r0, r7, r4, r0
        add r2, r2, r3
        subs r12, r12, #2
        ldr r4, [r2]
        eor r5, r8, r4
        uhadd8 r8, r8, r4
        and r5, r5, lr
        ldr r6, [r1]
        uadd8 r8, r8, r5
        ldr r5, [r2, #4]
        usada8 r0, r8, r6, r0
        pld [r1, r3]
        eor r7, r9, r5
        uhadd8 r9, r9, r5
        and r7, r7, lr
        ldr r8, [r1, #4]
        uadd8 r9, r9, r7
        ldr r6, [r2, #8]
        usada8 r0, r9, r8, r0
        pld [r2, r3]
        eor r8, r10, r6
        uhadd8 r10, r10, r6
        and r8, r8, lr
        ldr r9, [r1, #8]
        uadd8 r10, r10, r8
        ldr r7, [r2, #12]
        usada8 r0, r10, r9, r0
        eor r9, r11, r7
        uhadd8 r11, r11, r7
        and r9, r9, lr
        ldr r8, [r1, #12]
        uadd8 r11, r11, r9
        add r1, r1, r3
        usada8 r0, r11, r8, r0
        add r2, r2, r3
        bgt 1b

        pop {r4-r11, pc}

        @.endfunc

        .globl _ff_pix_abs8_armv6

        @.func _ff_pix_abs8_armv6
_ff_pix_abs8_armv6:
        pld [r2, r3]
        ldr r12, [sp]
        push {r4-r9, lr}
        mov r0, #0
        mov lr, #0
        ldrd r4, r5, [r1], r3
1:
        subs r12, r12, #2
        ldr r7, [r2, #4]
        ldr r6, [r2], r3
        ldrd r8, r9, [r1], r3
        usada8 r0, r4, r6, r0
        pld [r2, r3]
        usada8 lr, r5, r7, lr
        ldr r7, [r2, #4]
        ldr r6, [r2], r3
        beq 2f
        ldrd r4, r5, [r1], r3
        usada8 r0, r8, r6, r0
        pld [r2, r3]
        usada8 lr, r9, r7, lr
        b 1b
2:
        usada8 r0, r8, r6, r0
        usada8 lr, r9, r7, lr
        add r0, r0, lr
        pop {r4-r9, pc}

        @.endfunc

        .globl _ff_sse16_armv6

        @.func _ff_sse16_armv6
_ff_sse16_armv6:
        ldr r12, [sp]
        push {r4-r9, lr}
        mov r0, #0
1:
        ldrd r4, r5, [r1]
        ldr r8, [r2]
        uxtb16 lr, r4
        uxtb16 r4, r4, ror #8
        uxtb16 r9, r8
        uxtb16 r8, r8, ror #8
        ldr r7, [r2, #4]
        usub16 lr, lr, r9
        usub16 r4, r4, r8
        smlad r0, lr, lr, r0
        uxtb16 r6, r5
        uxtb16 lr, r5, ror #8
        uxtb16 r8, r7
        uxtb16 r9, r7, ror #8
        smlad r0, r4, r4, r0
        ldrd r4, r5, [r1, #8]
        usub16 r6, r6, r8
        usub16 r8, lr, r9
        ldr r7, [r2, #8]
        smlad r0, r6, r6, r0
        uxtb16 lr, r4
        uxtb16 r4, r4, ror #8
        uxtb16 r9, r7
        uxtb16 r7, r7, ror #8
        smlad r0, r8, r8, r0
        ldr r8, [r2, #12]
        usub16 lr, lr, r9
        usub16 r4, r4, r7
        smlad r0, lr, lr, r0
        uxtb16 r6, r5
        uxtb16 r5, r5, ror #8
        uxtb16 r9, r8
        uxtb16 r8, r8, ror #8
        smlad r0, r4, r4, r0
        usub16 r6, r6, r9
        usub16 r5, r5, r8
        smlad r0, r6, r6, r0
        add r1, r1, r3
        add r2, r2, r3
        subs r12, r12, #1
        smlad r0, r5, r5, r0
        bgt 1b

        pop {r4-r9, pc}

        @.endfunc

        .globl _ff_pix_norm1_armv6

        @.func _ff_pix_norm1_armv6
_ff_pix_norm1_armv6:
        push {r4-r6, lr}
        mov r12, #16
        mov lr, #0
1:
        ldm r0, {r2-r5}
        uxtb16 r6, r2
        uxtb16 r2, r2, ror #8
        smlad lr, r6, r6, lr
        uxtb16 r6, r3
        smlad lr, r2, r2, lr
        uxtb16 r3, r3, ror #8
        smlad lr, r6, r6, lr
        uxtb16 r6, r4
        smlad lr, r3, r3, lr
        uxtb16 r4, r4, ror #8
        smlad lr, r6, r6, lr
        uxtb16 r6, r5
        smlad lr, r4, r4, lr
        uxtb16 r5, r5, ror #8
        smlad lr, r6, r6, lr
        subs r12, r12, #1
        add r0, r0, r1
        smlad lr, r5, r5, lr
        bgt 1b

        mov r0, lr
        pop {r4-r6, pc}

        @.endfunc

        .globl _ff_pix_sum_armv6

        @.func _ff_pix_sum_armv6
_ff_pix_sum_armv6:
        push {r4-r7, lr}
        mov r12, #16
        mov r2, #0
        mov r3, #0
        mov lr, #0
        ldr r4, [r0]
1:
        subs r12, r12, #1
        ldr r5, [r0, #4]
        usada8 r2, r4, lr, r2
        ldr r6, [r0, #8]
        usada8 r3, r5, lr, r3
        ldr r7, [r0, #12]
        usada8 r2, r6, lr, r2
        beq 2f
        ldr r4, [r0, r1]!
        usada8 r3, r7, lr, r3
        bgt 1b
2:
        usada8 r3, r7, lr, r3
        add r0, r2, r3
        pop {r4-r7, pc}

        @.endfunc
.text
