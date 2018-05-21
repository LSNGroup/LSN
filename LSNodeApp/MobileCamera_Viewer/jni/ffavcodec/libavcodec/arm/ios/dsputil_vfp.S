
        .syntax unified
        .globl _ff_vector_fmul_vfp

        @.func _ff_vector_fmul_vfp
_ff_vector_fmul_vfp:
        vpush {d8-d15}
        mov r3, r0
        fmrx r12, fpscr
        orr r12, r12, #(3 << 16)
        fmxr fpscr, r12

        vldmia r3!, {s0-s3}
        vldmia r1!, {s8-s11}
        vldmia r3!, {s4-s7}
        vldmia r1!, {s12-s15}
        vmul.f32 s8, s0, s8
1:
        subs r2, r2, #16
        vmul.f32 s12, s4, s12
        vldmiage r3!, {s16-s19}
        vldmiage r1!, {s24-s27}
        vldmiage r3!, {s20-s23}
        vldmiage r1!, {s28-s31}
        vmulge.f32 s24, s16, s24
        vstmia r0!, {s8-s11}
        vstmia r0!, {s12-s15}
        vmulge.f32 s28, s20, s28
        vldmiagt r3!, {s0-s3}
        vldmiagt r1!, {s8-s11}
        vldmiagt r3!, {s4-s7}
        vldmiagt r1!, {s12-s15}
        vmulge.f32 s8, s0, s8
        vstmiage r0!, {s24-s27}
        vstmiage r0!, {s28-s31}
        bgt 1b

        bic r12, r12, #(7 << 16)
        fmxr fpscr, r12
        vpop {d8-d15}
        bx lr

        @.endfunc







        .globl _ff_vector_fmul_reverse_vfp

        @.func _ff_vector_fmul_reverse_vfp
_ff_vector_fmul_reverse_vfp:
        vpush {d8-d15}
        add r2, r2, r3, lsl #2
        vldmdb r2!, {s0-s3}
        vldmia r1!, {s8-s11}
        vldmdb r2!, {s4-s7}
        vldmia r1!, {s12-s15}
        vmul.f32 s8, s3, s8
        vmul.f32 s9, s2, s9
        vmul.f32 s10, s1, s10
        vmul.f32 s11, s0, s11
1:
        subs r3, r3, #16
        vldmdbge r2!, {s16-s19}
        vmul.f32 s12, s7, s12
        vldmiage r1!, {s24-s27}
        vmul.f32 s13, s6, s13
        vldmdbge r2!, {s20-s23}
        vmul.f32 s14, s5, s14
        vldmiage r1!, {s28-s31}
        vmul.f32 s15, s4, s15
        vmulge.f32 s24, s19, s24
        vldmdbgt r2!, {s0-s3}
        vmulge.f32 s25, s18, s25
        vstmia r0!, {s8-s13}
        vmulge.f32 s26, s17, s26
        vldmiagt r1!, {s8-s11}
        vmulge.f32 s27, s16, s27
        vmulge.f32 s28, s23, s28
        vldmdbgt r2!, {s4-s7}
        vmulge.f32 s29, s22, s29
        vstmia r0!, {s14-s15}
        vmulge.f32 s30, s21, s30
        vmulge.f32 s31, s20, s31
        vmulge.f32 s8, s3, s8
        vldmiagt r1!, {s12-s15}
        vmulge.f32 s9, s2, s9
        vmulge.f32 s10, s1, s10
        vstmiage r0!, {s24-s27}
        vmulge.f32 s11, s0, s11
        vstmiage r0!, {s28-s31}
        bgt 1b

        vpop {d8-d15}
        bx lr

        @.endfunc
# 143 "/Volumes/MC_Projects-1/000iOS/MobileCamera/../../../MC_Projects-1/MobileCamera/jni/ffavcodec/libavcodec/arm/dsputil_vfp.S"

        .globl _ff_float_to_int16_vfp

        @.func _ff_float_to_int16_vfp
_ff_float_to_int16_vfp:
        push {r4-r8,lr}
        vpush {d8-d11}
        vldmia r1!, {s16-s23}
        vcvt.s32.f32 s0, s16
        vcvt.s32.f32 s1, s17
        vcvt.s32.f32 s2, s18
        vcvt.s32.f32 s3, s19
        vcvt.s32.f32 s4, s20
        vcvt.s32.f32 s5, s21
        vcvt.s32.f32 s6, s22
        vcvt.s32.f32 s7, s23
1:
        subs r2, r2, #8
        vmov r3, r4, s0, s1
        vmov r5, r6, s2, s3
        vmov r7, r8, s4, s5
        vmov ip, lr, s6, s7
        vldmiagt r1!, {s16-s23}
        ssat r4, #16, r4
        ssat r3, #16, r3
        ssat r6, #16, r6
        ssat r5, #16, r5
        pkhbt r3, r3, r4, lsl #16
        pkhbt r4, r5, r6, lsl #16
        vcvtgt.s32.f32 s0, s16
        vcvtgt.s32.f32 s1, s17
        vcvtgt.s32.f32 s2, s18
        vcvtgt.s32.f32 s3, s19
        vcvtgt.s32.f32 s4, s20
        vcvtgt.s32.f32 s5, s21
        vcvtgt.s32.f32 s6, s22
        vcvtgt.s32.f32 s7, s23
        ssat r8, #16, r8
        ssat r7, #16, r7
        ssat lr, #16, lr
        ssat ip, #16, ip
        pkhbt r5, r7, r8, lsl #16
        pkhbt r6, ip, lr, lsl #16
        stmia r0!, {r3-r6}
        bgt 1b

        vpop {d8-d11}
        pop {r4-r8,pc}

        @.endfunc
.text
