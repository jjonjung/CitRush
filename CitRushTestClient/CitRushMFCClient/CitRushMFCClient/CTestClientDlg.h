#pragma once
#include "pch.h"
#include "Resource.h"
#include "Network/CHttpClient.h"
#include "Schemas/EnemySchemas.h"
#include "Schemas/JsonHelper.h"

class CTestClientDlg : public CDialogEx {
public:
    explicit CTestClientDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_MAIN_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK()     { /* Enter 키 무시 */ }
    virtual void OnCancel() { DestroyWindow(); }
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();

    afx_msg void OnBtnHealth();
    afx_msg void OnBtnMatchStart();
    afx_msg void OnBtnMatchEnd();

    afx_msg void OnBtnGetDecision();

    afx_msg void OnBtnScnRetreat();
    afx_msg void OnBtnScnIntercept();
    afx_msg void OnBtnScnPellet();
    afx_msg void OnBtnScnSplit();

    afx_msg void OnBtnClearLog();

    DECLARE_MESSAGE_MAP()

private:
    CEdit m_editServerUrl;
    CEdit m_editPort;
    CEdit m_editRoomId;

    CEdit    m_editPacX, m_editPacY, m_editPacZ;
    CEdit    m_editPacHP, m_editPacSpeed;
    CButton  m_chkInvuln;

    CButton  m_chkC1Alive;
    CEdit    m_editC1X, m_editC1Y, m_editC1Z, m_editC1HP;

    CButton  m_chkC2Alive;
    CEdit    m_editC2X, m_editC2Y, m_editC2Z, m_editC2HP;

    CEdit    m_editP1X, m_editP1Y, m_editP1Z, m_editP1HP, m_editP1Dist;

    CEdit    m_editResponse;
    CListBox m_listLog;

    CHttpClient m_http;
    int         m_reqNum = 0;
    bool        m_matchStarted = false;
    HICON       m_hIcon;

    void        Log(const CString& msg);
    void        UpdateHttpClient();
    CString     GetRoomId();

    FGetDecisionRequest  BuildDecisionRequest();
    void                 DisplayDecisionResponse(const FGetDecisionResponse& resp);

    float   GetF(CEdit& e, float def = 0.f);
    void    SetF(CEdit& e, float v);
    CString GetS(CEdit& e);
};
