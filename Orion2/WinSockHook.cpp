/**
* Orion2 - A MapleStory2 Dynamic Link Library Localhost
*
* @author Benny
* @author Dan
* @author Eric
*
*/
#include "WinSockHook.h"
#include "NMCOHook.h"

/* WSPConnect */
static LPWSPCONNECT _WSPConnect = NULL;
/* WSPGetPeerName */
static LPWSPGETPEERNAME _WSPGetPeerName = NULL;
/* WSPStartup */
static LPWSPSTARTUP _WSPStartup = NULL;

/* The original socket host address */
DWORD dwHostAddress = 0;
/* The re-routed socket host address */
DWORD dwRouteAddress = 0;

/* Hooks the Winsock Service Provider's Connect function to redirect the host to a new socket */
int WINAPI WSPConnect_Hook(SOCKET s, sockaddr* name, int namelen, LPWSABUF lpCallerData, LPWSABUF lpCalleeData, LPQOS lpSQOS, LPQOS lpGQOS, LPINT lpErrno) {
	/* Retrieve a string buffer of the current socket address (IP) */
	char pBuff[50];
	DWORD dwStringLength = 50;
	WSAAddressToStringA(name, namelen, NULL, pBuff, &dwStringLength);

	sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(name);
	unsigned short pPort = addr->sin_port;

	VM_START
		if (strstr(pBuff, NEXON_IP_NA) || strstr(pBuff, NEXON_IP_SA) || strstr(pBuff, NEXON_IP_EU)) {
			/* Initialize the re-reoute socket address to redirect to */
			dwRouteAddress = inet_addr(CLIENT_IP);

#if DEBUG_MODE
			printf("[WSPConnect_Hook] Patching to new address: %s\n", CLIENT_IP);
#endif

			/* Copy the original host address and back it up */
			memcpy(&dwHostAddress, &addr->sin_addr, sizeof(DWORD));
			/* Update the host address to the route address */
			memcpy(&addr->sin_addr, &dwRouteAddress, sizeof(DWORD));
		}
		else
		{
#if DEBUG_MODE
			printf("[WSPConnect_Hook] Connecting to socket address: %s\n", pBuff);
#endif
		}
	VM_END
		return _WSPConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);
}

/* Hooks the Winsock Service Provider's GetPeerName function to pretend to be connected to the host */
int WINAPI WSPGetPeerName_Hook(SOCKET s, sockaddr* name, LPINT namelen, LPINT lpErrno) {
	int nResult = _WSPGetPeerName(s, name, namelen, lpErrno);

	if (nResult == 0) {
		sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(name);

		/* Check if the returned address is the routed address */
		if (addr->sin_addr.S_un.S_addr == dwRouteAddress) {
			/* Return the socket address back to the host address */
			memcpy(&addr->sin_addr, &dwHostAddress, sizeof(DWORD));
		}
	}

	return nResult;
}

/* Hooks the Winsock Service Provider's Startup function to initiate the SPI and spoof the socket */
bool Hook_WSPStartup(bool bEnable) {
	/* Initialize the WSPStartup module and jump to hook if successful */
	if (!_WSPStartup) {
		VM_START
		HMODULE hModule = LoadLibraryA("MSWSOCK");

		if (hModule) {
			_WSPStartup = reinterpret_cast<LPWSPSTARTUP>(GetProcAddress(hModule, "WSPStartup"));

			if (_WSPStartup) {
				goto Hook;
			}
		}
		VM_END
		return false;
	}

Hook:
	LPWSPSTARTUP WSPStartup_Hook = [](WORD wVersionRequested, LPWSPDATA lpWSPData, LPWSAPROTOCOL_INFOW lpProtocolInfo, WSPUPCALLTABLE UpcallTable, LPWSPPROC_TABLE lpProcTable) -> int {
		VM_START
		int nResult = _WSPStartup(wVersionRequested, lpWSPData, lpProtocolInfo, UpcallTable, lpProcTable);

		if (nResult == 0) {
			/* Redirect WSPConnect to our hook */
			_WSPConnect = lpProcTable->lpWSPConnect;
			lpProcTable->lpWSPConnect = reinterpret_cast<LPWSPCONNECT>(WSPConnect_Hook);

			/* Redirect WSPGetPeerName to our hook */
			_WSPGetPeerName = lpProcTable->lpWSPGetPeerName;
			lpProcTable->lpWSPGetPeerName = WSPGetPeerName_Hook;
		}
		VM_END
		return nResult;
	};

	/* Enable the WSPStartup hook */
	return SetHook(bEnable, reinterpret_cast<void**>(&_WSPStartup), WSPStartup_Hook);
}