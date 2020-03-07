#include "MainDlg.h"


LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	//
	m_config.LoadConfig();
	m_radioNamedPipeOrSharedMemory = m_config.bUseSharedMemory ? 1 : 0;
	DoDataExchange(DDX_LOAD);

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	DoDataExchange(DDX_SAVE);

	if (m_config.bEnableHandleCache == false && m_config.bEnableIPC == false) {
		MessageBox(L"少なくとも、どちらか一方にチェックを入れてください。");
		return 0;
	}
	m_config.bUseSharedMemory = m_radioNamedPipeOrSharedMemory == 1 ? true : false;

	if (!m_config.SaveConfig()) {
		CString errorMsg = L"設定の保存に失敗しました。";
		CString exeFolderPath = GetExeDirectory().wstring().c_str();
		exeFolderPath.MakeLower();
		if (exeFolderPath.Find(LR"(\program files (x86)\)") != -1 || exeFolderPath.Find(LR"(\program files\)") != -1) {
			errorMsg += L"\nAviUtlフォルダを [Program Files]や[Program Files (x86)]フォルダ内に置かないでください";
		}
		MessageBox(errorMsg, L"InputPipeMain - エラー", MB_ICONERROR);
	}

	CloseDialog(0);
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(0);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}