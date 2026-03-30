// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using System.Text;
using UnrealBuildTool;

public class UE_CITRUSH : ModuleRules
{
	public UE_CITRUSH(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine",
			"InputCore","UMG", "Slate", "SlateCore","EnhancedInput",
			"ChaosVehicles", "PhysicsCore",
			"OnlineSubsystem", "OnlineSubsystemUtils", 
			"Voice", 
			"GameplayAbilities", "GameplayTags",
			"StateTreeModule", "GameplayStateTreeModule"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"DeveloperSettings",
			"UMG", "Slate", "SlateCore", "AdvancedWidgets",
			"ApplicationCore", 
			"MoviePlayer", "MediaAssets",
			"Json", "JsonUtilities", "HTTP", "WebSockets",
			"Sockets", "SteamSockets", "OnlineSubsystemSteam",
			"GameplayTasks", "EngineCameras",
			"Niagara",
			"AIModule", "NavigationSystem",
			"Chaos", "ChaosSolverEngine", "GeometryCollectionEngine"
		});

        AddSteamworks(Target);
        PrivateIncludePaths.Add(ModuleDirectory);

		// 에디터 전용 모듈
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"Blutility",
				"UMGEditor"
			});
		}
	}
	
	private void AddSteamworks(ReadOnlyTargetRules Target)
	{
		string SteamSDKPath = FindSteamSDK();

		if (string.IsNullOrEmpty(SteamSDKPath))
		{
			PublicDefinitions.Add("WITH_STEAMWORKS=0");
			System.Console.WriteLine("WARNING: Steam SDK not found.");
			return;
		}
		
		PublicDefinitions.Add("WITH_STEAMWORKS=1");
		System.Console.WriteLine("Found Steamworks SDK at: " + SteamSDKPath);

		// 헤더 경로
		PublicIncludePaths.Add(Path.Combine(SteamSDKPath, "sdk", "public"));

		// 플랫폼별 라이브러리
		AddPlatformLibraries(Target, SteamSDKPath);

		// steam_appid.txt 복사
		CopyAppIDFile(Target);

		//PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
	}

	private string FindSteamSDK()
    {
	    // UE 플랫폼 버전이니 하드코딩
        string[] Versions = { "Steamv161", "Steamv157", "Steamv153" };

        foreach (string Version in Versions)
        {
            string Path = System.IO.Path.Combine(
                EngineDirectory,
                "Source",
                "ThirdParty",
                "Steamworks",
                Version
            );

            if (Directory.Exists(Path))
                return Path;
        }

        return null;
    }

    private void AddPlatformLibraries(ReadOnlyTargetRules Target, string SteamSDKPath)
    {
	    string preProcessorSteamSDK = "STEAM_SDK_ENABLED=1";
	    if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string BinPath = Path.Combine(SteamSDKPath, "sdk", "redistributable_bin", "win64");

            PublicAdditionalLibraries.Add(Path.Combine(BinPath, "steam_api64.lib"));
            PublicDelayLoadDLLs.Add("steam_api64.dll");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            string BinPath = Path.Combine(SteamSDKPath, "sdk", "redistributable_bin", "linux64");
            string LibFile = Path.Combine(BinPath, "libsteam_api.so");

            PublicAdditionalLibraries.Add(LibFile);
            RuntimeDependencies.Add(LibFile);
        }
        else
        {
	        System.Console.WriteLine("WARNING: Steam SDK not found.");
	        preProcessorSteamSDK.Replace('1', '0');
        }
	    PublicDefinitions.Add(preProcessorSteamSDK);
    }

    private void CopyAppIDFile(ReadOnlyTargetRules Target)
    {
        // 프로젝트 루트의 steam_appid.txt를 패키징 시 복사
        string ProjectRoot = Path.Combine(ModuleDirectory, "..", "..");
        string AppIDFile = Path.Combine(ProjectRoot, "steam_appid.txt");

        if (File.Exists(AppIDFile))
        {
            if (Target.Platform == UnrealTargetPlatform.Win64)
            {
                RuntimeDependencies.Add(
                    "$(ProjectDir)/Binaries/Win64/steam_appid.txt",
                    AppIDFile
                );
            }
        }
        else
        {
            System.Console.WriteLine("WARNING: steam_appid.txt not found at " + AppIDFile);
        }
    }

}
