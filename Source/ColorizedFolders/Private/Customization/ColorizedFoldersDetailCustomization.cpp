// Copyright © 2025 Playton. All Rights Reserved.


#include "ColorizedFoldersDetailCustomization.h"

#include "ColorizedFoldersSettings.h"
#include "DesktopPlatformModule.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "ISettingsEditorModule.h"
#include "SPrimaryButton.h"
#include "SSimpleButton.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Themes/ColorizedFoldersManager.h"
#include "Themes/ColorizedFoldersTheme.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "ColorizedFolders"

TWeakPtr<SWindow> ThemeEditorWindow;
FString CurrentActiveThemeDisplayName;
FString OriginalThemeDisplayName;

class SFolderColorThemeEditor : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SFolderColorThemeEditor)
	{
	}
		SLATE_EVENT(FOnFolderColorThemeEditorClosed, OnThemeEditorClosed)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, TSharedRef<SWindow> InParentWindow)
	{
		OnThemeEditorClosed = InArgs._OnThemeEditorClosed;
		ParentWindow = InParentWindow;
		InParentWindow->SetOnWindowClosed(FOnWindowClosed::CreateSP(this, &SFolderColorThemeEditor::OnParentWindowClosed));

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.ViewIdentifier = TEXT("FolderColorThemeEditor");

		TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
		DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& PropAndParent)
		{
			static const FName CurrentThemeIdName("CurrentThemeId");
			return PropAndParent.Property.GetFName() != CurrentThemeIdName;
		}));

		DetailsView->SetObject(&UColorizedFoldersManager::Get());
		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(6.0f, 3.0f)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(0.6f)
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.Padding(5.0f, 2.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ThemeName", "Name"))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(2.0f)
					.VAlign(VAlign_Center)
					.Padding(5.0f, 2.0f)
					[
						SAssignNew(EditableThemeName, SEditableTextBox)
						.SelectAllTextWhenFocused(true)
						.Text(this, &SFolderColorThemeEditor::GetThemeName)
						.OnTextCommitted(this, &SFolderColorThemeEditor::OnThemeNameCommitted)
						.OnTextChanged(this, &SFolderColorThemeEditor::OnThemeNameChanged)
					]
				]
				+ SVerticalBox::Slot()
					.Padding(6.0f, 3.0f)
					[
						DetailsView
					]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Bottom)
				.Padding(6.0f, 3.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Bottom)
					.Padding(4, 3)
					[
						SNew(SPrimaryButton)
						.Text(LOCTEXT("SaveThemeButton", "Save"))
						.OnClicked(this, &SFolderColorThemeEditor::OnSaveClicked)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Bottom)
					.Padding(4, 3)
					[
						SNew(SButton)
						.Text(LOCTEXT("CancelThemeEditingButton", "Cancel"))
						.OnClicked(this, &SFolderColorThemeEditor::OnCancelClicked)
					]
				]
			]
		];
	}

private:
	FText GetThemeName() const
	{
		return UColorizedFoldersManager::Get().GetCurrentTheme().DisplayName;
	}

	bool ValidateThemeName(const FText& ThemeName)
	{
		FText OutErrorMsg;

		if (ThemeName.IsEmpty())
		{
			OutErrorMsg = LOCTEXT("ThemeNameEmpty", "Theme name cannot be empty.");
			EditableThemeName->SetError(OutErrorMsg);
			return false;
		}

		if (1) return 1;

		const TArray<FColorizedFolderTheme> ThemeOptions = UColorizedFoldersManager::Get().GetThemes();
		for (const auto& Theme : ThemeOptions)
		{
			if (Theme.DisplayName.EqualTo(ThemeName) && !CurrentActiveThemeDisplayName.Equals(ThemeName.ToString()))
			{
				OutErrorMsg = FText::Format(LOCTEXT("RenameThemeAlreadyExists", "A theme already exists with the name '{0}'."), ThemeName);
				EditableThemeName->SetError(OutErrorMsg);
				return false;
			}
		}

		EditableThemeName->SetError(FText::GetEmpty());
		return true;
	}

	FReply OnSaveClicked()
	{
		FString FileName;
		bool bSuccess = true;
		FString PrevFileName;
		
		const FColorizedFolderTheme& Theme = UColorizedFoldersManager::Get().GetCurrentTheme();

		// Update name is taken: Do not save.
		if (!ValidateThemeName(EditableThemeName->GetText()))
		{
			bSuccess = false;
		}
		// Duplicating a theme
		else if (Theme.Filename.IsEmpty())
		{
			UColorizedFoldersManager::Get().SetCurrentThemeDisplayName(EditableThemeName->GetText());
			FileName = UColorizedFoldersManager::GetUserThemeDir() / Theme.DisplayName.ToString() + TEXT(".json");
			EditableThemeName->SetError(FText::GetEmpty());
		}
		// Modifying a theme
		else
		{
			PrevFileName = Theme.Filename;
			UColorizedFoldersManager::Get().SetCurrentThemeDisplayName(EditableThemeName->GetText());
			FileName = UColorizedFoldersManager::GetUserThemeDir() / Theme.DisplayName.ToString() + TEXT(".json");
			EditableThemeName->SetError(FText::GetEmpty());
		}

		if (!FileName.IsEmpty() && bSuccess)
		{
			UColorizedFoldersManager::Get().SaveCurrentThemeAs(FileName);
			
			// If the user modified an existing user-specific theme name, delete the old one.
			if (!PrevFileName.IsEmpty() && !PrevFileName.Equals(FileName))
			{
				IPlatformFile::GetPlatformPhysical().DeleteFile(*PrevFileName);
			}
			EditableThemeName->SetError(FText::GetEmpty());

			ParentWindow.Pin()->SetOnWindowClosed(FOnWindowClosed());
			ParentWindow.Pin()->RequestDestroyWindow();

			OnThemeEditorClosed.ExecuteIfBound(true);
		}
		
		return FReply::Handled();
	}

	FReply OnCancelClicked()
	{
		ParentWindow.Pin()->SetOnWindowClosed(FOnWindowClosed());
		ParentWindow.Pin()->RequestDestroyWindow();

		OnThemeEditorClosed.ExecuteIfBound(false);
		
		return FReply::Handled();
	}

	void OnThemeNameCommitted(const FText& InText, ETextCommit::Type InCommitType)
	{
		if (!ValidateThemeName(InText))
		{
			const FText OriginalTheme = FText::FromString(OriginalThemeDisplayName);
			EditableThemeName->SetText(OriginalTheme);
			EditableThemeName->SetError(FText::GetEmpty());
		}
		else
		{
			EditableThemeName->SetText(InText);
		}
	}
	void OnThemeNameChanged(const FText& InText)
	{
		ValidateThemeName(InText);
	}

	void OnParentWindowClosed(const TSharedRef<SWindow>&)
	{
		OnCancelClicked();
	}

private:
	FOnFolderColorThemeEditorClosed OnThemeEditorClosed;
	TSharedPtr<SEditableTextBox> EditableThemeName; 
	TWeakPtr<SWindow> ParentWindow;
};

TSharedRef<IPropertyTypeCustomization> FColorizedFoldersPropertyCustomization::MakeInstance()
{
	return MakeShareable(new FColorizedFoldersPropertyCustomization());
}

void FColorizedFoldersPropertyCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> PropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FColorizedFoldersPropertyCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	uint32 NumChildren = 0;
	TSharedPtr<IPropertyHandle> SchemeArrayProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FColorizedFolderColorSchemeList, Schemes));

	SchemeArrayProperty->GetNumChildren(NumChildren);
	for (uint32 ChildIdx = 0; ChildIdx < NumChildren; ++ChildIdx)
	{
		IDetailPropertyRow& Row = ChildBuilder.AddProperty(SchemeArrayProperty->GetChildHandle(ChildIdx).ToSharedRef());
		FString DisplayName = TEXT("Scheme ") + FString::FromInt(ChildIdx + 1);
		
		Row.DisplayName(FText::FromString(DisplayName));

		Row.CustomWidget(true)
		.NameContent()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(FText::FromString(DisplayName))
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)

			// Add a preview image to showcase the color
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Padding(0, 0, 5, 0)
			[
				SNew(SBox)
				.WidthOverride(128)
				.HeightOverride(20)
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("GenericWhiteBox"))
					.ColorAndOpacity_Lambda([ChildIdx, SchemeArrayProperty]()
					{
						FLinearColor Color;
						return Color;
					})
				]
			]
		];
	}
}








TSharedRef<IDetailCustomization> FColorizedFoldersDetailCustomization::MakeInstance()
{
	return MakeShareable(new FColorizedFoldersDetailCustomization());
}

void FColorizedFoldersDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& ThemeCategory = DetailBuilder.EditCategory("ContentBrowser");
	
	TArray<UObject*> Objects = { &UColorizedFoldersManager::Get() };

	if (IDetailPropertyRow* ThemeRow = ThemeCategory.AddExternalObjectProperty(Objects, "CurrentThemeId"))
	{
		MakeThemePickerRow(*ThemeRow);
	}

	ThemeCategory.AddCustomRow(FText::FromString(TEXT("RefreshTheme")))
	.NameContent()
	[
		SNew(SButton)
		.Text(LOCTEXT("ReloadTheme","Reload Theme"))
		.OnClicked_Lambda([]()
		{
			UColorizedFoldersManager::Get().OnThemeChanged().Broadcast(UColorizedFoldersManager::Get().GetCurrentTheme().Id);
			return FReply::Handled();
		})
	];
}

void FColorizedFoldersDetailCustomization::RefreshComboBox()
{
	TSharedPtr<FString> SelectedTheme;
	GenerateThemeOptions(SelectedTheme);
	ComboBox->RefreshOptions();
	ComboBox->SetSelectedItem(SelectedTheme);
}

// validate theme name without error messages: 
bool IsThemeNameValid(const FString& ThemeName)
{
	const TArray<FColorizedFolderTheme> ThemeOptions = UColorizedFoldersManager::Get().GetThemes();

	for (const auto& Theme : ThemeOptions)
	{
		// show error message whenever there's duplicate (and different from the previous name) 
		if (Theme.DisplayName.ToString().Equals(ThemeName))
		{
			return false;
		}
	}
	return true;
}
void GetThemeIdFromPath(FString& ThemePath, FString& ImportedThemeID)
{
	FString ThemeData;

	if (FFileHelper::LoadFileToString(ThemeData, *ThemePath))
	{
		TSharedRef<TJsonReader<>> ReaderRef = TJsonReaderFactory<>::Create(ThemeData);
		TJsonReader<>& Reader = ReaderRef.Get();

		TSharedPtr<FJsonObject> ObjectPtr = MakeShareable(new FJsonObject()); 

		if (FJsonSerializer::Deserialize(Reader, ObjectPtr) && ObjectPtr.IsValid())
		{
			// Just check that the theme has Id. We won't load them unless the theme is used
			ObjectPtr->TryGetStringField(TEXT("Id"), ImportedThemeID); 
		}
	}
}

void FColorizedFoldersDetailCustomization::PromptToImportTheme(const FString& ImportPath)
{
	TArray<FString> OutFiles;
	const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	if (FDesktopPlatformModule::Get()->OpenFileDialog(
		ParentWindowHandle,
		LOCTEXT("ImportThemeDialogTitle", "Import theme...").ToString(),
		FPaths::GetPath(ImportPath),
		TEXT(""),
		TEXT("JSON files (*.json)|*.json"),
		EFileDialogFlags::None,
		OutFiles))
	{
		FString SourcePath = OutFiles[0];
		const FString DestPath = UColorizedFoldersManager::GetUserThemeDir() / FPaths::GetCleanFilename(SourcePath);

		FString PathPart, Extension, FilenameWithoutExtension;
		FPaths::Split(SourcePath, PathPart, FilenameWithoutExtension, Extension);

		// If theme name exists, don't import (to prevent from overwriting existing theme files)
		if (!IsThemeNameValid(FilenameWithoutExtension))
		{
			FNotificationInfo Notification(LOCTEXT("ImportThemeFailureNameExists", "Import theme failed: Theme name already exists"));
			Notification.ExpireDuration = 3.0f;
			Notification.bUseSuccessFailIcons = true;
			
			FSlateNotificationManager::Get().AddNotification(Notification)->SetCompletionState(SNotificationItem::CS_Fail);
		}
		// If theme name doesn't exist, import the theme
		else
		{
			const int32 NumOfThemesBefore = UColorizedFoldersManager::Get().GetThemes().Num();

			if (IPlatformFile::GetPlatformPhysical().CopyFile(*DestPath, *SourcePath))
			{
				// Update the theme list
				UColorizedFoldersManager::Get().LoadThemes();

				// If valid theme: the theme Num will be updated
				if (UColorizedFoldersManager::Get().GetThemes().Num() != NumOfThemesBefore)
				{
					// Extract ID as a FString directly from a JSON file
					FString ImportedThemeID;
					GetThemeIdFromPath(SourcePath, ImportedThemeID);

					// Convert the FString ID to FGuid
					FGuid ImportedThemeGuid = FGuid(ImportedThemeID);
					UColorizedFoldersManager::Get().SetCurrentThemeId_Direct(ImportedThemeGuid);

					FNotificationInfo Notification(LOCTEXT("ImportThemeSuccess", "Import theme succeeded"));
					Notification.ExpireDuration = 3.0f;
					Notification.bUseSuccessFailIcons = true;

					FSlateNotificationManager::Get().AddNotification(Notification)->SetCompletionState(SNotificationItem::CS_Success);

					ISettingsEditorModule& SettingsEditorModule = FModuleManager::GetModuleChecked<ISettingsEditorModule>("SettingsEditor");
					SettingsEditorModule.OnApplicationRestartRequired();
				}
				// If invalid theme: delete the copied file
				else
				{
					// Incomplete themes will not reach here
					IPlatformFile::GetPlatformPhysical().DeleteFile(*DestPath);

					FNotificationInfo Notification(LOCTEXT("ImportThemeFailure", "Import theme failed: Theme id already exists"));
					Notification.ExpireDuration = 3.0f;
					Notification.bUseSuccessFailIcons = true;

					FSlateNotificationManager::Get().AddNotification(Notification)->SetCompletionState(SNotificationItem::CS_Fail);
				}
			}
			// If unable to copy the file to user-specific theme location, do nothing
			else
			{
				FNotificationInfo Notification(LOCTEXT("ImportThemeFailure", "Import theme failed"));
				Notification.ExpireDuration = 3.0f;
				Notification.bUseSuccessFailIcons = true;

				FSlateNotificationManager::Get().AddNotification(Notification)->SetCompletionState(SNotificationItem::CS_Fail);
			}
		}
	}
}

void FColorizedFoldersDetailCustomization::MakeThemePickerRow(IDetailPropertyRow& PropertyRow)
{
	TSharedPtr<FString> SelectedItem;
	GenerateThemeOptions(SelectedItem);
	
	FDetailWidgetRow& CustomRow = PropertyRow.CustomWidget(false);

	ComboBox =
		SNew(STextComboBox)
		.OptionsSource(&ThemeOptions)
		.InitiallySelectedItem(SelectedItem)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.OnGetTextLabelForItem(this, &FColorizedFoldersDetailCustomization::GetTextLabelForThemeEntry)
		.OnSelectionChanged(this, &FColorizedFoldersDetailCustomization::OnThemePicked);
	
	CustomRow
	.NameContent()
	[
		PropertyRow.GetPropertyHandle()->CreatePropertyNameWidget(LOCTEXT("ActiveFolderColorSchemeDisplayName", "Active Folder Color Theme"))
	]
	.ValueContent()
	.MaxDesiredWidth(350.0f)
	[
		SNew(SHorizontalBox)
		.IsEnabled(this, &FColorizedFoldersDetailCustomization::IsThemeEditingEnabled)

		// ComboBox
		+ SHorizontalBox::Slot()
		[
			SNew(SBox)
			.WidthOverride(125.0f)
			[
				ComboBox.ToSharedRef()
			]
		]
		
		// Buttons
		// Edit button
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.AutoWidth()
		[
			SNew(SSimpleButton)
			.Icon(FAppStyle::Get().GetBrush("Icons.Edit"))
			.ToolTipText(LOCTEXT("EditButtonTooltip", "Edit this theme"))
			.IsEnabled_Lambda([]()
			{
				return !(UColorizedFoldersManager::Get().IsEngineTheme());
			})
			.OnClicked(this, &FColorizedFoldersDetailCustomization::OnEditThemeClicked)
		]
		// Duplicate button
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.AutoWidth()
		[
			SNew(SSimpleButton)
			.Icon(FAppStyle::Get().GetBrush("Icons.Duplicate"))
			.ToolTipText(LOCTEXT("DuplicateButtonTooltip", "Duplicate this theme and edit it"))
			.OnClicked(this, &FColorizedFoldersDetailCustomization::OnDuplicateAndEditThemeClicked)
		]
		// Export button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SSimpleButton)
			.Icon(FAppStyle::Get().GetBrush("Themes.Export"))
			.ToolTipText(LOCTEXT("ExportButtonTooltip", "Export this theme to a file on your computer"))
			.OnClicked(this, &FColorizedFoldersDetailCustomization::OnExportThemeClicked)
		]
		// Import button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SSimpleButton)
			.Icon(FAppStyle::Get().GetBrush("Themes.Import"))
			.ToolTipText(LOCTEXT("ImportButtonTooltip", "Import a theme from a file on your computer"))
			.OnClicked(this, &FColorizedFoldersDetailCustomization::OnImportThemeClicked)
		]
		// Delete button
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.AutoWidth()
		[
			SNew(SSimpleButton)
			.Icon(FAppStyle::Get().GetBrush("Icons.Delete"))
			.ToolTipText(LOCTEXT("DeleteButtonTooltip", "Delete this theme"))
			.IsEnabled_Lambda([]()
			{
				return !(UColorizedFoldersManager::Get().IsEngineTheme() || UColorizedFoldersManager::Get().IsProjectTheme());
			})
			.OnClicked(this, &FColorizedFoldersDetailCustomization::OnDeleteThemeClicked)
		]
	];
}

static void OnThemeEditorClosed(bool bSaved, TWeakPtr<FColorizedFoldersDetailCustomization> ActiveCustomization, FGuid CreatedThemeId, FGuid PrevThemeId)
{
	if (!bSaved)
	{
		if (PrevThemeId.IsValid())
		{
			UColorizedFoldersManager::Get().ApplyTheme(PrevThemeId);

			if (CreatedThemeId.IsValid())
			{
				UColorizedFoldersManager::Get().RemoveTheme(CreatedThemeId);
			}
			if (ActiveCustomization.IsValid())
			{
				ActiveCustomization.Pin()->RefreshComboBox();
			}
		}
		else
		{
			UColorizedFoldersManager::Get().ApplyDefaultTheme();
		}
	}
	
	UColorizedFoldersManager::Get().OnThemeChanged().Broadcast(UColorizedFoldersManager::Get().GetCurrentTheme().Id);
}

bool FColorizedFoldersDetailCustomization::IsThemeEditingEnabled() const
{
	return !ThemeEditorWindow.IsValid();
}

void FColorizedFoldersDetailCustomization::GenerateThemeOptions(TSharedPtr<FString>& OutSelectedTheme)
{
	const TArray<FColorizedFolderTheme>& Themes = UColorizedFoldersManager::Get().GetThemes();

	ThemeOptions.Empty(Themes.Num());
	int32 Index = 0;
	for (const FColorizedFolderTheme& Theme : Themes)
	{
		TSharedRef<FString> ThemeString = MakeShared<FString>(FString::FromInt(Index));

		if (UColorizedFoldersManager::Get().GetCurrentTheme() == Theme)
		{
			OutSelectedTheme = ThemeString;
		}

		ThemeOptions.Add(ThemeString);
		++Index;
	}
}

FReply FColorizedFoldersDetailCustomization::OnExportThemeClicked()
{
	TArray<FString> OutFiles;
	const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	const FString ExportPath = FPlatformProcess::UserDir();
	const FString DefaultFileName = UColorizedFoldersManager::Get().GetCurrentTheme().DisplayName.ToString();

	if (FDesktopPlatformModule::Get()->SaveFileDialog(
		ParentWindowHandle,
		LOCTEXT("ExportThemeDialogTitle", "Export current theme...").ToString(),
		FPaths::GetPath(ExportPath),
		DefaultFileName,
		TEXT("JSON files (*.json)|*.json"),
		EFileDialogFlags::None,
		OutFiles))
	{
		const FString SourcePath = UColorizedFoldersManager::Get().GetCurrentTheme().Filename;
		const FString DestPath = OutFiles[0];

		if (IPlatformFile::GetPlatformPhysical().CopyFile(*DestPath, *SourcePath))
		{
			FNotificationInfo Notification(LOCTEXT("ExportThemeSuccess", "Export theme succeeded"));
			Notification.ExpireDuration = 3.0f;
			Notification.bUseSuccessFailIcons = false;

			FSlateNotificationManager::Get().AddNotification(Notification)->SetCompletionState(SNotificationItem::CS_Success);
		}
		else
		{
			FNotificationInfo Notification(LOCTEXT("ExportThemeFailure", "Export theme failed"));
			Notification.ExpireDuration = 3.0f;
			Notification.bUseSuccessFailIcons = false;

			FSlateNotificationManager::Get().AddNotification(Notification)->SetCompletionState(SNotificationItem::CS_Fail);
		}
	}
	
	return FReply::Handled();
}

FReply FColorizedFoldersDetailCustomization::OnImportThemeClicked()
{
	PromptToImportTheme(FPlatformProcess::UserDir());
	return FReply::Handled();
}

FReply FColorizedFoldersDetailCustomization::OnDeleteThemeClicked()
{
	const auto PrevActiveTheme = UColorizedFoldersManager::Get().GetCurrentTheme();

	// Are you sure you want to do this?
	const FText FileNameToRemove = FText::FromString(PrevActiveTheme.DisplayName.ToString());
	const FText TextBody = FText::Format(LOCTEXT("ActionRemoveMsg", "Are you sure you want to permanently delete the folder-color theme \"{0}\"? This action cannot be undone."), FileNameToRemove);
	const FText TextTitle = FText::Format(LOCTEXT("RemoveTheme_Title", "Remove Theme \"{0}\"?"), FileNameToRemove);

	if (FMessageDialog::Open(EAppMsgType::OkCancel, TextBody, TextTitle) == EAppReturnType::Ok)
	{
		// Fallback to default theme
		UColorizedFoldersManager::Get().ApplyDefaultTheme();

		// Remove the previously active theme
		const FString Filename = UColorizedFoldersManager::GetUserThemeDir() / PrevActiveTheme.DisplayName.ToString() + TEXT(".json");
		IFileManager::Get().Delete(*Filename);
		UColorizedFoldersManager::Get().RemoveTheme(PrevActiveTheme.Id);
		RefreshComboBox();
	}
	
	return FReply::Handled();
}

FReply FColorizedFoldersDetailCustomization::OnDuplicateAndEditThemeClicked()
{
	FGuid PrevActiveTheme = UColorizedFoldersManager::Get().GetCurrentTheme().Id;
	
	FGuid NewThemeId = UColorizedFoldersManager::Get().DuplicateActiveTheme();
	UColorizedFoldersManager::Get().ApplyTheme(NewThemeId);
	// Set the new theme name to empty so the user can name it
	UColorizedFoldersManager::Get().SetCurrentThemeDisplayName(FText::GetEmpty());
	CurrentActiveThemeDisplayName = UColorizedFoldersManager::Get().GetCurrentTheme().DisplayName.ToString();
	OriginalThemeDisplayName = UColorizedFoldersManager::Get().GetCurrentTheme().DisplayName.ToString();

	RefreshComboBox();

	// Open the theme editor window
	OpenThemeEditorWindow(FOnFolderColorThemeEditorClosed::CreateStatic(&OnThemeEditorClosed, TWeakPtr<FColorizedFoldersDetailCustomization>(SharedThis(this)), NewThemeId, PrevActiveTheme));
	
	return FReply::Handled();
}

FReply FColorizedFoldersDetailCustomization::OnEditThemeClicked()
{
	FGuid CurrentlyActiveTheme = UColorizedFoldersManager::Get().GetCurrentTheme().Id;
	CurrentActiveThemeDisplayName = UColorizedFoldersManager::Get().GetCurrentTheme().DisplayName.ToString();
	OriginalThemeDisplayName = CurrentActiveThemeDisplayName;
	
	OpenThemeEditorWindow(FOnFolderColorThemeEditorClosed::CreateStatic(&OnThemeEditorClosed, TWeakPtr<FColorizedFoldersDetailCustomization>(SharedThis(this)), FGuid(), CurrentlyActiveTheme));
	
	return FReply::Handled();
}

FString FColorizedFoldersDetailCustomization::GetTextLabelForThemeEntry(TSharedPtr<FString> Entry)
{
	const TArray<FColorizedFolderTheme>& Themes = UColorizedFoldersManager::Get().GetThemes();
	return Themes[FCString::Atoi(**Entry)].DisplayName.ToString();
}

void FColorizedFoldersDetailCustomization::OnThemePicked(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	UColorizedFoldersSettings* Settings = UColorizedFoldersSettings::GetMutable();

	// Set current applied theme to selected theme
	const TArray<FColorizedFolderTheme>& Themes = UColorizedFoldersManager::Get().GetThemes();
	Settings->CurrentAppliedTheme = Themes[FCString::Atoi(**NewSelection)].Id;

	// If set directly in code, the theme was already applied
	if (SelectInfo != ESelectInfo::Direct)
	{
		Settings->SaveConfig();
		UColorizedFoldersManager::Get().SetCurrentThemeId_Direct(Settings->CurrentAppliedTheme);

		ISettingsEditorModule& SettingsEditorModule = FModuleManager::GetModuleChecked<ISettingsEditorModule>("SettingsEditor");
		SettingsEditorModule.OnApplicationRestartRequired();
	}
}

void FColorizedFoldersDetailCustomization::OpenThemeEditorWindow(FOnFolderColorThemeEditorClosed OnThemeEditorClosed)
{
	if (!ThemeEditorWindow.IsValid())
	{
		TSharedRef<SWindow> NewWindow = SNew(SWindow)
			.Title(LOCTEXT("ThemeEditorWindowTitle", "Theme Editor"))
			.SizingRule(ESizingRule::UserSized)
			.ClientSize(FVector2D(900, 600))
			.SupportsMaximize(false)
			.SupportsMinimize(false);

		TSharedRef<SFolderColorThemeEditor> ThemeEditor =
			SNew(SFolderColorThemeEditor, NewWindow)
			.OnThemeEditorClosed(OnThemeEditorClosed);
		
		NewWindow->SetContent(ThemeEditor);

		if (TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(ComboBox.ToSharedRef()))
		{
			FSlateApplication::Get().AddWindowAsNativeChild(NewWindow, ParentWindow.ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(NewWindow);
		}

		ThemeEditorWindow = NewWindow;
	}
}


#undef LOCTEXT_NAMESPACE