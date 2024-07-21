// Copyright Jayou, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class UnrealURP : ModuleRules
{
	public UnrealURP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);

		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				Path.Combine(GetModuleDirectory("Renderer"), "Private"),
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"RenderCore",
				"RHI",
				"Projects",
				"Renderer",
				"Niagara",
				"UMG",
				// ... add private dependencies that you statically link with here ...	
			}
			);

		if (DefaultBuildSettings == (BuildSettingsVersion)4)
		{
			PrivateDependencyModuleNames.Add("ImageCore");
        }

        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
