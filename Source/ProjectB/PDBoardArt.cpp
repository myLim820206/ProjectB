// 시안 그림 조각 (D-403): Alice 인게임 시안(docs/images/ui-1120/reference.webp)에서 잘라 낸 바뀌지 않는 그림
// (야자수 · WAVE 간판, BAR Alice 로고, 칵테일 잔 켜짐 · 꺼짐)을 Content/UIArt/*.png 에서 실행 중에 읽는다.
// PNG 는 미리 곱한 알파 (tools/ui_art/cut_ref.py), 붓은 PreMultipliedAlpha 로 찍는다. 패키지는 DefaultGame.ini 의 DirectoriesToAlwaysStageAsNonUFS 로 싣는다
#include "PDBoardDraw.h"
#include "PDTitleLogo.inl" // 시작 화면 벽빛 (D-465)
#include "PDLogo.h"
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

void SPDBoardWidget::PaintTitleWall(FCanvas& C, float TX, float TY, float TW, const float* NB, const float* PB, float CB) const
{
	static const TCHAR* Edge[3] = {TEXT("14d2eb"), TEXT("ff2dcd"), TEXT("ffd714")}; // PaintAliceSign 관 바깥 테와 같은 색
	constexpr int32 S = 8, GW = 160, GH = 90; // 화면 1280 × 720 의 1/8 (빛은 부드러워 늘려 찍어도 된다)
	if (TitleLight.Num() == 0)
	{
		SPDBoardWidget* Self = const_cast<SPDBoardWidget*>(this);
		Self->TitleLight.SetNum(3);
		const float K = TW / PDTitleLogo::SrcW;
		static const float Rad[2] = {60.f, 180.f}, Gain[2] = {2.0f, 3.6f}; // 시안 G: 가까운 빛 + 넓은 빛 (가우스 σ, 화면 화소)
		for (int32 Col = 0; Col < 3; ++Col)
		{
			// 관 모양(너비 = 관 굵기 × 2)을 칸에 덮인 넓이로 뿌린다
			TArray<float> M, Tmp, L;
			M.SetNumZeroed(GW * GH); Tmp.SetNumZeroed(GW * GH); L.SetNumZeroed(GW * GH);
			for (int32 Si = 0; Si < PDTitleLogo::NumStrokes; ++Si)
			{
				const PDTitleLogo::FStroke& St = PDTitleLogo::Strokes[Si];
				if (St.Color != Col) continue;
				const float Wd = PDTitleLogo::Tube[Col] * K * 0.8f * 2.f;
				for (int32 Q = 0; Q + 1 < St.N; ++Q)
				{
					const FVector2f A(TX + St.P[Q * 2] * K, TY + St.P[Q * 2 + 1] * K), B(TX + St.P[Q * 2 + 2] * K, TY + St.P[Q * 2 + 3] * K);
					const float Len = FVector2f::Distance(A, B);
					const int32 N = FMath::Max(1, FMath::CeilToInt(Len / 2.f));
					for (int32 I = 0; I < N; ++I)
					{
						const FVector2f P = (A + (B - A) * ((I + 0.5f) / N)) / (float)S;
						const int32 X = FMath::FloorToInt(P.X), Y = FMath::FloorToInt(P.Y);
						if (X >= 0 && X < GW && Y >= 0 && Y < GH) M[Y * GW + X] += Wd * (Len / N) / (float)(S * S);
					}
				}
			}
			for (int32 Ri = 0; Ri < 2; ++Ri) // 가우스 흐림 (가로 → 세로)
			{
				const float Sig = Rad[Ri] / S;
				const int32 R = FMath::CeilToInt(Sig * 3.f);
				TArray<float> Ker; Ker.SetNum(2 * R + 1);
				float Sum = 0.f;
				for (int32 Kx = 0; Kx <= 2 * R; ++Kx) { Ker[Kx] = FMath::Exp(-FMath::Square((float)(Kx - R)) / (2.f * Sig * Sig)); Sum += Ker[Kx]; }
				for (float& V : Ker) V /= Sum;
				for (int32 Y = 0; Y < GH; ++Y)
					for (int32 X = 0; X < GW; ++X)
					{
						float Acc = 0.f;
						for (int32 Kx = FMath::Max(0, R - X); Kx <= 2 * R && X + Kx - R < GW; ++Kx) Acc += M[Y * GW + X + Kx - R] * Ker[Kx];
						Tmp[Y * GW + X] = Acc;
					}
				for (int32 Y = 0; Y < GH; ++Y)
					for (int32 X = 0; X < GW; ++X)
					{
						float Acc = 0.f;
						for (int32 Ky = FMath::Max(0, R - Y); Ky <= 2 * R && Y + Ky - R < GH; ++Ky) Acc += Tmp[(Y + Ky - R) * GW + X] * Ker[Ky];
						L[Y * GW + X] += Acc * Gain[Ri];
					}
			}
			float Peak = 0.f;
			for (float V : L) Peak = FMath::Max(Peak, V);
			const float Norm = FMath::Clamp(Peak, 1e-4f, 1.f); // 1 아래면 꼭대기에 맞춰 8비트를 다 쓰고, 넘으면 1 에서 자른다
			Self->TitleLightNorm[Col] = Norm;
			TArray<FColor> Px; Px.SetNumUninitialized(GW * GH);
			for (int32 I = 0; I < GW * GH; ++I) { const uint8 V = (uint8)FMath::RoundToInt(FMath::Clamp(L[I] / Norm, 0.f, 1.f) * 255.f); Px[I] = FColor(V, V, V, 0); } // 알파 0 = 미리 곱한 알파로 더하기
			UTexture2D* Tex = UTexture2D::CreateTransient(GW, GH, PF_B8G8R8A8);
			if (!Tex) continue;
			Tex->SRGB = false;
			Tex->Filter = TF_Bilinear;
			Tex->LODGroup = TEXTUREGROUP_UI;
			Tex->AddToRoot();
			if (FTexturePlatformData* PD = Tex->GetPlatformData())
			{
				void* Dst = PD->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(Dst, Px.GetData(), (SIZE_T)GW * GH * sizeof(FColor));
				PD->Mips[0].BulkData.Unlock();
				Tex->UpdateResource();
			}
			FSlateBrush& Br = Self->TitleLight[Col];
			Br.SetResourceObject(Tex);
			Br.ImageSize = FVector2D(GW, GH);
			Br.DrawAs = ESlateBrushDrawType::Image;
		}
	}
	// 색별 밝기 = 그 색 글자 묶음 밝기의 평균 (꺼진 글자 0)
	float Bc[3] = {0.f, 0.f, 0.f};
	for (int32 I = 0; I < PDLogo::BlockLetters; ++I) Bc[0] += (NB[I] > 0.25f ? NB[I] : 0.f) / PDLogo::BlockLetters;
	for (int32 I = 0; I < PDLogo::ScriptLetters; ++I) Bc[1] += (PB[I] > 0.25f ? PB[I] : 0.f) / PDLogo::ScriptLetters;
	Bc[2] = CB > 0.25f ? CB : 0.f;
	for (int32 Col = 0; Col < 3; ++Col)
	{
		if (!TitleLight.IsValidIndex(Col) || !TitleLight[Col].GetResourceObject() || Bc[Col] <= 0.f) continue;
		const FColor E = FColor::FromHex(Edge[Col]);
		const float Sc = TitleLightNorm[Col] * Bc[Col];
		const FLinearColor Tint = FLinearColor::FromSRGBColor(FColor((uint8)FMath::Min(255.f, E.R * Sc), (uint8)FMath::Min(255.f, E.G * Sc), (uint8)FMath::Min(255.f, E.B * Sc), 255));
		C.Image(C.NextLayer(), &TitleLight[Col], 0.f, 0.f, DesignW, DesignH, Tint);
	}
	C.NextLayer();
	// 벽돌 (D-464 크기 192 × 102): 면마다 반사율 0.7 ~ 1 (검은 상자로 빛을 줄인다), 줄눈은 어둡게(반사율 0.12) + 가운데 흐린 선
	const float BrW = 192.f, BrH = 102.f;
	for (int32 R = -1; 20.f + R * BrH < DesignH; ++R)
		for (int32 Kc = -1; Kc * BrW < DesignW + BrW; ++Kc)
		{
			const uint32 Hs = (uint32)((R + 3) * 131 + (Kc + 3) * 7919) * 2654435761u;
			const float V = 0.7f + 0.3f * ((Hs >> 8) & 1023) / 1023.f;
			const float X = Kc * BrW + (R % 2 ? BrW / 2.f : 0.f), Y = 20.f + R * BrH;
			if (V < 0.99f) C.Box(X, Y, BrW, BrH, FLinearColor(0.f, 0.f, 0.f, 1.f - V));
		}
	const FLinearColor Mortar = Hex(TEXT("06060b"), 0.88f), Brick = Hex(TEXT("3c303a"), 0.45f);
	for (int32 Pass = 0; Pass < 2; ++Pass)
		for (int32 R = -1; 20.f + R * BrH < DesignH; ++R)
		{
			const float Y = 20.f + R * BrH;
			C.Lines({FVector2f(0.f, Y), FVector2f(DesignW, Y)}, Pass ? Brick : Mortar, Pass ? 1.f : 4.f);
			for (int32 Kc = 0; Kc * BrW < DesignW + BrW; ++Kc)
			{
				const float X = Kc * BrW + (R % 2 ? BrW / 2.f : 0.f);
				C.Lines({FVector2f(X, Y), FVector2f(X, Y + BrH)}, Pass ? Brick : Mortar, Pass ? 1.f : 4.f);
			}
		}
}

const FSlateBrush* SPDBoardWidget::Art(EUiArt A) const
{
	const int32 I = (int32)A;
	return UiArt.IsValidIndex(I) && UiArt[I].GetResourceObject() ? &UiArt[I] : nullptr;
}
