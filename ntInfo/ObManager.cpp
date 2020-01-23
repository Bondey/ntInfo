#include <windows.h>
#include <stdio.h>
//https://comp.os.ms-windows.programmer.win32.narkive.com/6sVp3dRF/nt-object-namespace
//https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-mqmq/e5f0e8f9-d67b-4ac0-93a4-1e7e96e87a8c
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

//static WCHAR obList[500][100];
int total = 0;

// retorna un listado de elementos en el ObjectManager
int getObElems(WCHAR* elem, WCHAR obList[500][100])
{
	int cont = 0;
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
			return 0;
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
				return 0;
			}
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
			
			CloseHandle(DirectoryHandle);
		}
		FreeLibrary(hModule);
		return cont;
	}

	return 0;
}