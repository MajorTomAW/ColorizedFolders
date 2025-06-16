// Copyright Epic Games, Inc. All Rights Reserved.

#include "ColorizedFoldersSettings.h"
#include "ColorizedFoldersUtils.h"
#include "ContentBrowserDataSubsystem.h"
#include "ContentBrowserItemData.h"
#include "IContentBrowserDataModule.h"
#include "ISettingsModule.h"
#include "Customization/ColorizedFoldersDetailCustomization.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Themes/ColorizedFoldersManager.h"

#define LOCTEXT_NAMESPACE "ColorizedFolders"

class FColorizedFoldersModule final : public IModuleInterface
{
	using FThisModule = FColorizedFoldersModule;
	
public:
	//~Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~End IModuleInterface

private:
	void OnPostEngineInit();
	
	void StartColorizingFolders();
	void RequestFolderColorUpdate();

	void OnItemDataUpdated(TArrayView<const FContentBrowserItemDataUpdate> DataUpdates);
	void OnRequestUpdate(const FGuid& Id);
};
IMPLEMENT_MODULE(FColorizedFoldersModule, ColorizedFolders)

void FColorizedFoldersModule::StartupModule()
{
	ISettingsModule& SettingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>("Settings");
	SettingsModule.RegisterSettings("Editor", "General", "Colorized Folders",
		LOCTEXT("ColorizedFoldersSettingsName", "Colorized Folders"),
		LOCTEXT("ColorizedFoldersSettingsDescription", "Configure the color schemes for folders in the Content Browser."),
		UColorizedFoldersSettings::GetMutable()
	);

	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FThisModule::OnPostEngineInit);
}

void FColorizedFoldersModule::ShutdownModule()
{
	ISettingsModule& SettingsModule = FModuleManager::GetModuleChecked<ISettingsModule>("Settings");
	SettingsModule.UnregisterSettings("Editor", "General", "Colorized Folders");

	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	if (const IContentBrowserDataModule* ContentBrowser = IContentBrowserDataModule::GetPtr())
	{
		if (UContentBrowserDataSubsystem* ContentBrowserSub = ContentBrowser->GetSubsystem())
		{
			ContentBrowserSub->OnItemDataUpdated().RemoveAll(this);
		}
	}
}


void FColorizedFoldersModule::OnPostEngineInit()
{
	StartColorizingFolders();

#if ALLOW_THEMES
	UColorizedFoldersManager::Get().LoadThemes();
#endif

	UColorizedFoldersSettings* Settings = UColorizedFoldersSettings::GetMutable();
	Settings->Init();
	
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.RegisterCustomPropertyTypeLayout("ColorizedFolderColorSchemeList",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FColorizedFoldersPropertyCustomization::MakeInstance));
	PropertyEditorModule.RegisterCustomClassLayout("ColorizedFoldersSettings",
		FOnGetDetailCustomizationInstance::CreateStatic(&FColorizedFoldersDetailCustomization::MakeInstance));

	PropertyEditorModule.NotifyCustomizationModuleChanged();

	// Request initial update
	RequestFolderColorUpdate();
}

void FColorizedFoldersModule::StartColorizingFolders()
{
	UColorizedFoldersManager::Get().OnThemeChanged().AddRaw(this, &FThisModule::OnRequestUpdate);

	// Assign a delegate that triggers whenever a new item is added to the content browser.
	// I honestly don't know if this is the right way to do it, but it works.
	// Also, I didn't experience any performance issues yet, so I guess it's fine :p
	if (const IContentBrowserDataModule* ContentBrowser = IContentBrowserDataModule::GetPtr())
	{
		ContentBrowser->GetSubsystem()->OnItemDataUpdated().AddRaw(this, &FThisModule::OnItemDataUpdated);
	}
}

void FColorizedFoldersModule::RequestFolderColorUpdate()
{
	using namespace UE::ColorizedFolders;
	
	IFileManager& FileManager = IFileManager::Get();
	IPluginManager& PluginManager = IPluginManager::Get();
	const UColorizedFoldersManager& ThemeManager = UColorizedFoldersManager::Get();

	TArray<FString> Dirs;
	FColorizedFoldersDirIterator DirIterator(Dirs);

	// Scan the game content directory
	DirIterator.SetRootName(TEXT("Game"));
	FileManager.IterateDirectoryRecursively(*FPaths::ProjectContentDir(), DirIterator);

	// Scan the plugin content directories
	TArray<TSharedRef<IPlugin>> DiscoveredPlugins = PluginManager.GetDiscoveredPlugins();
	for (TSharedRef Plugin : DiscoveredPlugins)
	{
		if (ShouldIterateThroughPlugin(Plugin))
		{
			// Update virtual-path so we can remove it
			DirIterator.SetVirtualPath(Plugin->GetDescriptor().EditorCustomVirtualPath);
			DirIterator.SetRootName(Plugin->GetName());
			
			FileManager.IterateDirectoryRecursively(*Plugin->GetContentDir(), DirIterator);
		}
	}

	// Colorize the folders
	TArray<FString> Tracking = Dirs;
	for (int i = 0; i < NUM_FOLDER_SCHEMES; ++i)
	{
		ColorizeDirsAccordingToScheme(Dirs, ThemeManager.GetScheme(i), Tracking);
	}

	// Clear the tracking array
	/*if (UEditorsFavouriteSettings::Get()->bClearNotListedFolderColors)
	{
		for (const FString& Dir : Tracking)
		{
			AssetViewUtils::SetPathColor(Dir, TOptional<FLinearColor>());
		}	
	}*/
}

void FColorizedFoldersModule::OnItemDataUpdated(TArrayView<const FContentBrowserItemDataUpdate> DataUpdates)
{
	// Maybe we shouldn't even bind to this event if we don't want to live update folders.
	// But that would mean we would have to restart the editor to apply the settings.
	if (!UColorizedFoldersSettings::Get()->IsLiveUpdateFoldersEnabled())
	{
		return;
	}

	// Request a folder update if any of the items are folders
	for (const auto& Data : DataUpdates)
	{
		if (Data.GetItemData().IsFolder())
		{
			RequestFolderColorUpdate();
			return;
		}
	}
}

void FColorizedFoldersModule::OnRequestUpdate(const FGuid& Id)
{
	// Maybe we shouldn't even bind to this event if we don't want to live update folders
	// But that would mean we would have to restart the editor to apply the settings
	if (!UColorizedFoldersSettings::Get()->IsLiveUpdateFoldersEnabled())
	{
		return;
	}
	
	RequestFolderColorUpdate();
}

#undef LOCTEXT_NAMESPACE