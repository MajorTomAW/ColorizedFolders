// Copyright © 2025 MajorT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Themes/ColorizedFoldersTheme.h"
#include "UObject/Object.h"

#include "ColorizedFoldersSettings.generated.h"

/** Settings for the folder color schemes. */
UCLASS(Config=EditorPerProjectUserSettings, DefaultConfig, DisplayName="Colorized Folders Settings")
class UColorizedFoldersSettings : public UObject
{
	GENERATED_BODY()

public:
	UColorizedFoldersSettings();
	static const UColorizedFoldersSettings* Get();
	static UColorizedFoldersSettings* GetMutable();

	/** Initializes the settings and applies the current theme. */
	void Init();

	DECLARE_EVENT(UColorizedFoldersSettings, FOnRequestUpdateFolders);
	FOnRequestUpdateFolders OnRequestUpdateFolders;

	bool IsLiveUpdateFoldersEnabled() const
	{
		return bLiveUpdateFolders;
	}

protected:
	//~ Begin UObject Interface
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

public:
	/** The currently applied theme. */
	UPROPERTY(Config)
	FGuid CurrentAppliedTheme;

	/**
	 * Determines whether folders should update immediately after being created/renamed/deleted or if the settings have changed.
	 * This is enabled by default as it provides a more responsive experience.
	 *
	 * But if you have a large project with many folders, you may want to disable this to avoid performance issues.
	 * Taking effect after restarting the editor.
	 */
	UPROPERTY(Config, EditDefaultsOnly, Category = ContentBrowser)
	bool bLiveUpdateFolders = true;

	/**
	 * List of folders to ignore.
	 */
	UPROPERTY(VisibleDefaultsOnly, Category = ContentBrowser, meta = (RelativeToGameContentDir, LongPackageName))
	TArray<FDirectoryPath> FolderBlacklist;

	/** List of all known folder color themes. */
	TArray<FColorizedFolderTheme> FolderColorThemes;
};
