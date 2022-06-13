
#include "PluginWrapper.h"

#include "CodeConvert.h"

#ifndef INPUT_PIPE_MAIN
#include "Logger.h"
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