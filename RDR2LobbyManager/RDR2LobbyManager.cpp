// 
//	Litt rotete kode, var et kjapt prosjekt..
//

#include "framework.h"
#include "RDR2LobbyManager.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND TipText1, PassordBoks, OnOffBtn;
HWND ToolTip1;
bool SwitchIsOn = false;
std::wstring ThePassword = L"";
std::mutex gMutexLock;
std::vector<std::wstring> StartupMetaPath;


//
//	Kjører i egen tråd hver sekund for å sjekke om passord blir forandret.
//	Slår da av (fjerner) Startup.meta til brukeren aktivt aktiverer igjen.
//
void WatchPasswordChanges()
{
	while(1) {
		gMutexLock.lock();
		wchar_t Buf[1024] = {0};
		SendMessageW(PassordBoks, WM_GETTEXT, 1024,	(LPARAM)&Buf);

		// Slett metafiler og oppdater bryter hvis brukeren tukler med passord.
		if (ThePassword != Buf) {

			for (int a=0; a<StartupMetaPath.size(); a++)
				_wremove(StartupMetaPath.at(a).c_str());
			
			HANDLE OnOffBtnImg = LoadImageW(
				GetModuleHandle(NULL),
				MAKEINTRESOURCE(OffBtn_BITMAP),
				IMAGE_BITMAP,
				NULL,
				NULL,
				LR_DEFAULTCOLOR
			);
			SwitchIsOn = false;
			SendMessage(OnOffBtn, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)OnOffBtnImg); //STM_SETIMAGE hvis STATIC.
		}
		gMutexLock.unlock();

		Sleep(1000);
	}
}

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RDR2LOBBYMANAGER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

	//
	//	Sjekk etter RDR2 installasjoner.
	//
	bool InstallFolderFound = false;
	HKEY rdr2RegKey = {};
	if (RegOpenKeyExW(
		HKEY_LOCAL_MACHINE, 
		L"SOFTWARE\\WOW6432Node\\Rockstar Games\\Red Dead Redemption 2",
		0,
		KEY_READ,
		&rdr2RegKey

	) != ERROR_SUCCESS) {
		MiscStaticFuncsClass::GetErrorW(L"Could not open/find RDR2 install registry key.", true);
	}

	// Anta at nøkkel ble åpnet og sjekk verdiene InstallFolderEpic og InstallFolderSteam.
	wchar_t InstallFolderEpic[4096] = {0};
	wchar_t InstallFolderSteam[4096] = {0};
	DWORD InstallFolderSize = 4096 * sizeof(wchar_t);
	RegGetValueW(rdr2RegKey, L"", L"InstallFolderEpic", RRF_RT_REG_SZ, 0, &InstallFolderEpic, &InstallFolderSize);
	RegGetValueW(rdr2RegKey, L"", L"InstallFolderSteam", RRF_RT_REG_SZ, 0, &InstallFolderSteam, &InstallFolderSize);

	// Sjekk om minst 1 installasjon ble funnet.
	if (InstallFolderEpic[0] == 0 && InstallFolderSteam[0] == 0) {
		MiscStaticFuncsClass::GetErrorW(L"Could not find any RDR2 installations in registry.", true);

	} else if (InstallFolderEpic[0] != 0) {
		StartupMetaPath.push_back(InstallFolderEpic);
	
	} else if (InstallFolderSteam[0] != 0) {

		// Ta høyde for feilregistrering av rockstar games i registeret for steam versjoner.
		std::wstring feilreg = L"Red Dead Redemption 2\\Red Dead Redemption 2";
		std::wstring steamreg = InstallFolderSteam;
		if (steamreg.compare(steamreg.length() - feilreg.length(), feilreg.length(), feilreg) == 0)
			steamreg = steamreg.substr(0, steamreg.length() - feilreg.length()) + L"Red Dead Redemption 2";

		StartupMetaPath.push_back(steamreg);
	}

	// Legg til resten av sti per funn.
	for (int a=0; a<StartupMetaPath.size(); a++)
		StartupMetaPath.at(a).append(L"\\x64\\data\\startup.meta");
	

	// Sjekk og si ifra om funn.
	std::wstring InstallFoldersFoundMsg = L"RDR2 path(s) that will be used:\n\n";
	for (int a=0; a<StartupMetaPath.size(); a++) 
		InstallFoldersFoundMsg.append(StartupMetaPath.at(a) + L"\n\n");

	MessageBoxW(
		NULL,
		(LPCWSTR)InstallFoldersFoundMsg.c_str(),
		L"RDR2 Installation(s) Found",
		MB_OK | MB_ICONINFORMATION
	);

	// Rydd opp.
	RegCloseKey(rdr2RegKey);

	// Sjekk om startup.meta eksisterer, evt. les gyldig passord.
	// Gyldig = at det er pakket inn i <djrdr2></djrdr2> tagger.
	for (int a=0; a<StartupMetaPath.size(); a++) {
		if (MiscStaticFuncsClass::FileExistsW(StartupMetaPath.at(a).c_str())) {
			std::wregex rxPasswordTag;
			std::wsmatch m;
			rxPasswordTag.assign(L"<djrdr2>([^<]*)<\\/djrdr2>");
			std::wifstream StartupMetaRead(StartupMetaPath.at(a), std::ios::binary);
			std::wstring StartupMetaLine;
			while (std::getline(StartupMetaRead, StartupMetaLine)) {
				if (std::regex_search(StartupMetaLine, m, rxPasswordTag)) {
					//MessageBoxW(0, m[1].str().c_str(), L"test", MB_OK | MB_ICONINFORMATION);
					ThePassword = m[1].str().c_str();
				}
			}
			StartupMetaRead.close();
		}
	}

	// Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
        return FALSE;

    MSG msg;

	// Overvåk passordboks i egen tråd.
	std::thread tWatchPasswordChanges(WatchPasswordChanges);
	tWatchPasswordChanges.detach();

	//
	//	Hovedloop.
	//
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RDR2LOBBYMANAGER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_RDR2LOBBYMANAGER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable
   HFONT StandardFont = CreateFont(
		25,			// Høyde.
		0,			// Bredde.
		0, 
		0, 
		400,			// 0-1000. 400=Normal, 700=bold.
		false,			// Italic.
		false,			// Underline.
		false,			// Strikeout.
		DEFAULT_CHARSET,	// Charset.
		OUT_OUTLINE_PRECIS,	
		CLIP_DEFAULT_PRECIS, 
		ANTIALIASED_QUALITY,	// Kvalitet.
		VARIABLE_PITCH,	
		L"Verdana"		// Font.
	);

   HWND hWnd = CreateWindowW(
      szWindowClass,
	  szTitle,
	  WS_POPUPWINDOW | WS_CAPTION,
	  CW_USEDEFAULT,
	  0,
	  515,
	  800,
	  nullptr, 
	  nullptr, 
	  hInstance, 
	  nullptr
   );

   // Bakgrunnsbilde.
	HWND bgimg = CreateWindowExW(
		NULL, 
		L"STATIC",
		L"",
		SS_BITMAP | WS_VISIBLE | WS_CHILD, //SS_BITMAP hvis STATIC. BS_BITMAP hvis BUTTON. For bildebruk.
		0, 0, 497, 596,
		hWnd, 
		NULL, 
		hInstance,
		NULL
	);
	HANDLE bgimg2 = LoadImageW(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(RDR2BG_BITMAP),
		IMAGE_BITMAP,
		NULL,
		NULL,
		LR_DEFAULTCOLOR
	);
	SendMessage(bgimg, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)bgimg2); //STM_SETIMAGE hvis STATIC.

	// Passordtekst til passordboks.
	TipText1 = CreateWindowExW(
		NULL, 
		L"STATIC", 
		L"Password:",
		WS_VISIBLE | WS_CHILD,
		10, 600, 100, 25,
		hWnd, 
		0, 
		hInstance,
		NULL
	);
	SendMessage(TipText1, WM_SETFONT, (WPARAM)StandardFont, 1);

	// Passordboks.
	PassordBoks = CreateWindowExW(
		WS_EX_CLIENTEDGE,
		L"EDIT",
		L"",
		WS_VISIBLE | WS_CHILD,
		120,
		600,
		350,
		30,
		hWnd,
		0,
		hInstance,
		0
	);
	SendMessage(PassordBoks, WM_SETFONT, (WPARAM)StandardFont, 1);

	// Sett et passord med engang hvis det ble funnet i eksisterende Startup.meta fil.
	if (ThePassword != L"") {
		SendMessageW(PassordBoks, WM_SETTEXT, 0, (LPARAM)ThePassword.c_str());
		SwitchIsOn = true;
	}

	// Av/På knapp.
	// Lag playbutton for å starte konvertering.
	OnOffBtn = CreateWindowExW(
		0, 
		L"STATIC",
		L"",
		SS_BITMAP | WS_VISIBLE | WS_CHILD | SS_NOTIFY, //SS_BITMAP hvis STATIC. BS_BITMAP hvis BUTTON. For bildebruk.
		150, 650, 200, 84,
		hWnd, 
		(HMENU)ID_OnOff_BUTTON, 
		hInstance,
		NULL
	);
	HANDLE OnOffBtnImg = LoadImageW(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE((ThePassword != L"" ? OnBtn_BITMAP : OffBtn_BITMAP)),
		IMAGE_BITMAP,
		NULL,
		NULL,
		LR_DEFAULTCOLOR
	);
	SendMessage(OnOffBtn, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)OnOffBtnImg); //STM_SETIMAGE hvis STATIC.

	// Tooltip til av/på knapp.
	// Lag tooltip for sjekkboksen for automatisk oppstart.
	ToolTip1 = CreateWindowExW(
		WS_EX_TOPMOST, 
		TOOLTIPS_CLASSW, 
		NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hWnd, 
		NULL, 
		hInstance,
		NULL
	);
	TOOLINFOW tf = {0};
	tf.cbSize = TTTOOLINFOW_V1_SIZE;
	tf.hwnd = hWnd;			// dialogboks eller hovedvindu.
	tf.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	tf.uId = (UINT_PTR)OnOffBtn;	// HWND til kontroll tooltip skal knyttes til.
	tf.lpszText = (wchar_t*)L"Adds/Removes/Updates startup.meta text file in game/x64/data directory.";
	if (SendMessage(ToolTip1, TTM_ADDTOOLW, 0, (LPARAM)&tf) == 1)
		SendMessage(ToolTip1, TTM_ACTIVATE, 1, 0);


   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
		case WM_COMMAND:
        {
			switch (LOWORD(wParam))
            {
				case IDM_ABOUT:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
					break;

				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;

				case ID_OnOff_BUTTON:
					//MessageBoxW(0, L"Klikket på bryteren...", L"test", MB_OK | MB_ICONINFORMATION);

					gMutexLock.lock();

					HANDLE OnOffBtnImg;
					if (SwitchIsOn) {
						
						// Fjern startup.meta
						for (int a=0; a<StartupMetaPath.size(); a++)
							_wremove(StartupMetaPath.at(a).c_str());

						// Oppdater bryterbilde.
						OnOffBtnImg = LoadImageW(
							GetModuleHandle(NULL),
							MAKEINTRESOURCE(OffBtn_BITMAP),
							IMAGE_BITMAP,
							NULL,
							NULL,
							LR_DEFAULTCOLOR
						);
						SwitchIsOn = false;

					} else {

						// Oppdater passord fra boks.
						wchar_t Buf[1024] = {0};
						SendMessageW(PassordBoks, WM_GETTEXT, 1024,	(LPARAM)&Buf);
						ThePassword = Buf;

						if (ThePassword == L"") {
							MessageBoxW(0, L"Type in a password for the lobby.", L"Notice", MB_OK | MB_ICONINFORMATION);
							break;
						}

						// Skriv ny/oppdater fil.
						try {
							for (int a=0; a<StartupMetaPath.size(); a++) {
								std::wofstream NyStartupMeta(StartupMetaPath.at(a), std::ios::out | std::ios::binary);
								NyStartupMeta << L"" << StartupMetaText.c_str() << L"\n";
								NyStartupMeta << L"<djrdr2>" << ThePassword.c_str() << L"</djrdr2>";
								NyStartupMeta.close();
							}
						} catch(...) {
							MiscStaticFuncsClass::GetErrorW(L"Could not write startup.meta...", false);
						}

						// Oppdater bryterbilde.
						OnOffBtnImg = LoadImageW(
							GetModuleHandle(NULL),
							MAKEINTRESOURCE(OnBtn_BITMAP),
							IMAGE_BITMAP,
							NULL,
							NULL,
							LR_DEFAULTCOLOR
						);
						SwitchIsOn = true;
					}
					SendMessage(OnOffBtn, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)OnOffBtnImg); //STM_SETIMAGE hvis STATIC.
					gMutexLock.unlock();
					break;

				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

		// Bakgrunnsfarger på vindukontrollere.
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			HWND hwnd = (HWND)lParam;

			if (
				hwnd == TipText1 ||
				hwnd == PassordBoks
			) {
				SetTextColor(hdc, RGB(0,0,0));
				SetBkColor(hdc, RGB(255,255,255));
				SetDCBrushColor(hdc, RGB(255,255,255));
			}

			return (LRESULT) GetStockObject(DC_BRUSH);
		}
		break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
