#include <algorithm>
#include <iostream>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <nlohmann/json.hpp>

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


int callValidator(const std::string& exePath, const std::string& consoleCommand, bool castToFile, const std::string& castingFilePath = "") {

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

	ZeroMemory(&piProcInfo, sizeof(piProcInfo));
	ZeroMemory(&siStartInfo, sizeof(siStartInfo));

	if (castToFile)
	{
		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.hStdError = g_hChildStd_OUT_Wr;
		siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
	}

	if (!CreateProcess
	(
		exePath.c_str(),
		augmentedExePathLPSTR,
		NULL,
		NULL,
		castToFile,
		CREATE_NO_WINDOW,
		NULL,
		NULL,
		&siStartInfo,
		&piProcInfo
	))
	{
		std::cout << "Unable to start " << exePath << std::endl;
		return 0;
	}

	ResumeThread(piProcInfo.hThread);
	DWORD dwExitCode;
	//DWORD dwExitCode;
	if (WaitForSingleObject(piProcInfo.hProcess, INFINITE) == WAIT_OBJECT_0) {
		if (GetExitCodeProcess(piProcInfo.hProcess, &dwExitCode)) {
			if (dwExitCode == STILL_ACTIVE) { std::cout << exePath + " is still active." << std::endl; }
			else { std::cout << exePath + " has terminated with exit code: " << dwExitCode << std::endl; }
		}
		else { std::cerr << "Error getting exit code: " << GetLastError() << std::endl; }
	}
	else { std::cerr << "WaitForSingleObject failed: " << GetLastError() << std::endl; }

	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
	CloseHandle(g_hChildStd_OUT_Wr);

	if (!castToFile) { return 1; }

	std::ofstream castingFile;
	castingFile.open(castingFilePath, std::ofstream::out | std::ofstream::trunc);
	const int bufferSize = 4096;

	DWORD dwRead, dwWritten;
	CHAR chBuf[bufferSize];
	BOOL bSuccessCCatch = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	for (;;)
	{
		bSuccessCCatch = ReadFile(g_hChildStd_OUT_Rd, chBuf, bufferSize, &dwRead, NULL);
		if (!bSuccessCCatch || dwRead == 0) break;
		castingFile << chBuf;
	}

	castingFile.close();
	return 1;
}


bool cjvalValid(const std::string& path) {
	std::fstream  infile(path);
	if (infile.is_open()) {   //checking whether the file is open
		std::string subLine;
		while (std::getline(infile, subLine)) { //read data from file object and put it into string.
			if (subLine == "File is valid") { return true; }
		}
		infile.close(); //close the file object.
	}
	return false;
}

bool val3dityValid(const std::string& path) {
	std::ifstream infile(path);

	nlohmann::json inJson = nlohmann::json::parse(infile);

	if (!inJson.contains("all_errors"))
	{
		return false;
	}

	if (inJson["all_errors"].size() != 0)
	{
		return false;
	}


	return true;
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

void removeAllTemp(const std::vector<std::string>& pathList) {
	for (size_t i = 0; i < pathList.size(); i++)
	{
		remove(pathList[i].c_str());
	}
	return;
}

int main(int argc, char** argv)
{
	if (!checkInput(argc, argv)) { return 0; }

	std::string val3dityTPath = "temp.json";
	std::string cjvalTPath = "temp.txt";
	std::string valLogPath = "val3dity.log";

	std::vector<std::string> PathList = { val3dityTPath, cjvalTPath, valLogPath};

	// TODO: add flags & validate flags 
	// TODO: flag store Val3Dity & cjval output
	// TODO: flag help

	std::string cjvalPath = findPath("applicationEXE/cjval.exe");
	std::string val3dityPath = findPath("applicationEXE/val3dity.exe");

	if (cjvalPath == "" || val3dityPath == "")
	{
		std::cout << "Unable to find validators" << std::endl;
		return 0;
	}

	std::cout << "[INFO] Checking Format" << std::endl;
	if (
		!callValidator(
			cjvalPath, 
			std::string(argv[1]), 
			true, 
			cjvalTPath)
		)
	{ return 0; }
	
	std::cout << "[INFO] Checking Geometry" << std::endl;
	if (
		!callValidator(
			val3dityPath, 
			std::string(argv[1]) + " --report " + val3dityTPath,
			false)
		) 
	{ return 0; }
	std::cout << std::endl;

	//read the report txt
	if (!cjvalValid(cjvalTPath))
	{
		std::cout << "[WARNING] Syntax error found by cjval" << std::endl;
		removeAllTemp(PathList);
		return 0;
	}
	std::cout << "[INFO] No syntax error found valid by cjval" << std::endl;

	//read the report json
	if (!val3dityValid(val3dityTPath))
	{
		std::cout << "[WARNING] Invalid geometry found by val3dity" << std::endl;
		removeAllTemp(PathList);
		return 0;
	}
	std::cout << "[INFO] No invalid geometry found by val3dity" << std::endl;

	// delete all temp files on exit
	removeAllTemp(PathList);

	//TODO: add custom checks

	return 0;
}

