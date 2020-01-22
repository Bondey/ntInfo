#include <stdio.h>
#include <windows.h>

#include "ObManager.h"
#include "AclManager.h"
#include "SrvManager.h"

WCHAR* paramtow(char* param) {
	size_t sizeofparm = 0;
	const char* caca = param;
	size_t size = strlen(caca) + 1;
	wchar_t* paramelem = new wchar_t[size];
	mbstowcs_s(&sizeofparm, paramelem, 100, caca, strlen(caca));
	return paramelem;
}
void main(int argc, char* argv[]) {

	WCHAR obList[500][100];
	WCHAR* elem = new wchar_t[20];
	elem = (WCHAR*)L"\\";
	int total = 0;
	int secs = 1;
	int showACLs = 0;

	if (argc > 1) {

		for (int i = 0; i < argc; i++) {
			if (!strcmp(argv[i], "-h")) {
				printf_s("USAGE:\n");
				printf_s("KernelInf.exe [flags]\n");
				printf_s("Flags:\n");
				printf_s("-o ObjectTypeName\tObjects to show (if not set, will be shown RootObjects)\n");
				printf_s("-a\t\t\tOnly will show objects which ACL leaves privs for 'AllUsers' or 'LoggedUser'\n\n");
				printf_s("-ss\t\t\tTry to start as much Services as posible befor checking Objects (Driver/Devices)\n\n");
				return;
			}
			if (!strcmp(argv[i], "-o")) {
				elem = paramtow(argv[i + 1]);
			}
			if (!strcmp(argv[i], "-a")) {
				showACLs = 1;
			}
			if (!strcmp(argv[i], "-ss")) {
				StartAllServices();
				Sleep(secs*2000);
			}
		}

	}
	
	total = getObElems(elem, obList);
	for (int i = 0; i < total; i++) {
		WCHAR deviceN[100];
		swprintf(deviceN, 100, L"\\\\.\\%s", obList[i]);
		if (showACLs){
			GetDaclInfo(deviceN, 0);
		}else {
			wprintf_s(L"%s\\ %s\n",elem, obList[i]);
		}
	}
	

}