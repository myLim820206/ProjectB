using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectBEditorTarget : TargetRules
{
	public ProjectBEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ProjectB");
	}
}
