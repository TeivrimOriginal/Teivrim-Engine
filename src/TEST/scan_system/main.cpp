#include <windows.h>
#include <setupapi.h>
#include <iostream>
#include <string>

// Вручную импортируем функции для MinGW
extern "C" {
    __declspec(dllimport) HDEVINFO WINAPI SetupDiGetClassDevsA(
        const GUID* ClassGuid,
        PCSTR Enumerator,
        HWND hwndParent,
        DWORD Flags
    );
    
    __declspec(dllimport) BOOL WINAPI SetupDiEnumDeviceInfo(
        HDEVINFO DeviceInfoSet,
        DWORD MemberIndex,
        PSP_DEVINFO_DATA DeviceInfoData
    );
    
    __declspec(dllimport) BOOL WINAPI SetupDiGetDeviceRegistryPropertyA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        DWORD Property,
        PDWORD PropertyRegDataType,
        PBYTE PropertyBuffer,
        DWORD PropertyBufferSize,
        PDWORD RequiredSize
    );
    
    __declspec(dllimport) BOOL WINAPI SetupDiDestroyDeviceInfoList(
        HDEVINFO DeviceInfoSet
    );
}

// GUID для дисплея (вручную)
const GUID GUID_DEVCLASS_DISPLAY = 
    {0x4d36e968, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};

using namespace std;

int main() {
    // CPU
    HKEY hKey; char c[256]; DWORD s=256;
    RegOpenKeyA(HKEY_LOCAL_MACHINE,"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",&hKey);
    RegQueryValueExA(hKey,"ProcessorNameString",0,0,(LPBYTE)c,&s);
    cout<<"CPU:"<<c<<endl;
    
    // RAM  
    MEMORYSTATUSEX m={sizeof(m)};
    GlobalMemoryStatusEx(&m);
    cout<<"RAM:"<<m.ullTotalPhys/1073741824.0<<"GB"<<endl;
    
    // GPU
    HDEVINFO dev=SetupDiGetClassDevsA(&GUID_DEVCLASS_DISPLAY,0,0,DIGCF_PRESENT);
    SP_DEVINFO_DATA data={sizeof(data)};
    string gpu="Unknown";
    
    if(SetupDiEnumDeviceInfo(dev,0,&data)){
        char name[256];
        if(SetupDiGetDeviceRegistryPropertyA(dev,&data,SPDRP_DEVICEDESC,0,(PBYTE)name,256,0)){
            gpu=name;
        }
    }
    SetupDiDestroyDeviceInfoList(dev);
    
    cout<<"GPU:"<<gpu<<endl;
    
    string pizda; cin>>pizda;
    return 0;
}