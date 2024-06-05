// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class kID_Unreal : ModuleRules
{
	public kID_Unreal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "UMG", "Core", "Json", "HTTP", "NetCommon", "NetCore", "CoreUObject", 
						"Engine", "InputCore", "EnhancedInput", "Sockets", "Networking", "UnrealEd" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}
