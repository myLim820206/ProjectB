using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectBTarget : TargetRules
{
	public ProjectBTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ProjectB");
	}
}
