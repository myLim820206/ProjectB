using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectBEditorTarget : TargetRules
{
	public ProjectBEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("ProjectB");
	}
}
