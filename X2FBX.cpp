// X2FBX.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "ueXReader.h"
#include <stdio.h>  
#include <windows.h> 
#include<iostream>;
#include <direct.h>


using namespace std;

vector<string> extractFiles(const string& folder)
{
	vector<string> Result;

	if (!exists(filesystem::path(folder)))
	{  //目录不存在直接返回
		return Result;
	}
	auto begin = filesystem::recursive_directory_iterator(folder); //获取文件系统迭代器
	auto end = filesystem::recursive_directory_iterator();    //end迭代器 

	std::vector<string> Vec_del;
	for (auto it = begin; it != end; it++)
	{
		const string spacer(it.depth() * 2, ' ');  //这个是用来排版的空格
		auto& entry = *it;
		if (filesystem::is_regular_file(entry))
		{
			Result.push_back(entry.path().string());
		}
		else if (filesystem::is_directory(entry))
		{
			auto Temp = extractFiles(entry.path().string());
			for (auto Iter : Temp)
			{
				Result.push_back(Iter);
			}
			if (Temp.size() == 0)
			{
				string a = entry.path().string();
				Vec_del.push_back(a);

			}
		}
	}

	for (auto i : Vec_del)
	{
		filesystem::remove(i);
	}

	return Result;
}
HWND GetConsoleHwnd(void)
{
	#define MY_BUFSIZE 1024 // Buffer size for console window titles.
	HWND hwndFound;         // This is what is returned to the caller.
	char pszNewWindowTitle[MY_BUFSIZE]; // Contains fabricated
	// WindowTitle.
	char pszOldWindowTitle[MY_BUFSIZE]; // Contains original
	// WindowTitle.

	// Fetch current window title.

	GetConsoleTitleA(pszOldWindowTitle, MY_BUFSIZE);

	// Format a "unique" NewWindowTitle.

	wsprintfA(pszNewWindowTitle, "%d/%d",
		GetTickCount(),
		GetCurrentProcessId());

	// Change current window title.

	SetConsoleTitleA(pszNewWindowTitle);

	// Ensure window title has been updated.

	Sleep(40);

	// Look for NewWindowTitle.

	hwndFound = FindWindowA(NULL, pszNewWindowTitle);

	// Restore original window title.

	SetConsoleTitleA(pszOldWindowTitle);

	return(hwndFound);
}
//0 是文件夹输入，1是按名字输入，2是创造立方体
int input_type = 1;

int main(int argc, char* argv[])
{
	//0 是输入文件夹   模式
	if (input_type==0)
	{
		string InputFolderPath = argv[1];
		string OutFolderPath = argv[2];

		vector<string> Allfiles = extractFiles(InputFolderPath);
		vector<string> Xfiles;

		int Allfiles_num = Allfiles.size();
		//筛选出.X文件，添加到vector里面
		for (int i = 0; i < Allfiles_num; i++)
		{
			auto idx = Allfiles[i].find(".X");

			if (idx == string::npos)//不存在。
			{
				continue;
			}
			else//存在。
			{
				//筛选出 .X 文件，记录在 Xfiles里
				Xfiles.push_back(Allfiles[i]);
			}
		}
		//在控制台下显示进度  
		const int NUM = Xfiles.size();//需要转化的FBX数量  
		printf("将要转换%d个.X文件 \n", Xfiles.size());
		for (int i = 0; i < NUM; i++)
		{
			HWND curhund = GetConsoleHwnd();
			string X = ".X";
			string FBX = ".FBX";
			string tmp = Xfiles[i];
			string s = tmp.replace(tmp.find(InputFolderPath), InputFolderPath.length(), OutFolderPath);
			s = s.replace(s.find(X), X.length(), FBX);

			printf("正在转换第%d个\r", i + 1);
			getXInfo(Xfiles[i], curhund, s);
			putchar('\n');
		}
		printf("  恭喜！  转换完成！");
		return 0;
	}
	//1 是输入单个文件  模式
	if (input_type == 1)
	{
		cout << "点击右上角关闭按钮退出\n";


		//cout << "请输入.X文件全路径：";
		string ProcessPath = argv[0];
		string RootPath = filesystem::path(ProcessPath).root_path().string();
		string TmpFolder = RootPath + "FbxTmp";
		string cmd = "mkdir " + TmpFolder;
		//system("mkdir D:\\FbxTmp");
		system(cmd.c_str());


		string InputXfile = argv[1];
		string OutFbx = argv[2];
		
		//string OutTmp = "D:\\FbxTmp\\TemporaryReameFBX.fbx";
		string OutTmp = TmpFolder+"\\TemporaryFile.FBX";

		HWND curhund = GetConsoleHwnd();
		string s;
		string x = ".X";
		string littlex = ".x";
		//要一个fbx作为string的复制，因为在replace的时候会改变输入值
		string fbx = InputXfile;
		string::size_type idx = InputXfile.find(x);
		if (idx==string::npos)
		{
			InputXfile.replace(InputXfile.find(littlex), littlex.length(), x);
		}

		s = InputXfile.replace(InputXfile.find(x), x.length(), ".FBX");


		getXInfo(fbx, curhund, OutTmp);

		filesystem::rename(OutTmp, OutFbx);

		cout << "转化完成！\n";

		

		return 0;
	}

	if (input_type == 2)
	{
		CreateCube();
	}

}



char* WcharToChar(wchar_t* wc)
{
	//Release();
	int len = WideCharToMultiByte(CP_ACP, 0, wc, wcslen(wc), NULL, 0, NULL, NULL);
	char* m_char = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, wc, wcslen(wc), m_char, len, NULL, NULL);
	m_char[len] = '\0';
	return m_char;
}

char* WcharToUChar(wchar_t* wc)
{
	//Release();
	int len = WideCharToMultiByte(CP_UTF8, 0, wc, wcslen(wc), NULL, 0, NULL, NULL);
	char* m_char = new char[len + 1];
	WideCharToMultiByte(CP_UTF8, 0, wc, wcslen(wc), m_char, len, NULL, NULL);
	m_char[len] = '\0';
	return m_char;
}


// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
