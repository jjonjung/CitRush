#include "../pch.h"
#include "CHttpClient.h"

CHttpClient::CHttpClient() = default;
CHttpClient::~CHttpClient() = default;

void CHttpClient::SetBaseURL(const std::string& host, int port) {
    std::string h = host;
    if (h.substr(0, 7) == "http://")  h = h.substr(7);
    if (h.substr(0, 8) == "https://") h = h.substr(8);
    while (!h.empty() && h.back() == '/') h.pop_back();

    m_host = ToWide(h);
    m_port = (port > 0) ? port : 8000;
}

bool CHttpClient::Get(const std::string& path, std::string& outBody, int& outStatus) {
    return SendRequest(L"GET", ToWide(path), "", outBody, outStatus);
}

bool CHttpClient::Post(const std::string& path, const std::string& jsonBody,
                       std::string& outBody, int& outStatus) {
    return SendRequest(L"POST", ToWide(path), jsonBody, outBody, outStatus);
}

bool CHttpClient::SendRequest(const std::wstring& method, const std::wstring& path,
                               const std::string& body,
                               std::string& outBody, int& outStatus) {
    outBody.clear();
    outStatus = 0;
    m_lastError.clear();

    HINTERNET hSession = WinHttpOpen(
        L"CitRushMFCClient/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { m_lastError = "WinHttpOpen failed"; return false; }

    HINTERNET hConnect = WinHttpConnect(hSession, m_host.c_str(), (INTERNET_PORT)m_port, 0);
    if (!hConnect) {
        m_lastError = "WinHttpConnect failed";
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, method.c_str(), path.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        m_lastError = "WinHttpOpenRequest failed";
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    WinHttpSetTimeouts(hRequest, m_timeout, m_timeout, m_timeout, m_timeout);

    if (method == L"POST") {
        WinHttpAddRequestHeaders(hRequest,
            L"Content-Type: application/json\r\n",
            (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);
    }

    LPVOID pBody  = body.empty() ? nullptr : (LPVOID)body.c_str();
    DWORD  bodyLen= (DWORD)body.size();

    bool ok = (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                   pBody, bodyLen, bodyLen, 0) != FALSE);
    if (ok) ok = (WinHttpReceiveResponse(hRequest, nullptr) != FALSE);

    if (ok) {
        DWORD statusCode = 0, statusSize = sizeof(DWORD);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            nullptr, &statusCode, &statusSize, nullptr);
        outStatus = (int)statusCode;

        DWORD available = 0;
        while (WinHttpQueryDataAvailable(hRequest, &available) && available > 0) {
            std::string chunk(available, '\0');
            DWORD read = 0;
            WinHttpReadData(hRequest, &chunk[0], available, &read);
            outBody.append(chunk.c_str(), read);
        }
    } else {
        DWORD err = GetLastError();
        char buf[64];
        sprintf_s(buf, "WinHTTP error: 0x%08X", err);
        m_lastError = buf;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok && outStatus > 0;
}

std::wstring CHttpClient::ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], len);
    return w;
}
