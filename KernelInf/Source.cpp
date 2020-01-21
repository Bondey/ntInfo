
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <aclapi.h>
//https://comp.os.ms-windows.programmer.win32.narkive.com/6sVp3dRF/nt-object-namespace
//https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-mqmq/e5f0e8f9-d67b-4ac0-93a4-1e7e96e87a8c
typedef  enum
{
	MQSEC_DELETE_MESSAGE = 0x00000001,
	MQSEC_PEEK_MESSAGE = 0x00000002,
	MQSEC_WRITE_MESSAGE = 0x00000004,
	MQSEC_DELETE_JOURNAL_MESSAGE = 0x00000008,
	MQSEC_SET_QUEUE_PROPERTIES = 0x00000010,
	MQSEC_GET_QUEUE_PROPERTIES = 0x00000020,
	MQSEC_DELETE_QUEUE = 0x00010000,
	MQSEC_GET_QUEUE_PERMISSIONS = 0x00020000,
	MQSEC_CHANGE_QUEUE_PERMISSIONS = 0x00040000,
	MQSEC_TAKE_QUEUE_OWNERSHIP = 0x00080000,
	MQSEC_RECEIVE_MESSAGE = (MQSEC_DELETE_MESSAGE | MQSEC_PEEK_MESSAGE),
	MQSEC_RECEIVE_JOURNAL_MESSAGE = (MQSEC_DELETE_JOURNAL_MESSAGE | MQSEC_PEEK_MESSAGE),
	MQSEC_QUEUE_GENERIC_READ = (MQSEC_GET_QUEUE_PROPERTIES | MQSEC_GET_QUEUE_PERMISSIONS | MQSEC_RECEIVE_MESSAGE | MQSEC_RECEIVE_JOURNAL_MESSAGE),
	MQSEC_QUEUE_GENERIC_WRITE = (MQSEC_GET_QUEUE_PROPERTIES | MQSEC_GET_QUEUE_PERMISSIONS | MQSEC_WRITE_MESSAGE),
	MQSEC_QUEUE_GENERIC_ALL = (MQSEC_RECEIVE_MESSAGE | MQSEC_RECEIVE_JOURNAL_MESSAGE | MQSEC_WRITE_MESSAGE | MQSEC_SET_QUEUE_PROPERTIES | MQSEC_GET_QUEUE_PROPERTIES | MQSEC_DELETE_QUEUE | MQSEC_GET_QUEUE_PERMISSIONS | MQSEC_CHANGE_QUEUE_PERMISSIONS | MQSEC_TAKE_QUEUE_OWNERSHIP)
} MQQUEUEACCESSMASK;

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING* PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG uLength;
	HANDLE hRootDirectory;
	PUNICODE_STRING pObjectName;
	ULONG uAttributes;
	PVOID pSecurityDescriptor;
	PVOID pSecurityQualityOfService;
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES* POBJECT_ATTRIBUTES;

#define InitializeObjectAttributes( p, n, a, r, s ) { \
(p)->uLength = sizeof( OBJECT_ATTRIBUTES ); \
(p)->hRootDirectory = r; \
(p)->uAttributes = a; \
(p)->pObjectName = n; \
(p)->pSecurityDescriptor = s; \
(p)->pSecurityQualityOfService = NULL; \
}

#define OBJ_CASE_INSENSITIVE 0x00000040L

#define DIRECTORY_QUERY 0x0001
#define DIRECTORY_TRAVERSE 0x0002
#define DIRECTORY_CREATE_OBJECT 0x0004
#define DIRECTORY_CREATE_SUBDIRECTORY 0x0008
#define DIRECTORY_ALL_ACCESS STANDARD_RIGHTS_REQUIRED | 0xF

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

NTSTATUS(WINAPI* pRtlInitUnicodeString)(PUNICODE_STRING, PCWSTR);
NTSTATUS(WINAPI* pNtOpenDirectoryObject)(
	OUT PHANDLE DirectoryHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);
NTSTATUS(WINAPI* pNtQueryDirectoryObject)(
	IN HANDLE DirectoryHandle,
	OUT PVOID Buffer,
	IN ULONG Length,
	IN BOOLEAN ReturnSingleEntry,
	IN BOOLEAN RestartScan,
	IN OUT PULONG Context,
	OUT PULONG ReturnLength OPTIONAL
	);

typedef struct _OBJECT_DIRECTORY_INFORMATION {
	UNICODE_STRING Name;
	UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, * POBJECT_DIRECTORY_INFORMATION;

UNICODE_STRING DirectoryName;
HANDLE DirectoryHandle = NULL;
OBJECT_ATTRIBUTES Attributes;
NTSTATUS Status;
#define BUFFER_SIZE 1024
CHAR Buffer[BUFFER_SIZE];
ULONG ReturnedLength, Context = 0;
POBJECT_DIRECTORY_INFORMATION pDirectoryInfo;

static WCHAR obList[500][100];
int total = 0;

// retorna un listado de elementos en el ObjectManager
int getObElems(WCHAR* elem)
{
	HMODULE hModule = LoadLibrary("NTDLL.DLL");
	if (hModule)
	{
		pRtlInitUnicodeString = (NTSTATUS(WINAPI*)(PUNICODE_STRING, PCWSTR))
			GetProcAddress(hModule, "RtlInitUnicodeString");
		pNtOpenDirectoryObject = (NTSTATUS(WINAPI*)(PHANDLE, ACCESS_MASK,
			POBJECT_ATTRIBUTES)) GetProcAddress(hModule, "NtOpenDirectoryObject");
		pNtQueryDirectoryObject = (NTSTATUS(WINAPI*)(HANDLE, PVOID, ULONG,
			BOOLEAN, BOOLEAN, PULONG, PULONG)) GetProcAddress(hModule,
				"NtQueryDirectoryObject");
		if (!pRtlInitUnicodeString || !pNtOpenDirectoryObject ||
			!pNtQueryDirectoryObject)
		{
			printf("Error getting functions addresses");
			FreeLibrary(hModule);
			return 1;
		}
		
		WCHAR Root[MAX_PATH];
		swprintf(Root, MAX_PATH, L"%s", elem);
		//WCHAR Root[MAX_PATH] = L"\\";
		pRtlInitUnicodeString(&DirectoryName, Root);
		
		InitializeObjectAttributes(&Attributes, &DirectoryName,
			OBJ_CASE_INSENSITIVE, NULL, NULL);

		Status = pNtOpenDirectoryObject(&DirectoryHandle, STANDARD_RIGHTS_READ
			| DIRECTORY_QUERY | DIRECTORY_TRAVERSE, &Attributes);
		if (NT_SUCCESS(Status))
		{
			Status = pNtQueryDirectoryObject(DirectoryHandle,
				Buffer,
				sizeof(Buffer),
				TRUE,
				TRUE,
				&Context,
				&ReturnedLength);
			if (!NT_SUCCESS(Status))
			{
				printf("NtQueryDirectoryObject() Error : %d", Status);
				FreeLibrary(hModule);
				return 1;
			}
			int cont = 0;
			while (NT_SUCCESS(Status))
			{
				pDirectoryInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;
				while (pDirectoryInfo->Name.Length != 0)
				{
					swprintf(obList[cont], 100, L"%s", pDirectoryInfo->Name.Buffer);
					cont++;
					pDirectoryInfo++;
				}
				Status = pNtQueryDirectoryObject(DirectoryHandle,
					Buffer,
					sizeof(Buffer),
					TRUE,
					FALSE,
					&Context,
					&ReturnedLength);
			}
			total = cont;
			CloseHandle(DirectoryHandle);
		}
		FreeLibrary(hModule);
	}
	
	return 0;
}
// shit, moving above code out of main cpp and getting device list here is fking dificult D:

// https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms700142(v%3Dvs.85)
// Por ahora no lo utilizo
HRESULT DisplayPermissions(
	ACCESS_MASK amMask
)
{

	if ((amMask & MQSEC_QUEUE_GENERIC_ALL) == MQSEC_QUEUE_GENERIC_ALL)
	{
		wprintf(L"\tFull_Control\n ");
	}

	if ((amMask & MQSEC_DELETE_QUEUE) == MQSEC_DELETE_QUEUE)
	{
		wprintf(L"\tDelete\n ");
	}

	if ((amMask & MQSEC_RECEIVE_MESSAGE) == MQSEC_RECEIVE_MESSAGE)
	{
		wprintf(L"\tReceive_Message\n ");
	}

	if ((amMask & MQSEC_DELETE_MESSAGE) == MQSEC_DELETE_MESSAGE)
	{
		wprintf(L"\tDelete_Message\n ");
	}

	if ((amMask & MQSEC_PEEK_MESSAGE) == MQSEC_PEEK_MESSAGE)
	{
		wprintf(L"\tPeek_Message\n ");
	}

	if ((amMask & MQSEC_RECEIVE_JOURNAL_MESSAGE) == MQSEC_RECEIVE_JOURNAL_MESSAGE)
	{
		wprintf(L"\tReceive_Journal_Message\n ");
	}

	if ((amMask & MQSEC_DELETE_JOURNAL_MESSAGE) == MQSEC_DELETE_JOURNAL_MESSAGE)
	{
		wprintf(L"\tDelete_Journal_Message\n ");
	}

	if ((amMask & MQSEC_GET_QUEUE_PROPERTIES) == MQSEC_GET_QUEUE_PROPERTIES)
	{
		wprintf(L"\tGet_Properties\n ");
	}

	if ((amMask & MQSEC_SET_QUEUE_PROPERTIES) == MQSEC_SET_QUEUE_PROPERTIES)
	{
		wprintf(L"\tSet_Properties\n ");
	}

	if ((amMask & MQSEC_GET_QUEUE_PERMISSIONS) == MQSEC_GET_QUEUE_PERMISSIONS)
	{
		wprintf(L"\tGet_Permissions\n ");
	}

	if ((amMask & MQSEC_CHANGE_QUEUE_PERMISSIONS) == MQSEC_CHANGE_QUEUE_PERMISSIONS)
	{
		wprintf(L"\tSet_Permissions\n ");
	}

	if ((amMask & MQSEC_TAKE_QUEUE_OWNERSHIP) == MQSEC_TAKE_QUEUE_OWNERSHIP)
	{
		wprintf(L"\tTake_Ownership\n ");
	}

	if ((amMask & MQSEC_WRITE_MESSAGE) == MQSEC_WRITE_MESSAGE)
	{
		wprintf(L"\tSend_Message\n");
	}

	return S_OK;
}

int GetDaclInfo(
	WCHAR* object,
	int tInfo
)
{
	DWORD dwError;
	PACL ExistingDacl;
	PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;

	dwError = GetNamedSecurityInfoW(
		object,
		SE_FILE_OBJECT,
		DACL_SECURITY_INFORMATION,
		NULL,
		NULL,
		&ExistingDacl,
		NULL,
		&pSecurityDescriptor
	);
	if (dwError != ERROR_SUCCESS) {
		return 1;
	}

	PACL pDacl = NULL;
	ACL_SIZE_INFORMATION aclsizeinfo;
	ACCESS_ALLOWED_ACE* pAce = NULL;
	SID_NAME_USE eSidType;
	DWORD dwErrorCode = 0;
	HRESULT hr = 0;

	// Create buffers that may be large enough.  
	const DWORD INITIAL_SIZE = 256;
	DWORD cchAccName = 0;
	DWORD cchDomainName = 0;
	DWORD dwAccBufferSize = INITIAL_SIZE;
	DWORD dwDomainBufferSize = INITIAL_SIZE;
	DWORD cAce;
	WCHAR* wszAccName = NULL;
	WCHAR* wszDomainName = NULL;

	// Retrieve a pointer to the DACL in the security descriptor.  
	BOOL fDaclPresent = FALSE;
	BOOL fDaclDefaulted = TRUE;
	GetSecurityDescriptorDacl(
		pSecurityDescriptor,
		&fDaclPresent,
		&pDacl,
		&fDaclDefaulted
	);

	
	// Retrieve the ACL_SIZE_INFORMATION structure to find the number of ACEs in the DACL.  
	GetAclInformation(
		pDacl,
		&aclsizeinfo,
		sizeof(aclsizeinfo),
		AclSizeInformation
	);

	// Create buffers for the account name and the domain name.  
	wszAccName = new WCHAR[dwAccBufferSize];
	if (wszAccName == NULL)
	{
		return 1;
	}
	wszDomainName = new WCHAR[dwDomainBufferSize];
	if (wszDomainName == NULL)
	{
		return 1;
	}
	memset(wszAccName, 0, dwAccBufferSize * sizeof(WCHAR));
	memset(wszDomainName, 0, dwDomainBufferSize * sizeof(WCHAR));

	LPCWSTR wszComputerName = L"\0";
	int res = 1;
	// Loop through the ACEs and display the information.  
	for (cAce = 0; cAce < aclsizeinfo.AceCount && hr == 0; cAce++)
	{

		// Get ACE info  
		if (GetAce(
			pDacl,
			cAce,
			(LPVOID*)&pAce
		) == FALSE)
		{
			wprintf(L"GetAce failed. GetLastError returned: %d\n", GetLastError());
			continue;
		}

		// Obtain the account name and domain name for the SID in the ACE.  
		for (; ; )
		{

			// Set the character-count variables to the buffer sizes.  
			cchAccName = dwAccBufferSize;
			cchDomainName = dwDomainBufferSize;
			if (LookupAccountSidW(
				wszComputerName, // NULL for the local computer  
				&pAce->SidStart,
				wszAccName,
				&cchAccName,
				wszDomainName,
				&cchDomainName,
				&eSidType
			) == TRUE)
			{
				break;
			}

			// Check if one of the buffers was too small.  
			if ((cchAccName > dwAccBufferSize) || (cchDomainName > dwDomainBufferSize))
			{

				// Reallocate memory for the buffers and try again.  
				wprintf(L"The name buffers were too small. They will be reallocated.\n");
				delete[] wszAccName;
				delete[] wszDomainName;
				wszAccName = new WCHAR[cchAccName];
				if (wszAccName == NULL)
				{
					return 1;
				}
				wszDomainName = new WCHAR[cchDomainName];
				if (wszDomainName == NULL)
				{
					return 1;
				}
				memset(wszAccName, 0, cchAccName * sizeof(WCHAR));
				memset(wszDomainName, 0, cchDomainName * sizeof(WCHAR));
				dwAccBufferSize = cchAccName;
				dwDomainBufferSize = cchDomainName;
				continue;
			}

			// Something went wrong in the call to LookupAccountSid.  
			// Check if an unexpected error occurred.  
			if (GetLastError() == ERROR_NONE_MAPPED)
			{
				
				wszDomainName[0] = L'\0';
				if (dwAccBufferSize > wcslen(L"!Unknown!"))
				{
					wszAccName[dwAccBufferSize - 1] = L'\0';
				}
				break;
			}
			else
			{
				dwErrorCode = GetLastError();
				wprintf(L"LookupAccountSid failed. GetLastError returned: %d\n", dwErrorCode);
				delete[] wszAccName;
				delete[] wszDomainName;
				return HRESULT_FROM_WIN32(dwErrorCode);
			}
		}
		
		switch (pAce->Header.AceType)
		{
		case ACCESS_ALLOWED_ACE_TYPE:
			if (!tInfo & (!wcscmp(wszAccName,L"Todos") || !wcscmp(wszAccName, L"Usuarios autentificados") )){
				wprintf(L"\n\nDevice %s has acces for:",object);
				if (wszDomainName[0] == 0)
				{
					wprintf(L"\n%s\n", wszAccName);
				}
				else wprintf(L"\n%s\\%s\n", wszDomainName, wszAccName);
				DisplayPermissions(pAce->Mask);
				res = 0;
			}
			else if (tInfo) {
				if (wszDomainName[0] == 0)
				{
					wprintf(L"\n%s\n", wszAccName);
				}
				else wprintf(L"\n%s\\%s\n", wszDomainName, wszAccName);
			}
			break;

		default:
			break;
		}
	}

	// Free memory allocated for buffers.  
	delete[] wszAccName;
	delete[] wszDomainName;

	return res;
}

int GetDevDriv(WCHAR* devname) {
	wprintf(L"obtainDriverPathAndIRPOffset4: %s \n",devname);
	return 0;
}

void main() {
	WCHAR* elem = (WCHAR*)L"\\Device";
	getObElems(elem);
	
	for (int i = 0; i < total; i++) {
		wprintf_s(L"Objects: %s\n", obList[i]);
		//WCHAR deviceN[100];
		//swprintf(deviceN, 100, L"\\\\.\\%s", obList[i]);
		//if (GetDaclInfo(deviceN,0) == 0) {
		//	//GetDevDriv(deviceN);
		//}
	}
}