#include "stdafx.h"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD DriverUnload;
EVT_WDF_DEVICE_FILE_CREATE WdfDeviceFileCreate;
EVT_WDF_FILE_CLOSE WdfDeviceFileClose;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL WdfQueueDeviceIoControl;
EVT_WDF_IO_QUEUE_IO_WRITE WdfQueueWrite;
EVT_WDF_IO_QUEUE_IO_READ WdfQueueRead;

extern void EnableSvme();

extern BOOLEAN SupportCheckMSR();
extern BOOLEAN SupportCheckIsAMD();
extern BOOLEAN SupportCheckSVM();

BOOLEAN PrintAndConfirmCpuSuport() {
    BOOLEAN isAmd = SupportCheckIsAMD();
    DbgPrintInfo("IsAMD: %d\n", isAmd);
    
    /* If its not AMD no point checking the rest */
    if (!isAmd) {
        return FALSE;
    }

    BOOLEAN msrOk = SupportCheckMSR();
    BOOLEAN svmOk = SupportCheckSVM();
    DbgPrintInfo("RSMSR/WRMSR: %d\n", msrOk);
    DbgPrintInfo("SVM: %d\n", svmOk);

    return msrOk && svmOk;
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

    WDFDEVICE wdfDevice;
    status = WdfDeviceCreate(&pDevice, WDF_NO_OBJECT_ATTRIBUTES, &wdfDevice);
    if (!NT_SUCCESS(status))
    {
        WdfDeviceInitFree(pDevice);
        return status;
    }

    /*
        WdfDeviceInitFree should NEVER be called after successful WdfDeviceCreate
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
    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(request);
    UNREFERENCED_PARAMETER(fileObjectd);
   
    DbgPrintInfo("WdfDeviceFileCreate\n");

    EnableSvme();

    WdfRequestComplete(request, STATUS_SUCCESS);
}

VOID 
WdfDeviceFileClose(
    _In_ WDFFILEOBJECT fileObject
)
{
    UNREFERENCED_PARAMETER(fileObject);
    DbgPrintInfo("WdfDeviceFileClose\n");
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
