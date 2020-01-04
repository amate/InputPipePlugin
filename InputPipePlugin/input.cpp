//----------------------------------------------------------------------------------
//		サンプルAVI(vfw経由)入力プラグイン  for AviUtl ver0.98以降
//----------------------------------------------------------------------------------
#include "pch.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <random>
#include <mutex>
#include <chrono>
#include <windows.h>
//#include	<vfw.h>
//#pragma comment(lib, "Vfw32.lib")

#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")

#include	"input.h"

#include "..\Share\Logger.h"
#include "..\Share\IPC.h"
#include "..\Share\Common.h"
#include "..\Share\CodeConvert.h"

extern HMODULE g_hModule;
HMODULE	g_hWinputDll = NULL;
INPUT_PLUGIN_TABLE* g_winputPluginTable = nullptr;

BindProcess	g_bindProcess;
LPCWSTR		kPipeName = LR"(\\.\pipe\InputPipePlugin)";
NamedPipe	g_namedPipe;

//std::vector<BYTE> g_lastInfoGetData;

Config		m_config;

struct FrameAudioVideoBufferSize
{
	int OneFrameBufferSize;
	int PerAudioSampleBufferSize;
};

std::unordered_map<INPUT_HANDLE, FrameAudioVideoBufferSize> g_mapFrameBufferSize;

std::unordered_map<INPUT_HANDLE, std::unique_ptr<BYTE[]>> g_mapInputHandleInfoGetData;

using HandleCache = std::tuple<std::string, INPUT_HANDLE, int>;
std::vector<HandleCache>	g_vecHandleCache;

std::mutex	g_mtxIPC;

std::wstring	g_randamString;
SharedMemory	g_videoSharedMemory;
SharedMemory	g_audioSharedMemory;

// for Logger
std::string	LogFileName()
{
	return (GetExeDirectory() / L"InputPipePlugin.log").string();
}


//#define NO_REMOTE
//#define DEBUG_PROCESSINGTIME


//---------------------------------------------------------------------
//		入力プラグイン構造体定義
//---------------------------------------------------------------------
INPUT_PLUGIN_TABLE input_plugin_table = {
	INPUT_PLUGIN_FLAG_VIDEO|INPUT_PLUGIN_FLAG_AUDIO,	//	フラグ
														//	INPUT_PLUGIN_FLAG_VIDEO	: 画像をサポートする
														//	INPUT_PLUGIN_FLAG_AUDIO	: 音声をサポートする
	"InputPipePlugin",									//	プラグインの名前
	"any files (*.*)\0*.*\0",							//	入力ファイルフィルタ
	"Plugin to bypass lwinput.aui By amate version " PLUGIN_VERSION,		//	プラグインの情報
	func_init,											//	DLL開始時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_exit,											//	DLL終了時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_open,											//	入力ファイルをオープンする関数へのポインタ
	func_close,											//	入力ファイルをクローズする関数へのポインタ
	func_info_get,										//	入力ファイルの情報を取得する関数へのポインタ
	func_read_video,									//	画像データを読み込む関数へのポインタ
	func_read_audio,									//	音声データを読み込む関数へのポインタ
	func_is_keyframe,									//	キーフレームか調べる関数へのポインタ (NULLなら全てキーフレーム)
	func_config,										//	入力設定のダイアログを要求された時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
};


//---------------------------------------------------------------------
//		入力プラグイン構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C INPUT_PLUGIN_TABLE __declspec(dllexport) * __stdcall GetInputPluginTable( void )
{
	g_hWinputDll = ::LoadLibrary((GetExeDirectory() / L"lwinput.aui").c_str());
	assert(g_hWinputDll);
	if (g_hWinputDll == NULL) {
		WARN_LOG << L"lwinput.aui not found";
		MessageBox(NULL, L"同じフォルダ内に lwinput.aui が存在しません\nlwinput.aui が存在するフォルダに  InputPipePlugin.aui と InputPipeMain.exe をコピーしてください", L"InputPipePluginエラー", MB_ICONERROR);
		return &input_plugin_table;
	}

	using GetInputPluginTableFunc = INPUT_PLUGIN_TABLE * (__stdcall*)(void);
	GetInputPluginTableFunc funcGetTable = (GetInputPluginTableFunc)::GetProcAddress(g_hWinputDll, "GetInputPluginTable");
	g_winputPluginTable = funcGetTable();

	//::FreeLibrary(g_hWinputDll);
	//g_hWinputDll = NULL;

	return &input_plugin_table;
}


#if 0
//---------------------------------------------------------------------
//		ファイルハンドル構造体
//---------------------------------------------------------------------
typedef struct {
	int				flag;
	PAVIFILE		pfile;
	PAVISTREAM		pvideo,paudio;
	AVIFILEINFO		fileinfo;
	AVISTREAMINFO	videoinfo,audioinfo;
	void			*videoformat;
	LONG			videoformatsize;
	void			*audioformat;
	LONG			audioformatsize;
} FILE_HANDLE;
#define FILE_HANDLE_FLAG_VIDEO		1
#define FILE_HANDLE_FLAG_AUDIO		2
#endif

//---------------------------------------------------------------------
//		初期化
//---------------------------------------------------------------------
BOOL func_init( void )
{
	INFO_LOG << L"func_init";

	if (g_hWinputDll == NULL) {
		return FALSE;
	}

	m_config.LoadConfig();
	//m_config.bUseSharedMemory = true;

	if (m_config.bEnableIPC) {
		INFO_LOG << "EnableIPC";

		enum { kRandamStrLength = 32 };
		std::random_device  randdev;
		WCHAR tempstr[] = L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
		std::uniform_int_distribution<int> dist(0, std::size(tempstr) - 2);
		std::wstring randamString = L"_";
		for (int i = 0; i < kRandamStrLength; ++i) {
			randamString += tempstr[dist(randdev)];
		}
		randamString += L"_";
		std::wstring pipeName = std::wstring(kPipeName) + randamString;
		bool ret = g_namedPipe.CreateNamedPipe(pipeName);
		// INFO_LOG << L"CreateNamedPipe: " << pipeName << L" ret: " << ret;

		auto InputPipeMainPath = GetExeDirectory() / L"InputPipeMain.exe";

		std::wstring commandLine = pipeName;
		if (m_config.bUseSharedMemory) {
			g_randamString = randamString;
			commandLine += L" -sharedMemory";
		}
		bool startRet = g_bindProcess.StartProcess(InputPipeMainPath.native(), commandLine);
		// INFO_LOG << L"StartProcess: " << L" ret: " << startRet;

		if (!g_namedPipe.ConnectNamedPipe()) {
			ERROR_LOG << L"ConnectNamedPipe failed";
			MessageBox(NULL, L"名前付きパイプへの接続が失敗しました", L"InputPipePluginエラー", MB_ICONERROR);
			return FALSE;
		}
	}
	//BOOL b = g_winputPluginTable->func_init();
	//return b;

	return TRUE;
}


//---------------------------------------------------------------------
//		終了
//---------------------------------------------------------------------
BOOL func_exit( void )
{
	INFO_LOG << L"func_exit";

	if (g_hWinputDll == NULL) {
		return FALSE;
	}

	if (m_config.bEnableIPC) {
		g_videoSharedMemory.CloseHandle();
		g_audioSharedMemory.CloseHandle();

		g_namedPipe.Disconnect();
		g_bindProcess.StopProcess();
	}
	//BOOL b = g_winputPluginTable->func_exit();
	//return b;

	return TRUE;
}


//---------------------------------------------------------------------
//		ファイルオープン
//---------------------------------------------------------------------
INPUT_HANDLE func_open( LPSTR file )
{
	INFO_LOG << L"func_open: " << CodeConvert::UTF16fromShiftJIS(file);

	if (g_hWinputDll == NULL) {
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(g_mtxIPC);

#ifndef NO_REMOTE
	if (m_config.bEnableHandleCache) {
		std::string fileName = file;
		auto itfound = std::find_if(g_vecHandleCache.begin(), g_vecHandleCache.end(),
			[fileName](const HandleCache& handleCache) {
				return std::get<std::string>(handleCache) == fileName;
			});
		if (itfound != g_vecHandleCache.end()) {
			int& refCount = std::get<int>(*itfound);
			INFO_LOG << L"Cache found : " << refCount << L" -> " << (refCount + 1);
			++refCount;
			return std::get<INPUT_HANDLE>(*itfound);
		}
	}
	if (m_config.bEnableIPC) {
		size_t paramSize = strnlen_s(file, MAX_PATH) + 1;
		std::shared_ptr<ToWinputData> toData((ToWinputData*)new BYTE[kToWindDataHeaderSize + paramSize],
			[](ToWinputData* p) {
				delete[](BYTE*)p;
			});
		toData->header.callFunc = CallFunc::kOpen;
		toData->header.paramSize = paramSize;
		memcpy_s(toData->paramData, paramSize, file, paramSize);
		g_namedPipe.Write((const BYTE*)toData.get(), ToWinputDataTotalSize(*toData));

		std::vector<BYTE> headerData = g_namedPipe.Read(kFromWinputDataHeaderSize);
		FromWinputData* fromData = (FromWinputData*)headerData.data();
		//INFO_LOG << L"Read: " << fromData->returnSize << L" bytes";
		assert(fromData->callFunc == CallFunc::kOpen);
		std::vector<BYTE> readBody = g_namedPipe.Read(fromData->returnSize);
		INPUT_HANDLE ih2 = *(INPUT_HANDLE*)readBody.data();
		INFO_LOG << ih2;

		if (m_config.bEnableHandleCache) {
			g_vecHandleCache.emplace_back(std::string(file), ih2, 1);
		}
		return ih2;
	} else {
		INPUT_HANDLE ih = g_winputPluginTable->func_open(file);
		if (m_config.bEnableHandleCache) {
			g_vecHandleCache.emplace_back(std::string(file), ih, 1);
		}
		return ih;
	}
#else
	INPUT_HANDLE ih = g_winputPluginTable->func_open(file);
	return ih;
#endif
}


//---------------------------------------------------------------------
//		ファイルクローズ
//---------------------------------------------------------------------
BOOL func_close( INPUT_HANDLE ih )
{
	INFO_LOG << L"func_close: " << ih;
	std::lock_guard<std::mutex> lock(g_mtxIPC);
#ifndef NO_REMOTE
	if (m_config.bEnableHandleCache) {
		auto itfound = std::find_if(g_vecHandleCache.begin(), g_vecHandleCache.end(),
			[ih](const HandleCache& handleCache) {
				return std::get<INPUT_HANDLE>(handleCache) == ih;
			});
		assert(itfound != g_vecHandleCache.end());
		if (itfound == g_vecHandleCache.end()) {
			ERROR_LOG << L"func_close g_vecFileInputHandle.erase failed: " << ih;
		} else {
			int& refCount = std::get<int>(*itfound);
			INFO_LOG << L"Cache refCount : " << refCount << L" -> " << (refCount - 1);
			--refCount;
			if (refCount > 0) {
				return TRUE;		// まだ消さない！

			} else {
				INFO_LOG << L"Cache delete: " << ih;
				g_vecHandleCache.erase(itfound);
			}
		}
	}

	auto itfound = g_mapInputHandleInfoGetData.find(ih);
	if (itfound != g_mapInputHandleInfoGetData.end()) {
		g_mapInputHandleInfoGetData.erase(itfound);
	}
	if (m_config.bEnableIPC) {
		StandardParamPack spp = { ih };
		auto toData = GenerateToInputData(CallFunc::kClose, spp);
		g_namedPipe.Write((const BYTE*)toData.get(), ToWinputDataTotalSize(*toData));

		std::vector<BYTE> headerData = g_namedPipe.Read(kFromWinputDataHeaderSize);
		FromWinputData* fromData = (FromWinputData*)headerData.data();
		//INFO_LOG << L"Read: " << fromData->returnSize << L" bytes";
		assert(fromData->callFunc == CallFunc::kClose);
		std::vector<BYTE> readBody = g_namedPipe.Read(fromData->returnSize);
		auto retData = ParseFromInputData<BOOL>(readBody);

		g_mapFrameBufferSize.erase(ih);
		return retData.first;
	} else {
		BOOL b = g_winputPluginTable->func_close(ih);
		return b;
	}
#else
	BOOL b = g_winputPluginTable->func_close(ih);
	return b;
#endif
}


//---------------------------------------------------------------------
//		ファイルの情報
//---------------------------------------------------------------------
BOOL func_info_get( INPUT_HANDLE ih,INPUT_INFO *iip )
{
	INFO_LOG << L"func_info_get";
	std::lock_guard<std::mutex> lock(g_mtxIPC);
#ifndef NO_REMOTE

	auto itfound = g_mapInputHandleInfoGetData.find(ih);
	if (itfound != g_mapInputHandleInfoGetData.end()) {
		INFO_LOG << L"InfoGetData cache found!";
		*iip = *reinterpret_cast<INPUT_INFO*>(itfound->second.get());
		return TRUE;
	}
	if (m_config.bEnableIPC) {
		StandardParamPack spp = { ih };
		auto toData = GenerateToInputData(CallFunc::kInfoGet, spp);
		g_namedPipe.Write((const BYTE*)toData.get(), ToWinputDataTotalSize(*toData));

		std::vector<BYTE> headerData = g_namedPipe.Read(kFromWinputDataHeaderSize);
		FromWinputData* fromData = (FromWinputData*)headerData.data();
		INFO_LOG << L"Read: " << fromData->returnSize << L" bytes";
		assert(fromData->callFunc == CallFunc::kInfoGet);
		std::vector<BYTE> readBody = g_namedPipe.Read(fromData->returnSize);
		auto retData = ParseFromInputData<BOOL>(readBody);
		if (retData.first) {
			auto tempInputInfo = (INPUT_INFO*)retData.second;
			const int infoGetDataSize = sizeof(INPUT_INFO) + tempInputInfo->format_size + tempInputInfo->audio_format_size;
			auto infoGetData = std::make_unique<BYTE[]>(infoGetDataSize);
			memcpy_s(infoGetData.get(), infoGetDataSize, tempInputInfo, infoGetDataSize);

			auto igData = reinterpret_cast<INPUT_INFO*>(infoGetData.get());
			igData->format = (BITMAPINFOHEADER*)(infoGetData.get() + sizeof(INPUT_INFO));
			igData->audio_format = (WAVEFORMATEX*)(infoGetData.get() + sizeof(INPUT_INFO) + igData->format_size);
			*iip = *igData;
			g_mapInputHandleInfoGetData.emplace(ih, std::move(infoGetData));

			const int OneFrameBufferSize = iip->format->biWidth * iip->format->biHeight * (iip->format->biBitCount / 8);
			const int PerAudioSampleBufferSize = iip->audio_format->nChannels * (iip->audio_format->wBitsPerSample / 8);
			g_mapFrameBufferSize.emplace(ih, FrameAudioVideoBufferSize{ OneFrameBufferSize , PerAudioSampleBufferSize });

			INFO_LOG << L"OneFrameBufferSize: " << OneFrameBufferSize << L" PerAudioSampleBufferSize: " << PerAudioSampleBufferSize;
		} else {
			ERROR_LOG << L"func_info_get failed, ih: " << ih;
		}
		return retData.first;

	} else {
		BOOL b = g_winputPluginTable->func_info_get(ih, iip);
		if (b) {
			const int infoGetDataSize = sizeof(INPUT_INFO) + iip->format_size + iip->audio_format_size;
			auto infoGetData = std::make_unique<BYTE[]>(infoGetDataSize);
			memcpy_s(infoGetData.get(), infoGetDataSize, iip, sizeof(INPUT_INFO));

			auto igData = reinterpret_cast<INPUT_INFO*>(infoGetData.get());			
			::memcpy_s(infoGetData.get() + sizeof(INPUT_INFO),
				igData->format_size,
				igData->format, igData->format_size);
			::memcpy_s(infoGetData.get() + sizeof(INPUT_INFO) + igData->format_size,
				igData->audio_format_size,
				igData->audio_format, igData->audio_format_size);
			igData->format = (BITMAPINFOHEADER*)(infoGetData.get() + sizeof(INPUT_INFO));
			igData->audio_format = (WAVEFORMATEX*)(infoGetData.get() + sizeof(INPUT_INFO) + igData->format_size);
			g_mapInputHandleInfoGetData.emplace(ih, std::move(infoGetData));
		}

		return b;
	}
#else
	BOOL b = g_winputPluginTable->func_info_get(ih, iip);
	return b;
#endif
}

//---------------------------------------------------------------------
//		画像読み込み
//---------------------------------------------------------------------
int func_read_video( INPUT_HANDLE ih,int frame,void *buf )
{
	//INFO_LOG << L"func_read_video" << L" frame: " << frame;
	std::lock_guard<std::mutex> lock(g_mtxIPC);

#ifdef DEBUG_PROCESSINGTIME
	auto startTime = std::chrono::high_resolution_clock::now();
#endif

#ifndef NO_REMOTE
	if (m_config.bEnableIPC) {
		const int OneFrameBufferSize = g_mapFrameBufferSize[ih].OneFrameBufferSize;

		StandardParamPack spp = { ih, frame };
		spp.perBufferSize = OneFrameBufferSize;
		auto toData = GenerateToInputData(CallFunc::kReadVideo, spp);
		g_namedPipe.Write((const BYTE*)toData.get(), ToWinputDataTotalSize(*toData));

		std::vector<BYTE> headerData = g_namedPipe.Read(kFromWinputDataHeaderSize);
		FromWinputData* fromData = (FromWinputData*)headerData.data();
		//INFO_LOG << L"Read: " << fromData->returnSize << L" bytes";
		assert(fromData->callFunc == CallFunc::kReadVideo);
		int readBytes = 0;
		int nRet = g_namedPipe.Read((BYTE*)& readBytes, sizeof(readBytes));
		assert(nRet == sizeof(readBytes));
		assert((readBytes + sizeof(int)) == fromData->returnSize);
		if (m_config.bUseSharedMemory) {
			void* sharedBufferView;
			bool sharedMemoryReopen = false;
			g_namedPipe.Read((BYTE*)&sharedMemoryReopen, sizeof(sharedMemoryReopen));
			if (sharedMemoryReopen) {
				g_videoSharedMemory.CloseHandle();
				std::wstring sharedMemoryName = kVideoSharedMemoryPrefix + g_randamString + std::to_wstring(spp.perBufferSize);
				sharedBufferView = g_videoSharedMemory.OpenSharedMemory(sharedMemoryName.c_str(), true);
				assert(sharedBufferView);
				if (!sharedBufferView) {
					DWORD error = ::GetLastError();
					std::wstring errorMsg = L"g_videoSharedMemory.OpenSharedMemoryに失敗\nsharedMemoryName: " + sharedMemoryName + L"\nGetLastError: " + std::to_wstring(error);
					MessageBox(NULL, errorMsg.c_str(), L"InputPipePluginエラー", MB_ICONERROR);
					return 0;
				}
			} else {
				sharedBufferView = g_videoSharedMemory.GetPointer();
			}
			memcpy_s(buf, readBytes, sharedBufferView, readBytes);
		} else {
			nRet = g_namedPipe.Read((BYTE*)buf, readBytes);
			assert(nRet == readBytes);
		}

#ifdef DEBUG_PROCESSINGTIME	// 処理時間計測
		auto processingTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
		static bool firstSkip = true;

		static std::vector<long long> processingTimes;
		enum { kMaxTime = 100 };
		if (!firstSkip) {
			processingTimes.emplace_back(processingTime);
			if (processingTimes.size() == kMaxTime) {
				long long avgTime = 0;
				for (auto time : processingTimes) {
					avgTime += time;
				}
				avgTime /= 100;
				WARN_LOG << L"processingTime : " << processingTime << L"µs avg: " << avgTime << L"µs";
				processingTimes.clear();
			}
		} else {
			firstSkip = false;
			processingTimes.reserve(kMaxTime);
		}
#endif
		return readBytes;

	} else {
		int readBytes = g_winputPluginTable->func_read_video(ih, frame, buf);
		if (readBytes == 0) {
			// 画像の取得に失敗したので、前のフレームを取得して目的のフレームの生成を促す
			int prevFrame = frame - 1;
			if (prevFrame < 0) {
				prevFrame = frame + 1;
			}
			int prevReadBytes = g_winputPluginTable->func_read_video(ih, prevFrame, buf);
			if (prevReadBytes == 0) {
				assert(false);
				//ERROR_LOG << L"prevReadBytes == 0";
			}
			readBytes = g_winputPluginTable->func_read_video(ih, frame, buf);
			if (readBytes == 0) {
				assert(false);
				ERROR_LOG << L"readBytes == 0 : retry func_read_video failed";
			}
		}

#ifdef DEBUG_PROCESSINGTIME 	// 処理時間計測
		auto processingTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
		static bool firstSkip = true;

		static std::vector<long long> processingTimes;
		enum { kMaxTime = 100 };
		if (!firstSkip) {
			processingTimes.emplace_back(processingTime);
			if (processingTimes.size() == kMaxTime) {
				long long avgTime = 0;
				for (auto time : processingTimes) {
					avgTime += time;
				}
				avgTime /= 100;
				WARN_LOG << L"processingTime : " << processingTime << L"µs avg: " << avgTime << L"µs";
				processingTimes.clear();
			}
		} else {
			firstSkip = false;
			processingTimes.reserve(kMaxTime);
		}
#endif
		return readBytes;
	}
#else
	int n = g_winputPluginTable->func_read_video(ih, frame, buf);
	return n;
#endif
}

#undef DEBUG_PROCESSINGTIME

//---------------------------------------------------------------------
//		音声読み込み
//---------------------------------------------------------------------
int func_read_audio(INPUT_HANDLE ih, int start, int length, void* buf)
{
	//INFO_LOG << L"func_read_audio: " << ih << L" start: " << start << L" length: " << length;
	std::lock_guard<std::mutex> lock(g_mtxIPC);

#ifdef DEBUG_PROCESSINGTIME
	auto startTime = std::chrono::high_resolution_clock::now();
#endif

#ifndef NO_REMOTE
	if (m_config.bEnableIPC) {
		const int PerAudioSampleBufferSize = g_mapFrameBufferSize[ih].PerAudioSampleBufferSize;

		StandardParamPack spp = { ih, start, length, PerAudioSampleBufferSize };
		auto toData = GenerateToInputData(CallFunc::kReadAudio, spp);
		g_namedPipe.Write((const BYTE*)toData.get(), ToWinputDataTotalSize(*toData));

		std::vector<BYTE> headerData = g_namedPipe.Read(kFromWinputDataHeaderSize);
		FromWinputData* fromData = (FromWinputData*)headerData.data();
		//INFO_LOG << L"Read: " << fromData->returnSize << L" bytes";
		assert(fromData->callFunc == CallFunc::kReadAudio);
		int readSample = 0;
		int nRet = g_namedPipe.Read((BYTE*)& readSample, sizeof(readSample));
		assert(nRet == sizeof(readSample));
		const int audioBufferSize = fromData->returnSize - sizeof(int);
		assert(audioBufferSize >= 0);
		if (m_config.bUseSharedMemory) {
			void* sharedBufferView;
			bool sharedMemoryReopen = false;
			g_namedPipe.Read((BYTE*)&sharedMemoryReopen, sizeof(sharedMemoryReopen));
			if (sharedMemoryReopen) {
				g_audioSharedMemory.CloseHandle();
				std::wstring sharedMemoryName = kAudioSharedMemoryPrefix + g_randamString + std::to_wstring(audioBufferSize);
				sharedBufferView = g_audioSharedMemory.OpenSharedMemory(sharedMemoryName.c_str(), true);
				if (!sharedBufferView) {
					DWORD error = ::GetLastError();
					std::wstring errorMsg = L"g_audioSharedMemory.OpenSharedMemoryに失敗\nsharedMemoryName: " + sharedMemoryName + L"\nGetLastError: " + std::to_wstring(error);
					MessageBox(NULL, errorMsg.c_str(), L"InputPipePluginエラー", MB_ICONERROR);
					return 0;
				}
				assert(sharedBufferView);
			} else {
				sharedBufferView = g_audioSharedMemory.GetPointer();
			}
			memcpy_s(buf, audioBufferSize, sharedBufferView, audioBufferSize);
		} else {
			nRet = g_namedPipe.Read((BYTE*)buf, audioBufferSize);
			assert(nRet == audioBufferSize);
		}

#ifdef DEBUG_PROCESSINGTIME	// 処理時間計測
		auto processingTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
		static bool firstSkip = true;

		static std::vector<long long> processingTimes;
		enum { kMaxTime = 100 };
		if (!firstSkip) {
			processingTimes.emplace_back(processingTime);
			if (processingTimes.size() == kMaxTime) {
				long long avgTime = 0;
				for (auto time : processingTimes) {
					avgTime += time;
				}
				avgTime /= 100;
				WARN_LOG << L"processingTime : " << processingTime << L"µs avg: " << avgTime << L"µs";
				processingTimes.clear();
			}
		} else {
			firstSkip = false;
			processingTimes.reserve(kMaxTime);
		}
#endif
		return readSample;
	} else {
		int n = g_winputPluginTable->func_read_audio(ih, start, length, buf);
#ifdef DEBUG_PROCESSINGTIME	// 処理時間計測
		auto processingTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
		static bool firstSkip = true;

		static std::vector<long long> processingTimes;
		enum { kMaxTime = 100 };
		if (!firstSkip) {
			processingTimes.emplace_back(processingTime);
			if (processingTimes.size() == kMaxTime) {
				long long avgTime = 0;
				for (auto time : processingTimes) {
					avgTime += time;
				}
				avgTime /= 100;
				WARN_LOG << L"processingTime : " << processingTime << L"µs avg: " << avgTime << L"µs";
				processingTimes.clear();
	}
} else {
			firstSkip = false;
			processingTimes.reserve(kMaxTime);
		}
#endif
		return n;
	}
#if 0
	std::vector<BYTE> readBody = g_namedPipe.Read(fromData->returnSize);
	auto retData = ParseFromInputData<int>(readBody);
	const int requestReadBytes = PerAudioSampleBufferSize * length;
	::memcpy_s(buf, requestReadBytes, retData.second, requestReadBytes);

	return retData.first;
#endif
#else

	int n = g_winputPluginTable->func_read_audio(ih, start, length, buf);
	return n;
#endif
#if 0
	FILE_HANDLE * fp = (FILE_HANDLE*)ih;
	LONG		size;
	int			samplesize;

	samplesize = ((WAVEFORMATEX*)fp->audioformat)->nBlockAlign;
	if (AVIStreamRead(fp->paudio, start, length, buf, samplesize * length, NULL, &size)) return 0;
	return size;
#endif
}


//---------------------------------------------------------------------
//		キーフレーム情報
//---------------------------------------------------------------------
BOOL func_is_keyframe(INPUT_HANDLE ih, int frame)
{
	// INFO_LOG << L"func_is_keyframe" << L" frame:" << frame;
	std::lock_guard<std::mutex> lock(g_mtxIPC);

#ifndef NO_REMOTE
	if (m_config.bEnableIPC) {
		StandardParamPack spp = { ih, frame };
		auto toData = GenerateToInputData(CallFunc::kIsKeyframe, spp);
		g_namedPipe.Write((const BYTE*)toData.get(), ToWinputDataTotalSize(*toData));

		std::vector<BYTE> headerData = g_namedPipe.Read(kFromWinputDataHeaderSize);
		FromWinputData* fromData = (FromWinputData*)headerData.data();
		assert(fromData->callFunc == CallFunc::kIsKeyframe);
		std::vector<BYTE> readBody = g_namedPipe.Read(fromData->returnSize);
		auto retData = ParseFromInputData<BOOL>(readBody);
		return retData.first;
	} else {
		BOOL b = g_winputPluginTable->func_is_keyframe(ih, frame);
		return b;
	}
#else
	BOOL b = g_winputPluginTable->func_is_keyframe(ih, frame);
	return b;
#endif
#if 0
	FILE_HANDLE	*fp = (FILE_HANDLE *)ih;

	return AVIStreamIsKeyFrame(fp->pvideo,frame);
#endif
}


//---------------------------------------------------------------------
//		設定ダイアログ
//---------------------------------------------------------------------
BOOL func_config( HWND hwnd, HINSTANCE dll_hinst )
{
	// INFO_LOG << L"func_config";
#ifndef NO_REMOTE
	auto InputPipeMainPath = GetExeDirectory() / L"InputPipeMain.exe";
	::ShellExecute(NULL, NULL, InputPipeMainPath.c_str(), L" -config", NULL, SW_SHOWNORMAL);
	return TRUE;

#else
	BOOL b = g_winputPluginTable->func_config(hwnd, dll_hinst);
	return b;
#endif
#if 0
	MessageBoxA(hwnd,"サンプルダイアログ","入力設定",MB_OK);

	//	DLLを開放されても設定が残るように保存しておいてください。

	return TRUE;
#endif
}


