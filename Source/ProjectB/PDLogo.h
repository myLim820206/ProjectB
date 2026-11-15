// NEON PALLAS 로고 선 (D-253). 글꼴 없이 손으로 그린 네온 튜브 — tools/neon/logo_strokes.py 와 같은 점
// 좌표: 글자 기준선 y=0, 위가 음수. 글자마다 따로 (글자별 깜빡임)
#pragma once
#include "CoreMinimal.h"

namespace PDLogo
{
	// NEON 블록 글자: 속이 빈 외곽선 (너비 44, 높이 60). 글자 간격 56, 가로는 0.8배로 그린다
	constexpr float NeonAdv = 56.f;
	struct FPoly { const float* P; int32 N; };
	static const float NeonN[] = {0, 0, 0, -60, 12, -60, 32, -22, 32, -60, 44, -60, 44, 0, 32, 0, 12, -38, 12, 0, 0, 0};
	static const float NeonE[] = {0, 0, 0, -60, 42, -60, 42, -49, 12, -49, 12, -36, 36, -36, 36, -25, 12, -25, 12, -11, 42, -11, 42, 0, 0, 0};
	static const float NeonO1[] = {10, 0, 0, -10, 0, -50, 10, -60, 34, -60, 44, -50, 44, -10, 34, 0, 10, 0};
	static const float NeonO2[] = {15, -12, 12, -15, 12, -45, 15, -48, 29, -48, 32, -45, 32, -15, 29, -12, 15, -12};
	inline int32 NeonPolys(int32 Letter, FPoly Out[2]) // Letter 0..3 = N E O N
	{
		if (Letter == 1) { Out[0] = {NeonE, UE_ARRAY_COUNT(NeonE) / 2}; return 1; }
		if (Letter == 2) { Out[0] = {NeonO1, UE_ARRAY_COUNT(NeonO1) / 2}; Out[1] = {NeonO2, UE_ARRAY_COUNT(NeonO2) / 2}; return 2; }
		Out[0] = {NeonN, UE_ARRAY_COUNT(NeonN) / 2};
		return 1;
	}

	// Pallas 필기체 한 줄 튜브: 조절점 (Catmull-Rom 으로 잇는다)
	static const float PalP1[] = {4, 8, 10, -14, 18, -46, 26, -78};
	static const float PalP2[] = {16, -62, 30, -80, 56, -84, 72, -70, 68, -50, 46, -40, 24, -42};
	static const float PalA1[] = {104, -36, 90, -44, 74, -40, 64, -20, 68, -2, 82, 0, 98, -16, 104, -38, 100, -24, 100, -6, 106, 0, 116, -4};
	static const float PalL1[] = {116, -4, 128, -26, 140, -62, 142, -84, 132, -88, 124, -68, 122, -30, 126, -4, 136, 0, 148, -6};
	static const float PalL2[] = {148, -6, 160, -28, 172, -64, 174, -84, 164, -88, 156, -68, 154, -30, 158, -4, 168, 0, 180, -6};
	static const float PalA2[] = {212, -36, 198, -44, 182, -40, 172, -20, 176, -2, 190, 0, 206, -16, 212, -38, 208, -24, 208, -6, 214, 0, 226, -4};
	static const float PalS[] = {226, -4, 236, -18, 244, -40, 250, -28, 258, -14, 252, -2, 236, 2, 224, -6};
	constexpr int32 PallasLetters = 6;
	inline int32 PallasStrokes(int32 Letter, FPoly Out[2])
	{
		switch (Letter)
		{
		case 0: Out[0] = {PalP1, UE_ARRAY_COUNT(PalP1) / 2}; Out[1] = {PalP2, UE_ARRAY_COUNT(PalP2) / 2}; return 2;
		case 1: Out[0] = {PalA1, UE_ARRAY_COUNT(PalA1) / 2}; return 1;
		case 2: Out[0] = {PalL1, UE_ARRAY_COUNT(PalL1) / 2}; return 1;
		case 3: Out[0] = {PalL2, UE_ARRAY_COUNT(PalL2) / 2}; return 1;
		case 4: Out[0] = {PalA2, UE_ARRAY_COUNT(PalA2) / 2}; return 1;
		default: Out[0] = {PalS, UE_ARRAY_COUNT(PalS) / 2}; return 1;
		}
	}
	// 글자마다 '빠직' 불티가 튀는 자리 (오른쪽 위)
	static const float PallasSpark[PallasLetters][2] = {{70, -72}, {104, -38}, {142, -84}, {174, -84}, {212, -38}, {248, -38}};

	// 조절점을 지나는 부드러운 곡선 (끝점 복제)
	inline TArray<FVector2f> Catmull(const FPoly& S, FVector2f O, float K, float Mirror = 0.f, int32 Seg = 8)
	{
		auto Pt = [&](int32 I) { I = FMath::Clamp(I, 0, S.N - 1); return FVector2f(S.P[I * 2], S.P[I * 2 + 1]); };
		TArray<FVector2f> R;
		auto Put = [&](FVector2f V) { FVector2f W = O + V * K; if (Mirror > 0.f) W.Y = 2.f * Mirror - W.Y; R.Add(W); };
		for (int32 I = 0; I + 1 < S.N; ++I)
		{
			const FVector2f P0 = Pt(I - 1), P1 = Pt(I), P2 = Pt(I + 1), P3 = Pt(I + 2);
			for (int32 J = 0; J < Seg; ++J)
			{
				const float T = J / (float)Seg, T2 = T * T, T3 = T2 * T;
				Put(0.5f * ((2.f * P1) + (P2 - P0) * T + (2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * T2 + (3.f * P1 - P0 - 3.f * P2 + P3) * T3));
			}
		}
		Put(Pt(S.N - 1));
		return R;
	}
}
