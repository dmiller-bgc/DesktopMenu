#include "stdafx.h"
#include "afxwin.h"
#include "shlobj.h"
#include "shlwapi.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

class CDesktopMenuFrame : public CFrameWnd
{
public:
    CDesktopMenuFrame() {}

protected:
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMenuCommand(UINT nID);
    afx_msg void OnDestroy();

    DECLARE_MESSAGE_MAP()

private:
    HMENU CreateDesktopMenu();
    void FillSubMenu(HMENU hSubMenu, LPITEMIDLIST parentPidl);
};

BEGIN_MESSAGE_MAP(CDesktopMenuFrame, CFrameWnd)
    ON_WM_RBUTTONDOWN()
    ON_COMMAND_RANGE(0, 0xFFFF, OnMenuCommand) // Handle all menu commands
    ON_WM_DESTROY()
END_MESSAGE_MAP()

void CDesktopMenuFrame::OnRButtonDown(UINT nFlags, CPoint point)
{
    HMENU hMenu = CreateDesktopMenu();
    if (hMenu)
    {
        ClientToScreen(&point);
        TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, point.x, point.y, 0, m_hWnd, NULL);
        DestroyMenu(hMenu);
    }
}

void CDesktopMenuFrame::OnMenuCommand(UINT nID)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)nID;
    if (pidl)
    {
        ShellExecuteA(NULL, "open", NULL, NULL, (LPCTSTR)pidl, SW_SHOWNORMAL);
    }
}

void CDesktopMenuFrame::OnDestroy()
{
    CFrameWnd::OnDestroy();
    PostQuitMessage(0);
}

HMENU CDesktopMenuFrame::CreateDesktopMenu()
{
    HMENU hMenu = CreatePopupMenu();
    if (hMenu == nullptr)
    {
        return nullptr;
    }

    IShellFolder* pDesktopFolder = nullptr;
    HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
    if (FAILED(hr))
    {
        DestroyMenu(hMenu);
        return nullptr;
    }

    IEnumIDList* pEnumIDList = nullptr;
    hr = pDesktopFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIDList);
    if (FAILED(hr))
    {
        pDesktopFolder->Release();
        DestroyMenu(hMenu);
        return nullptr;
    }

    LPITEMIDLIST pidl;
    while (pEnumIDList->Next(1, &pidl, NULL) == S_OK)
    {
        STRRET str;
        hr = pDesktopFolder->GetDisplayNameOf(pidl, SHGDN_NORMAL, &str);
        if (SUCCEEDED(hr))
        {
            char displayName[MAX_PATH];
            StrRetToBuf(&str, pidl, displayName, MAX_PATH);

            SHFILEINFO fileInfo;
            if (SHGetFileInfo((LPCTSTR)pidl, 0, &fileInfo, sizeof(fileInfo), SHGFI_PIDL | SHGFI_ATTRIBUTES))
            {
                if (fileInfo.dwAttributes & SFGAO_FOLDER)
                {
                    HMENU subMenu = CreatePopupMenu();
                    if (subMenu)
                    {
                        AppendMenuA(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)subMenu, displayName);
                        FillSubMenu(subMenu, pidl);
                    }
                    else
                    {
                        AppendMenuA(hMenu, MF_STRING, (UINT_PTR)pidl, displayName);
                    }
                }
                else
                {
                    AppendMenuA(hMenu, MF_STRING, (UINT_PTR)pidl, displayName);
                }
            }
            else
            {
                AppendMenuA(hMenu, MF_STRING, (UINT_PTR)pidl, displayName);
            }
        }
        CoTaskMemFree(pidl);
    }

    pEnumIDList->Release();
    pDesktopFolder->Release();
    return hMenu;
}

void CDesktopMenuFrame::FillSubMenu(HMENU hSubMenu, LPITEMIDLIST parentPidl)
{
    IShellFolder* pParentFolder = nullptr;
    HRESULT hr = SHBindToParent(parentPidl, IID_IShellFolder, (void**)&pParentFolder, NULL);
    if (FAILED(hr))
    {
        return;
    }

    IEnumIDList* pEnumIDList = nullptr;
    hr = pParentFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIDList);
    if (FAILED(hr))
    {
        pParentFolder->Release();
        return;
    }

    LPITEMIDLIST pidl;
    while (pEnumIDList->Next(1, &pidl, NULL) == S_OK)
    {
        STRRET str;
        hr = pParentFolder->GetDisplayNameOf(pidl, SHGDN_NORMAL, &str);
        if (SUCCEEDED(hr))
        {
            char displayName[MAX_PATH];
            StrRetToBuf(&str, pidl, displayName, MAX_PATH);

            SHFILEINFO fileInfo;
            if (SHGetFileInfo((LPCTSTR)pidl, 0, &fileInfo, sizeof(fileInfo), SHGFI_PIDL | SHGFI_ATTRIBUTES))
            {
                if (fileInfo.dwAttributes & SFGAO_FOLDER)
                {
                    HMENU subSubMenu = CreatePopupMenu();
                    if (subSubMenu)
                    {
                        AppendMenuA(hSubMenu, MF_STRING | MF_POPUP, (UINT_PTR)subSubMenu, displayName);
                        FillSubMenu(subSubMenu, pidl);
                    }
                    else
                    {
                        AppendMenuA(hSubMenu, MF_STRING, (UINT_PTR)pidl, displayName);
                    }
                }
                else
                {
                    AppendMenuA(hSubMenu, MF_STRING, (UINT_PTR)pidl, displayName);
                }
            }
            else
            {
                AppendMenuA(hSubMenu, MF_STRING, (UINT_PTR)pidl, displayName);
            }
        }
        CoTaskMemFree(pidl);
    }
    pEnumIDList->Release();
    pParentFolder->Release();
}

class CDesktopMenuApp : public CWinApp
{
public:
    virtual BOOL InitInstance();
};

CDesktopMenuApp theApp;

BOOL CDesktopMenuApp::InitInstance()
{
    CFrameWnd* pFrame = new CDesktopMenuFrame();
    if (!pFrame)
        return FALSE;
    m_pMainWnd = pFrame;
    pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL);
    pFrame->ShowWindow(SW_SHOW);
    pFrame->UpdateWindow();
    return TRUE;
}