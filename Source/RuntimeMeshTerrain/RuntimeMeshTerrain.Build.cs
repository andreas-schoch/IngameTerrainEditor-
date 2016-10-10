// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RuntimeMeshTerrain : ModuleRules
{
	public RuntimeMeshTerrain(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "ProceduralMeshComponent", "SimplexNoise" });

        PublicDependencyModuleNames.AddRange(new string[] { "ShaderCore", "RenderCore", "RHI", "RuntimeMeshComponent" });

    }
}
