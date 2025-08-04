#include "stdafx.h"
#include "VMState.h"

#define POOL_TAG 'rvrD'

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD DriverUnload;
EVT_WDF_DEVICE_FILE_CREATE WdfDeviceFileCreate;
EVT_WDF_FILE_CLOSE WdfDeviceFileClose;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL WdfQueueDeviceIoControl;
EVT_WDF_IO_QUEUE_IO_WRITE WdfQueueWrite;
EVT_WDF_IO_QUEUE_IO_READ WdfQueueRead;
EVT_WDF_OBJECT_CONTEXT_CLEANUP WdfDeviceContextCleanup;

extern void EnableSvme();
extern void DisableSvme();
extern void SetHSave(ULONG_PTR physicalAddress);

extern BOOLEAN SupportCheckMSR();
extern BOOLEAN SupportCheckIsAMD();
extern BOOLEAN SupportCheckCanEnableSVM();

typedef struct _DEVICE_CONTEXT
{
    VM_STATE* pGuestStates;
    ULONG totalProcessorCount;
}DEVICE_CONTEXT, *PDEVICE_CONTEXTE;
WDF_DECLARE_CONTEXT_TYPE(DEVICE_CONTEXT);

BOOLEAN PrintAndConfirmCpuSuport() {
    BOOLEAN isAmd = SupportCheckIsAMD();
    DbgPrintInfo("IsAMD: %d\n", isAmd);

    /* If its not AMD no point checking the rest */
    if (!isAmd) {
        return FALSE;
    }

    /* Need to be able to use RDMSR for other checks to be ok */
    BOOLEAN msrOk = SupportCheckMSR();
    DbgPrintInfo("RSMSR/WRMSR: %d\n", msrOk);
    if (!msrOk) {
        return FALSE;
    }

    BOOLEAN svmOk = SupportCheckCanEnableSVM();
    DbgPrintInfo("Can enable SVM: %d\n", svmOk);

    return svmOk;
}

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    if (PrintAndConfirmCpuSuport() == FALSE) {
        DbgPrintErr("CPU does not meet requirements\n");
        return STATUS_NOT_SUPPORTED;
    }

    /* Create a driver object */
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, NULL);
    config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
    config.EvtDriverUnload = DriverUnload;

    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    WDFDRIVER wdfDriver;
    NTSTATUS status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, &wdfDriver);
    if (!NT_SUCCESS(status))
    {
        DbgPrintErr("Failed to create driver: 0x%X\n", status);
        return status;
    }

    /* Create a device object */
    PWDFDEVICE_INIT pDevice = WdfControlDeviceInitAllocate(wdfDriver, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL);
    if (!pDevice)
    {
        DbgPrintErr("Failed to WdfControlDeviceInitAllocate: 0x%X\n", status);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DECLARE_CONST_UNICODE_STRING(deviceName, L"\\Device\\Dyper");
    status = WdfDeviceInitAssignName(pDevice, &deviceName);
    if (!NT_SUCCESS(status))
    {
        DbgPrintErr("Failed to assign device name: 0x%X\n", status);
        WdfDeviceInitFree(pDevice);
        return status;
    }

    WdfDeviceInitSetCharacteristics(pDevice, FILE_DEVICE_SECURE_OPEN, FALSE);
    WdfDeviceInitSetExclusive(pDevice, TRUE);

    WDF_FILEOBJECT_CONFIG fileEventCallbacks;
    WDF_FILEOBJECT_CONFIG_INIT(&fileEventCallbacks, WdfDeviceFileCreate, WdfDeviceFileClose, WDF_NO_EVENT_CALLBACK);
    WdfDeviceInitSetFileObjectConfig(pDevice, &fileEventCallbacks, WDF_NO_OBJECT_ATTRIBUTES);

    /* Associate a Device context */
    WDF_OBJECT_ATTRIBUTES objAttrs;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objAttrs, DEVICE_CONTEXT);
    objAttrs.EvtCleanupCallback = &WdfDeviceContextCleanup;

    WDFDEVICE wdfDevice;
    status = WdfDeviceCreate(&pDevice, &objAttrs, &wdfDevice);
    if (!NT_SUCCESS(status))
    {
        WdfDeviceInitFree(pDevice);
        return status;
    }

    /*
        WdfDeviceInitFree should NEVER be called after successful WdfDeviceCreate.
        Context will be freed in the callback.
    */

    /* Symlink for usermode */
    DECLARE_CONST_UNICODE_STRING(dosDeviceName, L"\\DosDevices\\Dyper");
    status = WdfDeviceCreateSymbolicLink(wdfDevice, &dosDeviceName);
    if (!NT_SUCCESS(status))
    {
        DbgPrintErr("Failed to WdfDeviceCreateSymbolicLink: 0x%X\n", status);
        return status;
    }

    /*  WDF Queue for Read, Write and IOCTL */
    WDF_IO_QUEUE_CONFIG ioQueueConfig;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchSequential);

    ioQueueConfig.EvtIoDeviceControl = WdfQueueDeviceIoControl;
    ioQueueConfig.EvtIoRead = WdfQueueRead;
    ioQueueConfig.EvtIoWrite = WdfQueueWrite;
    ioQueueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(wdfDevice, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status))
    {
        DbgPrintErr("Failed to WdfIoQueueCreate: 0x%X\n", status);
        return status;
    }

    /* Populate device context */
    DEVICE_CONTEXT* pDeviceContext = WdfObjectGet_DEVICE_CONTEXT(wdfDevice);
    pDeviceContext->totalProcessorCount = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);

    pDeviceContext->pGuestStates = ExAllocatePool2(POOL_FLAG_PAGED, pDeviceContext->totalProcessorCount * sizeof(VM_STATE), POOL_TAG);
    if (!pDeviceContext->pGuestStates) {
        DbgPrintErr("Failed to allocate device context for %u processors\n", pDeviceContext->totalProcessorCount);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DbgPrintInfo("Driver initialised. Allocated VMState for %u processors\n", pDeviceContext->totalProcessorCount);
    WdfControlFinishInitializing(wdfDevice);
    return status;
}

VOID
DriverUnload(
    _In_ WDFDRIVER DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    DbgPrintInfo("Unloaded\n");
}

VOID
WdfDeviceFileCreate(
    _In_ WDFDEVICE device,
    _In_ WDFREQUEST request,
    _In_ WDFFILEOBJECT fileObjectd
)
{
    UNREFERENCED_PARAMETER(request);
    UNREFERENCED_PARAMETER(fileObjectd);

    DbgPrintInfo("WdfDeviceFileCreate\n");

    PHYSICAL_ADDRESS lowest;
    lowest.QuadPart = 0;

    PHYSICAL_ADDRESS highest;
    highest.QuadPart = MAXLONGLONG;

    PHYSICAL_ADDRESS zero;
    zero.QuadPart = 0;

    DEVICE_CONTEXT* pContext = WdfObjectGet_DEVICE_CONTEXT(device);

    /*
    Iterate all processors and turn on SVME.
    Technically possible a new CPU appeared since the guest areas where allocated
    Do not exceed pContext->totalProcessorCount, which describes the maximum number of guest state
    */
    ULONG procsInited = 0;
    USHORT groupCount = KeQueryActiveGroupCount();
    for (USHORT groupNum = 0; groupNum < groupCount; groupNum++) {

        GROUP_AFFINITY current;
        GROUP_AFFINITY first;
        RtlZeroMemory(&current, sizeof(current));
        RtlZeroMemory(&first, sizeof(first));

        ULONG procCount = KeQueryActiveProcessorCountEx(groupNum);
        for (ULONG procNum = 0; 
            procNum < procCount && procsInited <= pContext->totalProcessorCount;
            procNum++) {
            
            current.Group = groupNum;
            current.Mask = 1i64 << procNum;
            
            /* Store the default group affinity */
            if (procNum == 0) {
                KeSetSystemGroupAffinityThread(&current, &first);
            }
            else {
                KeSetSystemGroupAffinityThread(&current, NULL);
            }

            /* Virtual machine control block */
            pContext->pGuestStates[procsInited].pVMCB = MmAllocateContiguousMemorySpecifyCache(VMCB_SIZE, lowest, highest, zero, MmCached);
            if (!pContext->pGuestStates[procsInited].pVMCB) {
                DbgPrintErr("[GROUP=%u, CPU=%u] Error allocating VMCB\n", groupNum, procNum);
                continue;
            }
            RtlSecureZeroMemory(pContext->pGuestStates[procsInited].pVMCB, VMCB_SIZE);


            /* HSAVE MSR */
            pContext->pGuestStates[procsInited].pHSave = MmAllocateContiguousMemorySpecifyCache(HSAVE_SIZE, lowest, highest, zero, MmCached);
            if (!pContext->pGuestStates[procsInited].pHSave) {
                
                /* This would have been allocated, need to free for next loop */
                MmFreeContiguousMemory(pContext->pGuestStates[procsInited].pVMCB);

                DbgPrintErr("[GROUP=%u, CPU=%u] Error allocating HSAVE\n", groupNum, procNum);
                continue;
            }
            RtlSecureZeroMemory(pContext->pGuestStates[procsInited].pHSave, HSAVE_SIZE);

            PHYSICAL_ADDRESS pa = MmGetPhysicalAddress(pContext->pGuestStates[procsInited].pHSave);
            SetHSave(pa.QuadPart);
            EnableSvme();
            DbgPrintInfo("[GROUP=%u, CPU=%u] Turned on SVME\n", groupNum, procNum);

            procsInited++;
        }
        KeRevertToUserGroupAffinityThread(&first);
    }

    /* Should never trigger unless a new CPU appeared */
    if (procsInited != pContext->totalProcessorCount) {
        DbgPrintInfo("Mismatch between number of processors and number initialised. total=%u, inited=%u",
            pContext->totalProcessorCount, procsInited);
    }

    WdfRequestComplete(request, STATUS_SUCCESS);
}

VOID
WdfDeviceFileClose(
    _In_ WDFFILEOBJECT fileObject
)
{
    DbgPrintInfo("WdfDeviceFileClose\n");

    DEVICE_CONTEXT* pContext = WdfObjectGet_DEVICE_CONTEXT(WdfFileObjectGetDevice(fileObject));

    /* Iterate all processors and turn off SVME */
    ULONG procsDeled = 0;
    USHORT groupCount = KeQueryActiveGroupCount();
    for (USHORT groupNum = 0; groupNum < groupCount; groupNum++) {

        GROUP_AFFINITY current;
        GROUP_AFFINITY first;
        RtlZeroMemory(&current, sizeof(current));
        RtlZeroMemory(&first, sizeof(first));

        ULONG procCount = KeQueryActiveProcessorCountEx(groupNum);
        for (ULONG procNum = 0;
            procNum < procCount;
            procNum++) {

            current.Group = groupNum;
            current.Mask = 1i64 << procNum;

            /* Store the affinity of the first processor in the group */
            if (procNum == 0) {
                KeSetSystemGroupAffinityThread(&current, &first);
            }
            else {
                KeSetSystemGroupAffinityThread(&current, NULL);
            }
           
            DisableSvme();
            DbgPrintInfo("[GROUP=%u, CPU=%u] Turned off SVME\n", groupNum, procNum);

            procsDeled++;
        }
        KeRevertToUserGroupAffinityThread(&first);
    }

    /* Should never trigger unless a new CPU appeared */
    if (procsDeled != pContext->totalProcessorCount) {
        DbgPrintInfo("Mismatch between number of processors and number initialised. total=%u, deled=%u",
            pContext->totalProcessorCount, procsDeled);
    }

    /* Iterate all the states allocated and free memory */
    for (ULONG i = 0; i < pContext->totalProcessorCount; ++i) {
        if (pContext->pGuestStates[i].pVMCB) {
            MmFreeContiguousMemory(pContext->pGuestStates[i].pVMCB);
            pContext->pGuestStates[i].pVMCB = NULL;
        }
        if (pContext->pGuestStates[i].pHSave) {
            MmFreeContiguousMemory(pContext->pGuestStates[i].pHSave);
            pContext->pGuestStates[i].pHSave = NULL;
        }
    }
}

VOID
WdfQueueRead(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Length);

    DbgPrintInfo("WdfQueueRead\n");
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 0);
}


VOID
WdfQueueWrite(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Length);

    DbgPrintInfo("WdfQueueWrite\n");
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 0);
}


VOID
WdfQueueDeviceIoControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);

    DbgPrintInfo("WdfQueueDeviceIoControl\n");
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 0);
}

VOID 
WdfDeviceContextCleanup(
    _In_ WDFOBJECT device
)
{
    DEVICE_CONTEXT* pContext = WdfObjectGet_DEVICE_CONTEXT(device);
    if (pContext) {
        ExFreePoolWithTag(pContext->pGuestStates, POOL_TAG);
    }
}