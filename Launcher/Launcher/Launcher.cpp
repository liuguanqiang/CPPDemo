// Launcher.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include<windows.h>
#include <iostream>
#include"stdio.h"
#include <tlhelp32.h>
#include<thread>

#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" ) // 设置入口地址
using namespace std;

LPCWSTR stringToLPCWSTR(string orig);
DWORD FindProcess(char* name);
bool KillProcess(DWORD dwPid);
char* WcharToChar(const wchar_t* wp);
bool Pipelink();
void setIsTimeout();
void KillAllProcess();


//读取超时
bool isTimeOut = false;
DWORD processId = 0;
char * processName = "核桃编程.exe";
//LPCSTR appFilePath = "核桃编程.exe";
LPCSTR appFilePath = "C:\\hetao\\hetaoScratch\\核桃编程.exe";

int main()
{
	HANDLE hMutex = CreateMutex(NULL, FALSE, _T("核桃编程Launcher"));
	try
	{
		//判断Launcher是否已经启动，防止用户多次点击重复启动
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			printf("核桃编程Launcher已经启动\n");
			CloseHandle(hMutex);
			return 0;
		}
		//检测是否存在核桃编程进程
		//1 不存在  直接启动
		//2 存在   进行管道通信  如果能通  退出自己  不能通 强制杀死核桃编程进程  重启核桃编程
		
		 processId = FindProcess(processName);
		if (processId > 0) {
			bool isLink = Pipelink();
			if (isLink) {
				//system("PAUSE");
				return 0;
			}
			KillAllProcess();
		}
	}
	catch (const std::exception& ex)
	{
		cout << "error：  " << ex.what() << endl;
		UINT cb = WinExec(appFilePath, SW_SHOWMAXIMIZED);
		Sleep(3000);
		return 0;
	}

	UINT cb = WinExec(appFilePath, SW_SHOWMAXIMIZED);
	Sleep(3000);
	//system("PAUSE");
	return 0;
}

void KillAllProcess() {
	while (processId > 0)
	{
		bool isKill = KillProcess(processId);
		Sleep(150);
		processId = FindProcess(processName);
	}
}

//管道通信是否成功
bool Pipelink() {
	BOOL bRet = WaitNamedPipe(TEXT("\\\\.\\Pipe\\walnutCodingClient"), NMPWAIT_NOWAIT);
	if (!bRet)
	{
		printf("连接服务端管道失败!\n");
		return false;
	}
	HANDLE hPipe = CreateFile(			//管道属于一种特殊的文件
		TEXT("\\\\.\\Pipe\\walnutCodingClient"),	//创建的文件名
		GENERIC_READ | GENERIC_WRITE,	//文件模式
		0,								//是否共享
		NULL,							//指向一个SECURITY_ATTRIBUTES结构的指针
		OPEN_EXISTING,					//创建参数
		FILE_ATTRIBUTE_NORMAL,			//文件属性(隐藏,只读)NORMAL为默认属性
		NULL);							//模板创建文件的句柄

	if (INVALID_HANDLE_VALUE == hPipe)
	{
		printf("创建管道失败!\n");
		return false;
	}
	else
	{
		thread timeoutThread(setIsTimeout);
		timeoutThread.detach();

		char buf[256] = "ht";
		DWORD wlen = 0;
		if (WriteFile(hPipe, buf, sizeof(buf), &wlen, 0) == FALSE)	//向服务器发送内容
		{
			isTimeOut = true;
			printf("向服务器发送内容失败!\n");
			CloseHandle(hPipe);//关闭管道
			return false;
		}
		else
		{
			char rbuf[256] = "";
			DWORD rlen = 0;
			if (ReadFile(hPipe, rbuf, sizeof(rbuf), &rlen, NULL) == TRUE) {	//接受服务发送过来的内容
				isTimeOut = true;
				printf("接收到服务端消息: data = %s, size = %d\n", rbuf, rlen);
				CloseHandle(hPipe);//关闭管道
				return true;
			}
		}
		isTimeOut = true;
		CloseHandle(hPipe);//关闭管道
		return false;
	}
}

void setIsTimeout() {
	Sleep(3000);
	if (isTimeOut==false) {
		printf("超时!\n");
		KillAllProcess();
		UINT cb = WinExec(appFilePath, SW_SHOWMAXIMIZED);
		cout << "WinExec ：  " << cb << endl;
		Sleep(3000);
		exit(1);
	}
}

LPCWSTR stringToLPCWSTR(string orig)
{
	size_t origsize = orig.length() + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t *wcstring = (wchar_t *)malloc(sizeof(wchar_t)*(orig.length() - 1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);
	return wcstring;
}

//查找进程
DWORD FindProcess(char* name)
{
	PROCESSENTRY32 pe;
	DWORD id = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
		return 0;
	while (1)
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32Next(hSnapshot, &pe) == FALSE)
			break;
		char* output = WcharToChar(pe.szExeFile);
		if (strcmp(output, name) == 0)
		{
			id = pe.th32ProcessID;
			break;
		}
	}
	CloseHandle(hSnapshot);
	return id;
}

//Wchar转Char
char* WcharToChar(const wchar_t* wp)
{
	int len = WideCharToMultiByte(CP_ACP, 0, wp, wcslen(wp), NULL, 0, NULL, NULL);
	char* m_char = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, wp, wcslen(wp), m_char, len, NULL, NULL);
	m_char[len] = '\0';
	return m_char;
}

//杀死进程
bool KillProcess(DWORD ProcessId)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, ProcessId);
	if (hProcess == NULL)
		return false;
	if (!TerminateProcess(hProcess, 0))
		return false;
	return true;
}