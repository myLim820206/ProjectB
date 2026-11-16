// BAR ALICE 로고 선 (D-309, D-311, 예전 NEON PALLAS D-253 자리). 글꼴 없이 손으로 그린 네온 튜브 — tools/neon/logo_alice.py 와 같은 점
// 좌표: 글자 기준선 y=0, 위가 음수. 글자마다 따로 (글자별 깜빡임)
#pragma once
#include "CoreMinimal.h"

namespace PDLogo
{
	// BAR 블록 글자: 속이 빈 외곽선 (너비 44, 높이 60). 글자 간격 56, 가로는 0.8배로 그린다
	constexpr float BlockAdv = 56.f;
	constexpr int32 BlockLetters = 3;
	constexpr float BlockW = (BlockLetters - 1) * BlockAdv + 44.f * 0.8f; // 단어 폭 (배율 1)
	struct FPoly { const float* P; int32 N; };
	static const float BarB0[] = {0, 0, 0, -60, 31, -60, 41, -52, 41, -39, 35, -31, 44, -23, 44, -9, 35, 0, 0, 0};
	static const float BarB1[] = {12, -49, 27, -49, 29, -46, 29, -40, 27, -37, 12, -37, 12, -49};
	static const float BarB2[] = {12, -25, 30, -25, 32, -22, 32, -14, 30, -11, 12, -11, 12, -25};
	static const float BarA0[] = {0, 0, 16, -60, 28, -60, 44, 0, 32, 0, 29, -12, 15, -12, 12, 0, 0, 0};
	static const float BarA1[] = {18, -23, 26, -23, 22, -42, 18, -23};
	static const float BarR0[] = {0, 0, 0, -60, 31, -60, 41, -51, 41, -35, 33, -27, 44, 0, 31, 0, 22, -24, 12, -24, 12, 0, 0, 0};
	static const float BarR1[] = {12, -49, 27, -49, 29, -46, 29, -38, 27, -35, 12, -35, 12, -49};
	inline int32 BlockPolys(int32 Letter, FPoly Out[3]) // Letter 0..2 = B A R
	{
		if (Letter == 0) { Out[0] = {BarB0, UE_ARRAY_COUNT(BarB0) / 2}; Out[1] = {BarB1, UE_ARRAY_COUNT(BarB1) / 2}; Out[2] = {BarB2, UE_ARRAY_COUNT(BarB2) / 2}; return 3; }
		if (Letter == 1) { Out[0] = {BarA0, UE_ARRAY_COUNT(BarA0) / 2}; Out[1] = {BarA1, UE_ARRAY_COUNT(BarA1) / 2}; return 2; }
		Out[0] = {BarR0, UE_ARRAY_COUNT(BarR0) / 2}; Out[1] = {BarR1, UE_ARRAY_COUNT(BarR1) / 2};
		return 2;
	}

	// Alice 필기체 한 줄 튜브: 조절점 (Catmull-Rom 으로 잇는다). 폭 ScriptW, 대문자 ~86 · 소문자 ~40
	constexpr float ScriptW = 210.f;
	static const float AliA1[] = {0, 4, 12, -24, 27, -58, 40, -82, 46, -82, 50, -60, 53, -30, 57, -8, 64, 0, 74, -6};
	static const float AliA2[] = {8, -30, 26, -38, 48, -38, 62, -32}; // 가로 획 (살짝 휘게)
	static const float AliL[] = {74, -6, 88, -34, 99, -68, 101, -86, 92, -90, 84, -70, 81, -32, 85, -5, 95, 0, 106, -8};
	static const float AliI1[] = {106, -8, 112, -24, 117, -40, 118, -24, 118, -7, 124, 0, 132, -5};
	static const float AliI2[] = {116, -55, 119, -58, 122, -55, 119, -52, 116, -55}; // i 점
	static const float AliC[] = {164, -33, 160, -40, 149, -42, 139, -29, 139, -9, 149, 0, 165, -8};
	static const float AliE[] = {165, -8, 180, -19, 192, -31, 190, -41, 180, -42, 172, -30, 172, -10, 182, 0, 198, -3, 210, -12};
	constexpr int32 ScriptLetters = 5;
	inline int32 ScriptStrokes(int32 Letter, FPoly Out[2])
	{
		switch (Letter)
		{
		case 0: Out[0] = {AliA1, UE_ARRAY_COUNT(AliA1) / 2}; Out[1] = {AliA2, UE_ARRAY_COUNT(AliA2) / 2}; return 2;
		case 1: Out[0] = {AliL, UE_ARRAY_COUNT(AliL) / 2}; return 1;
		case 2: Out[0] = {AliI1, UE_ARRAY_COUNT(AliI1) / 2}; Out[1] = {AliI2, UE_ARRAY_COUNT(AliI2) / 2}; return 2;
		case 3: Out[0] = {AliC, UE_ARRAY_COUNT(AliC) / 2}; return 1;
		default: Out[0] = {AliE, UE_ARRAY_COUNT(AliE) / 2}; return 1;
		}
	}
	// 글자마다 '빠직' 불티가 튀는 자리 (오른쪽 위)
	static const float ScriptSpark[ScriptLetters][2] = {{44, -82}, {100, -86}, {119, -56}, {162, -38}, {191, -38}};

	// 이어지는 글자 획을 조절점 단계에서 합친 줄 (D-299): 글자 끝과 다음 글자 시작이 같은 점이고 밝기가 같으면 한 줄로 → 이음매에서도 곡선이 매끈
	struct FRun { TArray<FVector2f> Pts; int32 Letter; };
	inline TArray<FRun> ScriptRuns(const float* LetterB)
	{
		TArray<FRun> Out;
		FPoly Ps[2];
		for (int32 I = 0; I < ScriptLetters; ++I)
		{
			const int32 N = ScriptStrokes(I, Ps);
			for (int32 J = 0; J < N; ++J)
			{
				TArray<FVector2f> C;
				for (int32 Q = 0; Q < Ps[J].N; ++Q) C.Add(FVector2f(Ps[J].P[Q * 2], Ps[J].P[Q * 2 + 1]));
				FRun* Last = Out.Num() ? &Out.Last() : nullptr;
				if (Last && FVector2f::Distance(Last->Pts.Last(), C[0]) < 0.01f && (!LetterB || FMath::IsNearlyEqual(LetterB[Last->Letter], LetterB[I]))) { C.RemoveAt(0); Last->Pts.Append(C); }
				else Out.Add({MoveTemp(C), I});
			}
		}
		return Out;
	}
	inline TArray<FVector2f> CatmullPts(const TArray<FVector2f>& Cp, FVector2f O, float K, float Mirror = 0.f, int32 Seg = 8)
	{
		const int32 N = Cp.Num();
		auto Pt = [&](int32 I) { return Cp[FMath::Clamp(I, 0, N - 1)]; };
		TArray<FVector2f> R;
		auto Put = [&](FVector2f V) { FVector2f W = O + V * K; if (Mirror > 0.f) W.Y = 2.f * Mirror - W.Y; R.Add(W); };
		for (int32 I = 0; I + 1 < N; ++I)
		{
			const FVector2f P0 = Pt(I - 1), P1 = Pt(I), P2 = Pt(I + 1), P3 = Pt(I + 2);
			for (int32 J = 0; J < Seg; ++J)
			{
				const float T = J / (float)Seg, T2 = T * T, T3 = T2 * T;
				Put(0.5f * ((2.f * P1) + (P2 - P0) * T + (2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * T2 + (3.f * P1 - P0 - 3.f * P2 + P3) * T3));
			}
		}
		Put(Pt(N - 1));
		return R;
	}
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
