#pragma once
#include "pch.h"

class CCitRushMFCClientApp : public CWinApp {
public:
    CCitRushMFCClientApp();
    virtual BOOL InitInstance();
    DECLARE_MESSAGE_MAP()
};

extern CCitRushMFCClientApp theApp;
