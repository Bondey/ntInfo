#include "windows.h"
#include <iostream>

int sendIOCtl( char* device, char* IOCTL_FILE, char* fileInPath, char* fileOutPath) {
	HANDLE DevHandler = CreateFile(device, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, 0, NULL);
	if (!DevHandler) {
		printf("could not get a Handler for device");
		return 1;
	}

	DWORD returned = 0;

	// IOCTL from file
	DWORD IOCTL_CODE=0;
	FILE* fp;
	fopen_s(&fp,IOCTL_FILE, "rb");
	if (fp != NULL) {
		fread(&IOCTL_CODE, sizeof(DWORD), 1, fp);
		if (ferror(fp) != 0) {
			fputs("Error reading file", stderr);
		}
		fclose(fp);
	}

	// In Buffer from file
	char inBuff[0xFFFF];
	size_t inLen;

	fopen_s(&fp,fileInPath, "r");
	if (fp != NULL) {
		inLen = fread(inBuff, sizeof(char), 0xFFFF, fp);
		if (ferror(fp) != 0) {
			fputs("Error reading file", stderr);
		}
		else {
			inBuff[inLen++] = '\0'; /* Just to be safe. */
		}

		fclose(fp);
	}

	// Out Buffer from file
	char OutBuff[0xFFFF];
	size_t OutLen=0xFFFF;

	DeviceIoControl(DevHandler, IOCTL_CODE, inBuff, inLen, OutBuff, OutLen, &returned, NULL);

	if (returned > 0){
		fopen_s(&fp,fileOutPath, "wb");
		if (fp) {
			fwrite(OutBuff, 1, returned, fp);
		}
		fclose(fp);
	}

	return returned;
}
