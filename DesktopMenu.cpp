#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

HMENU CreateDesktopMenu();
void FillSubMenu(HMENU hSubMenu, LPITEMIDLIST parentPidl);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_RBUTTONDOWN: {
        HMENU hMenu = CreateDesktopMenu();
        if (hMenu) {
            POINT cursorPos;
            GetCursorPos(&cursorPos);
            TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, cursorPos.x, cursorPos.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
        break;
    }
    case WM_COMMAND: {
        LPITEMIDLIST pidl = (LPITEMIDLIST)wParam;
        if (pidl) {
            ShellExecuteA(NULL, "open", NULL, NULL, (LPCTSTR)pidl, SW_SHOWNORMAL);
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    CoInitialize(NULL);

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "DesktopMenuClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    RegisterClassEx(&wcex);

    HWND hWnd = CreateWindow("DesktopMenuClass", "Desktop Menu", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    return (int)msg.wParam;
}

HMENU CreateDesktopMenu() {
    HMENU hMenu = CreatePopupMenu();
    if (hMenu == nullptr) {
        return nullptr;
    }

    IShellFolder* pDesktopFolder = nullptr;
    HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
    if (FAILED(hr)) {
        DestroyMenu(hMenu);
        return nullptr;
    }

    IEnumIDList* pEnumIDList = nullptr;
    hr = pDesktopFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIDList);
    if (FAILED(hr)) {
        pDesktopFolder->Release();
        DestroyMenu(hMenu);
        return nullptr;
    }

    LPITEMIDLIST pidl;
    while (pEnumIDList->Next(1, &pidl, NULL) == S_OK) {
        STRRET str;
        hr = pDesktopFolder->GetDisplayNameOf(pidl, SHGDN_NORMAL, &str);
        if (SUCCEEDED(hr)) {
            char displayName[MAX_PATH];
            StrRetToBuf(&str, pidl, displayName, MAX_PATH);

            SHFILEINFO fileInfo;
            if (SHGetFileInfo((LPCTSTR)pidl, 0, &fileInfo, sizeof(fileInfo), SHGFI_PIDL | SHGFI_ATTRIBUTES)) {

                if (fileInfo.dwAttributes & SFGAO_FOLDER) {
                    HMENU subMenu = CreatePopupMenu();
                    if (subMenu) {
                        AppendMenuA(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)subMenu, displayName);
                        FillSubMenu(subMenu, pidl);
                    } else {
                        AppendMenuA(hMenu, MF_STRING, (UINT_PTR)pidl, displayName);
                    }
                } else {
                    AppendMenuA(hMenu, MF_STRING, (UINT_PTR)pidl, displayName);
                }
            } else {
                AppendMenuA(hMenu, MF_STRING, (UINT_PTR)pidl, displayName);
            }
        }
        CoTaskMemFree(pidl);
    }

    pEnumIDList->Release();
    pDesktopFolder->Release();
    return hMenu;
}

void FillSubMenu(HMENU hSubMenu, LPITEMIDLIST parentPidl) {
    IShellFolder* pParentFolder = nullptr;
    HRESULT hr = SHBindToParent(parentPidl, IID_IShellFolder, (void**)&pParentFolder, NULL);
    if (FAILED(hr)) {
        return;
    }

    IEnumIDList* pEnumIDList = nullptr;
    hr = pParentFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIDList);
    if (FAILED(hr)) {
        pParentFolder->Release();
        return;
    }

    LPITEMIDLIST pidl;
    while (pEnumIDList->Next(1, &pidl, NULL) == S_OK) {
        STRRET str;
        hr = pParentFolder->GetDisplayNameOf(pidl, SHGDN_NORMAL, &str);
        if (SUCCEEDED(hr)) {
            char displayName[MAX_PATH];
            StrRetToBuf(&str, pidl, displayName, MAX_PATH);

            SHFILEINFO fileInfo;
            if (SHGetFileInfo((LPCTSTR)pidl, 0, &fileInfo, sizeof(fileInfo), SHGFI_PIDL | SHGFI_ATTRIBUTES)) {

                if (fileInfo.dwAttributes & SFGAO_FOLDER) {
                    HMENU subSubMenu = CreatePopupMenu();
                    if (subSubMenu) {
                        AppendMenuA(hSubMenu, MF_STRING | MF_POPUP, (UINT_PTR)subSubMenu, displayName);
                        FillSubMenu(subSubMenu, pidl);
                    } else {
                        AppendMenuA(hSubMenu, MF_STRING, (UINT_PTR)pidl, displayName);
                    }
                } else {
                    AppendMenuA(hSubMenu, MF_STRING, (UINT_PTR)pidl, displayName);
                }
            } else {
                AppendMenuA(hSubMenu, MF_STRING, (UINT_PTR)pidl, displayName);
            }
        }
        CoTaskMemFree(pidl);
    }
    pEnumIDList->Release();
    pParentFolder->Release();
}