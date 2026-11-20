// 시안 그림 조각 (D-403): Alice 인게임 시안(docs/images/ui-1120/reference.webp)에서 잘라 낸 바뀌지 않는 그림
// (야자수 · WAVE 간판, BAR Alice 로고, 칵테일 잔 켜짐 · 꺼짐)을 Content/UIArt/*.png 에서 실행 중에 읽는다.
// PNG 는 미리 곱한 알파 (tools/ui_art/cut_ref.py), 붓은 PreMultipliedAlpha 로 찍는다. 패키지는 DefaultGame.ini 의 DirectoriesToAlwaysStageAsNonUFS 로 싣는다
#include "PDBoardDraw.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"

void SPDBoardWidget::LoadUiArt()
{
	static const TCHAR* Names[(int32)EUiArt::Count] = {TEXT("sign"), TEXT("logo"), TEXT("glass_on"), TEXT("glass_off"), TEXT("tv"), TEXT("rpanel"), TEXT("rpanel_ko"), TEXT("title_logo")};
	UiArt.SetNum((int32)EUiArt::Count);
	for (int32 I = 0; I < (int32)EUiArt::Count; ++I)
	{
		const FString Path = FPaths::ProjectContentDir() / TEXT("UIArt") / (FString(Names[I]) + TEXT(".png"));
		UTexture2D* Tex = FPaths::FileExists(Path) ? FImageUtils::ImportFileAsTexture2D(Path) : nullptr;
		if (!Tex) continue;
		Tex->SRGB = true;
		Tex->Filter = TF_Bilinear;
		Tex->LODGroup = TEXTUREGROUP_UI;
		Tex->UpdateResource();
		Tex->AddToRoot(); // 게임 내내 쓴다
		FSlateBrush& B = UiArt[I];
		B.SetResourceObject(Tex);
		B.ImageSize = FVector2D(Tex->GetSizeX(), Tex->GetSizeY());
		B.DrawAs = ESlateBrushDrawType::Image;
	}
}

static UTexture2D* PDLoadPng(const FString& Name)
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("UIArt") / (Name + TEXT(".png"));
	UTexture2D* Tex = FPaths::FileExists(Path) ? FImageUtils::ImportFileAsTexture2D(Path) : nullptr;
	if (!Tex) return nullptr;
	Tex->SRGB = true;
	Tex->Filter = TF_Bilinear;
	Tex->LODGroup = TEXTUREGROUP_UI;
	Tex->UpdateResource();
	Tex->AddToRoot();
	return Tex;
}

bool SPDBoardWidget::CocktailArt(FCanvas& C, int32 Id, float GX, float BY, float H, float Alpha) const
{
	if (CockArt.Num() == 0) // 처음 쓸 때 읽는다 (60장)
	{
		SPDBoardWidget* Self = const_cast<SPDBoardWidget*>(this);
		Self->CockArt.SetNum(60);
		for (int32 K = 0; K < 60; ++K)
			if (UTexture2D* Tex = PDLoadPng(FString::Printf(TEXT("cock_%d_%d"), K / 5, K % 5)))
			{
				FSlateBrush& B = Self->CockArt[K];
				B.SetResourceObject(Tex);
				B.ImageSize = FVector2D(Tex->GetSizeX(), Tex->GetSizeY());
				B.DrawAs = ESlateBrushDrawType::Image;
			}
	}
	const int32 K = (Id % 12) * 5 + Id % 5;
	if (Id < 0 || !CockArt.IsValidIndex(K) || !CockArt[K].GetResourceObject()) return false;
	const float W = H * 0.8f; // 그림 240 × 300
	C.Image(C.NextLayer(), &CockArt[K], GX - W / 2.f, BY - H * (268.f / 300.f), W, H, FLinearColor(1.f, 1.f, 1.f, Alpha)); // 그림 속 잔 발 = 위에서 268/300
	return true;
}

const FSlateBrush* SPDBoardWidget::Art(EUiArt A) const
{
	const int32 I = (int32)A;
	return UiArt.IsValidIndex(I) && UiArt[I].GetResourceObject() ? &UiArt[I] : nullptr;
}
