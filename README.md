# KernelInf
My "Always on devel" Tool to automate the collection of different Windows Internals info on a shell tool. (GUIs are bullshit :)

# Usage

KernelInf.exe [flags]

Flags:

-o ObjectTypeName       Objects to show (if not set, will be shown RootObjects)

-a                      Only will show objects which ACL leaves privs for 'AllUsers' or 'LoggedUser'

-rgs                    Print all Services and Image Paths from Registry

-ik                     Install and start ntInfo Kernel Driver (unsigned)

-dk                     Stop and delete ntInfo Kernel Driver

-k DeviceObject         Prints Driver object and IRP handlers for this device

-ss                     Try to start as much Services as possible before checking Objects (Driver/Devices)

-ioctl DeviceName FileIOCTL FileInBuff FileOutBuff
                        Sends an IOCTL to [DeviceName], and reads a DWORD from FileIOCTL as IOCTL_CODE, 
                        a Input Buffer from [FileInBuff] and writes result on [FileOutbuff]            

# FAQ
¿Trying to read my code? sry...:
![Image of devel](https://github.com/Bondey/ntinfo/blob/master/misc/devel.jpg)