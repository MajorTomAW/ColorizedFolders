// Copyright © 2025 MajorT. All Rights Reserved.


#include "ColorizedFoldersSettings.h"

#include "Themes/ColorizedFoldersManager.h"

UColorizedFoldersSettings::UColorizedFoldersSettings()
{
}

const UColorizedFoldersSettings* UColorizedFoldersSettings::Get()
{
	return GetDefault<UColorizedFoldersSettings>();
}

UColorizedFoldersSettings* UColorizedFoldersSettings::GetMutable()
{
	return GetMutableDefault<UColorizedFoldersSettings>();
}

void UColorizedFoldersSettings::Init()
{
	if (CurrentAppliedTheme.IsValid())
	{
		UColorizedFoldersManager::Get().ApplyTheme(CurrentAppliedTheme);
	}
	else
	{
		CurrentAppliedTheme = UColorizedFoldersManager::GetCurrentThemeId();
		SaveConfig();
	}

	UColorizedFoldersManager::Get().ApplyTheme(UColorizedFoldersManager::Get().GetCurrentTheme().Id);
}

void UColorizedFoldersSettings::PostLoad()
{
	UObject::PostLoad();
}

#if WITH_EDITOR
void UColorizedFoldersSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property == nullptr)
	{
		return;
	}

	// Helper lambda to check if a property has the LiveUpdate metadata.
	auto IsLiveUpdate = [PropertyChangedEvent]()
	{
		return PropertyChangedEvent.Property->HasMetaData("LiveUpdate") ||
			PropertyChangedEvent.Property->GetOwnerProperty()->HasMetaData("LiveUpdate") ||
			PropertyChangedEvent.MemberProperty->HasMetaData("LiveUpdate");
	};

	// Request a live update if the property has the LiveUpdate metadata.
	if (IsLiveUpdate() && IsLiveUpdateFoldersEnabled())
	{
		OnRequestUpdateFolders.Broadcast();
	}
}
#endif