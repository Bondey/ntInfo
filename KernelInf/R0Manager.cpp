#include <stdio.h>
#include <windows.h>

#define SIOCTL_TYPE 40000
#define IOCTL_DEVINFO\
 CTL_CODE( SIOCTL_TYPE, 0x800, METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)

int TalkToDriv() {
	printf_s("IOCTL Code: %x",IOCTL_DEVINFO);
	return 0;
	HANDLE hDevice;
	WCHAR InBuff[20] = L"\\Device\\SysmonDrv";
	DWORD dwBytesRead = 0;
	char* ReadBuffer[100] = { 0 };

	hDevice = CreateFile("\\\\.\\ntKrnInfo", GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hDevice = INVALID_HANDLE_VALUE) {
		printf_s("could not get device handle");
		return 0;
	}

	DeviceIoControl(hDevice, IOCTL_DEVINFO, InBuff, 20*sizeof(WCHAR), ReadBuffer, sizeof(ReadBuffer), &dwBytesRead, NULL);
	
	for (int j = 0; j < 50; j++){
		printf_s("%02X", ReadBuffer[j]);
		j++;
	}

	CloseHandle(hDevice);
	return 1;
}