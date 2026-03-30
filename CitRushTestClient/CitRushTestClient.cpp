#include "pch.h"
#include "CitRushTestClient.h"
#include "CTestClientDlg.h"

BEGIN_MESSAGE_MAP(CCitRushTestClientApp, CWinApp)
END_MESSAGE_MAP()

CCitRushTestClientApp theApp;

CCitRushTestClientApp::CCitRushTestClientApp() {}

BOOL CCitRushTestClientApp::InitInstance() {
    CWinApp::InitInstance();
    AfxEnableControlContainer();

    CTestClientDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();
    return FALSE;
}
