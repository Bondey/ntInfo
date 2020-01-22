#include <stdio.h>
#include <windows.h>

int startSVC(char* svcname) {
	SC_HANDLE hSCM;
	hSCM = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
	if (!hSCM) {
		return 1;
	}

	SC_HANDLE schService = OpenService(
		hSCM,         // SCM database 
		svcname,            // name of service 
		SERVICE_START);  // full access 

	if (!StartService(
		schService,  // handle to service 
		0,           // number of arguments 
		NULL))      // no arguments 
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(hSCM);
		return 1;
	}
	else printf("Service start pending...\n");
	return 0;
}
int StartAllServices() {
	SC_HANDLE hSCM;
	hSCM = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
	if (!hSCM) {
		return 0;
	}
	int svccont = 0;
	void* buf = NULL;
	DWORD bufSize = 0;
	DWORD moreBytesNeeded, serviceCount;
	for (;;) {
		if (EnumServicesStatusEx(
			hSCM,
			SC_ENUM_PROCESS_INFO,
			SERVICE_WIN32,
			SERVICE_STATE_ALL,
			(LPBYTE)buf,
			bufSize,
			&moreBytesNeeded,
			&serviceCount,
			NULL,
			NULL)) {
			ENUM_SERVICE_STATUS_PROCESS* services = (ENUM_SERVICE_STATUS_PROCESS*)buf;
			for (DWORD i = 0; i < serviceCount; ++i) {
				switch (services[i].ServiceStatusProcess.dwCurrentState) {
					//case 4:
					//	printf_s("Status: SERVICE_RUNNING\n");
					//	break;
					//case 7:
					//	printf_s("Status: SERVICE_PAUSED\n");
					//	break;
				case 1:
					//printf_s("Status: SERVICE_STOPPED\n");
					if (startSVC(services[i].lpServiceName) == 0) {
						svccont++;
					}
					break;
				default:
					break;
				}
			}
			free(buf);
			CloseServiceHandle(hSCM);
			return svccont;
		}
		int err = GetLastError();
		if (ERROR_MORE_DATA != err) {
			free(buf);
			CloseServiceHandle(hSCM);
			return 0;
		}
		bufSize += moreBytesNeeded;
		free(buf);
		buf = malloc(bufSize);
	}

	CloseServiceHandle(hSCM);
}