# KernelInf
My "Always on devel" Tool to automate the collection of different Windows Internals info on a shell tool. (GUIs are bullshit :)

# Usage

KernelInf.exe [Flags]
Flags:

-o ObjectTypeName       Objects to show (if not set, will be shown RootObjects)

-a                      Only will show objects which ACL leaves privs for 'AllUsers' or 'LoggedUser'

-ss                     Try to start as much Services as posible before checking Objects (Driver/Devices)
