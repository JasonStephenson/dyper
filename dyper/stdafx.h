#pragma once

#include <ntddk.h>
#include <wdf.h>

#define DbgPrintErr(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID , DPFLTR_ERROR_LEVEL, "Dyper::%s " fmt, __FUNCTION__, __VA_ARGS__)

#define DbgPrintInfo(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID , DPFLTR_INFO_LEVEL, "Dyper::%s " fmt, __FUNCTION__, __VA_ARGS__)