
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
		FORMAT_MESSAGE_ALLOCATE_BUFFER  //      �e�L�X�g�̃��������蓖�Ă�v������
		| FORMAT_MESSAGE_FROM_SYSTEM    //      �G���[���b�Z�[�W��Windows���p�ӂ��Ă�����̂��g�p
		| FORMAT_MESSAGE_IGNORE_INSERTS,//      ���̈����𖳎����ăG���[�R�[�h�ɑ΂���G���[���b�Z�[�W���쐬����
		NULL, errorcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),//   ������w��
		(LPTSTR)&lpMsgBuf,                          //      ���b�Z�[�W�e�L�X�g���ۑ������o�b�t�@�ւ̃|�C���^
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
	// INPUT_INFO ���R�s�[
#ifndef _WIN64
	memcpy_s(infoGetData.get(), infoGetDataSize, iip, sizeof(INPUT_INFO));
#else
	INPUT_INFO32* pinfo = reinterpret_cast<INPUT_INFO32*>(infoGetData.get());
	pinfo->flag = iip->flag;
	pinfo->rate = iip->rate;
	pinfo->scale = iip->scale;
	pinfo->n = iip->n;
	pinfo->format_size = iip->format_size;
	pinfo->audio_n = iip->audio_n;
	pinfo->audio_format_size = iip->audio_format_size;
	pinfo->handler = iip->handler;
#endif
	auto igData = reinterpret_cast<INPUT_INFO32*>(infoGetData.get());

	// format��audio_format�����ꂼ�� �m�ۂ����������̌��ɃR�s�[
	::memcpy_s(infoGetData.get() + sizeof(INPUT_INFO32),
		iip->format_size,
		iip->format, iip->format_size);
	::memcpy_s(infoGetData.get() + sizeof(INPUT_INFO32) + iip->format_size,
		iip->audio_format_size,
		iip->audio_format, iip->audio_format_size);

	// ���ꂼ��̃t�H�[�}�b�g�̎w����������A�m�ۂ�������������ݒ�	(�Ӗ��Ȃ����� �k���|���Ɩ��ɂȂ邩������Ȃ�)
	if (igData->format_size > 0) {
		igData->format = reinterpret_cast<uint32_t>((BITMAPINFOHEADER*)(infoGetData.get() + sizeof(INPUT_INFO)));
	} else {
		igData->format = 0;	// nullptr;
	}
	if (igData->audio_format_size > 0) {
		igData->audio_format = reinterpret_cast<uint32_t>((WAVEFORMATEX*)(infoGetData.get() + sizeof(INPUT_INFO) + igData->format_size));
	} else {
		igData->audio_format = 0;	// nullptr;
	}
	return infoGetData;
}

std::unique_ptr<BYTE[]> DeserializeInputInfo(const BYTE* serializedData, INPUT_INFO* iip)
{
#ifdef _WIN64
	assert(FALSE);	// 64bit�łł̓f�V���A���C�Y�͌Ă΂Ȃ��͂�
#endif

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