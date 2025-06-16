// Copyright © 2025 MajorT. All Rights Reserved.


#include "ColorizedFoldersManager.h"

#include "Interfaces/IPluginManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ColorizedFoldersManager)

#define LOCTEXT_NAMESPACE "ColorizedFolders"

static const FString ThemesSubDir = TEXT("Slate/Themes/ContentBrowser");

UColorizedFoldersManager::UColorizedFoldersManager()
{
	InitDefaults();
}

void UColorizedFoldersManager::InitDefaults()
{
	// Fill in the default (empty) schemes.
	for (int32 i = 0; i < NUM_FOLDER_SCHEMES; ++i)
	{
		DefaultColorSchemes[i] = FColorizedFolderColorScheme();
	}
}

void UColorizedFoldersManager::SetDefaultTheme(int32 Id, FColorizedFolderColorScheme InScheme)
{
#if ALLOW_THEMES
	DefaultColorSchemes[Id] = InScheme;
#else
	LoadedThemes.Schemes[Id] = InScheme;
#endif
}

#if ALLOW_THEMES
void UColorizedFoldersManager::LoadThemes()
{
	LoadedThemes.Empty();

	// Load themes from engine, project, and user directories
	LoadThemesFromDirectory(GetPluginThemeDir());
	LoadThemesFromDirectory(GetEngineThemeDir());
	LoadThemesFromDirectory(GetProjectThemeDir());
	LoadThemesFromDirectory(GetUserThemeDir());

	EnsureValidCurrentTheme();
	ApplyTheme(CurrentThemeId);
}

void UColorizedFoldersManager::SaveCurrentThemeAs(const FString& InFilename)
{
	FColorizedFolderTheme& CurrentTheme = GetCurrentTheme_Mutable();
	CurrentTheme.Filename = InFilename;
	FString NewPath = CurrentTheme.Filename;
	{ // Save JSON
		FString Output;
		TSharedRef<TJsonWriter<>> WriterRef = TJsonWriterFactory<>::Create(&Output);
		TJsonWriter<>& Writer = WriterRef.Get();
		Writer.WriteObjectStart();
		Writer.WriteValue(TEXT("Version"), 1);
		Writer.WriteValue(TEXT("Id"), CurrentTheme.Id.ToString());
		Writer.WriteValue(TEXT("DisplayName"), CurrentTheme.DisplayName.ToString());
		
		{
			Writer.WriteObjectStart(TEXT("Schemes"));
			for (int32 SchemeIndex = 0; SchemeIndex < NUM_FOLDER_SCHEMES; ++SchemeIndex)
			{
				const FColorizedFolderColorScheme& Scheme = ActiveSchemes.Schemes[SchemeIndex];
				Writer.WriteObjectStart(FString::FromInt(SchemeIndex));

				{ // Scheme Color
					Writer.WriteValue(TEXT("SchemeColor"), Scheme.SchemeColor.ToString());
				}
				{ // Folder Names
					Writer.WriteArrayStart(TEXT("FolderNames"));
					for (const FString& FolderName : Scheme.ResolveFolderNames())
					{
						Writer.WriteValue(FolderName);
					}
					Writer.WriteArrayEnd();
				}
				{ // Explicit Paths
					Writer.WriteArrayStart(TEXT("ExplicitPaths"));
					for (const FString& ExplicitPath : Scheme.ResolveExplicitPaths())
					{
						Writer.WriteValue(ExplicitPath);
					}
					Writer.WriteArrayEnd();
				}

				Writer.WriteObjectEnd();
			}
			Writer.WriteObjectEnd();
		}

		Writer.WriteObjectEnd();
		Writer.Close();

		if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*InFilename))
		{
			FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*InFilename, false);

			// Create a new path if the filename has been changed.
			NewPath = GetUserThemeDir() / CurrentTheme.DisplayName.ToString() + TEXT(".json");

			if (!NewPath.Equals(CurrentTheme.Filename))
			{
				IFileManager::Get().Move(*NewPath, *InFilename);
			}
		}

		FFileHelper::SaveStringToFile(Output, *NewPath);
	}
}

void UColorizedFoldersManager::ApplyTheme(FGuid ThemeId)
{
	if (ThemeId.IsValid())
	{
		FColorizedFolderTheme* CurrentTheme;
		if (CurrentThemeId != ThemeId)
		{
			// Unload the current theme
			if (CurrentThemeId.IsValid())
			{
				CurrentTheme = &GetCurrentTheme_Mutable();
				// Unload existing schemes
				CurrentTheme->LoadedDefaultColorSchemes.Empty();
			}

			// Load the new theme
			if (FColorizedFolderTheme* Theme = LoadedThemes.FindByKey(ThemeId))
			{
				CurrentThemeId = ThemeId;
				SaveConfig();
			}
		}

		CurrentTheme = &GetCurrentTheme_Mutable();
		LoadThemeFolderSchemes(*CurrentTheme);

		// Apply the new colors
		if (CurrentTheme->LoadedDefaultColorSchemes.Num() > 0)
		{
			FMemory::Memcpy(ActiveSchemes.Schemes, CurrentTheme->LoadedDefaultColorSchemes.GetData(), sizeof(FColorizedFolderColorScheme) * CurrentTheme->LoadedDefaultColorSchemes.Num());
		}
	}
	OnThemeChanged().Broadcast(CurrentThemeId);
}

void UColorizedFoldersManager::ApplyDefaultTheme()
{
	ApplyTheme(DefaultTheme.Id);
}

bool UColorizedFoldersManager::IsEngineTheme() const
{
	// users cannot edit/delete engine-specific themes: 
	const FString EnginePath = GetEngineThemeDir() / GetCurrentTheme().DisplayName.ToString() + TEXT(".json");

	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (GetCurrentTheme() == DefaultTheme)
	{
		return true;
	}
	if (FileManager.FileExists(*EnginePath))
	{
		return true;
	}
	return false;
}

bool UColorizedFoldersManager::IsProjectTheme() const
{
	// users cannot edit/delete project-specific themes: 
	const FString ProjectPath = GetProjectThemeDir() / GetCurrentTheme().DisplayName.ToString() + TEXT(".json");

	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile(); 

	if (FileManager.FileExists(*ProjectPath))
	{
		return true; 
	}
	return false; 
}

void UColorizedFoldersManager::RemoveTheme(FGuid ThemeId)
{
	// The Current Theme cannot currently be removed.  Apply a new theme first
	if (CurrentThemeId != ThemeId)
	{
		LoadedThemes.RemoveAll([&ThemeId](const FColorizedFolderTheme& TestTheme) { return TestTheme.Id == ThemeId; });
	}
}

FGuid UColorizedFoldersManager::DuplicateActiveTheme()
{
	const FColorizedFolderTheme& CurrentTheme = GetCurrentTheme();

	FGuid NewThemeGuid = FGuid::NewGuid();
	FColorizedFolderTheme NewTheme;
	NewTheme.Id = NewThemeGuid;
	NewTheme.DisplayName = FText::Format(LOCTEXT("ThemeDuplicateCopyText", "{0} - Copy"), CurrentTheme.DisplayName);
	NewTheme.LoadedDefaultColorSchemes = MakeArrayView<FColorizedFolderColorScheme>(ActiveSchemes.Schemes, NUM_FOLDER_SCHEMES);

	LoadedThemes.Add(MoveTemp(NewTheme));

	return NewThemeGuid;
}

void UColorizedFoldersManager::SetCurrentThemeDisplayName(FText NewDisplayName)
{
	GetCurrentTheme_Mutable().DisplayName = NewDisplayName;
}

void UColorizedFoldersManager::ValidateActiveTheme()
{
	// This is necessary because the core style loads the color table before ProcessNewlyLoadedUObjects is called,
	// which means none of the config properties are in the class property link at that time.
	ReloadConfig();
	EnsureValidCurrentTheme();
	ApplyTheme(Get().GetCurrentTheme().Id);
}

FString UColorizedFoldersManager::GetEngineThemeDir()
{
	return FPaths::EngineContentDir() / ThemesSubDir;
}

FString UColorizedFoldersManager::GetProjectThemeDir()
{
	return FPaths::ProjectContentDir() / ThemesSubDir;
}

FString UColorizedFoldersManager::GetUserThemeDir()
{
	return FPlatformProcess::UserSettingsDir() / FApp::GetEpicProductIdentifier() / ThemesSubDir;
}

FString UColorizedFoldersManager::GetPluginThemeDir()
{
	IPluginManager& PluginManager = IPluginManager::Get();
	return PluginManager.FindPlugin("ColorizedFolders")->GetContentDir() / ThemesSubDir;
}

bool UColorizedFoldersManager::DoesThemeExist(const FGuid& ThemeId) const
{
	for (const auto& Theme : LoadedThemes)
	{
		if (Theme.Id == ThemeId)
		{
			return true;
		}
	}

	return false;
}

void UColorizedFoldersManager::LoadThemesFromDirectory(const FString& Directory)
{
	TArray<FString> ThemeFiles;
	IFileManager::Get().FindFiles(ThemeFiles, *Directory, TEXT(".json"));

	for (const FString& ThemeFile : ThemeFiles)
	{
		bool bValidFile = false;
		FString ThemeData;
		FString ThemeFilename = Directory / ThemeFile;
		if (FFileHelper::LoadFileToString(ThemeData, *ThemeFilename))
		{
			FColorizedFolderTheme Theme;
			if (ReadTheme(ThemeData, Theme))
			{
				if (FColorizedFolderTheme* ExistingTheme = LoadedThemes.FindByKey(Theme.Id))
				{
					// Just update the existing theme.
					// Themes with the same id can override an existing one.
					// This behavior mimics config file hierarchies
					ExistingTheme->Filename = MoveTemp(ThemeFilename);
				}
				else
				{
					// Theme not found, add a new one
					Theme.Filename = MoveTemp(ThemeFilename);
					LoadedThemes.Add(MoveTemp(Theme));
				}
			}
		}
	}
}

bool UColorizedFoldersManager::ReadTheme(const FString& ThemeData, FColorizedFolderTheme& OutTheme)
{
	TSharedRef<TJsonReader<>> ReaderRef = TJsonReaderFactory<>::Create(ThemeData);
	TJsonReader<>& Reader = ReaderRef.Get();
	
	TSharedPtr<FJsonObject> ObjectPtr;
	if (FJsonSerializer::Deserialize(Reader, ObjectPtr) && ObjectPtr.IsValid())
	{
		int32 Version = 0;
		if (!ObjectPtr->TryGetNumberField(TEXT("Version"), Version))
		{
			return false;
		}

		FString IdString;
		if (!ObjectPtr->TryGetStringField(TEXT("Id"), IdString) || !FGuid::Parse(IdString, OutTheme.Id))
		{
			return false;
		}

		FString DisplayString;
		if (!ObjectPtr->TryGetStringField(TEXT("DisplayName"), DisplayString))
		{
			return false;
		}

		OutTheme.DisplayName = FText::FromString(MoveTemp(DisplayString));

		// Just check that the theme has schemes. We won't load them unless the theme is used
		if (!ObjectPtr->HasField(TEXT("Schemes")))
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	
	return true;
}

void UColorizedFoldersManager::EnsureValidCurrentTheme()
{
	DefaultTheme.DisplayName = LOCTEXT("DefaultFolderColorTheme", "No Theme");
	DefaultTheme.Id = FGuid(0x13438026, 0x5FBB4A9C, 0xA00A1DC9, 0x770217B8);
	DefaultTheme.Filename = IPluginManager::Get().FindPlugin(TEXT("ColorizedFolders"))->GetBaseDir() / TEXT("Resources/Themes/NoTheme.json");

	int32 ThemeIndex = LoadedThemes.AddUnique(DefaultTheme);

	if (!CurrentThemeId.IsValid() || !LoadedThemes.Contains(CurrentThemeId))
	{
		CurrentThemeId = DefaultTheme.Id;
	}
}

void UColorizedFoldersManager::LoadThemeFolderSchemes(FColorizedFolderTheme& Theme)
{
	FString ThemeData;

	if (Theme.LoadedDefaultColorSchemes.IsEmpty())
	{
		Theme.LoadedDefaultColorSchemes = MakeArrayView<FColorizedFolderColorScheme>(DefaultColorSchemes, NUM_FOLDER_SCHEMES);
	}
	
	if (FFileHelper::LoadFileToString(ThemeData, *Theme.Filename))
	{
		TSharedRef<TJsonReader<>> ReaderRef = TJsonReaderFactory<>::Create(ThemeData);
		TJsonReader<>& Reader = ReaderRef.Get();

		TSharedPtr<FJsonObject> ObjectPtr;
		if (FJsonSerializer::Deserialize(Reader, ObjectPtr) && ObjectPtr.IsValid())
		{
			// Check that the theme has schemes. We won't load them unless the theme is used
			const TSharedPtr<FJsonObject>* SchemesObject = nullptr;
			if (ObjectPtr->TryGetObjectField(TEXT("Schemes"), SchemesObject))
			{
				for (int32 SchemeIndex = 0; SchemeIndex < NUM_FOLDER_SCHEMES; ++SchemeIndex)
				{
					const TSharedPtr<FJsonObject>* SchemeObject = nullptr;
					if ((*SchemesObject)->TryGetObjectField(FString::FromInt(SchemeIndex), SchemeObject))
					{
						FString ColorString;
						if ((*SchemeObject)->TryGetStringField(TEXT("SchemeColor"), ColorString))
						{
							Theme.LoadedDefaultColorSchemes[SchemeIndex].SchemeColor.InitFromString(ColorString);	
						}
						
						TArray<FString> FolderNames;
						if ((*SchemeObject)->TryGetStringArrayField(TEXT("FolderNames"), FolderNames))
						{
							Theme.LoadedDefaultColorSchemes[SchemeIndex].SaveArrayToFolders(FolderNames);	
						}
						
						TArray<FString> PathNames;
						if ((*SchemeObject)->TryGetStringArrayField(TEXT("ExplicitPaths"), PathNames))
						{
							Theme.LoadedDefaultColorSchemes[SchemeIndex].SaveArrayToPaths(PathNames);	
						}
					}
					SchemeObject = nullptr;
				}
			}
			SchemesObject = nullptr;
		}
	}
}

#if WITH_EDITOR
void UColorizedFoldersManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
#endif

#undef LOCTEXT_NAMESPACE