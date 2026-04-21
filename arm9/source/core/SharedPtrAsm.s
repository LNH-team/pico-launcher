.section .itcm
.arm

// r0 = &refCount
.global shared_ptr_increase_ref_count
.type shared_ptr_increase_ref_count, %function
shared_ptr_increase_ref_count:
    mrs r2, cpsr
    orr r1, r2, #0x80
    msr cpsr_c, r1
    ldr r12, [r0]
    add r12, r12, #1
    str r12, [r0]
    msr cpsr_c, r2
    bx lr
