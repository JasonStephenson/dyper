#pragma once

#define VMCB_SIZE 4096
#define HSAVE_SIZE 4096


/* Vol2 - Page 737 */
typedef struct _VMCB_CONTROL_AREA {
    
    /* Vector 0 */
    unsigned int CR0_READ : 16;
    unsigned int CR0_WRITE : 16;
    
    /* Vector 1 */
    unsigned int DR0_READ : 16;
    unsigned int DR0_WRITE : 16;
    
    /* Vector 2 */
    unsigned int exceptionVectors;
    
    /* Vector 3 */
    unsigned int INTR : 1;
    unsigned int NMI : 1;
    unsigned int SMI : 1;
    unsigned int INIT: 1;
    unsigned int VINTR: 1;
    unsigned int CR0_OTHER_WRITE: 1;
    unsigned int IDTR_READ: 1;
    unsigned int GDTR_READ : 1;
    unsigned int LDTR_READ : 1;
    unsigned int TR_READ : 1;
    unsigned int IDTR_WRITE: 1;
    unsigned int GDTR_WRITE : 1;
    unsigned int LDTR_WRITE : 1;
    unsigned int TR_WRITE : 1;
    unsigned int RDTMC : 1;
    unsigned int RDPMC : 1;
    unsigned int PUSHF : 1;
    unsigned int POPF : 1;
    unsigned int CPUID : 1;
    unsigned int RSM : 1;
    unsigned int IRET : 1;
    unsigned int INTN : 1;
    unsigned int INVD : 1;
    unsigned int PAUSE : 1;
    unsigned int HLT : 1;
    unsigned int INVLPG : 1;
    unsigned int INVLPGA : 1;
    unsigned int IOIO_PROT : 1;
    unsigned int MSR_PROT : 1;
    unsigned int TASK_SWITCH : 1;
    unsigned int FERR_FREEZE : 1;
    unsigned int SHUTDOWN_EVENTS : 1;
    
    /* Vector 4 */
    unsigned int VMRUN : 1;
    unsigned int VMCALL : 1;
    unsigned int VMLOAD : 1;
    unsigned int VMSAVE : 1;
    unsigned int STGI : 1;
    unsigned int CLGI : 1;
    unsigned int SKINIT : 1;
    unsigned int RDTSCP : 1;
    unsigned int ICEBP : 1;
    unsigned int WBINVD_WBNOINVD : 1;
    unsigned int MONITORX : 1;
    unsigned int MWAIT_MWAITX_UNCONDITIONAL : 1;
    unsigned int MWAIT_MWAITX_ARMED : 1;
    unsigned int XSETBV : 1;
    unsigned int RDPRU : 1;
    unsigned int EFER : 1;
    unsigned int CR : 16;
    
    /* Vector 5 */
    unsigned int INVLPGB : 1;
    unsigned int INVLPGB_ILLEGAL : 1;
    unsigned int INVPCID : 1;
    unsigned int MCOMMIT : 1;
    unsigned int TLBSYNC : 1;
    unsigned int BUSLOCK : 1;
    unsigned int HLT : 1;
    unsigned int RESERVED0 : 25;

    unsigned char RESERVED1[36];

    unsigned short pauseFilterThreshold;
    unsigned short pauseFilterCount;

    unsigned __int64 IOPM_BASE_PA;
    unsigned __int64 MSRPM_BASE_PA;
    unsigned __int64 TSC_OFFSET;

    unsigned __int64 GUEST_ASID : 32;
    unsigned __int64 TLB_CONTROL : 8;
    unsigned __int64 ALLOW_LARGER_RAP : 1;
    unsigned __int64 CLEAR_RAP : 1;
    unsigned __int64 RESERVED2 : 22;

    /* 0x60h */
    //JSTODO: CONTINUE


}VMCB_CONTROL_AREA, * PVMCB_CONTROL_AREA;

typedef struct _VM_STATE {
    /* Virtual machine control block */
    void* pVMCB;

    /* VM_HSAVE_PA MSR (C007_0117h) */
    void* pHSave;
}VM_STATE, * PVM_STATE;