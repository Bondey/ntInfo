#include <ntddk.h>
#include "ntstrsafe.h"
#include "Driver.h"

PDEVICE_OBJECT g_MyDevice; // Global pointer to our device object
const WCHAR deviceNameBuffer[] = L"\\Device\\ntKrnInfo";
const WCHAR deviceSymLinkBuffer[] = L"\\DosDevices\\ntKrnInfo";

#define SIOCTL_TYPE 40000
#define IOCTL_DEVINFO\
 CTL_CODE( SIOCTL_TYPE, 0x800, METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)

typedef struct _DRIVER_INFO
{
	PVOID DriverStart;
	ULONG DriverSize;
	char DriverName[150];
	PVOID IOCTLOFFSET[28];
} DRIVER_INFO, * PDRIVER_INFO;

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


DRIVER_INFO GetDriverfromDevice(WCHAR* DevName) {
	UNICODE_STRING uniName;
	DRIVER_INFO Drvinfo;
	FILE_OBJECT* keybdfo;
	DEVICE_OBJECT* keybddo;
	NTSTATUS status;
	
	RtlInitUnicodeString(&uniName, L"\\Device\\gpuenergydrv");  // or L"\\SystemRoot\\example.txt"
	status = IoGetDeviceObjectPointer(&uniName, FILE_READ_DATA, &keybdfo, &keybddo);

	if (!NT_SUCCESS(status)) {
		supLog("nt Status %x\n", status);
		return Drvinfo;
	}
	PDRIVER_OBJECT DrvObject = keybddo->DriverObject;
	UNICODE_STRING DrvName = DrvObject->DriverName;

	// Get Driver In-Memmory boundaries in order to check if dispatch functions are inside de driver or at other part..
	Drvinfo.DriverStart = DrvObject->DriverStart;
	Drvinfo.DriverSize = DrvObject->DriverSize;

	// Strings are hard in c brah...
	ANSI_STRING ADST;
	ADST.Buffer = (PCHAR)ExAllocatePool(NonPagedPool, DrvName.Length + 1);
	ADST.Length = 0;
	RtlUnicodeStringToAnsiString(&ADST, &DrvName, TRUE);
	int i = 0;
	for (i = 0; i < ADST.Length; i++) {
		Drvinfo.DriverName[i] = ADST.Buffer[i];
	}
	Drvinfo.DriverName[i + 1] = '\x0';


	for (int i = 0; i < 28; i++) {
		Drvinfo.IOCTLOFFSET[i] = DrvObject->MajorFunction[i];
	}
	
	ObDereferenceObject(keybdfo);
	return Drvinfo;
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
			DRIVER_INFO Drvinfo = GetDriverfromDevice(pBuf);
			supLog("Info: %s\n", Drvinfo.DriverName);

			ULONG len = pIoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
			RtlZeroMemory(pBuf, len);
			RtlCopyMemory(pBuf, &Drvinfo, sizeof(Drvinfo));
			break;
	}

	// Finish the I/O operation by simply completing the packet and returning
	// the same status as in the packet itself.
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 511;
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
