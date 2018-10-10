/**
* Orion2 - A MapleStory2 Dynamic Link Library Localhost
*
* @author Eric
*
*/
#include "Orion.h"
#include "OrionHacks.h"
#include "WinSockHook.h"
#include "NMCOHook.h"

void MapleStory2_Bypass(void) {
	while (true) {
		bool bInit = InitializeOrion2();
		if (bInit) {
			printf("Attempting to initialize Orion2... (Success!)\n");
			break;
		}
		printf("Attempting to initialize Orion2... (Failed)\n");
	}
}

int ZExceptionHandler__Report(/*ZExceptionHandler *this,*/ const char *sFormat, ...)
{
	CHAR sBuff[4096] = { 0 };
	va_list arglist;

	va_start(arglist, sFormat);

	int v2 = wvsprintfA((LPSTR)(&sBuff), sFormat, arglist);
		
	OutputDebugStringA(sBuff);
	
	va_end(arglist);

	return v2;
}

LONG WINAPI DumpUnhandledException(LPEXCEPTION_POINTERS pExceptionInfo)
{
	ZExceptionHandler__Report("==== DumpUnhandledException ==============================\r\n");

	auto  v6 = pExceptionInfo->ExceptionRecord;
	ZExceptionHandler__Report("Fault Address : %08X\r\n", v6->ExceptionAddress);
	ZExceptionHandler__Report("Exception code: %08X\r\n", v6->ExceptionCode);

	auto v8 = pExceptionInfo->ContextRecord;
	ZExceptionHandler__Report("Registers:\r\n");
	ZExceptionHandler__Report(
		"EAX:%08X\r\nEBX:%08X\r\nECX:%08X\r\nEDX:%08X\r\nESI:%08X\r\nEDI:%08X\r\n",
		v8->Eax, v8->Ebx, v8->Ecx,
		v8->Edx, v8->Esi, v8->Edi);
	ZExceptionHandler__Report("CS:EIP:%04X:%08X\r\n", v8->SegCs, v8->Eip);
	ZExceptionHandler__Report("SS:ESP:%04X:%08X  EBP:%08X\r\n", v8->SegSs, v8->Esp, v8->Ebp);
	ZExceptionHandler__Report("DS:%04X  ES:%04X  FS:%04X  GS:%04X\r\n", v8->SegDs, v8->SegEs, v8->SegFs, v8->SegGs);
	ZExceptionHandler__Report("Flags:%08X\r\n", v8->EFlags);

	ZExceptionHandler__Report("\r\n");

	return EXCEPTION_EXECUTE_HANDLER;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	/* Initial injection from process attach. */
	if (dwReason == DLL_PROCESS_ATTACH) {

		SetUnhandledExceptionFilter(DumpUnhandledException);

		/* Update the locale to properly decode foreign StringPools. */
		setlocale(LC_ALL, STRING_LOCALE);

#if DEBUG_MODE
			NotifyMessage("Injection successful - haulting process for memory alterations.", Orion::NotifyType::Information);

			AllocConsole();
			SetConsoleTitleA("Orion 2.0 - Make Orion Great Again");
			AttachConsole(GetCurrentProcessId());
			freopen("CON", "w", stdout);
			FreeConsole();
#endif

		/* Initiate the winsock hook for socket spoofing and redirection. */
		if (!Hook_WSPStartup(true)) {
			NotifyMessage("Failed to hook WSPStartup", Orion::NotifyType::Error);
			return FALSE;
		}

		/* Initiate the NMCO hook to fix our login passport handling. */
		if (!NMCOHook_Init()) {
			NotifyMessage("Failed to hook CallNMFunc", Orion::NotifyType::Error);
			return FALSE;
		}

		/* Initiate the CreateWindowExA hook to customize the main window. */
		if (!Hook_CreateWindowExA(true)) {
			NotifyMessage("Failed to hook CreateWindowExA", Orion::NotifyType::Error);
			return FALSE;
		}

		/* Hook GetCurrentDirectoryA and begin to rape the client. */
		if (!Hook_GetCurrentDirectoryA(true)) {
			NotifyMessage("Failed to hook GetCurrentDirectoryA", Orion::NotifyType::Error);
			return FALSE;
		}

		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MapleStory2_Bypass, 0, 0, 0);

		DisableThreadLibraryCalls(hModule);
	}

	return TRUE;
}
