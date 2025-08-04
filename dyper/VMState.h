#pragma once


#define VMCB_SIZE 4096
#define HSAVE_SIZE 4096

typedef struct _VM_STATE {
    /* Virtual machine control block */
    void* pVMCB;

    /* VM_HSAVE_PA MSR (C007_0117h) */
    void* pHSave;
}VM_STATE, * PVM_STATE;