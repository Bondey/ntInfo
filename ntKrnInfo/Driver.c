#include <ntddk.h>
#include "ntstrsafe.h"

PDEVICE_OBJECT g_MyDevice; // Global pointer to our device object
const WCHAR deviceNameBuffer[] = L"\\Device\\ntKrnInfo";
const WCHAR deviceSymLinkBuffer[] = L"\\DosDevices\\ntKrnInfo";

#define SIOCTL_TYPE 40000
#define IOCTL_DEVINFO\
 CTL_CODE( SIOCTL_TYPE, 0x800, METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)

PUNICODE_STRING GetDriverfromDevice(WCHAR* DevName) {
	UNICODE_STRING     uniName;
	OBJECT_ATTRIBUTES  objAttr;

	RtlInitUnicodeString(&uniName, L"\\\\Device\\SysmonDrv");  // or L"\\SystemRoot\\example.txt"
	InitializeObjectAttributes(&objAttr, &uniName,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL, NULL);

	HANDLE   handle;
	NTSTATUS ntstatus;
	IO_STATUS_BLOCK    ioStatusBlock;

	// Do not try to perform any file operations at higher IRQL levels.
	// Instead, you may use a work item or a system worker thread to perform file operations.

	if (KeGetCurrentIrql() != PASSIVE_LEVEL)
		return 0;

	ntstatus = ZwCreateFile(&handle,
		GENERIC_WRITE,
		&objAttr, &ioStatusBlock, NULL,
		FILE_ATTRIBUTE_NORMAL,
		0,
		FILE_OVERWRITE_IF,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL, 0);

	if (!NT_SUCCESS(ntstatus)) {
		DbgPrint("ZwCreateFile failed. Status 0x%x", ntstatus);
		return 0;
	}

	PVOID* DevObject = NULL;

	ntstatus = ObReferenceObjectByHandle(handle,
		0,
		NULL,
		KernelMode,
		DevObject,
		NULL);

	if (!NT_SUCCESS(ntstatus)) {
		DbgPrint("ObReferenceObjectByHandle failed. Status 0x%x", ntstatus);
		ZwClose(handle);
		return 0;
	}

	PDRIVER_OBJECT DrvObject = (PDRIVER_OBJECT)((ULONG_PTR)DevObject + 0x008);

	PUNICODE_STRING DrvName = (PUNICODE_STRING)((ULONG_PTR)DrvObject + 0x038);

	return DrvName;
}

void supLog(const char* format, ...) {
	UNICODE_STRING     uniName;
	OBJECT_ATTRIBUTES  objAttr;

	char msg[1024] = "";
	va_list vl;
	va_start(vl, format);
	_vsnprintf(msg, sizeof(msg) / sizeof(char), format, vl);

	RtlInitUnicodeString(&uniName, L"\\DosDevices\\C:\\Windows\\ntKrnInfo.txt");  // or L"\\SystemRoot\\example.txt"
	InitializeObjectAttributes(&objAttr, &uniName,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL, NULL);

	HANDLE   handle;
	NTSTATUS ntstatus;
	IO_STATUS_BLOCK    ioStatusBlock;

	if (KeGetCurrentIrql() != PASSIVE_LEVEL)
		return;


	ntstatus = ZwCreateFile(&handle,
		FILE_APPEND_DATA,
		&objAttr, &ioStatusBlock, NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		FILE_OPEN_IF,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL, 0);


	if (NT_SUCCESS(ntstatus)) {
		size_t cb;
		ntstatus = RtlStringCbLengthA(msg, sizeof(msg), &cb);
		if (NT_SUCCESS(ntstatus))
			ZwWriteFile(handle, NULL, NULL, NULL, &ioStatusBlock, msg, (ULONG)cb, NULL, NULL);
		ZwClose(handle);
	}
}

VOID OnUnload(IN PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING symLink;

	RtlInitUnicodeString(&symLink, deviceSymLinkBuffer);

	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(pDriverObject->DeviceObject);

	supLog("OnUnload called!");
}

NTSTATUS Function_IRP_MJ_CREATE(PDEVICE_OBJECT pDeviceObject, PIRP Irp)
{
	return STATUS_SUCCESS;
}

NTSTATUS Function_IRP_MJ_CLOSE(PDEVICE_OBJECT pDeviceObject, PIRP Irp)
{
	return STATUS_SUCCESS;
}

static NTSTATUS DriverDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Irp->IoStatus.Status;
}


NTSTATUS Function_IRP_DEVICE_CONTROL(PDEVICE_OBJECT pDeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION pIoStackLocation;
	PCHAR welcome = "IRPGot";
	PVOID pBuf = Irp->AssociatedIrp.SystemBuffer;

	pIoStackLocation = IoGetCurrentIrpStackLocation(Irp);
	switch (pIoStackLocation->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_DEVINFO:
			supLog("IOCTL DEVINFO Called\n");

			PUNICODE_STRING res = GetDriverfromDevice(pBuf);
			supLog("IOCTL DEVINFO Called\n");
			RtlZeroMemory(pBuf, pIoStackLocation->Parameters.DeviceIoControl.InputBufferLength);
			RtlCopyMemory(pBuf, res, pIoStackLocation->Parameters.DeviceIoControl.InputBufferLength);

			break;

	}

	// Finish the I/O operation by simply completing the packet and returning
	// the same status as in the packet itself.
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = strlen(welcome);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath) {

	NTSTATUS ntStatus = 0;
	UNICODE_STRING deviceNameUnicodeString, deviceSymLinkUnicodeString;

	// Normalize name and symbolic link.
	RtlInitUnicodeString(&deviceNameUnicodeString,
		deviceNameBuffer);
	RtlInitUnicodeString(&deviceSymLinkUnicodeString,
		deviceSymLinkBuffer);

	// Create the device.
	ntStatus = IoCreateDevice(pDriverObject,
		0, // For driver extension
		&deviceNameUnicodeString,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_UNKNOWN,
		FALSE,
		&g_MyDevice);

	// Create the symbolic link
	ntStatus = IoCreateSymbolicLink(&deviceSymLinkUnicodeString,
		&deviceNameUnicodeString);

	pDriverObject->DriverUnload = OnUnload;
	for (unsigned int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		pDriverObject->MajorFunction[i] = DriverDefaultHandler;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = Function_IRP_MJ_CREATE;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = Function_IRP_MJ_CLOSE;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Function_IRP_DEVICE_CONTROL;

	supLog("Driver loaded\n");

	return STATUS_SUCCESS;
}
