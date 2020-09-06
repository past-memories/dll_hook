// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
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
pDefaultAPI pOldAPI = NULL;            //��ָ��pOldAPI����ԭ��API�����ĵ�ַ��ͨ��GetProcAddress��ȡ��
char szOldAPI[12] = { 0x48,0xB8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xC3 };                //���ԭ��API�������ڴ��е�ǰ12���ֽ�
char szNewAPI[12] = { 0x48,0xB8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xC3 };    //������Ǵ�����5���ֽڣ�JMPָ�1���ֽڣ�+�º�����ַ��4���ֽڣ�

char szNewCloseAPI[12];
HMODULE g_hThisModule = NULL;
BOOL WINAPI NewAPI(HANDLE  hFile, LPCVOID lpBuffer, DWORD   nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	//Ϊ����Ϊ
	DWORD yes = GetCurrentProcessId();
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, yes);
	WriteProcessMemory(handle, pOldAPI, szOldAPI, 12, NULL);//��ԭԭ���� 
	
	char *pByte = (char*)lpBuffer;
	
	WCHAR wszClassName[256];
	MultiByteToWideChar(CP_ACP, MB_COMPOSITE, pByte, nNumberOfBytesToWrite + 1, wszClassName, 256);

	WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

	OutputDebugString(wszClassName);
	WriteProcessMemory(handle, pOldAPI, szNewAPI, 12, NULL);//ԭ����ִ���꣬����hook֮��
	CloseHandle(handle);
/*	OutputDebugString(L(("%d"),pByte)));	*/												   //Ϊ����Ϊ
	return TRUE ;
}

bool HookAPI()
{
	//�ҵ�API�������ڴ��еĵ�ַ
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
	ReadProcessMemory((void*)-1, pOldAPI, szOldAPI, 12, NULL);    //����ԭ����ǰ5���ֽ� 
	WriteProcessMemory((void*)-1, pOldAPI, szNewAPI, 12, NULL);    //д�����Ǵ�����5���ֽ� 
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