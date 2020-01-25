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

VOID SvcDelete(char* svcname)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	schService = OpenService(
		schSCManager,         // SCM database 
		svcname,            // name of service 
		SERVICE_START);  // full access 

	DeleteService(schService);
}

VOID SvcInstall(char* svcname, char* svcpath)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Create the service
	schService = CreateService(
		schSCManager,              // SCM database 
		svcname,                   // name of service 
		svcname,                   // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_KERNEL_DRIVER,     // service type 
		SERVICE_DEMAND_START,      // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		svcpath,                    // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if (schService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}
	else printf("Service installed successfully\n");
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
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