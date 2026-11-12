#include "UmbraSkillEditorLibrary.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

int32 UUmbraSkillEditorLibrary::ImportEngineCharacterBaseline()
{
    const FString Source = FPaths::ConvertRelativePathToFull(FPaths::RootDir() /
        TEXT("Templates/TemplateResources/High/Characters/Content/"));
    const FString Destination = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Characters/"));
    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files, *Source, TEXT("*.uasset"), true, false);
    if (Files.IsEmpty()) return -1;
    Files.Sort();
    // Preflight the whole import before any writes. This is a one-time baseline, not an asset updater.
    for (const FString& File : Files)
    {
        const FString Relative = File.Mid(Source.Len());
        if (IFileManager::Get().FileExists(*(Destination / Relative))) return -2;
    }
    const FString Mount = TEXT("/Game/Characters/");
    FPackageName::RegisterMountPoint(Mount, Source);
    ON_SCOPE_EXIT { FPackageName::UnRegisterMountPoint(Mount, Source); };
    TArray<UPackage*> Packages;
    // Load all source packages before saving; retain their original /Game/Characters references.
    for (const FString& File : Files)
    {
        const FString Relative = File.Mid(Source.Len());
        UPackage* Package = LoadPackage(nullptr, *(Mount + FPaths::ChangeExtension(Relative, TEXT(""))), LOAD_None);
        if (!Package) return -3;
        Package->FullyLoad();
        Packages.Add(Package);
    }
    for (int32 Index = 0; Index < Files.Num(); ++Index)
    {
        const FString Filename = Destination / Files[Index].Mid(Source.Len());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Packages[Index], nullptr, *Filename, Args)) return -4;
    }
    return Packages.Num();
}
