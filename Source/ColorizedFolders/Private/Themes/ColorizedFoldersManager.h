// Copyright © 2025 MajorT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ColorizedFoldersTheme.h"
#include "UObject/Object.h"

#include "ColorizedFoldersManager.generated.h"

UCLASS(Config=EditorSettings, MinimalAPI)
class UColorizedFoldersManager : public UObject
{
	GENERATED_BODY()
	friend class UColorizedFoldersSettings;
	
public:
	UColorizedFoldersManager();

	/** Initializes the manager with the default theme. */
	void InitDefaults();

	/** Sets a default theme to be used as a fallback if no theme is loaded. */
	void SetDefaultTheme(int32 Id, FColorizedFolderColorScheme InScheme);

	static UColorizedFoldersManager& Get()
	{
		return *GetMutableDefault<UColorizedFoldersManager>();
	}

	static const FGuid& GetCurrentThemeId()
	{
		return Get().CurrentThemeId;
	}

	static const FColorizedFolderColorScheme& GetScheme(int32 Index)
	{
		return Get().ActiveSchemes.Schemes[Index];
	}

	void SetCurrentThemeId_Direct(FGuid NewThemeId)
	{
		CurrentThemeId = NewThemeId;
	}

#if ALLOW_THEMES
	DECLARE_EVENT_OneParam(UColorizedFoldersSettings, FOnThemeChanged, const FGuid& /*NewThemeId*/)
	FOnThemeChanged& OnThemeChanged() { return ThemeChangedEvent; }

	/** Broadcasts whenever the folder color theme changes. */
	FOnThemeChanged ThemeChangedEvent;

	FColorizedFolderTheme DefaultTheme;
	TArray<FColorizedFolderTheme> LoadedThemes;
	FColorizedFolderColorScheme DefaultColorSchemes[NUM_FOLDER_SCHEMES];


	/** Sets a custom display name for a folder color scheme. */
	void SetSchemeDisplayName(const int32 Id, const FText& DisplayName)
	{
		ActiveSchemes.DisplayNames[Id] = DisplayName;
	}
	
	/** Gets the display name for a folder color scheme. */
	FText GetSchemeDisplayName(const int32 Id) const
	{
		return ActiveSchemes.DisplayNames[Id];
	}

	/** Loads all known themes from engine, project, and user directories */
	void LoadThemes();

	/** Saves the current theme to a file */
	void SaveCurrentThemeAs(const FString& InFilename);

	/** Applies a theme as the active theme */
	void ApplyTheme(FGuid ThemeId);

	/** Applies the default theme as the active theme */
	void ApplyDefaultTheme();

	/** Returns true if the active theme is an engine-specific theme */
	bool IsEngineTheme() const;

	/** Returns true if the active theme is a project-specific theme */
	bool IsProjectTheme() const;

	/** Removes a theme from the list of known themes */
	void RemoveTheme(FGuid ThemeId);

	/** Duplicates the active theme */
	FGuid DuplicateActiveTheme();

	/** Sets the display name for the current theme */
	void SetCurrentThemeDisplayName(FText NewDisplayName);

	/** Gets the current theme */
	const FColorizedFolderTheme& GetCurrentTheme() const
	{
		return *LoadedThemes.FindByKey(CurrentThemeId);
	}

	/** Gets all known themes */
	const TArray<FColorizedFolderTheme>& GetThemes() const
	{
		return LoadedThemes;
	}

	/** Validates that there is an active-loaded theme */
	void ValidateActiveTheme();

	/** Returns the engine theme directory. (Engine themes are project-agnostic) */
	static FString GetEngineThemeDir();

	/** Returns the project theme directory. (Project themes can override engine themes) */
	static FString GetProjectThemeDir();

	/** Returns the user theme directory. (Themes in this directory are per-user and override engine and project themes) */
	static FString GetUserThemeDir();

	/** Returns the plugins theme directory. */
	static FString GetPluginThemeDir();

	/** Returns true if the theme ID already exists in the theme dropdown */
	bool DoesThemeExist(const FGuid& ThemeId) const;

private:
	FColorizedFolderTheme& GetCurrentTheme_Mutable()
	{
		return *LoadedThemes.FindByKey(CurrentThemeId);
	}

	void LoadThemesFromDirectory(const FString& Directory);
	bool ReadTheme(const FString& ThemeData, FColorizedFolderTheme& OutTheme);
	void EnsureValidCurrentTheme();
	void LoadThemeFolderSchemes(FColorizedFolderTheme& Theme);

protected:
	//~ Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface
#endif

private:
	UPROPERTY(EditAnywhere, Config, Category=ContentBrowser)
	FGuid CurrentThemeId;

	UPROPERTY(EditAnywhere, Transient, Category=ContentBrowser)
	FColorizedFolderColorSchemeList ActiveSchemes;
};
