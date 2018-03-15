
        .globl _ff_dct_unquantize_h263_armv5te

        @.func _ff_dct_unquantize_h263_armv5te
_ff_dct_unquantize_h263_armv5te:
        push {r4-r9,lr}
        mov ip, #0
        subs r3, r3, #2
        ble 2f
        ldrd r4, [r0, #0]
1:
        ldrd r6, [r0, #8]

        rsbs r9, ip, r4, asr #16
        addgt r9, r2, #0
        rsblt r9, r2, #0
        smlatbne r9, r4, r1, r9

        rsbs lr, ip, r5, asr #16
        addgt lr, r2, #0
        rsblt lr, r2, #0
        smlatbne lr, r5, r1, lr

        rsbs r8, ip, r4, asl #16
        addgt r8, r2, #0
        rsblt r8, r2, #0
        smlabbne r4, r4, r1, r8

        rsbs r8, ip, r5, asl #16
        addgt r8, r2, #0
        rsblt r8, r2, #0
        smlabbne r5, r5, r1, r8

        strh r4, [r0], #2
        strh r9, [r0], #2
        strh r5, [r0], #2
        strh lr, [r0], #2

        rsbs r9, ip, r6, asr #16
        addgt r9, r2, #0
        rsblt r9, r2, #0
        smlatbne r9, r6, r1, r9

        rsbs lr, ip, r7, asr #16
        addgt lr, r2, #0
        rsblt lr, r2, #0
        smlatbne lr, r7, r1, lr

        rsbs r8, ip, r6, asl #16
        addgt r8, r2, #0
        rsblt r8, r2, #0
        smlabbne r6, r6, r1, r8

        rsbs r8, ip, r7, asl #16
        addgt r8, r2, #0
        rsblt r8, r2, #0
        smlabbne r7, r7, r1, r8

        strh r6, [r0], #2
        strh r9, [r0], #2
        strh r7, [r0], #2
        strh lr, [r0], #2

        subs r3, r3, #8
        ldrgtd r4, [r0, #0]
        bgt 1b

        adds r3, r3, #2
        pople {r4-r9,pc}
2:
        ldrsh r9, [r0, #0]
        ldrsh lr, [r0, #2]
        mov r8, r2
        cmp r9, #0
        rsblt r8, r2, #0
        smlabbne r9, r9, r1, r8
        mov r8, r2
        cmp lr, #0
        rsblt r8, r2, #0
        smlabbne lr, lr, r1, r8
        strh r9, [r0], #2
        strh lr, [r0], #2
        pop {r4-r9,pc}

        @.endfunc
.text
