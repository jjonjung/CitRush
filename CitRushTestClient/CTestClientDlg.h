#pragma once
#include "pch.h"
#include "resource.h"
#include "Network/CHttpClient.h"
#include "Schemas/EnemySchemas.h"
#include "Schemas/JsonHelper.h"

// ============================================================
// CTestClientDlg
// AI Enemy 서버 연동 테스트 메인 다이얼로그
// POST /api/v1/get_decision 로 Enemy 상태를 보내고
// directive_code(EAITactic 1~12) 응답을 즉시 확인
// ============================================================

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

    // ── 서버 연결 버튼 ──
    afx_msg void OnBtnHealth();
    afx_msg void OnBtnMatchStart();
    afx_msg void OnBtnMatchEnd();

    // ── 핵심 액션 ──
    afx_msg void OnBtnGetDecision();

    // ── 시나리오 프리셋 ──
    afx_msg void OnBtnScnRetreat();
    afx_msg void OnBtnScnIntercept();
    afx_msg void OnBtnScnPellet();
    afx_msg void OnBtnScnSplit();

    afx_msg void OnBtnClearLog();

    DECLARE_MESSAGE_MAP()

private:
    // ── 서버 설정 컨트롤 ──
    CEdit m_editServerUrl;
    CEdit m_editPort;
    CEdit m_editRoomId;

    // ── Pacman 컨트롤 ──
    CEdit    m_editPacX, m_editPacY, m_editPacZ;
    CEdit    m_editPacHP, m_editPacSpeed;
    CButton  m_chkInvuln;

    // ── Clone 1 컨트롤 ──
    CButton  m_chkC1Alive;
    CEdit    m_editC1X, m_editC1Y, m_editC1Z, m_editC1HP;

    // ── Clone 2 컨트롤 ──
    CButton  m_chkC2Alive;
    CEdit    m_editC2X, m_editC2Y, m_editC2Z, m_editC2HP;

    // ── Player 1 컨트롤 ──
    CEdit    m_editP1X, m_editP1Y, m_editP1Z, m_editP1HP, m_editP1Dist;

    // ── 출력 컨트롤 ──
    CEdit    m_editResponse;
    CListBox m_listLog;

    // ── 내부 상태 ──
    CHttpClient m_http;
    int         m_reqNum = 0;
    bool        m_matchStarted = false;
    HICON       m_hIcon;

    // ── 유틸리티 ──
    void        Log(const CString& msg);
    void        UpdateHttpClient();
    CString     GetRoomId();

    FGetDecisionRequest  BuildDecisionRequest();
    void                 DisplayDecisionResponse(const FGetDecisionResponse& resp);

    float   GetF(CEdit& e, float def = 0.f);
    void    SetF(CEdit& e, float v);
    CString GetS(CEdit& e);
};
