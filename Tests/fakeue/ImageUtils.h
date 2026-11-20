#pragma once
// 가짜 ImageUtils.h (D-403): FImageUtils::ImportFileAsTexture2D 선언만
#include "CoreMinimal.h"
struct FImageUtils
{
	static UTexture2D* ImportFileAsTexture2D(const FString& Filename) { (void)Filename; return nullptr; }
};
