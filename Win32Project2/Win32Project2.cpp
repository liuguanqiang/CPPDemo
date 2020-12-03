// Win32Project2.cpp : 定义应用程序的入口点。
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
// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

INT32 dwBuf[4];

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int Json_ReadInt(Json::Value JV, int ori_value = 0);
double Json_ReadDouble(Json::Value JV, double ori_value = 0.0);
string Json_ReadString(Json::Value JV, string ori_value = "");
bool Json_ReadBool(Json::Value JV, bool ori_value = true);
void ReadJsonFile(string appDir);
void UpdatePackageJsonFile(string path, string chromiumValue);
void PNPDeviceIDUpdatePackageJsonFile(string path);
string UTF8ToGB(const char* str);
string CheckCPUID(string json);

string ExeCmd(string pszCmd);
vector<string> split(const string& str, const string& delim);
void trim(string &s);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 在此放置代码。

	// 初始化全局字符串
	//LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadStringW(hInstance, IDC_WIN32PROJECT2, szWindowClass, MAX_LOADSTRING);
	//MyRegisterClass(hInstance);

	CString str1(lpCmdLine);
	std::string STDStr(CW2A(str1.GetString()));
	string appDir = STDStr;
	//string appDir = "C:\\hetao\\hetaoScratch";
	ReadJsonFile(appDir);

	// 执行应用程序初始化: 
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32PROJECT2));

	MSG msg;

	// 主消息循环: 
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
	unsigned len = str.size() * 2;// 预留字节数
	setlocale(LC_CTYPE, "");     //必须调用此函数
	wchar_t *p = new wchar_t[len];// 申请一段内存存放转换后的字符串
	mbstowcs(p, str.c_str(), len);// 转换
	std::wstring str1(p);
	delete[] p;// 释放申请的内存
	return str1;
}
string ExeCmd(string pszCmd)
{
	wstring pszCmd_w = Str2Wstr(pszCmd);
	wcout << "pszCmd_w is " << pszCmd_w << endl;
	// 创建匿名管道,write->read;
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	HANDLE hRead, hWrite;
	if (!CreatePipe(&hRead, &hWrite, &sa, 0))
	{
		return (" ");
	}
	// 设置命令行进程启动信息(以隐藏方式启动命令并定位其输出到hWrite
	STARTUPINFO si = { sizeof(STARTUPINFO) }; // Pointer to STARTUPINFO structure;
	GetStartupInfo(&si);
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	//si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE; //隐藏窗口；
	si.hStdError = hWrite;
	si.hStdError = hWrite;
	si.hStdOutput = hWrite; //管道的输入端口连接命令行的输出；
							// 启动命令行
	PROCESS_INFORMATION pi;// Pointer to PROCESS_INFORMATION structure;
	if (!CreateProcess(NULL, (LPWSTR)pszCmd_w.c_str(), NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
	{
		return ("Cannot create process");
	}
	CloseHandle(hWrite);//关闭管道的输入端口；
						// 读取命令行返回值
	string strRetTmp;
	char buff[1024] = { 0 };
	DWORD dwRead = 0;
	strRetTmp = buff;
	while (ReadFile(hRead, buff, 1024, &dwRead, NULL))//从管道的输出端获取命令行写入的数据；
	{
		strRetTmp += buff;
	}
	CloseHandle(hRead);//关闭管道的输出端口；
	return strRetTmp;
}

string Runcmd(const char *cmd) {
	string	resvec = "";
	FILE *pp = _popen(cmd, "r"); //建立管道
	if (!pp) {
		return resvec;
	}
	char tmp[1024]; //设置一个合适的长度，以存储每一行输出
	while (fgets(tmp, sizeof(tmp), pp) != NULL) {
		resvec += tmp;
	}
	_pclose(pp); //关闭管道
	return resvec;
}

//黑名单检查
string CheckCPUID(string json)
{
	string argsName = "";
	try
	{
		string cmd = "wmic PATH Win32_VideoController GET Description,PNPDeviceID";
		string cmdValue = ExeCmd(cmd);
		string vid = "", did = "";
		int vidIndex = cmdValue.find("VEN_", 0) + 4;
		if (vidIndex >= 4)
		{
			int vidaddIndex = cmdValue.find("&", vidIndex);
			if (vidaddIndex > 0)
			{
				vid = cmdValue.substr(vidIndex, vidaddIndex - vidIndex);
			}
		}
		int didIndex = cmdValue.find("DEV_", 0) + 4;
		if (didIndex >= 4)
		{
			int didaddIndex = cmdValue.find("&", didIndex);
			if (didaddIndex > 0)
			{
				did = cmdValue.substr(didIndex, didaddIndex - didIndex);
			}
		}
		if (vid == "" || did == "")
		{
			return argsName;
		}
		Json::CharReaderBuilder builder;
		Json::CharReader* JsonReader(builder.newCharReader());
		Json::Value root, ObjectTmp;
		JSONCPP_STRING errs;
		const char* pstr = json.c_str();
		if (!JsonReader->parse(pstr, pstr + strlen(pstr), &root, &errs))
		{
			return false;
		}
		Json::Value valRoot = root["--disable-gpu"];
		Json::Value::Members members = valRoot.getMemberNames();
		for (Json::Value::Members::iterator iterMember = members.begin(); iterMember != members.end(); iterMember++)   // 遍历每个key
		{
			string strKey = *iterMember;
			if (strKey == vid)
			{
				int t_size = valRoot[strKey].size();
				for (int i = 0; i < t_size; ++i)
				{
					string didKey = Json_ReadString(valRoot[strKey][i]);
					if (did == didKey)
					{
						argsName = "--disable-gpu";
						return argsName;
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
	try
	{
		CHAR my_documents[MAX_PATH];
		HRESULT result = SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
		if (result == S_OK)
		{
			string packagePath = appDir + "\\package.json";
			string htfile = strcat(my_documents, "\\ht.json");
			ifstream fin;
			fin.open(htfile);
			if (!fin)
			{
				//PNPDeviceIDUpdatePackageJsonFile(packagePath);
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
	catch (const std::exception&)
	{

	}
}


vector<string> split(const string& str, const string& delim) {
	vector<string> res;
	if ("" == str) return res;
	//先将要切割的字符串从string类型转换为char*类型  
	char * strs = new char[str.length() + 1]; //不要忘了  
	strcpy(strs, str.c_str());

	char * d = new char[delim.length() + 1];
	strcpy(d, delim.c_str());

	char *p = strtok(strs, d);
	while (p) {
		string s = p; //分割得到的字符串转换为string类型  
		res.push_back(s); //存入结果数组  
		p = strtok(NULL, d);
	}
	return res;
}

void trim(string &s)
{
	if (s.empty())
	{
		return;
	}
	s.erase(0, s.find_first_not_of(" "));
	s.erase(s.find_last_not_of(" ") + 1);
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
	string chromiumArgs = Json_ReadString(JsonRoot["chromium-args"]);
	vector<string> chromiumValueList = split(chromiumValue, " ");
	for (size_t i = 0; i < chromiumValueList.size(); i++)
	{
		string chromium = chromiumValueList[i];
		int didIndex = chromiumArgs.find(chromium, 0);
		if (didIndex >= 0)
		{
			continue;
		}
		else
		{
			if (chromiumArgs != "")
			{
				chromiumArgs += " ";
			}
			chromiumArgs += chromium;
			JsonRoot["chromium-args"] = Json::Value(chromiumArgs);
		}
	}

	ofstream fout(path);
	if (fout)
	{
		string strContext;
		strContext = JsonRoot.toStyledString();
		fout << strContext;
		fout.close();
	}
}

//通过黑名单检测更新PackageJson
void PNPDeviceIDUpdatePackageJsonFile(string path)
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
	string manifestUrl ="https://config.hetao101.com/scratch3/dev/cocos-client/gpuBlackList.json";
	CWininetHttp whttp = CWininetHttp();
	string json = whttp.RequestJsonInfo(manifestUrl, Hr_Get, "", "");
	if (json == "")
	{
		return;
	}
	string argsName = CheckCPUID(json);

	//string pnpdPath = "C:\\PNPDeviceID.json";
	//ifstream fin1;
	//fin1.open(pnpdPath);
	//if (!fin1)
	//{
	//	return;
	//}
	//ostringstream ostring1;
	//ostring1 << fin1.rdbuf();
	//fin1.close();
	//string strContext1 = ostring1.str();
	//Json::CharReaderBuilder builder1;
	//Json::CharReader* JsonReader1(builder1.newCharReader());
	//Json::Value JsonRoot1, ObjectTmp1;
	//JSONCPP_STRING errs1;
	//const char* pstr1 = strContext1.c_str();
	//if (!JsonReader1->parse(pstr1, pstr1 + strlen(pstr1), &JsonRoot1, &errs1))
	//{
	//	return;
	//}
	//argsName = CheckCPUID(strContext1);

	if (argsName == "")
	{
		return;
	}
	string chromiumArgs = Json_ReadString(JsonRoot["chromium-args"]);
	int didIndex = chromiumArgs.find(argsName, 0);
	if (didIndex >= 0)
	{
		return;
	}
	else
	{
		if (chromiumArgs != "")
		{
			chromiumArgs += " ";
		}
		chromiumArgs += argsName;
		JsonRoot["chromium-args"] = Json::Value(chromiumArgs);
	}
	ofstream fout(path);
	if (fout)
	{
		string strContext;
		strContext = JsonRoot.toStyledString();
		fout << strContext;
		fout.close();
	}
}

string UTF8ToGB(const char* str)
{
	string result;
	WCHAR *strSrc;
	LPSTR szRes;

	//获得临时变量的大小
	int i = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	strSrc = new WCHAR[i + 1];
	MultiByteToWideChar(CP_UTF8, 0, str, -1, strSrc, i);

	//获得临时变量的大小
	i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);
	szRes = new CHAR[i + 1];
	WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

	result = szRes;
	delete[]strSrc;
	delete[]szRes;

	return result;
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
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
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
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 将实例句柄存储在全局变量中

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
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 分析菜单选择: 
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
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
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

// “关于”框的消息处理程序。
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
