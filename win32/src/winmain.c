#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "callbacks.h"

HWND hwndTransferButton;
HWND hwndCancelButton;
HWND hwndLogEdit;

void Append(TCHAR *text)
{
  int outLength = GetWindowTextLength(hwndLogEdit);
  SendMessage(hwndLogEdit, EM_SETSEL, outLength, outLength);
  SendMessage(hwndLogEdit, EM_REPLACESEL, TRUE, (LPARAM)text);
}

void Clear()
{
  int outLength = GetWindowTextLength(hwndLogEdit);
  SendMessage(hwndLogEdit, EM_SETSEL, 0, outLength);
  SendMessage(hwndLogEdit, EM_REPLACESEL, TRUE, (LPARAM)L"");
}

// Our application entry point.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  INITCOMMONCONTROLSEX icc;
  WNDCLASSEX wc;
  LPCTSTR MainWndClass = TEXT("Barcode scanner interface");
  HWND hWnd;
  HACCEL hAccelerators;
  HMENU hSysMenu;
  MSG msg;
  HDC hdc;

  // Initialise common controls.
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&icc);

  // Class for our main window.
  wc.cbSize        = sizeof(wc);
  wc.style         = 0;
  wc.lpfnWndProc   = &MainWndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInstance;
  wc.hIcon         = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 0, 0,
                                       LR_DEFAULTSIZE | LR_DEFAULTCOLOR | LR_SHARED);
  wc.hCursor       = (HCURSOR) LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
  wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
  wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
  wc.lpszClassName = MainWndClass;
  wc.hIconSm       = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON,
                                       GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                       LR_DEFAULTCOLOR | LR_SHARED);

  // Register our window classes, or error.
  if (! RegisterClassEx(&wc))
  {
    MessageBox(NULL, TEXT("Error registering window class."), TEXT("Error"), MB_ICONERROR | MB_OK);
    return 0;
  }
  
  // Create instance of main window.
  hWnd = CreateWindowEx(0, MainWndClass, MainWndClass, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                        320, 300, NULL, NULL, hInstance, NULL);

  // Error if window creation failed.
  if (! hWnd)
  {
    MessageBox(NULL, TEXT("Error creating main window."), TEXT("Error"), MB_ICONERROR | MB_OK);
    return 0;
  }

  // Load accelerators.
  hAccelerators = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));

  // Add "about" to the system menu.
  hSysMenu = GetSystemMenu(hWnd, FALSE);
  InsertMenu(hSysMenu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
  InsertMenu(hSysMenu, 6, MF_BYPOSITION, ID_HELP_ABOUT, TEXT("About"));

  // Show window and force a paint.
  ShowWindow(hWnd, nCmdShow);

  hwndTransferButton = CreateWindow(
    L"BUTTON",
    L"Transfer",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
    5, // Xpos
    5, // Ypos
    100, // Width
    20, // Height
    hWnd, // Parent
    (HMENU)TransferID, // No menu
    hInstance,
    NULL
  );

#if 0
  hwndCancelButton = CreateWindow(
    L"BUTTON",
    L"Cancel",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
    5 + 100 + 5, // Xpos
    5, // Ypos
    100, // Width
    20, // Height
    hWnd, // Parent
    (HMENU)CancelID, // No menu
    hInstance,
    NULL
  );
#endif

  hwndLogEdit = CreateWindowEx(
    WS_EX_CLIENTEDGE,
    L"EDIT",
    L"",
    WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
    5,
    5 + 20 + 5,
    320 - 16 - 5 - 5,
    300 - 60 - (5 + 20 + 5) - 5,
    hWnd,
    (HMENU)LogID,
    hInstance,
    NULL
  );

/*
  {
    HDC hdc = GetDC(hWnd);
    RECT rect;
    GetClientRect(hWnd, &rect);
    DrawText(hdc, L"Hello, world!  This is a test of this thing.", -1, &rect, DT_WORDBREAK);
    ReleaseDC(NULL, hdc);
  }
*/

  UpdateWindow(hWnd);

//  Append(L"Hello\n");
//  Append(L"There\n");

  // Main message loop.
  while(GetMessage(&msg, NULL, 0, 0) > 0)
  {
    if (! TranslateAccelerator(msg.hwnd, hAccelerators, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return (int) msg.wParam;
}
