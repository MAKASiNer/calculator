#include <windows.h>
#include <string>
#include <array>

#include "calculator.h"
using calculator::evaluate;
calculator::Definition globals{};


int grid_point_width = 11, grid_point_height = 11; // grid system
#define GPX(point) int(point * grid_point_width)   // tranform from a grid point to x int coordinate
#define GPY(point) int(point * grid_point_height)  // tranform from a grid point to y int coordinate

int width = GPX(66);
int height = GPY(57);


HINSTANCE hInst;

HMENU hDisplay = (HMENU)0xf0;
HMENU hNumpad = (HMENU)0xf1;
HMENU hOprpad = (HMENU)0xf2;
HMENU hAlfpad = (HMENU)0xf3;
HMENU hCtrlpad = (HMENU)0xf4;

LPCWSTR title = L"Caculator";
LPCWSTR prompt = L"\n";


bool CALLBACK SetFont(HWND hwnd, LPARAM font) {
    SendMessage(hwnd, WM_SETFONT, font, true);
    return true;
}


void addText(HWND hwnd, const TCHAR* text) {
    int size = GetWindowTextLength(hwnd) + lstrlen(text) + 1;
    TCHAR* buf = (TCHAR*)GlobalAlloc(GPTR, size * sizeof(TCHAR));
    GetWindowText(hwnd, buf, size);
    SetWindowText(hwnd, lstrcat(buf, text));
    GlobalFree(buf);
}

void delText(HWND hwnd, int len) {
    int size = GetWindowTextLength(hwnd) + 1;
    TCHAR* buf = (TCHAR*)GlobalAlloc(GPTR, size * sizeof(TCHAR));
    GetWindowText(hwnd, buf, size - len);
    SetWindowText(hwnd, buf);
    GlobalFree(buf);
}


// events
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND hWndDisplay = FindWindowEx(hWnd, NULL, L"edit", NULL);
    HWND hWndResult = FindWindowEx(hWnd, NULL, L"button", L"calc");
    HWND hWndCe = FindWindowEx(hWnd, NULL, L"button", L"CE");
    HWND hWndC = FindWindowEx(hWnd, NULL, L"button", L"C ");

    switch (message) {

    case WM_LBUTTONDOWN:
        SetFocus(hWnd);
        break;

    case WM_VSCROLL:
        SendMessage(hWndDisplay, WM_VSCROLL, SB_BOTTOM, NULL);
        SetFocus(hWnd);
        break;

    case WM_KEYDOWN:
        if (wParam == VK_BACK) SendMessage(hWndC, BM_CLICK, NULL, NULL);
        else if (wParam == VK_DELETE) SendMessage(hWndCe, BM_CLICK, NULL, NULL);
        else if (wParam == VK_RETURN) {
            // SendMessage(hWnd, WM_COMMAND, (WPARAM)hCtrlpad, (LPARAM)hWndResult);
            SendMessage(hWndResult, BM_CLICK, NULL, NULL);
            SendMessage(hWndDisplay, WM_VSCROLL, SB_PAGEDOWN, NULL);
        }
        else {
            TCHAR key[16];
            GetKeyNameText(lParam, key, 15);
            SendMessage(FindWindowEx(hWnd, NULL, L"button", key), BM_CLICK, NULL, NULL);
        }
        break;

    case WM_COMMAND:
        if (wParam == (WPARAM)hNumpad || wParam == (WPARAM)hOprpad || wParam == (WPARAM)hAlfpad) {
            WCHAR wchr[2];
            GetWindowText((HWND)lParam, wchr, 2);
            addText(hWndDisplay, wchr);
            SendMessage(hWnd, WM_VSCROLL, NULL, NULL);
        }

        else if (wParam == (WPARAM)hCtrlpad) {

            // result button
            if ((HWND)lParam == hWndResult) {
                int size = GetWindowTextLength(hWndDisplay) + 1;
                TCHAR* buf = (TCHAR*)GlobalAlloc(GPTR, size * sizeof(TCHAR));
                GetWindowText(hWndDisplay, buf, size);

                // find current line
                TCHAR* begin = buf + size - 1;
                while (begin != buf) {
                    if (begin - buf >= 2) {
                        if (*(begin - 2) == L'\r' &&
                            *(begin - 1) == L'\n')
                            break;
                    }
                    begin--;
                };

                // calculating
                std::wstring wstr(begin, lstrlen(begin));

                if (wstr.size() > 0) {
                    try {
                        auto res = evaluate(std::string(wstr.begin(), wstr.end()), globals);
                        wstr = wstr = L"\r\n    = " + std::wstring(res.begin(), res.end()) + L"\r\n";
                    }
                    catch (const std::exception& err) {
                        std::string str(err.what());
                        wstr = L"\r\n    " + std::wstring(str.begin(), str.end()) + L"\r\n";
                    }
                    addText(hWndDisplay, wstr.c_str());
                }
                GlobalFree(buf);
            }

            // CE button
            else if ((HWND)lParam == hWndCe) {
                SetWindowText(hWndDisplay, NULL);
                globals.clear();
            }

            // C button
            else if ((HWND)lParam == hWndC) {
                delText(hWndDisplay, 1);
            }

            // scroll display
            SendMessage(hWnd, WM_VSCROLL, NULL, NULL);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}


// main
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    hInst = hInstance;

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"mainWind";
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    RegisterClassEx(&wcex);
    HWND hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        wcex.lpszClassName,
        title,
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    // display
    HWND display = CreateWindow(
        L"edit",
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOVSCROLL | ES_LOWERCASE | ES_MULTILINE,
        GPX(1), GPY(1), GPX(62), GPX(20),
        hWnd, (HMENU)hDisplay, hInst, NULL
    );

    // adding numpad buttons
    std::array<HWND, 10> numpad;
    for (int i = 0; i < numpad.size(); i++) {

        int w = GPX(7);
        int h = GPY(6);
        int x = GPX(1) + (i % 3) * w;
        int y = GPY(22) + (i / 3) * h;

        if (i == 9) x += w;

        numpad[i] = CreateWindow(
            L"button",
            std::to_wstring(i).c_str(),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hWnd, hNumpad, hInst, NULL
        );
    }

    // adding operator buttons
    std::array<HWND, 10> oprpad;
    for (int i = 0; i < oprpad.size(); i++) {
        static std::array<const wchar_t*, oprpad.size()> names = {
            L"+", L"-", L"*", L"/", L"^",
            L"~", L"(", L")", L"=", L"$"
        };

        int w = GPX(7);
        int h = GPY(6);
        int x = GPX(23) + (i % 2) * w;
        int y = GPY(22) + (i / 2) * h;

        oprpad[i] = CreateWindow(
            L"button",
            names[i],
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hWnd, hOprpad, hInst, NULL
        );
    }

    // adding alfpad buttons
    std::array<HWND, 27> alfpad;
    for (int i = 0; i < alfpad.size(); i++) {

        int w = GPX(5);
        int h = GPY(4);
        int x = GPX(38) + (i % 5) * w;
        int y = GPY(22) + (i / 5) * h;

        alfpad[i] = CreateWindow(
            L"button",
            (i == 26) ? L"," : std::wstring(1, L'a' + i).c_str(),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hWnd, hAlfpad, hInst, NULL
        );
    }

    // adding control buttons
    std::array<HWND, 3> ctrlpad;
    for (int i = 0; i < ctrlpad.size(); i++) {
        static std::array<const wchar_t*, oprpad.size()> names = {
            L"CE", L"C ",  L"calc"
        };

        int w = GPX(7);
        int h = GPY(4);
        int x = GPX(1) + i * w;
        int y = GPY(48);

        ctrlpad[i] = CreateWindow(
            L"button",
            names[i],
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hWnd, hCtrlpad, hInst, NULL
        );
    }

    // HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hFont = CreateFont(
        GPX(2.2), NULL,
        NULL, NULL,
        FW_NORMAL,
        NULL, NULL, NULL,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH,
        L"Calibri"
    );

    EnumChildWindows(hWnd, (WNDENUMPROC)SetFont, (LPARAM)hFont);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}