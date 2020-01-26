#pragma once
int StartAllServices();
VOID SvcInstall(char* svcname, char* svcpath);
VOID SvcDelete(char* svcname);
int SvcStart(char* svcname);
VOID __stdcall SvcStop(char* szSvcName);
