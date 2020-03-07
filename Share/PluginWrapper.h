/**
*	@brief MFVideoDecoderをAviUtlのファイルリーダープラグイン渡す関数として包み込む
*/

#pragma once

#include "..\InputPipePlugin\input.h"

INPUT_HANDLE Plugin_func_open(LPSTR file);
BOOL Plugin_func_close(INPUT_HANDLE ih);
BOOL Plugin_func_info_get(INPUT_HANDLE ih, INPUT_INFO* iip);
int Plugin_func_read_video(INPUT_HANDLE ih, int frame, void* buf);
int Plugin_func_read_audio(INPUT_HANDLE ih, int start, int length, void* buf);

