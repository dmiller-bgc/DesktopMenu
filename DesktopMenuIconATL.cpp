#include <atlbase.h>
#include <atlwin.h>
#include <atlcom.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <vector>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

class CDesktopMenuWindow : public CWindowImpl<CDesktopMenuWindow>
{
public:
    DECLARE_WND_CLASS("DesktopMenuClass")

    BEGIN_MSG_MAP(CDesktopMenuWindow)
        MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()

    LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if ((wParam & 0xFFF0) == SC_TASKLIST)
        {
            HMENU hMenu = CreateDesktopMenu();
            if (hMenu)
            {
                POINT cursorPos;
                GetCursorPos(&cursorPos);
                TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, cursorPos.x, cursorPos.y, 0, m_hWnd, NULL);
                DestroyMenu(hMenu);
            }
        }
        else
        {
            bHandled = FALSE;
        }
        return 0;
    }

    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST)wParam;
        if (pidl)
        {
            ShellExecuteA(NULL, "open", NULL, NULL, (LPCTSTR)pidl, SW_SHOWNORMAL);
        }
        return 0;
    }

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PostQuitMessage(0);
        return 0;
    }

private:
    HMENU CreateDesktopMenu()
    {
        HMENU hMenu = CreatePopupMenu();
        if (hMenu == nullptr)
        {
            return nullptr;
        }

        CComPtr<IShellFolder> pDesktopFolder;
        HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
        if (FAILED(hr))
        {
            DestroyMenu(hMenu);
            return nullptr;
        }

        CComPtr<IEnumIDList> pEnumIDList;
        hr = pDesktopFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIDList);
        if (FAILED(hr))
        {
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

        return hMenu;
    }

    void FillSubMenu(HMENU hSubMenu, LPITEMIDLIST parentPidl)
    {
        CComPtr<IShellFolder> pParentFolder;
        HRESULT hr = SHBindToParent(parentPidl, IID_IShellFolder, (void**)&pParentFolder, NULL);
        if (FAILED(hr))
        {
            return;
        }

        CComPtr<IEnumIDList> pEnumIDList;
        hr = pParentFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIDList);
        if (FAILED(hr))
        {
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
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HRESULT hRes = ::CoInitialize(NULL);
    ATLASSERT(SUCCEEDED(hRes));

    AtlAxWinInit();

    CDesktopMenuWindow wnd;

    RECT rc = { 0, 0, 0, 0 }; // Create a very small window
    wnd.Create(NULL, rc, "Desktop Menu", WS_OVERLAPPEDWINDOW | WS_MINIMIZE); //Start minimized

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    AtlAxWinTerm();
    ::CoUninitialize();

    return (int)msg.wParam;
}