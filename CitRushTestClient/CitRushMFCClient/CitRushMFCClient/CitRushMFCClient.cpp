#include "pch.h"
#include "CitRushMFCClient.h"
#include "CTestClientDlg.h"

BEGIN_MESSAGE_MAP(CCitRushMFCClientApp, CWinApp)
END_MESSAGE_MAP()

CCitRushMFCClientApp theApp;

CCitRushMFCClientApp::CCitRushMFCClientApp() {}

BOOL CCitRushMFCClientApp::InitInstance() {
    CWinApp::InitInstance();
    AfxEnableControlContainer();

    CTestClientDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();
    return FALSE;
}
