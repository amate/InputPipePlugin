
#include "Common.h"
#include "ptreeWrapper.h"

fs::path GetExeDirectory()
{
	WCHAR exePath[MAX_PATH] = L"";
	GetModuleFileName(g_hModule, exePath, MAX_PATH);
	fs::path exeFolder = exePath;
	return exeFolder.parent_path();
}


bool Config::LoadConfig()
{
	auto ptree = ptreeWrapper::LoadIniPtree(kConfigFileName);
	bEnableHandleCache = ptree.get<bool>(L"Config.bEnableHandleCache", true);
	bEnableIPC = ptree.get<bool>(L"Config.bEnableIPC", true);
	bUseSharedMemory = ptree.get<bool>(L"Config.bUseSharedMemory", false);

	return true;
}

bool Config::SaveConfig()
{
	ptreeWrapper::wptree ptree;
	ptree.put(L"Config.bEnableHandleCache", bEnableHandleCache);
	ptree.put(L"Config.bEnableIPC", bEnableIPC);
	ptree.put(L"Config.bUseSharedMemory", bUseSharedMemory);

	bool success = ptreeWrapper::SaveIniPtree(kConfigFileName, ptree);
	return success;
}