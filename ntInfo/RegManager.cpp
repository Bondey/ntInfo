#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

void listServicesWithImagePath()
{
	HKEY hKey;
	char* regs = (char*)"SYSTEM\\CurrentControlSet\\Services";

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		regs,
		0,
		KEY_READ,
		&hKey) == ERROR_SUCCESS
		)
	{
		TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
		DWORD    cbName;                   // size of name string 
		TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
		DWORD    cchClassName = MAX_PATH;  // size of class string 
		DWORD    cSubKeys = 0;               // number of subkeys 
		DWORD    cbMaxSubKey;              // longest subkey size 
		DWORD    cchMaxClass;              // longest class string 
		DWORD    cValues;              // number of values for key 
		DWORD    cchMaxValue;          // longest value name 
		DWORD    cbMaxValueData;       // longest value data 
		DWORD    cbSecurityDescriptor; // size of security descriptor 
		FILETIME ftLastWriteTime;      // last write time 

		DWORD i, retCode;

		TCHAR  achValue[MAX_VALUE_NAME];
		DWORD cchValue = MAX_VALUE_NAME;

		// Get the class name and the value count. 
		retCode = RegQueryInfoKey(
			hKey,                    // key handle 
			achClass,                // buffer for class name 
			&cchClassName,           // size of class string 
			NULL,                    // reserved 
			&cSubKeys,               // number of subkeys 
			&cbMaxSubKey,            // longest subkey size 
			&cchMaxClass,            // longest class string 
			&cValues,                // number of values for this key 
			&cchMaxValue,            // longest value name 
			&cbMaxValueData,         // longest value data 
			&cbSecurityDescriptor,   // security descriptor 
			&ftLastWriteTime);       // last write time 

		// Enumerate the subkeys, until RegEnumKeyEx fails.

		if (cSubKeys)
		{
			printf("\nNumber of subkeys: %d\n", cSubKeys);

			for (i = 0; i < cSubKeys; i++)
			{
				cbName = MAX_KEY_LENGTH;
				retCode = RegEnumKeyEx(hKey, i,
					achKey,
					&cbName,
					NULL,
					NULL,
					NULL,
					&ftLastWriteTime);
				if (retCode == ERROR_SUCCESS)
				{
					HKEY temphkey;
					char servicekey[300];
					sprintf_s(servicekey,"%s\\%s", regs, achKey);
					RegOpenKeyEx(HKEY_LOCAL_MACHINE, servicekey, 0, KEY_QUERY_VALUE, &temphkey);
					DWORD keyType = REG_SZ;
					char buf[255] = { 0 };
					DWORD bufSize = sizeof(buf);
					RegQueryValueExA(temphkey, "ImagePath", 0, &keyType, (LPBYTE)buf, &bufSize);
					if (bufSize<250 && buf[bufSize-2]=='s' && buf[bufSize - 3] == 'y' && buf[bufSize - 4] == 's'){
						printf_s("Service: %s\n", achKey);
						printf_s("\tImagePath %s\n", &buf);
					}
				}
			}
		}
	}
	RegCloseKey(hKey);
}
