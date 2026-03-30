#pragma once
#include <string>
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

// ============================================================
// CHttpClient
// WinHTTP 기반 동기식 HTTP 클라이언트 (외부 라이브러리 없음)
// /api/v1/health, /api/v1/get_decision 등 AI 서버 호출용
// ============================================================

class CHttpClient {
public:
    CHttpClient();
    ~CHttpClient();

    // 서버 주소 설정 (http:// 스킴 자동 제거)
    void SetBaseURL(const std::string& host, int port);

    // GET 요청
    bool Get(const std::string& path, std::string& outBody, int& outStatus);

    // POST 요청 (Content-Type: application/json)
    bool Post(const std::string& path, const std::string& jsonBody,
              std::string& outBody, int& outStatus);

    // 마지막 에러 메시지
    std::string GetLastError() const { return m_lastError; }

    // 타임아웃 설정 (ms, 기본 10초)
    void SetTimeout(DWORD ms) { m_timeout = ms; }

private:
    bool SendRequest(const std::wstring& method, const std::wstring& path,
                     const std::string& body,
                     std::string& outBody, int& outStatus);

    std::wstring ToWide(const std::string& s);

    std::wstring m_host;
    int          m_port    = 8000;
    DWORD        m_timeout = 10000;
    std::string  m_lastError;
};
