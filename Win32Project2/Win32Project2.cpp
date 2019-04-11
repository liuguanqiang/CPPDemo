// Win32Project2.cpp : ����Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "Win32Project2.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include "json\json.h"
#include <atlstr.h>
#include <shlobj.h>
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include "WinHttp.h"
#pragma comment(lib, "shell32.lib")

#define MAX_LOADSTRING 100

using namespace std;
// ȫ�ֱ���: 
HINSTANCE hInst;                                // ��ǰʵ��
WCHAR szTitle[MAX_LOADSTRING];                  // �������ı�
WCHAR szWindowClass[MAX_LOADSTRING];            // ����������

INT32 dwBuf[4];

// �˴���ģ���а����ĺ�����ǰ������: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int Json_ReadInt(Json::Value JV, int ori_value = 0);
double Json_ReadDouble(Json::Value JV, double ori_value = 0.0);
string Json_ReadString(Json::Value JV, string ori_value = "");
bool Json_ReadBool(Json::Value JV, bool ori_value = true);
void WriteJsonFile();
void ReadJsonFile(string appDir);
void UpdatePackageJsonFile(string path, string chromiumValue);
string UTF8ToGB(const char* str);
string CheckCPUID(string json);

string ExeCmd(string pszCmd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: �ڴ˷��ô��롣

	// ��ʼ��ȫ���ַ���
	//LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadStringW(hInstance, IDC_WIN32PROJECT2, szWindowClass, MAX_LOADSTRING);
	//MyRegisterClass(hInstance);

	CString str1(lpCmdLine);
	std::string STDStr(CW2A(str1.GetString()));
	string appDir = STDStr;
	ReadJsonFile(appDir);

	// ִ��Ӧ�ó����ʼ��: 
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32PROJECT2));

	MSG msg;

	// ����Ϣѭ��: 
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

wstring Str2Wstr(string str)
{
	unsigned len = str.size() * 2;// Ԥ���ֽ���
	setlocale(LC_CTYPE, "");     //������ô˺���
	wchar_t *p = new wchar_t[len];// ����һ���ڴ���ת������ַ���
	mbstowcs(p, str.c_str(), len);// ת��
	std::wstring str1(p);
	delete[] p;// �ͷ�������ڴ�
	return str1;
}
string ExeCmd(string pszCmd)
{
	wstring pszCmd_w = Str2Wstr(pszCmd);
	wcout << "pszCmd_w is " << pszCmd_w << endl;
	// ���������ܵ�,write->read;
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	HANDLE hRead, hWrite;
	if (!CreatePipe(&hRead, &hWrite, &sa, 0))
	{
		return (" ");
	}
	// ���������н���������Ϣ(�����ط�ʽ���������λ�������hWrite
	STARTUPINFO si = { sizeof(STARTUPINFO) }; // Pointer to STARTUPINFO structure;
	GetStartupInfo(&si);
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	//si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE; //���ش��ڣ�
	si.hStdError = hWrite;
	si.hStdError = hWrite;
	si.hStdOutput = hWrite; //�ܵ�������˿����������е������
							// ����������
	PROCESS_INFORMATION pi;// Pointer to PROCESS_INFORMATION structure;
	if (!CreateProcess(NULL, (LPWSTR)pszCmd_w.c_str(), NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
	{
		return ("Cannot create process");
	}
	CloseHandle(hWrite);//�رչܵ�������˿ڣ�
						// ��ȡ�����з���ֵ
	string strRetTmp;
	char buff[1024] = { 0 };
	DWORD dwRead = 0;
	strRetTmp = buff;
	while (ReadFile(hRead, buff, 1024, &dwRead, NULL))//�ӹܵ�������˻�ȡ������д������ݣ�
	{
		strRetTmp += buff;
	}
	CloseHandle(hRead);//�رչܵ�������˿ڣ�
	return strRetTmp;
}

string Runcmd(const char *cmd) {
	string	resvec = "";
	FILE *pp = _popen(cmd, "r"); //�����ܵ�
	if (!pp) {
		return resvec;
	}
	char tmp[1024]; //����һ�����ʵĳ��ȣ��Դ洢ÿһ�����
	while (fgets(tmp, sizeof(tmp), pp) != NULL) {
		resvec += tmp;
	}
	_pclose(pp); //�رչܵ�
	return resvec;
}

//���������
string CheckCPUID(string json)
{
	string argsName = "";
	try
	{
		string cmd = "wmic PATH Win32_VideoController GET Description,PNPDeviceID";
		string cmdValue = ExeCmd(cmd);
		string vid = "", did = "";
		int vidIndex = cmdValue.find("VEN_", 0) + 4;
		if (vidIndex >= 0)
		{
			vid = cmdValue.substr(vidIndex, 4);
		}
		int didIndex = cmdValue.find("DEV_", 0) + 4;
		if (didIndex >= 0)
		{
			did = cmdValue.substr(didIndex, 4);
		}
		if (vid == "" || did == "")
		{
			return argsName;
		}
		Json::CharReaderBuilder builder;
		Json::CharReader* JsonReader(builder.newCharReader());
		Json::Value valRoot, ObjectTmp;
		JSONCPP_STRING errs;
		const char* pstr = json.c_str();
		if (!JsonReader->parse(pstr, pstr + strlen(pstr), &valRoot, &errs))
		{
			return false;
		}
		Json::Value::Members members = valRoot.getMemberNames();
		for (Json::Value::Members::iterator iterMember = members.begin(); iterMember != members.end(); iterMember++)   // ����ÿ��key
		{
			string strKey = *iterMember;
			Json::Value child = valRoot[strKey];
			Json::Value::Members childmembers1 = child.getMemberNames();
			for (Json::Value::Members::iterator childMember1 = childmembers1.begin(); childMember1 != childmembers1.end(); childMember1++)   // �����ӽڵ�ÿ��key
			{
				string childKey = *childMember1;
				if (childKey == vid)
				{
					int t_size = valRoot[strKey][childKey].size();
					for (int i = 0; i < t_size; ++i)
					{
						if (did == Json_ReadString(valRoot[strKey][childKey][i]))
						{
							argsName = strKey;
							return argsName;
						}
					}
				}
			}
		}
	}
	catch (const std::exception&)
	{
		return "";
	}
	return argsName;
}

void ReadJsonFile(string appDir)
{
	CHAR my_documents[MAX_PATH];
	HRESULT result = SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
	if (result == S_OK)
	{
		appDir = "C:\\Program Files (x86)\\���ұ��";
		string packagePath = appDir + "\\package.json";
		string htfile = strcat(my_documents, "\\ht.json");
		ifstream fin;
		fin.open(htfile);
		if (!fin)
		{
			//__cpuidex(dwBuf, 1, 1);
			//char szTmp[33] = { NULL };
			//sprintf(szTmp, "%08X%08X", dwBuf[3], dwBuf[0]);
			//string cpuID = szTmp;
			CWininetHttp whttp = CWininetHttp();
			string url = "https://config.hetao101.com/scratch3/prod/windows/0.0.1.json";
			string json = whttp.RequestJsonInfo(url, Hr_Get, "", "");
			if (json == "")
			{
				return;
			}
			string htfile1 = "E:\\PNPDeviceID.json";
			ifstream fin;
			fin.open(htfile1);
			if (!fin)
			{
				return;
			}
			//"chromium-args" : "--ignore-gpu-blacklist1234",
			ostringstream ostring;
			ostring << fin.rdbuf();
			fin.close();
			string strContext = ostring.str();

			string argsName = CheckCPUID(strContext);
			if (argsName != "")
			{
				UpdatePackageJsonFile(packagePath, argsName);
			}
			return;
		}

		ostringstream ostring;
		ostring << fin.rdbuf();
		fin.close();
		string strContext = ostring.str();
		Json::CharReaderBuilder builder;
		Json::CharReader* JsonReader(builder.newCharReader());
		Json::Value JsonRoot, ObjectTmp;
		JSONCPP_STRING errs;
		const char* pstr = strContext.c_str();
		if (!JsonReader->parse(pstr, pstr + strlen(pstr), &JsonRoot, &errs))
		{
			return;
		}
		string chromiumArgs = Json_ReadString(JsonRoot["chromium-args"]);
		UpdatePackageJsonFile(packagePath, chromiumArgs);
	}
}

string UTF8ToGB(const char* str)
{
	string result;
	WCHAR *strSrc;
	LPSTR szRes;

	//�����ʱ�����Ĵ�С
	int i = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	strSrc = new WCHAR[i + 1];
	MultiByteToWideChar(CP_UTF8, 0, str, -1, strSrc, i);

	//�����ʱ�����Ĵ�С
	i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);
	szRes = new CHAR[i + 1];
	WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

	result = szRes;
	delete[]strSrc;
	delete[]szRes;

	return result;
}


void UpdatePackageJsonFile(string path, string chromiumValue)
{
	ifstream fin;
	fin.open(path);
	if (!fin)
	{
		return;
	}
	ostringstream ostring;
	ostring << fin.rdbuf();
	fin.close();
	string strContext = ostring.str();
	Json::CharReaderBuilder builder;
	Json::CharReader* JsonReader(builder.newCharReader());
	Json::Value JsonRoot, ObjectTmp;
	JSONCPP_STRING errs;
	const char* pstr = strContext.c_str();
	if (!JsonReader->parse(pstr, pstr + strlen(pstr), &JsonRoot, &errs))
	{
		return;
	}
	string oldChromiumArgs = Json_ReadString(JsonRoot["chromium-args"]);
	if (oldChromiumArgs == chromiumValue)
	{
		return;
	}
	JsonRoot["chromium-args"] = Json::Value(chromiumValue);
	ofstream fout(path);
	if (fout)
	{
		string strContext;
		strContext = JsonRoot.toStyledString();
		fout << strContext;
		fout.close();
	}
}


void WriteJsonFile()
{
	CHAR my_documents[MAX_PATH];
	HRESULT result = SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
	// ����Json�����������Ϊ��
	Json::Value JsonRoot;
	// д���ַ���
	JsonRoot["name"] = Json::Value(my_documents);
	// д������
	JsonRoot["age"] = Json::Value(22);
	// д�븡��������
	JsonRoot["height"] = Json::Value(1.78);
	// д�벼����
	JsonRoot["play_football"] = Json::Value(true);
	// д��Json����
	Json::Value JsonObj;
	JsonObj["sometime"] = Json::Value(2018);
	JsonObj["someone"] = Json::Value("Kelly");
	JsonObj["somewhere"] = Json::Value("city");
	JsonRoot["object"] = JsonObj;
	// ������д����������
	JsonRoot["number_array"].append(1);
	JsonRoot["number_array"].append(2);
	JsonRoot["number_array"].append(3);
	JsonRoot["number_array"].append(4);
	// ������д���ַ�������
	JsonRoot["string_array"].append("string01");
	JsonRoot["string_array"].append("string02");
	JsonRoot["string_array"].append("string03");
	// д��Json�������飬�������ɶ��󹹳�
	Json::Value JsonArr1, JsonArr2, JsonArr3;
	JsonArr1["string1"] = Json::Value("1-1");
	JsonArr2["string1"] = Json::Value("2-1");
	JsonArr3["string1"] = Json::Value("3-1");
	JsonRoot["object_array"].append(JsonArr1);
	JsonRoot["object_array"].append(JsonArr2);
	JsonRoot["object_array"].append(JsonArr3);
	// ����Json�ļ�����
	ofstream fout("E:\\jsonfile.json");
	if (fout)
	{
		string strContext;
		strContext = JsonRoot.toStyledString();
		fout << strContext;
		fout.close();
	}
}

///////////////////////////////////////////////////
int Json_ReadInt(Json::Value JV, int ori_value)
{
	int result = ori_value;
	Json::ValueType VT = JV.type();
	if (VT == Json::ValueType::intValue)
		result = JV.asInt();
	return result;
}
double Json_ReadDouble(Json::Value JV, double ori_value)
{
	double result = ori_value;
	Json::ValueType VT = JV.type();
	if (VT == Json::ValueType::realValue)
		result = JV.asDouble();
	return result;
}
string Json_ReadString(Json::Value JV, string ori_value)
{
	string result = ori_value;
	Json::ValueType VT = JV.type();
	if (VT == Json::ValueType::stringValue)
		result = JV.asCString();
	return result;
}
bool Json_ReadBool(Json::Value JV, bool ori_value)
{
	bool result = ori_value;
	Json::ValueType VT = JV.type();
	if (VT == Json::ValueType::booleanValue)
		result = JV.asBool();
	return result;
}



//
//  ����: MyRegisterClass()
//
//  Ŀ��: ע�ᴰ���ࡣ
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32PROJECT2));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WIN32PROJECT2);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   ����: InitInstance(HINSTANCE, int)
//
//   Ŀ��: ����ʵ�����������������
//
//   ע��: 
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  ����: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��:    ���������ڵ���Ϣ��
//
//  WM_COMMAND  - ����Ӧ�ó���˵�
//  WM_PAINT    - ����������
//  WM_DESTROY  - �����˳���Ϣ������
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// �����˵�ѡ��: 
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: �ڴ˴�����ʹ�� hdc ���κλ�ͼ����...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// �����ڡ������Ϣ��������
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}