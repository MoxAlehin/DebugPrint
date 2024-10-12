// Copyright MoxAlehin. All Rights Reserved.

#include "DebugPrint.h"
#include "DebugPrintDeveloperSettings.h"
#include "ISettingsModule.h"
#define LOCTEXT_NAMESPACE "FDebugPrintModule"

class UDebugPrintDeveloperSettings;
class ISettingsModule;

void FDebugPrintModule::StartupModule()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
			"Editor",
			"Plugins",
			"Debug Print",
			NSLOCTEXT("DebugPrint", "Debug Print Plugin Settings", "Debug Print"),
			NSLOCTEXT("DebugPrint", "Debug Print Plugin Settings Description", "Debug Print Settings"),
			GetMutableDefault<UDebugPrintDeveloperSettings>()  // Указатель на класс настроек
		);
	}
}

void FDebugPrintModule::ShutdownModule()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Editor", "Plugins", "Debug Print");
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDebugPrintModule, DebugPrint)