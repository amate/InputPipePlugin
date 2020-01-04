
#include "IPC.h"
#include <memory>
#include <atldef.h>
#include <assert.h>
//#include "Logger.h"


std::wstring GetLastErrorMessage(DWORD error)
{
	LPVOID pMsg = nullptr;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER  //      テキストのメモリ割り当てを要求する
		| FORMAT_MESSAGE_FROM_SYSTEM    //      エラーメッセージはWindowsが用意しているものを使用
		| FORMAT_MESSAGE_IGNORE_INSERTS,//      次の引数を無視してエラーコードに対するエラーメッセージを作成する
		NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),//   言語を指定
		(LPTSTR)&pMsg,                          //      メッセージテキストが保存されるバッファへのポインタ
		0,
		NULL);
	std::wstring errorMsg = (LPCWSTR)pMsg;
	LocalFree(pMsg);
	return errorMsg;
}

////////////////////////////////////////////////////////////////////////////////////
// BindProcess


bool BindProcess::StartProcess(const std::wstring& exePath, const std::wstring& commandLine)
{
	SECURITY_ATTRIBUTES securityAttributes = { sizeof(SECURITY_ATTRIBUTES) };
	securityAttributes.bInheritHandle = TRUE;
	m_hEventKillSwitch = ::CreateEvent(&securityAttributes, TRUE, FALSE, NULL);
	ATLASSERT(m_hEventKillSwitch);

	std::wstring kJobName = L"InputPipePlugin_Job" + std::to_wstring((uint64_t)m_hEventKillSwitch);
	m_hJob = ::CreateJobObject(nullptr, kJobName.c_str());
	ATLASSERT(m_hJob);
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION extendedLimit = {};
	extendedLimit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	SetInformationJobObject(m_hJob, JobObjectExtendedLimitInformation, &extendedLimit, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));

	{
		STARTUPINFO startUpInfo = { sizeof(STARTUPINFO) };
		PROCESS_INFORMATION processInfo = {};
		SECURITY_ATTRIBUTES securityAttributes = { sizeof(SECURITY_ATTRIBUTES) };
		securityAttributes.bInheritHandle = TRUE;
		std::wstring cmdLine = L" " + std::to_wstring((uint64_t)m_hEventKillSwitch) + L" " + commandLine;
		BOOL bRet = ::CreateProcess(exePath.c_str(), (LPWSTR)cmdLine.data(),
			nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startUpInfo, &processInfo);
		ATLASSERT(bRet);
		::CloseHandle(processInfo.hThread);
		::CloseHandle(processInfo.hProcess);
	}

	return true;
}

void BindProcess::StopProcess()
{
#if 1
	::SetEvent(m_hEventKillSwitch);
	::CloseHandle(m_hEventKillSwitch);

#ifdef _WIN64
	::SetEvent(m_hEventAPIHookTrapper64);
	::CloseHandle(m_hEventAPIHookTrapper64);
#endif

	::CloseHandle(m_hJob);
#endif
}


/////////////////////////////////////////////////////////////////////////////////////
// NamedPipe

NamedPipe::~NamedPipe()
{
	Disconnect();
}

bool NamedPipe::CreateNamedPipe(const std::wstring& pipeName)
{
	m_hPipe = ::CreateNamedPipe(pipeName.c_str(), 
		PIPE_ACCESS_DUPLEX,       // read/write access 
		PIPE_TYPE_BYTE |       // byte type pipe 
		PIPE_READMODE_BYTE |   // byte-read mode 
		PIPE_WAIT,                // blocking mode
		kMaxInstance, kBuffSize, kBuffSize, 0, nullptr);
	if (m_hPipe == INVALID_HANDLE_VALUE) {
		m_hPipe = NULL;
		return false;
	}
	
	return true;
}

bool NamedPipe::OpenNamedPipe(const std::wstring& pipeName)
{
	m_hPipe = ::CreateFile(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, NULL);
	if (m_hPipe == INVALID_HANDLE_VALUE) {
		m_hPipe = NULL;
		return false;

	}
	// The pipe connected; change to message-read mode. 
	//DWORD dwMode = PIPE_READMODE_MESSAGE;
	//BOOL fSuccess = ::SetNamedPipeHandleState(
	//	m_hPipe,    // pipe handle 
	//	&dwMode,  // new pipe mode 
	//	NULL,     // don't set maximum bytes 
	//	NULL);    // don't set maximum time 
	//if (!fSuccess) {
	//	assert(false);
	//	//_tprintf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
	//	return false;
	//}
	return true;
}

bool	NamedPipe::ConnectNamedPipe()
{
	BOOL b = FALSE;
	for (int i = 0; i < 10; ++i) {
		b = ::ConnectNamedPipe(m_hPipe, nullptr);
		if (b != 0) {
			break;
		}
		::Sleep(100);
	}
	return b != 0;
}

void NamedPipe::Disconnect()
{
	if (m_hPipe) {
		::FlushFileBuffers(m_hPipe);
		::DisconnectNamedPipe(m_hPipe);
		::CloseHandle(m_hPipe);
		m_hPipe = NULL;
	}
}

std::vector<BYTE> NamedPipe::Read(const int length)
{
	if (m_hPipe == NULL) {
		return std::vector<BYTE>();
	}
	std::vector<BYTE> readData(length);
	int readBytes = Read(readData.data(), length);
	readData.resize(readBytes);
	return readData;
}

int NamedPipe::Read(BYTE* data, const int length)
{
	if (m_hPipe == NULL) {
		return 0;
	}
	BYTE* writeDataPos = data;
	DWORD restReadBytes = length;
	int totalReadBytes = 0;
	while (restReadBytes > 0) {
#if 0
		DWORD BytesRead = 0;
		DWORD TotalBytesAvail = 0;
		DWORD BytesLeftThisMessage = 0;
		BOOL bRet = ::PeekNamedPipe(m_hPipe, nullptr, 0, &BytesRead, &TotalBytesAvail, &BytesLeftThisMessage);
#endif
		DWORD readyBytes = 0;
		BOOL bSuccess = ::ReadFile(m_hPipe, writeDataPos, restReadBytes, &readyBytes, nullptr);
		DWORD error = GetLastError();
		if ((!bSuccess || readyBytes == 0)) {
			if (error == 0) {	// WriteFile で送信されるバイト数が 0 の場合
				::Sleep(10);
				continue;
			}

			if (error == ERROR_BROKEN_PIPE) {
				//ERROR_LOG << L"NamedPipe::Read failed: client disconnected.";
			} else {
				//ERROR_LOG << L"NamedPipe::Read failed: GetLastError: " << GetLastErrorMessage(error) << L" [" << error << L"]";
			}
			assert(false);
			return totalReadBytes;

		} else {
			restReadBytes -= readyBytes;
			totalReadBytes = length - restReadBytes;
			writeDataPos += readyBytes;	// 書き込み位置更新
		}

	}
	return totalReadBytes;
}

void NamedPipe::Write(const BYTE* data, int length)
{
	if (m_hPipe == NULL) {
		assert(false);
		return;
	}
	assert(length > 0);
	DWORD writtenSize = 0;
	BOOL bSuccess = ::WriteFile(m_hPipe, data, length, &writtenSize, nullptr);
	if (!bSuccess || length != writtenSize) {
		DWORD error = GetLastError();
		assert(false);
		//ERROR_LOG << L"NamedPipe::Write failed: GetLastError: " << GetLastErrorMessage(error)<< L" [" << error << L"]";
	}
	//BOOL bFlushSuccess = ::FlushFileBuffers(m_hPipe);
}

