#include "stdafx.h"
//#define _SKSE64_
#define _F4SE_

////// F4SE //////
#ifdef _F4SE_
#include "common\common\IPrefix.h"
#include "f4se\f4se\PluginAPI.h"
#include "f4se\f4se_Common\f4se_version.h"

// For pre-Next Gen
extern "C" __declspec(dllexport) bool F4SEPlugin_Query(const F4SEInterface * F4SE, PluginInfo * info)
{
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "Achievements Mods Enabler Loader (F4SE, Pre-NG)";
	info->version = 1;

	return TRUE;
}

// For Next Gen
extern "C" {
	// Modified example
	__declspec(dllexport) F4SEPluginVersionData F4SEPlugin_Version =
	{
		F4SEPluginVersionData::kVersion,               // dataVersion
		1,                                             // pluginVersion
		"Achievements Mods Enabler Loader (F4SE, NG)", // plugin name
		"Sumwunn @ GitHub.com",                        // plugin author name

		F4SEPluginVersionData::kAddressIndependence_Signatures,  // addressIndependence
		F4SEPluginVersionData::kStructureIndependence_NoStructs, // structureIndependence

		{ 0 }, // compatibleVersions

		0, // seVersionRequired
	};
};

extern "C" __declspec(dllexport) bool F4SEPlugin_Load(const F4SEInterface * f4se)
{
	// Get dll path.
	TCHAR DllFilePath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, DllFilePath);
	_tcscat_s(DllFilePath, MAX_PATH, L"\\Data\\Plugins\\Sumwunn\\AchievementsModsEnabler.dll");

	// Return if already loaded.
	if (GetModuleHandle(DllFilePath) != 0) 
		return FALSE;

	// Load it.
	HMODULE hModule = LoadLibrary(DllFilePath);

	// Prevent crash.
	if (!hModule)
		return FALSE;

	FARPROC Function01 = GetProcAddress(hModule, "SetF4SEMode");
	FARPROC Function02 = GetProcAddress(hModule, "Setup");

	// Call stuff.
	Function01();
	Function02();

	return TRUE;
}

#endif

////// SKSE64 //////
#ifdef _SKSE64_
#include "common\common\IPrefix.h"
#include "SKSE64\SKSE64\PluginAPI.h"
#include "skse64\skse64_common\skse_version.h"

// For pre-AE
extern "C" __declspec(dllexport) bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
{
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "Achievements Mods Enabler Loader";
	info->version = 1;

	return TRUE;
}

// For AE
extern "C" {
	// Modified example
	__declspec(dllexport) SKSEPluginVersionData SKSEPlugin_Version =
	{
		SKSEPluginVersionData::kVersion,

		1, // version number
		"Achievements Mods Enabler Loader", // plugin name

		"Sumwunn @ GitHub.com", // plugin author name
		"", // support address

		SKSEPluginVersionData::kVersionIndependentEx_NoStructUse, // versionIndependenceEx
		SKSEPluginVersionData::kVersionIndependent_Signatures, // versionIndependence

		{ 0 }, // compatibleVersions

		0, // seVersionRequired; 0: works with any version of the script extender. you probably do not need to put anything here
	};
};

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSEInterface * skse)
{
	// Get dll path.
	TCHAR DllFilePath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, DllFilePath);
	_tcscat_s(DllFilePath, MAX_PATH, L"\\Data\\Plugins\\Sumwunn\\AchievementsModsEnabler.dll");

	// Return if already loaded.
	if (GetModuleHandle(DllFilePath) != 0) 
		return FALSE;

	// Load it.
	HMODULE hModule = LoadLibrary(DllFilePath);
	// Prevent crash.
	if (!hModule)
		return FALSE;

	FARPROC Function01 = GetProcAddress(hModule, "SetSKSEMode");
	FARPROC Function02 = GetProcAddress(hModule, "Setup");

	// Call stuff.
	Function01();
	Function02();

	return TRUE;
}

#endif