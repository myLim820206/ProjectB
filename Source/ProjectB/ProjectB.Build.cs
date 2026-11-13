using UnrealBuildTool;

public class ProjectB : ModuleRules
{
	public ProjectB(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore", "Niagara", "UMG" });
	}
}
