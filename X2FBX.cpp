// X2FBX.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "ueXReader.h"
#include <stdio.h>  
#include <windows.h>  


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

int main(int argc, char* argv[])
{
	string InputFolderPath = argv[1];
	string OutFolderPath = argv[2];
	vector<string> Allfiles = extractFiles(InputFolderPath);
	vector<string> Xfiles;

	int Allfiles_num= Allfiles.size();
	//筛选出.X文件，添加到vector里面
	for (int i=0;i< Allfiles_num;i++)
	{
		auto idx = Allfiles[i].find(".X");
		
		if (idx == string::npos)//不存在。
		{
			continue;
		}
		else//存在。
		{
			Xfiles.push_back(Allfiles[i]);
		}
	}

	//在控制台下显示进度  


	const int NUM=Xfiles.size();//任务完成总量  
	printf("将要转换%d个.X文件 \n", Xfiles.size());
	

	for (int i = 0; i < NUM; i++)
	{
		HWND curhund = GetConsoleHwnd();
		string X = ".X";
		string FBX = ".FBX";
		string tmp = Xfiles[i];
		string s = tmp.replace(tmp.find(InputFolderPath), InputFolderPath.length(), OutFolderPath);
		s = s.replace(s.find(X), X.length(), FBX);

		printf("正在转换第%d个\r", i+1);
		getXInfo(Xfiles[i], curhund, s);
		putchar('\n');
	}

	printf("  恭喜！  转换完成！");

	return 0;

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
