
#include "PluginWrapper.h"
#include <cassert>
#include "CodeConvert.h"


#ifndef INPUT_PIPE_MAIN
#include "Logger.h"
#endif

#ifdef INPUT_PIPE_MAIN64
#include <unordered_map>
#include <mutex>

std::mutex		m_mtxInputHandle;
std::unordered_map<INPUT_HANDLE32, INPUT_HANDLE>	g_mapInputHandle;

INPUT_HANDLE32	GenerateInputHandle32()
{
	static INPUT_HANDLE32	src = 0;
	++src;
	return src;
}


#endif

extern INPUT_PLUGIN_TABLE* g_winputPluginTable;

BOOL Plugin_func_init()
{
	if (g_winputPluginTable->func_init) {
		BOOL b = g_winputPluginTable->func_init();
		return b;

	} else {
		return TRUE;
	}
}

BOOL Plugin_func_exit()
{
	if (g_winputPluginTable->func_exit) {
		BOOL b = g_winputPluginTable->func_exit();
		return b;

	} else {
		return TRUE;
	}
}

#ifdef INPUT_PIPE_MAIN64

INPUT_HANDLE32 Plugin_func_open(LPSTR file)
{
	try {
		INPUT_HANDLE ih = g_winputPluginTable->func_open(file);
		if (ih) {
			std::lock_guard<std::mutex>	lock(m_mtxInputHandle);
			INPUT_HANDLE32 ih32 = GenerateInputHandle32();
			g_mapInputHandle[ih32] = ih;
			return ih32;
		}
		return 0;
	}
	catch (...) {
#ifndef INPUT_PIPE_MAIN
		ERROR_LOG << L"Plugin_func_open failed\nfile: " << CodeConvert::UTF16fromShiftJIS(file);
#endif
	}
	return 0;
}

BOOL Plugin_func_close(INPUT_HANDLE32 ih32)
{
	std::lock_guard<std::mutex>	lock(m_mtxInputHandle);
	auto it = g_mapInputHandle.find(ih32);
	if (it == g_mapInputHandle.end()) {
		assert(FALSE);
		return FALSE;
	}

	try {
		BOOL b = g_winputPluginTable->func_close(it->second);
		g_mapInputHandle.erase(it);
		return b;
	}
	catch (...) {
#ifndef INPUT_PIPE_MAIN
		ERROR_LOG << L"Plugin_func_close failed - ih: " << ih32;
#endif
	}
	return FALSE;
}

BOOL Plugin_func_info_get(INPUT_HANDLE32 ih32, INPUT_INFO* iip)
{
	std::lock_guard<std::mutex>	lock(m_mtxInputHandle);
	auto it = g_mapInputHandle.find(ih32);
	if (it == g_mapInputHandle.end()) {
		assert(FALSE);
		return FALSE;
	}
	try {
		BOOL b = g_winputPluginTable->func_info_get(it->second, iip);
		return b;
	}
	catch (...) {
#ifndef INPUT_PIPE_MAIN
		ERROR_LOG << L"Plugin_func_info_get failed - ih: " << ih32;
#endif
	}
	return FALSE;
}

int Plugin_func_read_video(INPUT_HANDLE32 ih32, int frame, void* buf)
{
	std::lock_guard<std::mutex>	lock(m_mtxInputHandle);
	auto it = g_mapInputHandle.find(ih32);
	if (it == g_mapInputHandle.end()) {
		assert(FALSE);
		return 0;
	}
	try {
		int readBytes = g_winputPluginTable->func_read_video(it->second, frame, buf);
		if (readBytes == 0) {
			// 画像の取得に失敗したので、前のフレームを取得して目的のフレームの生成を促す
			int prevFrame = frame - 1;
			if (prevFrame < 0) {
				prevFrame = frame + 1;
			}
			int prevReadBytes = g_winputPluginTable->func_read_video(it->second, prevFrame, buf);
			if (prevReadBytes == 0) {
				//assert(false);
				//ERROR_LOG << L"prevReadBytes == 0";
			}
			readBytes = g_winputPluginTable->func_read_video(it->second, frame, buf);
			if (readBytes == 0) {
				//assert(false);
#ifndef INPUT_PIPE_MAIN
				ERROR_LOG << L"retry func_read_video failed : readBytes == 0 - frame: " << frame;
#endif
			}
		}
		return readBytes;
	}
	catch (...) {
#ifndef INPUT_PIPE_MAIN
		ERROR_LOG << L"Plugin_func_read_video failed - ih: " << ih32 << L" frame: " << frame;
#endif
	}
	return 0;
}

int Plugin_func_read_audio(INPUT_HANDLE32 ih32, int start, int length, void* buf)
{
	std::lock_guard<std::mutex>	lock(m_mtxInputHandle);
	auto it = g_mapInputHandle.find(ih32);
	if (it == g_mapInputHandle.end()) {
		assert(FALSE);
		return 0;
	}
	try {
		int n = g_winputPluginTable->func_read_audio(it->second, start, length, buf);
		return n;
	}
	catch (...) {
#ifndef INPUT_PIPE_MAIN
		ERROR_LOG << L"Plugin_func_read_audio failed - ih: " << ih32 << " start: " << start;
#endif
	}
	return 0;
}

#else

INPUT_HANDLE Plugin_func_open(LPSTR file)
{
	try {
		INPUT_HANDLE ih = g_winputPluginTable->func_open(file);
		return ih;
	}
	catch (...) {
#ifndef INPUT_PIPE_MAIN
		ERROR_LOG << L"Plugin_func_open failed\nfile: " << CodeConvert::UTF16fromShiftJIS(file);
#endif
	}
	return nullptr;
}

BOOL Plugin_func_close(INPUT_HANDLE ih)
{
	try {
		BOOL b = g_winputPluginTable->func_close(ih);
		return b;
	}
	catch (...) {
#ifndef INPUT_PIPE_MAIN
		ERROR_LOG << L"Plugin_func_close failed - ih: " << ih;
#endif
	}
	return FALSE;
}

BOOL Plugin_func_info_get(INPUT_HANDLE ih, INPUT_INFO* iip)
{
	try {
		BOOL b = g_winputPluginTable->func_info_get(ih, iip);
		return b;
	} 
	catch (...) {
#ifndef INPUT_PIPE_MAIN
		ERROR_LOG << L"Plugin_func_info_get failed - ih: " << ih;
#endif
	}
	return FALSE;
}

int Plugin_func_read_video(INPUT_HANDLE ih, int frame, void* buf)
{
	try {
		int readBytes = g_winputPluginTable->func_read_video(ih, frame, buf);
		if (readBytes == 0) {
			// 画像の取得に失敗したので、前のフレームを取得して目的のフレームの生成を促す
			int prevFrame = frame - 1;
			if (prevFrame < 0) {
				prevFrame = frame + 1;
			}
			int prevReadBytes = g_winputPluginTable->func_read_video(ih, prevFrame, buf);
			if (prevReadBytes == 0) {
				//assert(false);
				//ERROR_LOG << L"prevReadBytes == 0";
			}
			readBytes = g_winputPluginTable->func_read_video(ih, frame, buf);
			if (readBytes == 0) {
				//assert(false);
#ifndef INPUT_PIPE_MAIN
				ERROR_LOG << L"retry func_read_video failed : readBytes == 0 - frame: " << frame;
#endif
			}
		}
		return readBytes;
	}
	catch (...) {
#ifndef INPUT_PIPE_MAIN
		ERROR_LOG << L"Plugin_func_read_video failed - ih: " << ih << L" frame: " << frame;
#endif
	}
	return 0;
}

int Plugin_func_read_audio(INPUT_HANDLE ih, int start, int length, void* buf)
{
	try {
		int n = g_winputPluginTable->func_read_audio(ih, start, length, buf);
		return n;
	}
	catch (...) {
#ifndef INPUT_PIPE_MAIN
		ERROR_LOG << L"Plugin_func_read_audio failed - ih: " << ih << " start: " << start;
#endif
	}
	return 0;
}

#endif
