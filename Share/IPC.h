#pragma once

#include <vector>
#include <string>
#include <assert.h>
#include <Windows.h>

class BindProcess
{
public:

	bool	StartProcess(const std::wstring& exePath, const std::wstring& commandLine);

	void	StopProcess();

private:
	HANDLE m_hJob;
	HANDLE m_hEventKillSwitch;
};


class NamedPipe
{
	enum { kBuffSize = 512, kMaxInstance = 1 };

public:
	~NamedPipe();

	bool	CreateNamedPipe(const std::wstring& pipeName);
	bool	OpenNamedPipe(const std::wstring& pipeName);

	/// クライアントが接続してくるまで待機する
	bool	ConnectNamedPipe();
	void	Disconnect();

	std::vector<BYTE> Read(const int length);
	int		Read(BYTE* data, const int length);
	void	Write(const BYTE* data, int length);


private:
	HANDLE m_hPipe;
};



/////////////////////////////////////////////
// SharedMemory

class SharedMemory
{
public:

	SharedMemory() : m_hMap(NULL), m_pView(nullptr)
	{
	}

	~SharedMemory()
	{
		CloseHandle();
	}

	HANDLE	Handle() const { return m_hMap; }
	void* GetPointer() const { return m_pView; }

	void	CloseHandle()
	{
		if (m_hMap) {
			if (m_pView) {
				::UnmapViewOfFile(m_pView);
				m_pView = nullptr;
			}
			::CloseHandle(m_hMap);
			m_hMap = NULL;
		}
	}

	void* CreateSharedMemory(LPCTSTR sharedMemName, DWORD size)
	{
		assert(m_hMap == NULL && m_pView == nullptr && sharedMemName);
		assert(size >= 0);
		m_hMap = ::CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, sharedMemName);
		DWORD error = ::GetLastError();
		assert(m_hMap);
		m_pView = ::MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);;
		assert(m_pView);
		return m_pView;
	}

	void* OpenSharedMemory(LPCTSTR sharedMemName, bool bReadOnly)
	{
		assert(m_hMap == NULL && m_pView == nullptr && sharedMemName);
		DWORD dwDesiredAccess = bReadOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;
		m_hMap = ::OpenFileMapping(dwDesiredAccess, FALSE, sharedMemName);
		if (m_hMap == NULL)
			return nullptr;
		assert(m_hMap);
		m_pView = ::MapViewOfFile(m_hMap, dwDesiredAccess, 0, 0, 0);;
		assert(m_pView);
		return m_pView;
	}

private:

	HANDLE	m_hMap;
	void* m_pView;
};



