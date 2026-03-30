#pragma once
#include "pch.h"

class CCitRushTestClientApp : public CWinApp {
public:
    CCitRushTestClientApp();
    virtual BOOL InitInstance();
    DECLARE_MESSAGE_MAP()
};

extern CCitRushTestClientApp theApp;
