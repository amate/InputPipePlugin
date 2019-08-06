#pragma once

#include <vector>
#include <string>
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