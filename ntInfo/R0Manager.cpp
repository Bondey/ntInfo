#include <stdio.h>
#include <windows.h>
#include <inttypes.h>
 
#define SIOCTL_TYPE 40000
#define IOCTL_DEVINFO\
 CTL_CODE( SIOCTL_TYPE, 0x800, METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)

typedef struct _STRING {
	USHORT Length;
	USHORT MaximumLength;
#ifdef MIDL_PASS
	[size_is(MaximumLength), length_is(Length)]
#endif // MIDL_PASS
	_Field_size_bytes_part_opt_(MaximumLength, Length) PCHAR Buffer;
} STRING;
typedef STRING* PSTRING;
typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;

typedef struct _DRIVER_INFO
{
	PVOID DriverStart;
	ULONG DriverSize;
	char DriverName[150];
	PVOID IOCTLOFFSET[28];
} DRIVER_INFO, * PDRIVER_INFO;

int TalkToDriv() {
	 
	HANDLE hDevice;
	WCHAR InBuff[20] = L"\\Device\\SysmonDrv";
	DWORD dwBytesRead = 0;
	char* ReadBuffer[1000];

	hDevice = CreateFile("\\\\.\\ntKrnInfo", GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hDevice == INVALID_HANDLE_VALUE) {
		printf_s("could not get device handle");
		return 0;
	}

	DeviceIoControl(hDevice, IOCTL_DEVINFO, InBuff, 20*sizeof(WCHAR), ReadBuffer, 1000, &dwBytesRead, NULL);

	PDRIVER_INFO Drvinfo = reinterpret_cast<DRIVER_INFO*>(ReadBuffer);
	
	printf_s("DriverName = %s\n", Drvinfo->DriverName);
	printf_s("DriverStart = 0x%" PRIx64 "\n", Drvinfo->DriverStart);
	printf_s("DriverLength = %i\n", Drvinfo->DriverSize);
	printf_s("DriverIRP Offsets:\n");
	for (int i = 0; i < 28; i++) {
		if((char*)Drvinfo->IOCTLOFFSET[i] > (char*)Drvinfo->DriverStart && (char*)Drvinfo->IOCTLOFFSET[i] < ((char*)Drvinfo->DriverStart+ Drvinfo->DriverSize)){
			printf_s("%i\t-> 0x%" PRIx64 "\n",i, Drvinfo->IOCTLOFFSET[i]);
		}
	}
	

	CloseHandle(hDevice);
	return 1;
}