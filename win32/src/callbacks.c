#include <stdio.h>
#include <string.h>
#include "callbacks.h"
#include "resource.h"

void Append(TCHAR *text);

void Clear();

#define LOCATIONS_VER "c:\\barcode\\locations.ver"
#define LOCATIONS_FILE "c:\\barcode\\locations.txt"
#define LOCATIONS_OLD "c:\\barcode\\locations.prv"
#define DATA_FILE "c:\\barcode\\data.txt"

#define NELEMS(X) (sizeof(X)/sizeof(X[0]))

#define sz(s) (s), (sizeof(s) - 1)

/* Maybe use to get com port properties... 
HDEVINFO handle = SetupDiGetClassDevs(&GUID_DEVCLASS_COMMPORT, NULL, NULL, DIGCF_PRESENT);
index = 0;
SP_DEVINFO_DATA data;
while (SetupDiEnumDeviceInfo(handle, index, &data)) {
  SetupDiGetDeviceProperty(handle, &data, DEVPROPKEY, DEVPROPTYPE, buffer, sizeof(buffer), &len, flags)
  ++index;
}
SetupDiDestroyDeviceInfoList(handle);
*/

// Get port names
// Returns the last found port name in buf1
LONG GetPortNames(TCHAR *buf1)
{
  TCHAR buf[80];
  DWORD count;
  DWORD port;
  HKEY hKey;
  LONG res;
  TCHAR regValue[MAX_PATH];
  TCHAR regName[MAX_PATH];
  DWORD lenValue;
  DWORD lenName;
  DWORD myType;
  res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey);
  if (res != ERROR_SUCCESS) {
    MessageBox(NULL, L"Couldn't open registry key HARDWARE\\DEVICEMAP\\SERIALCOMM", L"Status", MB_ICONWARNING | MB_OK);
    return res;
  }
  // Get number of port
  res = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL);
  if (res != ERROR_SUCCESS) {
    MessageBox(NULL, L"Couldn't get count", L"Status", MB_ICONWARNING | MB_OK);
    return res;
  }
  wsprintf(buf, L"Port count = %d\n", count);
  Append(buf);
  for (port = 0; port < count; ++port) {
    // TCHAR buf1[80];
    lenValue = NELEMS(regValue);
    lenName = NELEMS(regName);
    myType = REG_SZ;
    res = RegEnumValue(hKey, port, regValue, &lenValue, NULL, &myType, (LPBYTE)regName, &lenName);
    if (res != ERROR_SUCCESS) {
      MessageBox(NULL, L"Couldn't read port info", L"Status", MB_ICONWARNING | MB_OK);
      return res;
    }
    memmove(buf1, regName, lenName * sizeof(buf1[0]));
    buf1[lenName] = 0;
    wsprintf(buf, L"Found port %ls\n", buf1);
    Append(buf);

    // memmove(buf2, regValue, lenValue * sizeof(buf2[0]));
    // buf2[lenValue] = 0;
    // wsprintf(buf, L"  value is %ls\n", buf2);
    // Append(buf);
  }
  RegCloseKey(hKey);
  return 0;
}

// Protocol
//
// ATH\n                  Hello
//   OK\n
// ATE\n                  Erase location database
//   OK\n
// ATL"code","location"\n    Add location
//   OK\n
// ATDYYYYxMMxDDx\n          Set date
//   OK\n
// ATTHHxMMx\n               Set time
//   OK\n
// ATF                       Get first record
// ATN                       Get next record
//   "date","time","code","cart"\n   Returned card record format
//   E                       End of data

char ch = 'A';

/* Read a record from the serial port */

void GetRec(HANDLE f, char *buf)
{
  TCHAR msg[80];
  char ch;
  int x;
  int y;
  DWORD len;
  int count = 0;
  for (x = 0;;) {
    // WaitCommEvent(f, &len, NULL);
    ReadFile(f, &ch, 1, &len, NULL);
    if (len == 1) {
      count = 0;
      if (ch == '\n')
        break;
      else if (x != 78 && ch >= 32 && ch < 127) {
        buf[x++] = ch;
      }
    } else {
      Sleep(10);
      if (++count == 100) // Timeout
        break;
    }
  }
  buf[x] = 0;

  if (count == 100) {
    Append(L"Timeout!\n");
  }

  for (y = 0; buf[y]; ++y)
    msg[y] = buf[x];
  msg[y] = '\n';
  msg[y+1] = 0;
  Append(msg);
}

/* Get confirmation */

int GetOK(HANDLE f)
{
  char buf[80];
  GetRec(f, buf);
  if (buf[0] == 'O' && buf[1] == 'K' && !buf[2])
    return 1;
  else {
    Append(L"  Acknowledgement not received!\n");
    return 0;
  }
}

extern HWND hwndLogEdit;

/* Get version of locations.txt file: this is an integer between 0 - 99.
   If the file does not exist, return -1
*/

int get_file_version()
{
  TCHAR msg[80];
  char buf[80];
  char buf1[80];
  int ver = -1;
  FILE *g, *h;

  Append(L"Check file version...\n");

  g = fopen(LOCATIONS_FILE, "r");
  if (!g)
    return -1;

  h = fopen(LOCATIONS_OLD, "r");
  if (h) {
    while (fgets(buf, sizeof(buf), g)) {
      if (!fgets(buf1, sizeof(buf1), h)) {
        /* .prv file is short */
        rewind(g);
        fclose(h);
        goto copy;
      } else if (strcmp(buf, buf1)) {
        /* at least one line is different */
        rewind(g);
        fclose(h);
        goto copy;
      }
    }
    if (fgets(buf1, sizeof(buf1), h)) {
      /* .prv file is long */
      rewind(g);
      fclose(h);
      goto copy;
    }
    fclose(g);
    fclose(h);
    Append(L"File version has not changed\n");
    g = fopen(LOCATIONS_VER, "r");
    if (g) {
      Append(L"  parse version number\n");
      fscanf(g, "%d", &ver);
      fclose(g);
    }
    wsprintf(msg, L"File version number is %d\n", ver);
    Append(msg);
    return ver; /* No version change */
  }

  /* Copy locations file so we can detect version differences */
  Append(L"File has changed\n");
  Append(L"  Copy it for comparison\n");
  copy: h = fopen(LOCATIONS_OLD, "w");
  while (fgets(buf, sizeof(buf), g)) {
    fputs(buf, h);
  }
  fclose(g);
  fclose(h);

  /* Bump version number */
  Append(L"  bump version number\n");
  g = fopen(LOCATIONS_VER, "r");
  if (g) {
    Append(L"  parse version number\n");
    fscanf(g, "%d", &ver);
    fclose(g);
  }
  wsprintf(msg, L"Old file version number was %d\n", ver);
  Append(msg);
  if (ver >= 99 || ver < 0)
    ver = 0;
  else
    ver = ver + 1;
  wsprintf(msg, L"New file version number is %d\n", ver);
  Append(msg);
  g = fopen(LOCATIONS_VER, "w");
  fprintf(g, "%d\n", ver);
  fclose(g);

  return ver;
}

/* Compare file version with version in scanner- if different, return true
 */

int check_scanner_version(int file_version, HANDLE f)
{
  TCHAR msg[80];
  char buf[80];
  int ver = -2;
  DWORD len = 0;
  Append(L"Get version number from scanner\n");
  WriteFile(f, sz("ATV\n"), &len, NULL);
  GetRec(f, buf);
  sscanf(buf, "%d", &ver);
  wsprintf(msg, L"Scanner version number is %d\n", ver);
  Append(msg);
  if (ver != file_version)
    return 1;
  else
    return 0;
}

/* Record version in scanner */

int set_version(int file_version, HANDLE f)
{
  char buf[80];
  int ver = -2;
  DWORD len = 0;
  Append(L"Record version on scanner...\n");
  sprintf(buf, "ATV%d\n", file_version);
  WriteFile(f, buf, strlen(buf), &len, NULL);
  GetRec(f, buf);
  sscanf(buf, "%d", &ver);
  if (ver != file_version) {
    Append(L"Couldn't set version\n");
    return -1;
  } else
    return 0;
}

#define CREDLIM 1

// Window procedure for our main window.
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static HINSTANCE hInstance;

  switch (msg)
  {  
    case WM_SIZE:
    {
      RECT rect;
      GetClientRect(hWnd, &rect);
      MoveWindow(
          hwndLogEdit,
          5,
          5 + 20 + 5,
          rect.right - rect.left - 5 - 5,
          rect.bottom - rect.top - (5 + 20 + 5) - 5,
          TRUE
      );
      return 0;
    }

    case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
        case ID_HELP_ABOUT:
        {
          DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTDIALOG), hWnd, &AboutDialogProc);
          return 0;
        }

        case ID_FILE_EXIT:
        {
          DestroyWindow(hWnd);
          return 0;
        }

        case TransferID:
        {
          int cred; // Credit counter
          int ver;
          int count;
          char buf[80];
          char buf1[80];
          TCHAR msg[80];
          TCHAR devname[80];
          TCHAR portname[80];
          SYSTEMTIME tim;
          GetLocalTime(&tim);
          Clear();
          wsprintf(msg,L"Begin %4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d\n", 
                   tim.wYear,
                   tim.wMonth,
                   tim.wDay,
                   tim.wHour,
                   tim.wMinute,
                   tim.wSecond);
          Append(msg);
          if (GetPortNames(portname))
            return 0;
          wsprintf(devname, L"\\\\.\\%ls", portname);
          HANDLE f = CreateFile(devname,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
          if (f == INVALID_HANDLE_VALUE) {
            Append(L"Couldn't open serial port\n");
            goto fail;
          } else {
            DCB dcb = { 0 };
            int retry;
            DWORD len = 0;
            if (!GetCommState(f, &dcb)) {
              Append(L"GetCommState failed\n");
              goto fail;
            }
            dcb.BaudRate = CBR_115200;
            dcb.ByteSize = 8;
            dcb.StopBits = ONESTOPBIT;
            dcb.Parity = NOPARITY;
            if(!SetCommState(f, &dcb)) {
              Append(L"SetCommState failed\n");
              goto fail;
            }
//            WriteFile(f, &ch, 1, &len, NULL);
//            ++ch;
//            if (len != 1) {
//              MessageBox(NULL, L"WriteFile failed", L"Status", MB_ICONWARNING | MB_OK);
//            }
            COMMTIMEOUTS timeouts = { 0 };
            timeouts.ReadIntervalTimeout = 50;
            timeouts.ReadTotalTimeoutConstant = 50;
            timeouts.ReadTotalTimeoutMultiplier = 10;
            timeouts.WriteTotalTimeoutConstant = 50;
            timeouts.WriteTotalTimeoutMultiplier = 10;
            if (!SetCommTimeouts(f, &timeouts)) {
              Append(L"SetCommTimeouts failed\n");
              goto fail;
            }
            SetCommMask(f, EV_RXCHAR);
            for(retry = 0; retry != 3; ++retry) {
              Append(L"Say hello to scanner...\n");
              WriteFile(f, sz("ATH\n"), &len, NULL);
              if (GetOK(f))
                break;
              else
                Append(L"..Hmm, try again..\n");
             }
            if (retry == 3) {
              Append(L"Failed to contact scanner\n");
              goto fail;
            } else {
              Append(L"Connection established\n");
            }
            Append(L"Set date / time...\n");
            sprintf(buf,"ATD%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d\n",
                   tim.wYear,
                   tim.wMonth,
                   tim.wDay,
                   tim.wHour,
                   tim.wMinute,
                   tim.wSecond);
            WriteFile(f, buf, strlen(buf), &len, NULL);
            if (GetOK(f)) {
              Append(L"  OK\n");
            } else {
              goto fail;
            }
            Append(L"Check if locations need to be updated...\n");
            if (check_scanner_version((ver = get_file_version()), f)) {
              Append(L"Erase location table\n");
              WriteFile(f, sz("ATE\n"), &len, NULL);
              if (GetOK(f)) {
                Append(L"  OK\n");
              } else {
                goto fail;
              }
              Append(L"Send locations table\n");
              FILE *g = fopen(LOCATIONS_FILE, "r");
              if (g) {
                count = 0;
                cred = 0;
                while (fgets(buf1, sizeof(buf1), g)) {
                  ++count;
                  wsprintf(msg,L"   %hs", buf1);
                  Append(msg);
                  sprintf(buf,"ATL%s", buf1);
                  WriteFile(f, buf, strlen(buf), &len, NULL);
                  ++cred;
                  if (cred == CREDLIM) {
                    --cred;
                    if (!GetOK(f)) {
                      fclose(g);
                      goto fail;
                    }
                  }
                }
                fclose(g);
                while (cred--) {
                  if (!GetOK(f)) {
                    goto fail;
                  }
                }
                wsprintf(msg, L"Transferred %d records\n", count);
                Append(msg);
                if (set_version(ver, f))
                  goto fail;
              } else {
                Append(L"Couldn't open c:\\barcode\\locations.txt\n");
                goto fail;
              }
            } else {
              Append(L"Scanner has latest version.\n");
            }
#if 0
            Append(L"Check for old c:\\barcode\\data.txt file...\n");
            FILE *g = fopen(DATA_FILE, "r");
            if (g) {
              fclose(g);
              Append(L"  Old data still present!\n");
              goto fail;
            }
#endif
            Append(L"Get scan table\n");
            FILE *g = fopen(DATA_FILE, "a");
            count = 0;
            if (g) {
              for (;;) {
                if (count)
                  WriteFile(f, sz("ATN\n"), &len, NULL);
                else
                  WriteFile(f, sz("ATF\n"), &len, NULL);
                GetRec(f, buf);
                if (buf[0] == 'E') {
                  break;
                } else if (!buf[0]) {
                  Append(L"Couldn't get next record\n");
                  fclose(g);
                  goto fail;
                } else {
                  wsprintf(msg,L"  %hs\n", buf);
                  Append(msg);
                  if (fprintf(g, "%s\n", buf) <= 0) {
                    Append(L"Error writing file\n");
                    fclose(g);
                    goto fail;
                  }
                  ++count;
                }
              }
              if (fclose(g)) {
                Append(L"Error closing file\n");
                goto fail;
              }
              wsprintf(msg, L"Transferred %d records\n", count);
              Append(msg);
              // Erase table on device
              Append(L"Erase scan data on device\n");
              WriteFile(f, sz("ATX\n"), &len, NULL);
              if (GetOK(f)) {
                Append(L"  OK\n");
              } else {
                goto fail;
              }
            } else {
              Append(L"Couldn't open c:\\barcode\\data.txt\n");
              goto fail;
            }
            CloseHandle(f);
            Append(L"Transfer completed successfully!\n");
            return 0;
            fail:
            if (f != INVALID_HANDLE_VALUE)
              CloseHandle(f);
            Append(L"Transfer failed!\n");
            MessageBox(NULL, L"Transfer failed", L"Status", MB_ICONWARNING | MB_OK);
          }
          
          return 0;
        }

        case CancelID:
        {
          Clear();
          return 0;
        }
      }
      break;
    }

    case WM_GETMINMAXINFO:
    {
      MINMAXINFO *minMax = (MINMAXINFO*) lParam;
      minMax->ptMinTrackSize.x = 220;
      minMax->ptMinTrackSize.y = 110;

      return 0;
    }

    case WM_SYSCOMMAND:
    {
      switch (LOWORD(wParam))
      {
        case ID_HELP_ABOUT:
        {
          DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTDIALOG), hWnd, &AboutDialogProc);
          return 0;
        }
      }
      break;
    }
    
    case WM_CREATE:
    {
      hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
      return 0;
    }

    case WM_DESTROY:
    {
      PostQuitMessage(0);
      return 0;
    }
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}


// Dialog procedure for our "about" dialog.
INT_PTR CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
        case IDOK:
        case IDCANCEL:
        {
          EndDialog(hwndDlg, (INT_PTR) LOWORD(wParam));
          return (INT_PTR) TRUE;
        }
      }
      break;
    }

    case WM_INITDIALOG:
      return (INT_PTR) TRUE;
  }

  return (INT_PTR) FALSE;
}
