#pragma once
#include <string>
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

class CHttpClient {
public:
    CHttpClient();
    ~CHttpClient();

    void SetBaseURL(const std::string& host, int port);

    bool Get(const std::string& path, std::string& outBody, int& outStatus);

    bool Post(const std::string& path, const std::string& jsonBody,
              std::string& outBody, int& outStatus);

    std::string GetLastError() const { return m_lastError; }

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
