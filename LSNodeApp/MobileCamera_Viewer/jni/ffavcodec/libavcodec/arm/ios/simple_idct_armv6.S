
        .text
        .align
w13: .long (22725 | (19266 << 16))
w26: .long (21407 | (8867 << 16))
w42: .long (16383 | (21407 << 16))
w42n: .long (-16383&0xffff | (-21407 << 16))
w46: .long (16383 | (8867 << 16))
w57: .long (12873 | (4520 << 16))


        @.func idct_row_armv6
idct_row_armv6:
        push {lr}

        ldr lr, [r0, #12]
        ldr ip, [r0, #4]
        ldr r3, [r0, #8]
        ldr r2, [r0]
        orrs lr, lr, ip
        cmpeq lr, r3
        cmpeq lr, r2, lsr #16
        beq 1f
        push {r1}
        ldr ip, w42
        cmp lr, #0
        beq 2f

        ldr lr, w46
        mov r1, #(1<<(11-1))
        smlad r4, r2, ip, r1
        smlsd r7, r2, ip, r1
        ldr ip, w13
        ldr r10,w57
        smlad r5, r2, lr, r1
        smlsd r6, r2, lr, r1

        smuad r8, r3, ip
        smusdx r11,r3, r10
        ldr lr, [r0, #12]
        pkhtb r2, ip, r10,asr #16
        pkhbt r1, ip, r10,lsl #16
        smusdx r9, r2, r3
        smlad r8, lr, r10,r8
        smusdx r10,r3, r1

        ldr r3, w42n
        smlad r10,lr, r2, r10
        ldr r2, [r0, #4]
        smlsdx r11,lr, ip, r11
        ldr ip, w46
        smlad r9, lr, r1, r9

        smlad r5, r2, r3, r5
        smlsd r6, r2, r3, r6
        smlad r4, r2, ip, r4
        smlsd r7, r2, ip, r7
        b 3f

2:        ldr lr, w46
        ldr r10,w57
        mov r1, #(1<<(11-1))
        smlad r4, r2, ip, r1
        smlsd r7, r2, ip, r1
        ldr ip, w13
        smlad r5, r2, lr, r1
        smlsd r6, r2, lr, r1
        smusdx r11,r3, r10
        smuad r8, r3, ip
        pkhtb r2, ip, r10,asr #16
        pkhbt r1, ip, r10,lsl #16
        smusdx r9, r2, r3
        smusdx r10,r3, r1

3: pop {r1}
        add r3, r4, r8
        sub r2, r4, r8
        mov r4, r3, asr #11
        mov r8, r2, asr #11

        sub r3, r5, r9
        add r2, r5, r9
        mov r5, r3, asr #11
        mov r9, r2, asr #11

        add r3, r6, r10
        sub r2, r6, r10
        mov r6, r3, asr #11
        mov r10,r2, asr #11

        add r3, r7, r11
        sub r2, r7, r11
        mov r7, r3, asr #11
        mov r11,r2, asr #11

        strh r4, [r1]
        strh r5, [r1, #(16*2)]
        strh r6, [r1, #(16*4)]
        strh r7, [r1, #(16*6)]
        strh r11,[r1, #(16*1)]
        strh r10,[r1, #(16*3)]
        strh r9, [r1, #(16*5)]
        strh r8, [r1, #(16*7)]

        pop {pc}

1: mov r2, r2, lsl #3
        strh r2, [r1]
        strh r2, [r1, #(16*2)]
        strh r2, [r1, #(16*4)]
        strh r2, [r1, #(16*6)]
        strh r2, [r1, #(16*1)]
        strh r2, [r1, #(16*3)]
        strh r2, [r1, #(16*5)]
        strh r2, [r1, #(16*7)]
        pop {pc}

        @.endfunc







        @.func idct_col_armv6
idct_col_armv6:
        push {r1, lr}

        ldr r2, [r0]
        ldr ip, w42
        ldr r3, [r0, #8]
        ldr lr, w46
        mov r1, #(1<<(20-1))
        smlad r4, r2, ip, r1
        smlsd r7, r2, ip, r1
        ldr ip, w13
        ldr r10,w57
        smlad r5, r2, lr, r1
        smlsd r6, r2, lr, r1

        smuad r8, r3, ip
        smusdx r11,r3, r10
        ldr lr, [r0, #12]
        pkhtb r2, ip, r10,asr #16
        pkhbt r1, ip, r10,lsl #16
        smusdx r9, r2, r3
        smlad r8, lr, r10,r8
        smusdx r10,r3, r1

        ldr r3, w42n
        smlad r10,lr, r2, r10
        ldr r2, [r0, #4]
        smlsdx r11,lr, ip, r11
        ldr ip, w46
        smlad r9, lr, r1, r9

        smlad r5, r2, r3, r5
        smlsd r6, r2, r3, r6
        smlad r4, r2, ip, r4
        smlsd r7, r2, ip, r7
        pop {r1}
        add r3, r4, r8
        sub r2, r4, r8
        mov r4, r3, asr #20
        mov r8, r2, asr #20

        sub r3, r5, r9
        add r2, r5, r9
        mov r5, r3, asr #20
        mov r9, r2, asr #20

        add r3, r6, r10
        sub r2, r6, r10
        mov r6, r3, asr #20
        mov r10,r2, asr #20

        add r3, r7, r11
        sub r2, r7, r11
        mov r7, r3, asr #20
        mov r11,r2, asr #20

        strh r4, [r1]
        strh r5, [r1, #(16*1)]
        strh r6, [r1, #(16*2)]
        strh r7, [r1, #(16*3)]
        strh r11,[r1, #(16*4)]
        strh r10,[r1, #(16*5)]
        strh r9, [r1, #(16*6)]
        strh r8, [r1, #(16*7)]

        pop {pc}

        @.endfunc








        @.func idct_col_put_armv6
idct_col_put_armv6:
        push {r1, r2, lr}

        ldr r2, [r0]
        ldr ip, w42
        ldr r3, [r0, #8]
        ldr lr, w46
        mov r1, #(1<<(20-1))
        smlad r4, r2, ip, r1
        smlsd r7, r2, ip, r1
        ldr ip, w13
        ldr r10,w57
        smlad r5, r2, lr, r1
        smlsd r6, r2, lr, r1

        smuad r8, r3, ip
        smusdx r11,r3, r10
        ldr lr, [r0, #12]
        pkhtb r2, ip, r10,asr #16
        pkhbt r1, ip, r10,lsl #16
        smusdx r9, r2, r3
        smlad r8, lr, r10,r8
        smusdx r10,r3, r1

        ldr r3, w42n
        smlad r10,lr, r2, r10
        ldr r2, [r0, #4]
        smlsdx r11,lr, ip, r11
        ldr ip, w46
        smlad r9, lr, r1, r9

        smlad r5, r2, r3, r5
        smlsd r6, r2, r3, r6
        smlad r4, r2, ip, r4
        smlsd r7, r2, ip, r7
        pop {r1, r2}
        add r3, r4, r8
        sub ip, r4, r8
        usat r4, #8, r3, asr #20
        usat r8, #8, ip, asr #20

        sub r3, r5, r9
        add ip, r5, r9
        usat r5, #8, r3, asr #20
        usat r9, #8, ip, asr #20

        add r3, r6, r10
        sub ip, r6, r10
        usat r6, #8, r3, asr #20
        usat r10,#8, ip, asr #20

        add r3, r7, r11
        sub ip, r7, r11
        usat r7, #8, r3, asr #20
        usat r11,#8, ip, asr #20

        strb r4, [r1], r2
        strb r5, [r1], r2
        strb r6, [r1], r2
        strb r7, [r1], r2
        strb r11,[r1], r2
        strb r10,[r1], r2
        strb r9, [r1], r2
        strb r8, [r1], r2

        sub r1, r1, r2, lsl #3

        pop {pc}

        @.endfunc








        @.func idct_col_add_armv6
idct_col_add_armv6:
        push {r1, r2, lr}

        ldr r2, [r0]
        ldr ip, w42
        ldr r3, [r0, #8]
        ldr lr, w46
        mov r1, #(1<<(20-1))
        smlad r4, r2, ip, r1
        smlsd r7, r2, ip, r1
        ldr ip, w13
        ldr r10,w57
        smlad r5, r2, lr, r1
        smlsd r6, r2, lr, r1

        smuad r8, r3, ip
        smusdx r11,r3, r10
        ldr lr, [r0, #12]
        pkhtb r2, ip, r10,asr #16
        pkhbt r1, ip, r10,lsl #16
        smusdx r9, r2, r3
        smlad r8, lr, r10,r8
        smusdx r10,r3, r1

        ldr r3, w42n
        smlad r10,lr, r2, r10
        ldr r2, [r0, #4]
        smlsdx r11,lr, ip, r11
        ldr ip, w46
        smlad r9, lr, r1, r9

        smlad r5, r2, r3, r5
        smlsd r6, r2, r3, r6
        smlad r4, r2, ip, r4
        smlsd r7, r2, ip, r7
        pop {r1, r2}
        add ip, r4, r8
        sub lr, r4, r8
        sub r4, r5, r9
        add r8, r5, r9
        add r5, r6, r10
        sub r9, r6, r10
        add r6, r7, r11
        sub r10,r7, r11

        ldrb r3, [r1]
        ldrb r7, [r1, r2]
        ldrb r11,[r1, r2, lsl #2]
        add ip, r3, ip, asr #20
        usat ip, #8, ip
        add r4, r7, r4, asr #20
        strb ip, [r1], r2
        ldrb ip, [r1, r2]
        usat r4, #8, r4
        ldrb r11,[r1, r2, lsl #2]
        add r5, ip, r5, asr #20
        usat r5, #8, r5
        strb r4, [r1], r2
        ldrb r3, [r1, r2]
        ldrb ip, [r1, r2, lsl #2]
        strb r5, [r1], r2
        ldrb r7, [r1, r2]
        ldrb r4, [r1, r2, lsl #2]
        add r6, r3, r6, asr #20
        usat r6, #8, r6
        add r10,r7, r10,asr #20
        usat r10,#8, r10
        add r9, r11,r9, asr #20
        usat r9, #8, r9
        add r8, ip, r8, asr #20
        usat r8, #8, r8
        add lr, r4, lr, asr #20
        usat lr, #8, lr
        strb r6, [r1], r2
        strb r10,[r1], r2
        strb r9, [r1], r2
        strb r8, [r1], r2
        strb lr, [r1], r2

        sub r1, r1, r2, lsl #3

        pop {pc}

        @.endfunc








        .globl _ff_simple_idct_armv6

        @.func _ff_simple_idct_armv6
_ff_simple_idct_armv6:
        push {r4-r11, lr}
        sub sp, sp, #128

        mov r1, sp
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        sub r0, r0, #(16*5)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6

        sub r0, r0, #(16*7)
        mov r1, r0
        mov r0, sp
        bl idct_col_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_col_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_col_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_col_armv6
        sub r0, r0, #(16*5)
        add r1, r1, #2
        bl idct_col_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_col_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_col_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_col_armv6

        sub r0, r0, #(16*7)

        add sp, sp, #128
        pop {r4-r11, pc}

        @.endfunc


        .globl _ff_simple_idct_add_armv6

        @.func _ff_simple_idct_add_armv6
_ff_simple_idct_add_armv6:
        push {r0, r1, r4-r11, lr}
        sub sp, sp, #128

        mov r0, r2
        mov r1, sp
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        sub r0, r0, #(16*5)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6

        sub r0, r0, #(16*7)
        mov r0, sp
        ldr r1, [sp, #128]
        ldr r2, [sp, #(128+4)]
        bl idct_col_add_armv6
        add r0, r0, #(16*2)
        add r1, r1, #1
        bl idct_col_add_armv6
        add r0, r0, #(16*2)
        add r1, r1, #1
        bl idct_col_add_armv6
        add r0, r0, #(16*2)
        add r1, r1, #1
        bl idct_col_add_armv6
        sub r0, r0, #(16*5)
        add r1, r1, #1
        bl idct_col_add_armv6
        add r0, r0, #(16*2)
        add r1, r1, #1
        bl idct_col_add_armv6
        add r0, r0, #(16*2)
        add r1, r1, #1
        bl idct_col_add_armv6
        add r0, r0, #(16*2)
        add r1, r1, #1
        bl idct_col_add_armv6

        sub r0, r0, #(16*7)

        add sp, sp, #(128+8)
        pop {r4-r11, pc}

        @.endfunc


        .globl _ff_simple_idct_put_armv6

        @.func _ff_simple_idct_put_armv6
_ff_simple_idct_put_armv6:
        push {r0, r1, r4-r11, lr}
        sub sp, sp, #128

        mov r0, r2
        mov r1, sp
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        sub r0, r0, #(16*5)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6
        add r0, r0, #(16*2)
        add r1, r1, #2
        bl idct_row_armv6

        sub r0, r0, #(16*7)
        mov r0, sp
        ldr r1, [sp, #128]
        ldr r2, [sp, #(128+4)]
        bl idct_col_put_armv6
        add r0, r0, #(16*2)
        add r1, r1, #1
        bl idct_col_put_armv6
        add r0, r0, #(16*2)
        add r1, r1, #1
        bl idct_col_put_armv6
        add r0, r0, #(16*2)
        add r1, r1, #1
        bl idct_col_put_armv6
        sub r0, r0, #(16*5)
        add r1, r1, #1
        bl idct_col_put_armv6
        add r0, r0, #(16*2)
        add r1, r1, #1
        bl idct_col_put_armv6
        add r0, r0, #(16*2)
        add r1, r1, #1
        bl idct_col_put_armv6
        add r0, r0, #(16*2)
        add r1, r1, #1
        bl idct_col_put_armv6

        sub r0, r0, #(16*7)

        add sp, sp, #(128+8)
        pop {r4-r11, pc}

        @.endfunc
.text
