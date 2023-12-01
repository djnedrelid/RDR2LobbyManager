#pragma once

//
// Multiverktøy-klasse for diverse nyttige statiske funksjoner.
//

class MiscStaticFuncsClass
{
	private:
	public:

	//
	// lpszFunction = Manuell feilbeskjed.
	// HandleExit = Exit(EXIT_FAILURE).
	//
	static void GetErrorW(std::wstring lpszFunction, bool HandleExit)
	{
		unsigned long err = GetLastError();
		std::wstring lpDisplayBuf;
		wchar_t* lpMsgBuf;

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpMsgBuf,
			0,
			NULL
		);

		lpDisplayBuf.append(lpszFunction + L"\n\n");
		lpDisplayBuf.append(L"Details: (" + std::to_wstring(err) + L") ");
		lpDisplayBuf.append(lpMsgBuf);

		MessageBoxW(
			NULL,
			(LPCWSTR)lpDisplayBuf.c_str(),
			L"Critical Message",
			MB_OK | MB_ICONINFORMATION
		);

		if (HandleExit) {
			// EXIT_FAILURE = 1. 
			// EXIT_SUCCESS = 0.
			exit(EXIT_FAILURE);
		}
	}

	//
	// Returnerer true hvis fil eksisterer, false hvis ikke.
	//
	static bool FileExistsW(const wchar_t* f) 
	{
		// Er det en enkeltfil?
		if (PathIsDirectoryW(f) == 0) {
		
			// Sjekk om filen faktisk eksisterer.
			std::wifstream filtest(f);

			if (!filtest.good()) {
				return false;
			} else {
				filtest.close();
				return true;
			}
		} else {
			return false;
		}
	}
};
