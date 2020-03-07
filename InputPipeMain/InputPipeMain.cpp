// InputPipeMain.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "InputPipeMain.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <assert.h>
#include "..\Share\IPC.h"
//#include "..\Share\Logger.h"
#include "..\Share\Common.h"
#include "..\Share\PluginWrapper.h"
#include "..\InputPipePlugin\input.h"
#include "MainDlg.h"

// グローバル変数:
CAppModule _Module;
HMODULE g_hModule;

HMODULE	g_hWinputDll = NULL;
INPUT_PLUGIN_TABLE* g_winputPluginTable = nullptr;

std::vector<BYTE>	g_readVideoBuffer;
std::vector<BYTE>	g_readAudioBuffer;

// for Logger
std::string	LogFileName()
{
	return (GetExeDirectory() / L"InputPipeMain.log").string();
}

int Run()
{
	HRESULT hRes = ::CoInitialize(NULL);
	// If you are running on NT 4.0 or higher you can use the following call instead to 
	// make the EXE free threaded. This means that calls come in on a random RPC thread.
	//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, g_hModule);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = 0;
	{
		CMessageLoop theLoop;
		_Module.AddMessageLoop(&theLoop);

		CMainDlg dlgMain;

		if (dlgMain.Create(NULL) == NULL)
		{
			ATLTRACE(_T("Main dialog creation failed!\n"));
			return 0;
		}

		dlgMain.ShowWindow(SW_SHOWNORMAL);

		nRet = theLoop.Run();

		_Module.RemoveMessageLoop();
	}

	_Module.Term();
	::CoUninitialize();
	return nRet;
}

///////////////////////////////////////////////////////////////

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	g_hModule = hInstance;
#if 1
	std::wstring cmdLine = lpCmdLine;//::GetCommandLine();
	// INFO_LOG << L"CommandLine: " << cmdLine;
	if (cmdLine.empty()) {
		bool pluginExists = fs::exists((GetExeDirectory() / L"lwinput.aui"));
		MessageBox(NULL, pluginExists ? L"lwinput.aui が同じフォルダに存在しています！" 
									  : L"lwinput.aui が同じフォルダに存在しません...", L"InputPipeMain", MB_OK);
		return 0;
	} else if (cmdLine == L"-config") {
		int ret = Run();
		return ret;
	}

	auto llhandle = std::stoll(cmdLine);
	HANDLE hEvent = (HANDLE)llhandle;

	size_t spacePos = cmdLine.find(L' ');
	// INFO_LOG << L"spacePos: " << spacePos;
	assert(spacePos != std::wstring::npos);
	std::wstring pipeName = cmdLine.substr(spacePos + 1);
	size_t tailSpacePos = pipeName.find(L' ');
	if (tailSpacePos != std::wstring::npos) {
		pipeName.resize(tailSpacePos);
	}

	std::wstring kJobName = L"InputPipePlugin_Job" + std::to_wstring((uint64_t)hEvent);
	HANDLE hJob = ::OpenJobObject(JOB_OBJECT_ASSIGN_PROCESS, FALSE, kJobName.c_str());
	//assert(hJob);
	if (hJob == NULL) {
		return 0;
	}
	::AssignProcessToJobObject(hJob, GetCurrentProcess());
	::CloseHandle(hJob);

	{
		g_hWinputDll = ::LoadLibrary((GetExeDirectory() /L"lwinput.aui").c_str());
		assert(g_hWinputDll);
		if (g_hWinputDll == NULL) {
			//WARN_LOG << L"LoadLibrary failed (lwinput.aui)";
			return 0;
		}

		using GetInputPluginTableFunc = INPUT_PLUGIN_TABLE * (__stdcall*)(void);
		GetInputPluginTableFunc funcGetTable = (GetInputPluginTableFunc)::GetProcAddress(g_hWinputDll, "GetInputPluginTable");

		g_winputPluginTable = funcGetTable();
	}

#endif

	std::wstring randomString;
	SharedMemory	videoSharedMemory;
	SharedMemory	audioSharedMemory;
	std::pair<void*, int>	videoSharedBuffer = { nullptr, 0 };
	std::pair<void*, int>	audioSharedBuffer = { nullptr, 0 };
	bool bUseSharedMemory = false;
	if (cmdLine.find(L"-sharedMemory") != std::wstring::npos) {
		bUseSharedMemory = true;

		auto randPos = cmdLine.find(L'_');
		assert(randPos != std::wstring::npos);
		auto randPosEnd = cmdLine.find(L'_', randPos + 1);
		assert(randPosEnd != std::wstring::npos);
		randomString = cmdLine.substr(randPos, randPosEnd - randPos + 1);
	}


	// TODO: ここにコードを挿入してください。
	//InitHook();
	NamedPipe namedPipe;
	bool success = namedPipe.OpenNamedPipe(pipeName);
	if (!success) {
		return 0;
	}

	// for Debug
	CallFunc	lastCallFunc;
	bool activeLoop = true;
	while (activeLoop) {
		std::vector<BYTE> readData = namedPipe.Read(kToWindDataHeaderSize);
		if (readData.size() == 0) {
			assert(false);
			break;
		}
		ToWinputData* toData = (ToWinputData*)readData.data();
		lastCallFunc = toData->header.callFunc;

		std::vector<BYTE> dataBody = namedPipe.Read(toData->header.paramSize);
		switch (toData->header.callFunc) {
			case CallFunc::kOpen:
			{
				LPSTR file = (LPSTR)dataBody.data();
				INPUT_HANDLE ih = Plugin_func_open(file);
				//INFO_LOG << L"kOpen: " << ih;

				auto fromData = GenerateFromInputData(CallFunc::kOpen, ih, 0);
				namedPipe.Write((const BYTE*)fromData.get(), FromWinputDataTotalSize(*fromData));
				//INFO_LOG << L"Write: " << FromWinputDataTotalSize(*fromData) << L" bytes";
			}
			break;

			case CallFunc::kClose:
			{
				StandardParamPack* spp = (StandardParamPack*)dataBody.data();
				BOOL b = Plugin_func_close(spp->ih);
				//INFO_LOG << L"kClose: " << spp->ih;

				auto fromData = GenerateFromInputData(CallFunc::kClose, b, 0);
				namedPipe.Write((const BYTE*)fromData.get(), FromWinputDataTotalSize(*fromData));
				//INFO_LOG << L"Write: " << FromWinputDataTotalSize(*fromData) << L" bytes";
			}
			break;

			case CallFunc::kInfoGet:
			{

				StandardParamPack* spp = (StandardParamPack*)dataBody.data();
				INPUT_INFO inputInfo = {};
				BOOL b = Plugin_func_info_get(spp->ih, &inputInfo);
				assert(b);
				//INFO_LOG << L"kInfoGet: " << spp->ih;
				if (b) {
					const int totalInputInfoSize = CalcTotalInputInfoSize(&inputInfo);
					auto infoGetData = SerializeInputInfo(&inputInfo);

					auto fromData = GenerateFromInputData(CallFunc::kInfoGet, b, infoGetData.get(), totalInputInfoSize);
					namedPipe.Write((const BYTE*)fromData.get(), FromWinputDataTotalSize(*fromData));
					//INFO_LOG << L"Write: " << FromWinputDataTotalSize(*fromData) << L" bytes";

				} else {
					auto fromData = GenerateFromInputData(CallFunc::kInfoGet, b, 0);
					namedPipe.Write((const BYTE*)fromData.get(), FromWinputDataTotalSize(*fromData));
					//INFO_LOG << L"Write: " << FromWinputDataTotalSize(*fromData) << L" bytes";
				}
			}
			break;

			case CallFunc::kReadVideo:
			{
				StandardParamPack* spp = (StandardParamPack*)dataBody.data();
				void* buf;
				bool sharedMemoryReopen = false;
				if (bUseSharedMemory) {
					if (videoSharedBuffer.second < spp->perBufferSize) {
						videoSharedMemory.CloseHandle();
						std::wstring sharedMemoryName = kVideoSharedMemoryPrefix + randomString + std::to_wstring(spp->perBufferSize);
						videoSharedBuffer.first = videoSharedMemory.CreateSharedMemory(sharedMemoryName.c_str(), spp->perBufferSize);
						if (!videoSharedBuffer.first) {
							DWORD error = ::GetLastError();
							std::wstring errorMsg = L"videoSharedMemory.CreateSharedMemoryに失敗\nsharedMemoryName: " + sharedMemoryName + L"\nGetLastError: " + std::to_wstring(error);
							MessageBox(NULL, errorMsg.c_str(), L"InputPipeMainエラー", MB_ICONERROR);;
						}
						videoSharedBuffer.second = spp->perBufferSize;
						sharedMemoryReopen = true;
					}
					buf = videoSharedBuffer.first;
				} else {
					if (static_cast<int>(g_readVideoBuffer.size()) < spp->perBufferSize) {
						g_readVideoBuffer.resize(spp->perBufferSize);
					}
					buf = g_readVideoBuffer.data();
				}
				const int frame = spp->param1;
				int readBytes = Plugin_func_read_video(spp->ih, spp->param1, buf);
				//INFO_LOG << L"kReadVideo: " << spp->ih;

				namedPipe.Write((const BYTE*)&toData->header.callFunc, sizeof(toData->header.callFunc));
				std::int32_t totalSize = sizeof(int) + readBytes;
				namedPipe.Write((const BYTE*)&totalSize, sizeof(totalSize));
				namedPipe.Write((const BYTE*)&readBytes, sizeof(readBytes));
				if (bUseSharedMemory) {
					namedPipe.Write((const BYTE*)&sharedMemoryReopen, sizeof(sharedMemoryReopen));
				} else {
					namedPipe.Write((const BYTE*)g_readVideoBuffer.data(), readBytes);
				}
				//auto fromData = GenerateFromInputData(toData->header.callFunc, readBytes, g_readVideoBuffer.data(), readBytes);
				//namedPipe.Write((const BYTE*)fromData.get(), FromWinputDataTotalSize(*fromData));
				//INFO_LOG << L"Write: " << FromWinputDataTotalSize(*fromData) << L" bytes";
			}
			break;

			case CallFunc::kReadAudio:
			{
				StandardParamPack* spp = (StandardParamPack*)dataBody.data();
				const int PerAudioSampleBufferSize = spp->perBufferSize;
				const int requestReadBytes = PerAudioSampleBufferSize * spp->param2;
				void* buf;
				bool sharedMemoryReopen = false;
				if (bUseSharedMemory) {
					if (audioSharedBuffer.second < requestReadBytes) {
						audioSharedMemory.CloseHandle();
						std::wstring sharedMemoryName = kAudioSharedMemoryPrefix + randomString + std::to_wstring(requestReadBytes);
						audioSharedBuffer.first = audioSharedMemory.CreateSharedMemory(sharedMemoryName.c_str(), requestReadBytes);
						if (!audioSharedBuffer.first) {
							DWORD error = ::GetLastError();
							std::wstring errorMsg = L"audioSharedMemory.CreateSharedMemoryに失敗\nsharedMemoryName: " + sharedMemoryName + L"\nGetLastError: " + std::to_wstring(error);
							MessageBox(NULL, errorMsg.c_str(), L"InputPipeMainエラー", MB_ICONERROR);;
						}
						audioSharedBuffer.second = requestReadBytes;
						sharedMemoryReopen = true;
					}
					buf = audioSharedBuffer.first;
				} else {
					if (static_cast<int>(g_readAudioBuffer.size()) < requestReadBytes) {
						g_readAudioBuffer.resize(requestReadBytes);
					}
					buf = g_readAudioBuffer.data();
				}

				int readSample = Plugin_func_read_audio(spp->ih, spp->param1, spp->param2, buf);
				//assert(readSample > 0);
				//INFO_LOG << L"kReadAudio: " << spp->ih << L" readSample: " << readSample;
				const int readBufferSize = PerAudioSampleBufferSize * readSample;

				namedPipe.Write((const BYTE*)& toData->header.callFunc, sizeof(toData->header.callFunc));
				std::int32_t totalSize = sizeof(int) + readBufferSize;
				namedPipe.Write((const BYTE*)&totalSize, sizeof(totalSize));
				namedPipe.Write((const BYTE*)&readSample, sizeof(readSample));
				if (bUseSharedMemory) {
					namedPipe.Write((const BYTE*)&sharedMemoryReopen, sizeof(sharedMemoryReopen));
				} else {
					namedPipe.Write((const BYTE*)g_readAudioBuffer.data(), readBufferSize);
				}
				//auto fromData = GenerateFromInputData(toData->header.callFunc, readSample, g_readAudioBuffer.data(), readBufferSize);
				//namedPipe.Write((const BYTE*)fromData.get(), FromWinputDataTotalSize(*fromData));
				//INFO_LOG << L"Write: " << FromWinputDataTotalSize(*fromData) << L" bytes";
			}
			break;

			case CallFunc::kIsKeyframe:
			{
				//INFO_LOG << L"kIsKeyframe";

				StandardParamPack* spp = (StandardParamPack*)dataBody.data();
				BOOL b = g_winputPluginTable->func_is_keyframe(spp->ih, spp->param1);

				auto fromData = GenerateFromInputData(CallFunc::kIsKeyframe, b, 0);
				namedPipe.Write((const BYTE*)fromData.get(), FromWinputDataTotalSize(*fromData));
			}
			break;

			case CallFunc::kExit:
			{
				//INFO_LOG << L"kExit";
				namedPipe.Disconnect();
				activeLoop = false;
			}
			break;

			default:
				assert(false);
				break;
		}

	};
	::FreeLibrary(g_hWinputDll);
	g_hWinputDll = NULL;

#if 0
	int result = 0;
	DWORD ret = ::WaitForSingleObject(hEvent, INFINITE);
	if (ret == WAIT_OBJECT_0) {
		result = 1;
	} else if (ret == WAIT_TIMEOUT) {
		result = 2;
	} else {
		DWORD err = ::GetLastError();
		result = 3;
}
#endif
	return 0;
}

