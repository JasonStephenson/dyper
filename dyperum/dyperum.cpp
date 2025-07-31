#include <Windows.h>

#include <iostream>
#include <string>

extern "C" {
    void cpuid_call(uint32_t eax_in, uint32_t ecx_in, uint32_t* out);
}

std::string GetVendorId() {
    std::string ret;

    uint32_t regs[4]{};
    cpuid_call(0, 0, regs);

    char vendor[13]{};
    memcpy(&vendor[0], &regs[1], 4);
    memcpy(&vendor[4], &regs[3], 4);
    memcpy(&vendor[8], &regs[2], 4);
    vendor[12] = '\0';

    ret.assign(vendor);
    return ret;
}

/* AMD specific */
bool IsSVMSupported() {
    
    /* Extended function 8000_0001h */
    uint32_t regs[4]{};
    cpuid_call(0x80000001, 0, regs);

    /* ECX bit 2 (SVM Support) */
    return (regs[2] & (1 << 2)) != 0;
}

int main() {

    std::string const vendorId = GetVendorId();
    if (vendorId != "AuthenticAMD") {
        std::cout << "Intel not supported\n";
        return -1;
    }

    bool const IsSvmSupported = IsSVMSupported();
    if (!IsSvmSupported) {
        std::cout << "SVM not supported\n";
        return -1;
    }

    HANDLE hDevice = CreateFile(L"\\\\.\\Dyper",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, 
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        std::cout << std::hex;
        std::cout << "CreateFile error 0x" << GetLastError() << "\n";
        std::cout << std::dec;
        return -1;
    }

    std::cout << "Enter command\n";
    char c;
    std::cin >> c;

    bool running = true;
    while (running) {
        switch (c) {
        case 'X':
        case 'x':
            running = false;
            break;
        default:
            break;
        }
    }
    

    CloseHandle(hDevice);
    std::cout << "All ok\n";
}