// InputPipeMain.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "InputPipeMain.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <assert.h>
#include "..\Share\IPC.h"
#include "..\Share\Logger.h"
#include "..\Share\Common.h"
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
	const std::wstring pipeName = cmdLine.substr(spacePos + 1);

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
			WARN_LOG << L"LoadLibrary failed (lwinput.aui)";
			return 0;
		}

		using GetInputPluginTableFunc = INPUT_PLUGIN_TABLE * (__stdcall*)(void);
		GetInputPluginTableFunc funcGetTable = (GetInputPluginTableFunc)::GetProcAddress(g_hWinputDll, "GetInputPluginTable");

		g_winputPluginTable = funcGetTable();
	}

#endif
	// TODO: ここにコードを挿入してください。
	//InitHook();
	NamedPipe namedPipe;
	bool success = namedPipe.OpenNamedPipe(pipeName);
	if (!success) {
		return 0;
	}

	// for Debug
	CallFunc	lastCallFunc;

	for (;;) {
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
				INPUT_HANDLE ih = g_winputPluginTable->func_open(file);
				INFO_LOG << L"kOpen: " << ih;

				auto fromData = GenerateFromInputData(CallFunc::kOpen, ih, 0);
				namedPipe.Write((const BYTE*)fromData.get(), FromWinputDataTotalSize(*fromData));
				//INFO_LOG << L"Write: " << FromWinputDataTotalSize(*fromData) << L" bytes";
			}
			break;

			case CallFunc::kClose:
			{
				StandardParamPack* spp = (StandardParamPack*)dataBody.data();
				BOOL b = g_winputPluginTable->func_close(spp->ih);
				INFO_LOG << L"kClose: " << spp->ih;

				auto fromData = GenerateFromInputData(CallFunc::kClose, b, 0);
				namedPipe.Write((const BYTE*)fromData.get(), FromWinputDataTotalSize(*fromData));
				//INFO_LOG << L"Write: " << FromWinputDataTotalSize(*fromData) << L" bytes";
			}
			break;

			case CallFunc::kInfoGet:
			{

				StandardParamPack* spp = (StandardParamPack*)dataBody.data();
				INPUT_INFO inputInfo = {};
				BOOL b = g_winputPluginTable->func_info_get(spp->ih, &inputInfo);
				assert(b);
				INFO_LOG << L"kInfoGet: " << spp->ih;
				if (b) {
					int totalInputInfoSize = sizeof(INPUT_INFO) + inputInfo.format_size + inputInfo.audio_format_size;
					std::vector<BYTE> entireInputInfo(totalInputInfoSize);
					errno_t e = ::memcpy_s(entireInputInfo.data(), totalInputInfoSize, &inputInfo, sizeof(INPUT_INFO));
					e = ::memcpy_s(entireInputInfo.data() + sizeof(INPUT_INFO),
						inputInfo.format_size,
						inputInfo.format, inputInfo.format_size);
					e = ::memcpy_s(entireInputInfo.data() + sizeof(INPUT_INFO) + inputInfo.format_size,
						inputInfo.audio_format_size,
						inputInfo.audio_format, inputInfo.audio_format_size);

					auto fromData = GenerateFromInputData(CallFunc::kInfoGet, b, entireInputInfo.data(), totalInputInfoSize);
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
				if (static_cast<int>(g_readVideoBuffer.size()) < spp->perBufferSize) {
					g_readVideoBuffer.resize(spp->perBufferSize);
				}
				INPUT_INFO inputInfo = {};
				const int frame = spp->param1;
				int readBytes = g_winputPluginTable->func_read_video(spp->ih, spp->param1, g_readVideoBuffer.data());
				if (readBytes == 0) {
					// 画像の取得に失敗したので、前のフレームを取得して目的のフレームの生成を促す
					int prevFrame = frame - 1;
					if (prevFrame < 0) {
						prevFrame = frame + 1;
					}
					int prevReadBytes = g_winputPluginTable->func_read_video(spp->ih, prevFrame, g_readVideoBuffer.data());
					if (prevReadBytes == 0) {
						assert(false);
						ERROR_LOG << L"prevReadBytes == 0";
					}
					readBytes = g_winputPluginTable->func_read_video(spp->ih, frame, g_readVideoBuffer.data());
					if (readBytes == 0) {
						assert(false);
						ERROR_LOG << L"readBytes == 0 : retry func_read_video failed";
					}
				}
				//INFO_LOG << L"kReadVideo: " << spp->ih;

				namedPipe.Write((const BYTE*)&toData->header.callFunc, sizeof(toData->header.callFunc));
				std::int32_t totalSize = sizeof(int) + readBytes;
				namedPipe.Write((const BYTE*)&totalSize, sizeof(totalSize));
				namedPipe.Write((const BYTE*)&readBytes, sizeof(readBytes));
				namedPipe.Write((const BYTE*)g_readVideoBuffer.data(), readBytes);
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
				if (static_cast<int>(g_readAudioBuffer.size()) < requestReadBytes) {
					g_readAudioBuffer.resize(requestReadBytes);
				}
				int readSample = g_winputPluginTable->func_read_audio(spp->ih, spp->param1, spp->param2, g_readAudioBuffer.data());
				assert(readSample > 0);
				//INFO_LOG << L"kReadAudio: " << spp->ih << L" readSample: " << readSample;
				const int readBufferSize = PerAudioSampleBufferSize * readSample;

				namedPipe.Write((const BYTE*)& toData->header.callFunc, sizeof(toData->header.callFunc));
				std::int32_t totalSize = sizeof(int) + readBufferSize;
				namedPipe.Write((const BYTE*)&totalSize, sizeof(totalSize));
				namedPipe.Write((const BYTE*)&readSample, sizeof(readSample));
				namedPipe.Write((const BYTE*)g_readAudioBuffer.data(), readBufferSize);
				//auto fromData = GenerateFromInputData(toData->header.callFunc, readSample, g_readAudioBuffer.data(), readBufferSize);
				//namedPipe.Write((const BYTE*)fromData.get(), FromWinputDataTotalSize(*fromData));
				//INFO_LOG << L"Write: " << FromWinputDataTotalSize(*fromData) << L" bytes";
			}
			break;

			case CallFunc::kIsKeyframe:
			{
				INFO_LOG << L"kIsKeyframe";

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

