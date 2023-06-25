/**
*	@brief MFVideoDecoderをAviUtlのファイルリーダープラグイン渡す関数として包み込む
*/

#pragma once

#include "..\InputPipePlugin\input.h"

#if defined(INPUT_PIPE_MAIN) && defined(_WIN64)
#define	INPUT_PIPE_MAIN64
#endif


BOOL Plugin_func_init();
BOOL Plugin_func_exit();
#ifdef INPUT_PIPE_MAIN64
INPUT_HANDLE32 Plugin_func_open(LPSTR file);
BOOL Plugin_func_close(INPUT_HANDLE32 ih);
BOOL Plugin_func_info_get(INPUT_HANDLE32 ih, INPUT_INFO* iip);
int Plugin_func_read_video(INPUT_HANDLE32 ih, int frame, void* buf);
int Plugin_func_read_audio(INPUT_HANDLE32 ih, int start, int length, void* buf);
#else
INPUT_HANDLE Plugin_func_open(LPSTR file);
BOOL Plugin_func_close(INPUT_HANDLE ih);
BOOL Plugin_func_info_get(INPUT_HANDLE ih, INPUT_INFO* iip);
int Plugin_func_read_video(INPUT_HANDLE ih, int frame, void* buf);
int Plugin_func_read_audio(INPUT_HANDLE ih, int start, int length, void* buf);
#endif


