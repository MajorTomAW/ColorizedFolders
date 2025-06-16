// Copyright © 2025 MajorT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ColorizedFoldersTheme.generated.h"

#define NUM_FOLDER_SCHEMES 32

/** Single schema that maps folder names to a color. */
USTRUCT()
struct FColorizedFolderColorScheme
{
	GENERATED_BODY()

	/**
	 * A list of folder names that should use this color scheme.
	 * Separate multiple folder names with a comma.
	 */
	UPROPERTY(EditDefaultsOnly, Category = Scheme)
	FString FolderNames;

	/**
	 * A list of explicit paths that should use this color scheme.
	 * Separate multiple paths with a comma.
	 */
	UPROPERTY(EditDefaultsOnly, Category = Scheme)
	FString ExplicitPaths;

	/** The color to use for this color scheme. */
	UPROPERTY(EditDefaultsOnly, Category = Scheme)
	FLinearColor SchemeColor = FLinearColor();

	/** Resolves the folder names into a list of unique folder names. */
	TArray<FString> ResolveFolderNames() const;

	/** Converts a resolved list of folder names into a single string. */
	void SaveArrayToFolders(const TArray<FString>& FolderNames);

	/** Resolves the explicit paths into a list of unique paths. */
	TArray<FString> ResolveExplicitPaths() const;

	/** Converts a resolved list of explicit paths into a single string. */
	void SaveArrayToPaths(const TArray<FString>& ExplicitPaths);

	bool operator==(const FColorizedFolderColorScheme& Other) const
	{
		return FolderNames == Other.FolderNames &&
			ExplicitPaths == Other.ExplicitPaths &&
			SchemeColor == Other.SchemeColor;
	}
};

inline TArray<FString> FColorizedFolderColorScheme::ResolveFolderNames() const
{
	TArray<FString> UniqueFolderNames;
	FolderNames.ParseIntoArray(UniqueFolderNames, TEXT(","), true);
	UniqueFolderNames.Sort();
	return UniqueFolderNames;
}

inline void FColorizedFolderColorScheme::SaveArrayToFolders(const TArray<FString>& InFolderNames)
{
	if (InFolderNames.Num() == 0)
	{
		FolderNames = TEXT("");
		return;
	}

	// Split the folder names with a comma.
	FString Result;
	for (int32 i = 0; i < InFolderNames.Num(); ++i)
	{
		Result += InFolderNames[i];
		if (i < InFolderNames.Num() - 1)
		{
			Result += TEXT(",");
		}
	}
	FolderNames = Result;

	// Remove any whitespace.
	FolderNames = FolderNames.Replace(TEXT(" "), TEXT(""));
}

inline TArray<FString> FColorizedFolderColorScheme::ResolveExplicitPaths() const
{
	TArray<FString> UniquePaths;
	ExplicitPaths.ParseIntoArray(UniquePaths, TEXT(","), true);
	UniquePaths.Sort();
	return UniquePaths;
}

inline void FColorizedFolderColorScheme::SaveArrayToPaths(const TArray<FString>& InExplicitPaths)
{
	if (InExplicitPaths.Num() == 0)
	{
		ExplicitPaths = TEXT("");
		return;
	}
	
	FString Result;
	for (int32 i = 0; i < InExplicitPaths.Num(); ++i)
	{
		Result += InExplicitPaths[i];
		if (i < InExplicitPaths.Num() - 1)
		{
			Result += TEXT(",");
		}
	}
	ExplicitPaths = Result;

	// Remove any whitespace.
	ExplicitPaths = ExplicitPaths.Replace(TEXT(" "), TEXT(""));
}

/** Represents a list of folder color schemes. */
USTRUCT()
struct FColorizedFolderColorSchemeList
{
	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, Category = ContentBrowser, meta=(TitleProperty="Scheme"))
	FColorizedFolderColorScheme Schemes[NUM_FOLDER_SCHEMES];

	FText DisplayNames[NUM_FOLDER_SCHEMES];
};

/** Represents a theme of colorized folder schemes. */
USTRUCT()
struct FColorizedFolderTheme
{
	GENERATED_BODY()
	friend class UColorizedFoldersSettings;

	/** Unique ID for the theme */
	FGuid Id;
	
	/** Friendly, user-customizable theme name */
	FText DisplayName;
	
	/** Filename where the theme is stored */
	FString Filename;

	/** The default color schemes for this theme. Used for resetting to default. Not the active colors. */
	TArray<FColorizedFolderColorScheme> LoadedDefaultColorSchemes;

	bool operator==(const FColorizedFolderTheme& Other) const
	{
		return Id == Other.Id;
	}

	bool operator==(const FGuid& OtherId) const
	{
		return Id == OtherId;
	}
};