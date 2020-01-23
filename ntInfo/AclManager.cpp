#pragma once
#include <windows.h>
#include <stdio.h>
#include <aclapi.h>

// https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms700142(v%3Dvs.85)
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
			if (!tInfo & (!wcscmp(wszAccName, L"Todos") || !wcscmp(wszAccName, L"Usuarios autentificados"))) {
				wprintf(L"\n\nDevice %s has acces for:", object);
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