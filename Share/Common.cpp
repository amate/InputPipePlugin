
#include "Common.h"
#include "ptreeWrapper.h"

fs::path GetExeDirectory()
{
	WCHAR exePath[MAX_PATH] = L"";
	GetModuleFileName(g_hModule, exePath, MAX_PATH);
	fs::path exeFolder = exePath;
	return exeFolder.parent_path();
}

std::wstring GetLastErrorString(DWORD errorcode)
{
	LPVOID lpMsgBuf;
	DWORD ret = ::FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER  //      テキストのメモリ割り当てを要求する
		| FORMAT_MESSAGE_FROM_SYSTEM    //      エラーメッセージはWindowsが用意しているものを使用
		| FORMAT_MESSAGE_IGNORE_INSERTS,//      次の引数を無視してエラーコードに対するエラーメッセージを作成する
		NULL, errorcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),//   言語を指定
		(LPTSTR)&lpMsgBuf,                          //      メッセージテキストが保存されるバッファへのポインタ
		0,
		NULL);
	if (ret == 0) {
		return L"";
	}

	std::wstring errorMsg = (LPCWSTR)lpMsgBuf;
	::LocalFree(lpMsgBuf);
	return errorMsg;
}

////////////////////////////////////////////////////////////////


std::unique_ptr<BYTE[]> SerializeInputInfo(INPUT_INFO* iip)
{
	const int infoGetDataSize = CalcTotalInputInfoSize(iip);
	auto infoGetData = std::make_unique<BYTE[]>(infoGetDataSize);
	// INPUT_INFO をコピー
	memcpy_s(infoGetData.get(), infoGetDataSize, iip, sizeof(INPUT_INFO));
	auto igData = reinterpret_cast<INPUT_INFO*>(infoGetData.get());

	// formatとaudio_formatをそれぞれ 確保したメモリの後ろにコピー
	::memcpy_s(infoGetData.get() + sizeof(INPUT_INFO),
		iip->format_size,
		iip->format, iip->format_size);
	::memcpy_s(infoGetData.get() + sizeof(INPUT_INFO) + iip->format_size,
		iip->audio_format_size,
		iip->audio_format, iip->audio_format_size);

	// それぞれのフォーマットの指し示す先を、確保したメモリから設定
	if (igData->format_size > 0) {
		igData->format = (BITMAPINFOHEADER*)(infoGetData.get() + sizeof(INPUT_INFO));
	} else {
		igData->format = nullptr;
	}
	if (igData->audio_format_size > 0) {
		igData->audio_format = (WAVEFORMATEX*)(infoGetData.get() + sizeof(INPUT_INFO) + igData->format_size);
	} else {
		igData->audio_format = nullptr;
	}
	return infoGetData;
}

std::unique_ptr<BYTE[]> DeserializeInputInfo(const BYTE* serializedData, INPUT_INFO* iip)
{
	auto tempInputInfo = (INPUT_INFO*)serializedData;
	if (tempInputInfo->format_size > 0) {
		tempInputInfo->format = (BITMAPINFOHEADER*)(reinterpret_cast<BYTE*>(tempInputInfo) + sizeof(INPUT_INFO));
	} else {
		tempInputInfo->format = nullptr;
	}
	if (tempInputInfo->audio_format_size > 0) {
		tempInputInfo->audio_format = (WAVEFORMATEX*)(reinterpret_cast<BYTE*>(tempInputInfo) + sizeof(INPUT_INFO) + tempInputInfo->format_size);
	} else {
		tempInputInfo->audio_format = nullptr;
	}

	auto infoGetData = SerializeInputInfo(tempInputInfo);
	INPUT_INFO* igData = reinterpret_cast<INPUT_INFO*>(infoGetData.get());
	*iip = *igData;
	return infoGetData;
}

////////////////////////////////////////////////////////////////

bool Config::LoadConfig()
{
	auto ptree = ptreeWrapper::LoadIniPtree(kConfigFileName);
	bEnableHandleCache = ptree.get<bool>(L"Config.bEnableHandleCache", true);
	bEnableIPC = ptree.get<bool>(L"Config.bEnableIPC", true);
	bUseSharedMemory = ptree.get<bool>(L"Config.bUseSharedMemory", true);

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