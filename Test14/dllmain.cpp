// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
//#include <windows.h>
#include <tchar.h>
#include<thread>
#include<cstring>
using namespace std;

#ifndef ASSERT
#include <crtdbg.h>
#define ASSERT(X) _ASSERT(X);
#endif
#pragma warning(disable:4996)



typedef BOOL (WINAPI* pDefaultAPI)(
	HANDLE  hFile, LPCVOID lpBuffer, DWORD   nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped
	);
typedef BOOL (WINAPI *pCloseHandleAPI )(
	HANDLE hFile,                                  
	LPVOID lpBuffer,                                
	DWORD nNumberOfBytesToRead,  
	LPDWORD lpNumberOfBytesRead,    
	LPOVERLAPPED lpOverlapped

);

pCloseHandleAPI pCloseHandle;
pDefaultAPI pOldAPI = NULL;            //此指针pOldAPI保存原来API函数的地址（通过GetProcAddress获取）
char szOldAPI[12] = { 0x48,0xB8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xC3 };                //存放原来API函数在内存中的前12个字节
char szNewAPI[12] = { 0x48,0xB8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xC3 };    //存放我们处理后的5个字节，JMP指令（1个字节）+新函数地址（4个字节）

char szNewCloseAPI[12];
HMODULE g_hThisModule = NULL;
BOOL WINAPI NewAPI(HANDLE  hFile, LPCVOID lpBuffer, DWORD   nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	//为所欲为
	DWORD yes = GetCurrentProcessId();
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, yes);
	WriteProcessMemory(handle, pOldAPI, szOldAPI, 12, NULL);//还原原函数 
	
	char *pByte = (char*)lpBuffer;
	
	WCHAR wszClassName[256];
	MultiByteToWideChar(CP_ACP, MB_COMPOSITE, pByte, nNumberOfBytesToWrite + 1, wszClassName, 256);

	WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

	OutputDebugString(wszClassName);
	WriteProcessMemory(handle, pOldAPI, szNewAPI, 12, NULL);//原函数执行完，重新hook之！
	CloseHandle(handle);
/*	OutputDebugString(L(("%d"),pByte)));	*/												   //为所欲为
	return TRUE ;
}

bool HookAPI()
{
	//找到API函数在内存中的地址
	pOldAPI = 0;
	HMODULE hModule = LoadLibrary(_T("kernel32.dll"));
	pOldAPI = (pDefaultAPI)GetProcAddress(hModule, "WriteFile");
	pCloseHandle = (pCloseHandleAPI)GetProcAddress(hModule, "ReadFile");
	memcpy(szNewAPI + 2, &pOldAPI, 8);
	memcpy(szNewCloseAPI + 2, &pCloseHandle, 8);
	ASSERT(pOldAPI);
	ASSERT(pCloseHandle);
	if (!pOldAPI && !pCloseHandle)
	{
		FreeLibrary(hModule);
		return false;
	}


	DWORD64 dwJmpAddr = 0;
	dwJmpAddr = (DWORD64)NewAPI;
	memcpy(szNewAPI + 2, &dwJmpAddr, 8);
	FreeLibrary(hModule);
	ReadProcessMemory((void*)-1, pOldAPI, szOldAPI, 12, NULL);    //读出原来的前5个字节 
	WriteProcessMemory((void*)-1, pOldAPI, szNewAPI, 12, NULL);    //写入我们处理后的5个字节 
	return true;
}

bool UnHookAPI()
{
	return true;
}

extern"C" bool APIENTRY DllMain(HANDLE handle, DWORD dwReason, LPVOID reserved)
{
	g_hThisModule = (HMODULE)handle;
	
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		HookAPI();
		break;
	}
	case DLL_THREAD_ATTACH:
	{
		break;
	}
	case DLL_PROCESS_DETACH:
	{
		UnHookAPI();
		break;
	}
	case DLL_THREAD_DETACH:
	{
		break;
	}
	}

	return TRUE;
}