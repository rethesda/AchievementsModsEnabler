/*                             The MIT License (MIT)

Copyright (C) 2024 Sumwunn @ GitHub.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/

#include "stdafx.h"
#include <windows.h>
#include <fstream>

// Defined functions
// ASM
extern "C" void* apiHook_Older();
extern "C" void* apiHook_Newer();

extern "C" void* BinSearch(void* Search, int SearchLength, unsigned char* Bytes, int BytesLength, int AddMod, int SubMod);
extern "C" void* HookWrite(void*, void*);

// Because GetTextSectionData has multiple return types
extern "C" void* GetTEXTSectionAddr(HMODULE Module, int DataType);
extern "C" int GetTEXTSectionSize(HMODULE Module, int DataType);

// C++
bool BinPatch(HMODULE hModule, unsigned char* BytesToFind, int BytesToFindSize, int AddressModifierAdd, int AddressModifierSub, int apiHookType);

// The address we get from BinSearch
void* PatchAddress = NULL;
unsigned char OrigBytes[20] = { 0 };

// Data
// ScriptExtenderType
// 0 = None
// 1 = Fallout 4
// 2 = Skyrim SE
int ScriptExtenderType = 0;

// Return values
// 0 = Patching failed, bytes not found
// 1 = Patching successful, bytes found
// -1 = Process is NOT expected target
// -2 = Log file creation failed

extern "C" __declspec(dllexport) int Setup()
{
	////////////////////////////////
	////////// FALLOUT 4 //////////
	//////////////////////////////

	LPCTSTR ExpectedProcess01 = L"Fallout4.exe";
	// These bytes will land us just beneath where the achivements mods disabler code is at
	// I should move these into an array..
	unsigned char BytesToFind01_01[] = { 0xC3, 0x40, 0x32, 0xFF, 0x48, 0x89, 0x5C, 0x24, 0x40, 0x48, 0x89, 0x6C, 0x24, 0x48 }; // Fallout 4 pre-Creators Club update(pre - v1.10)
	unsigned char BytesToFind01_02[] = { 0xC3, 0xC6, 0x44, 0x24, 0x38, 0x00, 0x48, 0x8D, 0x44, 0x24, 0x38, 0x48, 0x89, 0x5C, 0x24, 0x20 }; // Fallout 4 Creators Club update (v1.10)
	unsigned char BytesToFind01_03[] = { 0x48, 0x83, 0xEC, 0x28, 0xC6, 0x44, 0x24, 0x38, 0x00, 0x84, 0xD2, 0x74, 0x1C, 0x48 }; /// Fallout 4 Next Gen (v1.10.980)
	// VR.
	LPCTSTR ExpectedProcess01_VR = L"Fallout4VR.exe";

	// We need to go back X bytes so we land at the right address
	int AddressModifierSub01_01 = 0x29; // Fallout 4 pre-Creators Club update (pre-v1.10)
	int AddressModifierSub01_02 = 0x28; // Fallout 4 Creators Club update (v1.10)
	int AddressModifierSub01_03 = 0x0; // Fallout 4 Next Gen (v1.10.980)

	////////////////////////////////
	////////// SKYRIM SE //////////
	//////////////////////////////

	LPCTSTR ExpectedProcess02 = L"SkyrimSE.exe";
	// These bytes will land us just beneath where the achivements mods disabler code is at
	unsigned char BytesToFind02_01[] = { 0xC3, 0x48, 0x89, 0x5C, 0x24, 0x40, 0x48, 0x89, 0x6C, 0x24, 0x48, 0x8B, 0xA9, 0x70, 0x0D, 0x00, 0x00 }; // Skyrim SE v1.1
	unsigned char BytesToFind02_02[] = { 0xC3, 0x40, 0x32, 0xFF, 0x48, 0x89, 0x5C, 0x24, 0x40, 0x48, 0x89, 0x6C, 0x24, 0x48 }; // Skyrim SE v1.2
	unsigned char BytesToFind02_03[] = { 0xC3, 0xC6, 0x44, 0x24, 0x38, 0x00, 0x48, 0x8D, 0x44, 0x24, 0x38, 0x48, 0x89, 0x5C, 0x24, 0x20 }; // Skyrim SE Creators Club update (v1.5.3.0+)
	unsigned char BytesToFind02_04[] = { 0x48, 0x83, 0xEC, 0x28, 0xC6, 0x44, 0x24, 0x38, 0x00, 0x84, 0xD2, 0x74, 0x1C, 0x48 }; // Skyrim AE update (v1.6.318.0+)
	unsigned char BytesToFind02_05[] = { 0x0F, 0xB6, 0x44, 0x24, 0x38, 0x48, 0x8B, 0x5C, 0x24, 0x20, 0x48, 0x83, 0xC4, 0x28, 0xC3 }; // Skyrim AE update (v1.6.318.0+) alternative

	// VR.
	LPCTSTR ExpectedProcess02_VR = L"SkyrimVR.exe";

	// We need to go back X bytes so we land at the right address
	int AddressModifierSub02_01 = 0x35; // Skyrim SE v1.1
	int AddressModifierSub02_02 = 0x30; // Skyrim SE v1.2+
	int AddressModifierSub02_03 = 0x28; // Skyrim SE Creators Club update (v1.5.3.0+)
	int AddressModifierSub02_04 = 0x00; // Skyrim AE update (v1.6.318.0+)
	int AddressModifierSub02_05 = 0x5F; // Skyrim AE update (v1.6.318.0+) alternative

	/////////////////////////////
	////////// CODE ////////////
	///////////////////////////

	//////// Setup Part 1 - Config ////////

	TCHAR ConfigFilePath[MAX_PATH];
	bool enableLogging = true;
	// 0 = Disable
	// 1 = Enable
	bool ignoreProcessName = false;
	// 0 = Expected process name detection enabled
	// 1 = Ignore Fallout4.exe name detection. Allows mod to work regardless of EXE name
	// 2 = Ignore SkyrimSE.exe name detection. Allows mod to work regardless of EXE name

	// Get config path
	GetCurrentDirectory(MAX_PATH, ConfigFilePath);
	// Dll loader path
	_tcscat_s(ConfigFilePath, MAX_PATH, L"\\Data\\Plugins\\Sumwunn\\AchievementsModsEnabler.ini");
	// Get config settings
	enableLogging = GetPrivateProfileInt(L"General", L"bEnableLogging", 1, ConfigFilePath);
	ignoreProcessName = GetPrivateProfileInt(L"General", L"bIgnoreExpectedProcessName", 0, ConfigFilePath);

	// Misc
	HMODULE hModule = NULL;
	std::ofstream LogFileHandle;

	//////// Setup Part 2 - Addresses & Logging ////////
	if (enableLogging) 
	{
		// Open up fresh log file
		LogFileHandle.open(L"Data\\Plugins\\Sumwunn\\AchievementsModsEnabler.log");

		// Log file creation failed
		if (!LogFileHandle)
			return -2;
	}

	////////////////////////////////
	////////// FALLOUT 4 //////////
	//////////////////////////////
	if (ignoreProcessName || ScriptExtenderType == 1)
	{
		hModule = GetModuleHandle(NULL);
	}
	else 
	{
		hModule = GetModuleHandle(ExpectedProcess01);

		if (hModule == NULL)
			hModule = GetModuleHandle(ExpectedProcess01_VR);
	}
	if (hModule != NULL)
	{
		// Find bytes and patch them
		if (!BinPatch(hModule, BytesToFind01_01, sizeof BytesToFind01_01, NULL, AddressModifierSub01_01, 1) &&
			!BinPatch(hModule, BytesToFind01_02, sizeof BytesToFind01_02, NULL, AddressModifierSub01_02, 2) &&
			!BinPatch(hModule, BytesToFind01_03, sizeof BytesToFind01_03, NULL, AddressModifierSub01_03, 2))
		{
			if (enableLogging)
			{
				// Bytes not found
				// Log message
				LogFileHandle << "NO" << std::endl;
				// Cleanup
				LogFileHandle.close();
			}

			return 0;
		}
		else
		{
			if (enableLogging)
			{
				// Bytes found
				// Log message
				LogFileHandle << "YES" << std::endl;
				// Cleanup
				LogFileHandle.close();
			}

			return 1;
		}
	}
	
	////////////////////////////////
	////////// SKYRIM SE //////////
	//////////////////////////////
	if (ignoreProcessName || ScriptExtenderType == 2)
		hModule = GetModuleHandle(NULL);
	else
	{
		hModule = GetModuleHandle(ExpectedProcess02);

		if (hModule == NULL) 
			hModule = GetModuleHandle(ExpectedProcess02_VR);
	}
	if (hModule != NULL)
	{
		// Find bytes and patch them
		if (!BinPatch(hModule, BytesToFind02_01, sizeof BytesToFind02_01, NULL, AddressModifierSub02_01, 1) && 
			!BinPatch(hModule, BytesToFind02_02, sizeof BytesToFind02_02, NULL, AddressModifierSub02_02, 1) && 
			!BinPatch(hModule, BytesToFind02_03, sizeof BytesToFind02_03, NULL, AddressModifierSub02_03, 2) && 
			!BinPatch(hModule, BytesToFind02_04, sizeof BytesToFind02_04, NULL, AddressModifierSub02_04, 2) &&
			!BinPatch(hModule, BytesToFind02_05, sizeof BytesToFind02_05, NULL, AddressModifierSub02_05, 2))
		{
			if (enableLogging)
			{
				// Bytes not found
				// Log message
				LogFileHandle << "NO" << std::endl;
				// Cleanup
				LogFileHandle.close();
			}

			return 0;
		}
		else
		{
			if (enableLogging)
			{
				// Bytes found
				// Log message
				LogFileHandle << "YES" << std::endl;
				// Cleanup
				LogFileHandle.close();
			}

			return 1;
		}
	}

	if (enableLogging)
	{
		// Process not found
		// Cleanup
		// Log message
		LogFileHandle << "Fallout4.exe & SkyrimSE.exe not detected." << std::endl;
		LogFileHandle.close();
	}

	return -1;
}

// apiHookType: 1 = Older
// apiHookType: 2 = Newer
bool BinPatch(HMODULE hModule, unsigned char* BytesToFind, int BytesToFindSize, int AddressModifierAdd, int AddressModifierSub, int apiHookType) // BinSearch + MemCpy patching
{
	// The address we get from GetTextSectionAddr
	void* SearchAddress = NULL;
	// The size too
	int SearchSize = NULL;

	// Get size and address of ExpectedProcess's .text section
	SearchSize = GetTEXTSectionSize(hModule, 1);
	SearchAddress = GetTEXTSectionAddr(hModule, 2);
	// Get address and patch it
	PatchAddress = BinSearch(SearchAddress, SearchSize, BytesToFind, BytesToFindSize, AddressModifierAdd, AddressModifierSub);

	// Bytes not found
	if (PatchAddress == NULL)
		return false;
	// Bytes found
	else
	{
		// Backup patched bytes
		memcpy(OrigBytes, PatchAddress, sizeof OrigBytes);

		// Hook Type
		switch (apiHookType)
		{
		case 1:
		{
			HookWrite(PatchAddress, apiHook_Older);
		}
		break;
		case 2:
		{
			HookWrite(PatchAddress, apiHook_Newer);
		}
		break;
		}

		return true;
	}

	return false;
}

// For DllMain().. but not needed (I guess)
void HookRestore()
{
	DWORD lpflOldProtect = NULL;

	VirtualProtect(PatchAddress, sizeof OrigBytes, PAGE_EXECUTE_READWRITE, &lpflOldProtect);
	memcpy(PatchAddress, OrigBytes, sizeof OrigBytes);
	VirtualProtect(PatchAddress, sizeof OrigBytes, lpflOldProtect, &lpflOldProtect);

	return;
}

////// Script Extender //////
extern "C" __declspec(dllexport) void SetF4SEMode()
{
	ScriptExtenderType = 1;

	return;
}

extern "C" __declspec(dllexport) void SetSKSEMode()
{
	ScriptExtenderType = 2;

	return;
}
