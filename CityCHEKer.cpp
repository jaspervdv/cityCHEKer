// CityCHEKer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <algorithm>
#include <iostream>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <Windows.h>
#include <strsafe.h>

int checkInput(int argCount, char** argValues) {

	if (argCount < 2)
	{
		std::cout << "no input path supplied" << std::endl;
		return 0;
	}

	std::string inputPath = argValues[1];

	struct stat sb;
	if (stat(inputPath.c_str(), &sb) != 0 || (sb.st_mode & S_IFDIR))
	{
		std::cout << "The Path is invalid!" << std::endl;
		return 0;
	}

	std::transform(inputPath.begin(), inputPath.end(), inputPath.begin(), ::tolower);
	if (inputPath.substr(inputPath.find_last_of(".") + 1) != "json") {
		std::cout << "No CityJSON file supplied!" << std::endl;
		return 0;
	}
	return 1;
}


int callValidator(const std::string& exePath, const std::string& consoleCommand, bool castToFile) {

	HANDLE g_hChildStd_OUT_Rd = NULL;
	HANDLE g_hChildStd_OUT_Wr = NULL;

	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
		return 0;

	if(!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		return 0;

	TCHAR szCmdline[] = TEXT("child");
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFOA siStartInfo;
	BOOL bSuccessProcess = FALSE;

	std::string augmentedExePath = exePath + " " + consoleCommand;
	LPSTR augmentedExePathLPSTR = const_cast<char*>(augmentedExePath.c_str());

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	bSuccessProcess = CreateProcessA
	(
		exePath.c_str(),
		augmentedExePathLPSTR,
		NULL,
		NULL,
		TRUE,
		0,
		NULL,
		NULL,
		&siStartInfo,
		&piProcInfo
	);

	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
	CloseHandle(g_hChildStd_OUT_Wr);

	if (!castToFile) { return 1; }

	std::ofstream castingFile;
	castingFile.open("~temp.txt", std::ofstream::out | std::ofstream::trunc);
	const int bufferSize = 4096;

	DWORD dwRead, dwWritten;
	CHAR chBuf[bufferSize];
	BOOL bSuccessCCatch = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	for (;;)
	{
		bSuccessCCatch = ReadFile(g_hChildStd_OUT_Rd, chBuf, bufferSize, &dwRead, NULL);
		if (!bSuccessCCatch || dwRead == 0) break;

		std::string t = chBuf;
		std::vector<std::string> ansiiStringList = { "[1m", "[0m", "�", "", "nts", "", "", "=[0m", "✅"};

		for (size_t i = 0; i < ansiiStringList.size(); i++)
		{
			std::string::size_type j = t.find(ansiiStringList[i]);
			if (j != std::string::npos)
				t.erase(j, ansiiStringList[i].length());
		}
		castingFile << t;
	}

	castingFile.close();
	return 1;
}


std::string findPath(const std::string& pathEnd) {

	std::string tempPath = pathEnd;

	struct stat sb;
	if (stat(tempPath.c_str(), &sb) == 0) { return tempPath; }

	for (size_t i = 0; i < 3; i++)
	{
		tempPath = "../" + tempPath;
		if (stat(tempPath.c_str(), &sb) == 0) { return tempPath; }
	}
	return "";
}

int main(int argc, char** argv)
{
	if (!checkInput(argc, argv)) { return 0; }

	std::string cjvalPath = findPath("applicationEXE/cjval.exe");
	std::string val3dityPath = findPath("applicationEXE/val3dity.exe");

	if (cjvalPath == "" || val3dityPath == "")
	{
		std::cout << "Unable to find validators" << std::endl;
		return 0;
	}

	std::cout << "[INFO] Checking Format" << std::endl;
	if (!callValidator(cjvalPath, std::string(argv[1]), true)) { return 0; }

	std::cout << "[INFO] Checking Geometry" << std::endl;
	if (!callValidator(val3dityPath, std::string(argv[1]) + " --report ~temp.json", false)) { return 0; }
	//TODO: read the report txt
	//TODO: delete the report txt

	//TODO: read the report json
	//TODO: delete the report json

	//TODO: add custom checks

	return 0;
}

