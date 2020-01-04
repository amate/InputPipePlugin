#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <utility>
#include <boost\filesystem.hpp>

#include "..\InputPipePlugin\input.h"

#if 0

BOOL func_init(void);
BOOL func_exit(void);
INPUT_HANDLE func_open(LPSTR file);
BOOL func_close(INPUT_HANDLE ih);
BOOL func_info_get(INPUT_HANDLE ih, INPUT_INFO* iip);
int func_read_video(INPUT_HANDLE ih, int frame, void* buf);
int func_read_audio(INPUT_HANDLE ih, int start, int length, void* buf);
BOOL func_is_keyframe(INPUT_HANDLE ih, int frame);
BOOL func_config(HWND hwnd, HINSTANCE dll_hinst);

#endif

////////////////////////////////////////////////////////////////

#define		PLUGIN_VERSION	"1.6"


constexpr	int kVideoBufferSurplusBytes = 0x3FF;

constexpr LPCWSTR kVideoSharedMemoryPrefix = L"InputPipePluginVideo_";
constexpr LPCWSTR kAudioSharedMemoryPrefix = L"InputPipePluginAudio_";

////////////////////////////////////////////////////////////////

namespace fs = boost::filesystem;
extern HMODULE g_hModule;

/// 現在実行中の exeのあるフォルダのパスを返す
fs::path GetExeDirectory();

////////////////////////////////////////////////////////////////

constexpr LPCSTR	kConfigFileName = "InputPipePluginConfig.ini";

struct Config
{
	bool	bEnableHandleCache;
	bool	bEnableIPC;
	bool	bUseSharedMemory;

	bool	LoadConfig();
	bool	SaveConfig();
};

////////////////////////////////////////////////////////////////

enum class CallFunc : std::int32_t
{
	kOpen = 1,
	kClose,
	kInfoGet,
	kReadVideo,
	kReadAudio,
	kIsKeyframe,
	kConfig,

	kExit,
};

struct StandardParamPack
{
	INPUT_HANDLE ih;
	int param1;
	int param2;
	int perBufferSize;
};


struct ToWinputData
{
	struct {
		CallFunc callFunc;
		std::int32_t paramSize;
	} header;
	unsigned char paramData[1];
};

constexpr int kToWindDataHeaderSize = offsetof(ToWinputData, paramData);

inline int ToWinputDataTotalSize(const ToWinputData& data) { 
	return kToWindDataHeaderSize + data.header.paramSize;
}

template<class RetParam1T>
std::shared_ptr<ToWinputData> GenerateToInputData(CallFunc callFunc, RetParam1T retParam)
{
	size_t paramSize = sizeof(retParam);
	std::shared_ptr<ToWinputData> toData((ToWinputData*)new BYTE[kToWindDataHeaderSize + paramSize], 
		[](ToWinputData* p) {
			delete[] (BYTE*)p;
	});
	toData->header.callFunc = callFunc;
	toData->header.paramSize = paramSize;
	memcpy_s(toData->paramData, paramSize, &retParam, paramSize);
	return toData;
}

/////////////////////////////////////////////////////////////////////

struct FromWinputData
{
	CallFunc callFunc;
	std::int32_t returnSize;
	unsigned char returnData[1];
};

constexpr int kFromWinputDataHeaderSize = offsetof(FromWinputData, returnData);

inline int FromWinputDataTotalSize(const FromWinputData& data) {
	return kFromWinputDataHeaderSize + data.returnSize;
}

template<class RetT, class RetParamT>
std::shared_ptr<FromWinputData> GenerateFromInputData(CallFunc callFunc, RetT ret, RetParamT retParam)
{
	size_t returnSize = sizeof(ret) + sizeof(retParam);
	std::shared_ptr<FromWinputData> fromData((FromWinputData*)new BYTE[kToWindDataHeaderSize + returnSize], 
		[](FromWinputData* p) {
		delete[] (BYTE*)p;  
	});
	fromData->callFunc = callFunc;
	fromData->returnSize = returnSize;
	memcpy_s(fromData->returnData, sizeof(ret), &ret, sizeof(ret));
	memcpy_s(fromData->returnData + sizeof(ret), sizeof(retParam), &retParam, sizeof(retParam));
	return fromData;
}

template<class RetT>
std::shared_ptr<FromWinputData> GenerateFromInputData(CallFunc callFunc, RetT ret, const BYTE* retParam, int retParamSize)
{
	size_t returnSize = sizeof(ret) + retParamSize;
	std::shared_ptr<FromWinputData> fromData((FromWinputData*)new BYTE[kToWindDataHeaderSize + returnSize],
		[](FromWinputData* p) {
			delete[](BYTE*)p;
		});
	fromData->callFunc = callFunc;
	fromData->returnSize = returnSize;
	memcpy_s(fromData->returnData, sizeof(ret), &ret, sizeof(ret));
	memcpy_s(fromData->returnData + sizeof(ret), retParamSize, retParam, retParamSize);
	return fromData;
}

template<class RetT>
std::pair<RetT, BYTE*> ParseFromInputData(std::vector<BYTE>& readBody)
{
	return { *(RetT*)readBody.data(), readBody.data() + sizeof(RetT) };
}
