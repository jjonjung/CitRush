#include "pch.h"
#include "CTestClientDlg.h"

BEGIN_MESSAGE_MAP(CTestClientDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BTN_HEALTH,         &CTestClientDlg::OnBtnHealth)
    ON_BN_CLICKED(IDC_BTN_MATCH_START,    &CTestClientDlg::OnBtnMatchStart)
    ON_BN_CLICKED(IDC_BTN_MATCH_END,      &CTestClientDlg::OnBtnMatchEnd)
    ON_BN_CLICKED(IDC_BTN_GET_DECISION,   &CTestClientDlg::OnBtnGetDecision)
    ON_BN_CLICKED(IDC_BTN_SCN_RETREAT,    &CTestClientDlg::OnBtnScnRetreat)
    ON_BN_CLICKED(IDC_BTN_SCN_INTERCEPT,  &CTestClientDlg::OnBtnScnIntercept)
    ON_BN_CLICKED(IDC_BTN_SCN_PELLET,     &CTestClientDlg::OnBtnScnPellet)
    ON_BN_CLICKED(IDC_BTN_SCN_SPLIT,      &CTestClientDlg::OnBtnScnSplit)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG,      &CTestClientDlg::OnBtnClearLog)
END_MESSAGE_MAP()

CTestClientDlg::CTestClientDlg(CWnd* pParent)
    : CDialogEx(IDD_MAIN_DIALOG, pParent), m_hIcon(nullptr) {}

void CTestClientDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_SERVER_URL,  m_editServerUrl);
    DDX_Control(pDX, IDC_EDIT_PORT,        m_editPort);
    DDX_Control(pDX, IDC_EDIT_ROOM_ID,     m_editRoomId);
    DDX_Control(pDX, IDC_EDIT_PAC_X,       m_editPacX);
    DDX_Control(pDX, IDC_EDIT_PAC_Y,       m_editPacY);
    DDX_Control(pDX, IDC_EDIT_PAC_Z,       m_editPacZ);
    DDX_Control(pDX, IDC_EDIT_PAC_HP,      m_editPacHP);
    DDX_Control(pDX, IDC_EDIT_PAC_SPEED,   m_editPacSpeed);
    DDX_Control(pDX, IDC_CHK_INVULN,       m_chkInvuln);
    DDX_Control(pDX, IDC_CHK_C1_ALIVE,     m_chkC1Alive);
    DDX_Control(pDX, IDC_EDIT_C1_X,        m_editC1X);
    DDX_Control(pDX, IDC_EDIT_C1_Y,        m_editC1Y);
    DDX_Control(pDX, IDC_EDIT_C1_Z,        m_editC1Z);
    DDX_Control(pDX, IDC_EDIT_C1_HP,       m_editC1HP);
    DDX_Control(pDX, IDC_CHK_C2_ALIVE,     m_chkC2Alive);
    DDX_Control(pDX, IDC_EDIT_C2_X,        m_editC2X);
    DDX_Control(pDX, IDC_EDIT_C2_Y,        m_editC2Y);
    DDX_Control(pDX, IDC_EDIT_C2_Z,        m_editC2Z);
    DDX_Control(pDX, IDC_EDIT_C2_HP,       m_editC2HP);
    DDX_Control(pDX, IDC_EDIT_P1_X,        m_editP1X);
    DDX_Control(pDX, IDC_EDIT_P1_Y,        m_editP1Y);
    DDX_Control(pDX, IDC_EDIT_P1_Z,        m_editP1Z);
    DDX_Control(pDX, IDC_EDIT_P1_HP,       m_editP1HP);
    DDX_Control(pDX, IDC_EDIT_P1_DIST,     m_editP1Dist);
    DDX_Control(pDX, IDC_EDIT_RESPONSE,    m_editResponse);
    DDX_Control(pDX, IDC_LIST_LOG,         m_listLog);
}

BOOL CTestClientDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    m_editServerUrl.SetWindowText(L"http://34.64.141.47");
    m_editPort.SetWindowText(L"8000");
    m_editRoomId.SetWindowText(L"TEST_ROOM_001");

    SetF(m_editPacX, 0.f);   SetF(m_editPacY, 0.f);   SetF(m_editPacZ, 0.f);
    SetF(m_editPacHP, 100.f); SetF(m_editPacSpeed, 5.f);
    m_chkInvuln.SetCheck(BST_UNCHECKED);

    m_chkC1Alive.SetCheck(BST_CHECKED);
    SetF(m_editC1X, 100.f);  SetF(m_editC1Y, 50.f);  SetF(m_editC1Z, 0.f);
    SetF(m_editC1HP, 1.f);

    m_chkC2Alive.SetCheck(BST_CHECKED);
    SetF(m_editC2X, -100.f); SetF(m_editC2Y, 50.f);  SetF(m_editC2Z, 0.f);
    SetF(m_editC2HP, 1.f);

    SetF(m_editP1X, 300.f);  SetF(m_editP1Y, 0.f);  SetF(m_editP1Z, 0.f);
    SetF(m_editP1HP, 100.f); SetF(m_editP1Dist, 300.f);

    Log(L"초기화 완료. 서버 URL과 RoomID를 설정 후 Match Start -> Get Decision 순서로 테스트하세요.");
    return TRUE;
}

void CTestClientDlg::OnBtnHealth() {
    UpdateHttpClient();
    std::string body; int status;
    bool ok = m_http.Get("/api/v1/health", body, status);
    CString msg;
    if (ok && status == 200) {
        std::string srv = JsonHelper::ExtractStr(body, "status");
        msg.Format(L"[Health] %d | status=%s", status, CA2W(srv.c_str()));
    } else {
        msg.Format(L"[Health] FAILED status=%d err=%s",
                   status, CA2W(m_http.GetLastError().c_str()));
    }
    Log(msg);
    m_editResponse.SetWindowText(CA2W(body.c_str()));
}

void CTestClientDlg::OnBtnMatchStart() {
    UpdateHttpClient();

    FMatchStartRequest req;
    req.room_id = CT2A(GetRoomId(), CP_UTF8);
    req.player_ids   = { "STEAM_TEST_CMD",  "STEAM_TEST_D1", "STEAM_TEST_D2" };
    req.player_names = { "TestCommander",   "TestDriver1",   "TestDriver2"   };
    req.player_types = { "COMMANDER",       "DRIVER",        "DRIVER"        };

    std::string body; int status;
    bool ok = m_http.Post("/api/v1/match/start",
                           JsonHelper::BuildMatchStartRequest(req), body, status);
    CString msg;
    msg.Format(L"[Match Start] status=%d ok=%s", status, ok ? L"YES" : L"NO");
    Log(msg);
    m_editResponse.SetWindowText(CA2W(body.c_str()));
    if (ok && status == 200) m_matchStarted = true;
}

void CTestClientDlg::OnBtnMatchEnd() {
    UpdateHttpClient();

    FMatchEndRequest req;
    req.room_id    = CT2A(GetRoomId(), CP_UTF8);
    req.result     = "ABORTED";
    req.end_reason = "SURRENDER";

    std::string body; int status;
    bool ok = m_http.Post("/api/v1/match/end",
                           JsonHelper::BuildMatchEndRequest(req), body, status);
    CString msg;
    msg.Format(L"[Match End] status=%d ok=%s", status, ok ? L"YES" : L"NO");
    Log(msg);
    m_editResponse.SetWindowText(CA2W(body.c_str()));
    if (ok) m_matchStarted = false;
}

void CTestClientDlg::OnBtnGetDecision() {
    UpdateHttpClient();

    FGetDecisionRequest req = BuildDecisionRequest();
    std::string jsonBody = JsonHelper::BuildDecisionRequest(req);

    Log(L"[GET DECISION] 요청 중...");

    std::string respBody; int status;
    bool ok = m_http.Post("/api/v1/get_decision", jsonBody, respBody, status);

    if (!ok || status == 0) {
        CString err;
        err.Format(L"[GET DECISION] 연결 실패. err=%s",
                   CA2W(m_http.GetLastError().c_str()));
        Log(err);
        m_editResponse.SetWindowText(err);
        return;
    }

    FGetDecisionResponse resp = JsonHelper::ParseDecisionResponse(respBody);
    DisplayDecisionResponse(resp);
}

void CTestClientDlg::OnBtnScnRetreat() {
    SetF(m_editPacHP, 12.f);
    SetF(m_editPacSpeed, 3.f);
    m_chkInvuln.SetCheck(BST_UNCHECKED);
    SetF(m_editP1X, 80.f); SetF(m_editP1Dist, 80.f);
    SetF(m_editC1X, 70.f); SetF(m_editC2X, -70.f);
    Log(L"[시나리오] 팩맨 HP=12, 플레이어 근접(80) -> RETREAT 예상");
}

void CTestClientDlg::OnBtnScnIntercept() {
    SetF(m_editPacHP, 80.f);
    SetF(m_editPacSpeed, 8.f);
    m_chkInvuln.SetCheck(BST_UNCHECKED);
    SetF(m_editP1X, 450.f); SetF(m_editP1Dist, 450.f);
    SetF(m_editC1X, 200.f); SetF(m_editC2X, -200.f);
    Log(L"[시나리오] 팩맨 HP=80, 플레이어 접근(450) -> INTERCEPT 예상");
}

void CTestClientDlg::OnBtnScnPellet() {
    SetF(m_editPacHP, 70.f);
    SetF(m_editPacX, 200.f); SetF(m_editPacY, 100.f);
    SetF(m_editPacSpeed, 5.f);
    SetF(m_editP1X, 900.f); SetF(m_editP1Dist, 900.f);
    Log(L"[시나리오] P-Pellet 쿨다운=0, 플레이어 원거리(900) -> CONSUME_PELLET 예상");
}

void CTestClientDlg::OnBtnScnSplit() {
    SetF(m_editPacHP, 90.f);
    SetF(m_editPacX, 0.f); SetF(m_editPacY, 0.f);
    SetF(m_editC1X, 400.f); SetF(m_editC1Y, 400.f);
    SetF(m_editC2X, -400.f); SetF(m_editC2Y, -400.f);
    SetF(m_editP1Dist, 1200.f);
    Log(L"[시나리오] 분산 포지션, 플레이어 원거리(1200) -> SPLIT_FORMATION 예상");
}

void CTestClientDlg::OnBtnClearLog() {
    m_listLog.ResetContent();
    m_editResponse.SetWindowText(L"");
}

void CTestClientDlg::Log(const CString& msg) {
    SYSTEMTIME st; GetLocalTime(&st);
    CString entry;
    entry.Format(L"[%02d:%02d:%02d] %s", st.wHour, st.wMinute, st.wSecond, (LPCWSTR)msg);
    m_listLog.InsertString(0, entry);
    while (m_listLog.GetCount() > 300) m_listLog.DeleteString(m_listLog.GetCount() - 1);
}

void CTestClientDlg::UpdateHttpClient() {
    CString sUrl, sPort;
    m_editServerUrl.GetWindowText(sUrl);
    m_editPort.GetWindowText(sPort);
    int port = _ttoi(sPort);
    m_http.SetBaseURL(CT2A(sUrl, CP_UTF8), port);
}

CString CTestClientDlg::GetRoomId() {
    CString s; m_editRoomId.GetWindowText(s); return s;
}

FGetDecisionRequest CTestClientDlg::BuildDecisionRequest() {
    FGetDecisionRequest req;

    req.room_id = CT2A(GetRoomId(), CP_UTF8);
    ++m_reqNum;
    char buf[128];
    sprintf_s(buf, "%s_%d", req.room_id.c_str(), m_reqNum);
    req.request_num = buf;

    req.global_context.time_remaining    = 300.f;
    req.global_context.game_time_seconds = 0.f;
    req.global_context.score             = 0;

    req.ai_squad_context.pacman_main.position.x  = GetF(m_editPacX);
    req.ai_squad_context.pacman_main.position.y  = GetF(m_editPacY);
    req.ai_squad_context.pacman_main.position.z  = GetF(m_editPacZ);
    req.ai_squad_context.pacman_main.hp           = GetF(m_editPacHP, 100.f);
    req.ai_squad_context.pacman_main.speed        = GetF(m_editPacSpeed);
    req.ai_squad_context.pacman_main.is_invulnerable =
        (m_chkInvuln.GetCheck() == BST_CHECKED);

    FCloneUnit c1;
    c1.unit_id   = "clone_1";
    c1.is_alive  = (m_chkC1Alive.GetCheck() == BST_CHECKED);
    c1.position.x= GetF(m_editC1X); c1.position.y= GetF(m_editC1Y); c1.position.z= GetF(m_editC1Z);
    c1.hp        = GetF(m_editC1HP, 1.f);
    req.ai_squad_context.clones.push_back(c1);

    FCloneUnit c2;
    c2.unit_id   = "clone_2";
    c2.is_alive  = (m_chkC2Alive.GetCheck() == BST_CHECKED);
    c2.position.x= GetF(m_editC2X); c2.position.y= GetF(m_editC2Y); c2.position.z= GetF(m_editC2Z);
    c2.hp        = GetF(m_editC2HP, 1.f);
    req.ai_squad_context.clones.push_back(c2);

    FPlayerTeamContext p1;
    p1.player_id   = "STEAM_TEST_CMD";
    p1.player_name = "TestCommander";
    p1.player_type = "COMMANDER";
    p1.role        = 1;
    p1.position.x  = GetF(m_editP1X);
    p1.position.y  = GetF(m_editP1Y);
    p1.position.z  = GetF(m_editP1Z);
    p1.hp          = GetF(m_editP1HP, 100.f);
    p1.distance_to_pacman        = GetF(m_editP1Dist, 300.f);
    p1.navmesh_cost_to_pacman    = p1.distance_to_pacman;
    p1.navmesh_distance_to_pacman= p1.distance_to_pacman;
    p1.navmesh_path_valid        = true;
    req.player_team_context.push_back(p1);

    return req;
}

void CTestClientDlg::DisplayDecisionResponse(const FGetDecisionResponse& resp) {
    CString display;
    if (!resp.success) {
        display.Format(L"[FAILED] status=%s\r\n\r\n%s",
                       CA2W(resp.status.c_str()), CA2W(resp.raw_json.c_str()));
        m_editResponse.SetWindowText(display);
        Log(L"[GET DECISION] 실패: " + CString(CA2W(resp.status.c_str())));
        return;
    }

    display.Format(L"OK | latency=%.0fms | confidence=%.2f | fallback=%s\r\n",
                   resp.meta.latency_ms, resp.decision.confidence,
                   resp.meta.fallback_used ? L"YES" : L"NO");

    display += L"squad_objective: ";
    display += CA2W(resp.decision.squad_objective.c_str());
    display += L"\r\n";

    if (!resp.decision.reasoning.empty()) {
        display += L"reasoning: ";
        display += CA2W(resp.decision.reasoning.c_str());
        display += L"\r\n";
    }
    display += L"\r\n";

    for (const auto& cmd : resp.decision.unit_commands) {
        CString line;
        line.Format(L"  [%-12s]  %s (%d)   pos=(%.0f, %.0f, %.0f)",
                    CA2W(cmd.unit_id.c_str()),
                    CA2W(cmd.directive_name.c_str()),
                    cmd.directive_code,
                    cmd.params.target_position.x,
                    cmd.params.target_position.y,
                    cmd.params.target_position.z);
        display += line + L"\r\n";

        CString logLine;
        logLine.Format(L"[GET DECISION] %s -> %s(%d)",
                       CA2W(cmd.unit_id.c_str()),
                       CA2W(cmd.directive_name.c_str()),
                       cmd.directive_code);
        Log(logLine);
    }

    if (resp.has_pending_tts) {
        display += L"\r\n[!] Overseer TTS 대기 중 - POST /api/v1/overseer/tts 호출 필요\r\n";
    }

    m_editResponse.SetWindowText(display);
}

float CTestClientDlg::GetF(CEdit& e, float def) {
    CString s; e.GetWindowText(s);
    if (s.IsEmpty()) return def;
    try { return (float)_wtof(s); } catch (...) { return def; }
}

void CTestClientDlg::SetF(CEdit& e, float v) {
    CString s; s.Format(L"%.1f", v); e.SetWindowText(s);
}

CString CTestClientDlg::GetS(CEdit& e) {
    CString s; e.GetWindowText(s); return s;
}

void CTestClientDlg::OnPaint() {
    if (IsIconic()) {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width()  - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
    } else {
        CDialogEx::OnPaint();
    }
}

HCURSOR CTestClientDlg::OnQueryDragIcon() {
    return static_cast<HCURSOR>(m_hIcon);
}
