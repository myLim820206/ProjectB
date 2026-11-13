// 스킬 트리 노드 아이콘 (D-119): tools/neon/neon.py 의 stat_icon / keystone_icon 과 같은 좌표
// 반지름 r 기준 (-1..1) 좌표를 화면 점으로 바꿔 선 묶음(FLines)을 만든다. 겹친 카드·동전·사람은 뒤 선을 지운다
// 무늬 기호(잡식가)와 광대 모자(조커 +1, 광대의 길)는 붓(FCanvas)이 직접 그리므로 Extra 로 알려 준다
#pragma once

#include "CoreMinimal.h"

#include <initializer_list>

namespace PDIcon
{
using FLine = TArray<FVector2f>;
using FLines = TArray<FLine>;

// 점 변환: 회전 Ang(도) → 축척 Sc → 이동 (OX, OY) → 반지름 R 배 → 중심 (CX, CY)
struct FXf
{
	float CX, CY, R, C, S, OX, OY, Sc;
	FXf(float InCX, float InCY, float InR, float Ang = 0.f, float InOX = 0.f, float InOY = 0.f, float InSc = 1.f)
		: CX(InCX), CY(InCY), R(InR), C(FMath::Cos(FMath::DegreesToRadians(Ang))), S(FMath::Sin(FMath::DegreesToRadians(Ang))), OX(InOX), OY(InOY), Sc(InSc) {}
	FVector2f operator()(float X, float Y) const { return FVector2f(CX + (OX + (X * C - Y * S) * Sc) * R, CY + (OY + (X * S + Y * C) * Sc) * R); }
};

inline FLine Poly(const FXf& P, std::initializer_list<FVector2f> Pts, bool bClose = true)
{
	FLine L;
	for (const FVector2f& Q : Pts) L.Add(P(Q.X, Q.Y));
	if (bClose && L.Num()) { const FVector2f First = L[0]; L.Add(First); }
	return L;
}
inline FLine Seg(const FXf& P, float X0, float Y0, float X1, float Y1) { return {P(X0, Y0), P(X1, Y1)}; }
inline FLine Arc(const FXf& P, float X, float Y, float Rad, float A0, float A1, int32 N = 16, float Sx = 1.f)
{
	FLine L;
	for (int32 K = 0; K <= N; ++K)
	{
		const float A = FMath::DegreesToRadians(A0 + (A1 - A0) * K / N);
		L.Add(P(X + FMath::Cos(A) * Rad * Sx, Y + FMath::Sin(A) * Rad));
	}
	return L;
}
inline FLine Ring(const FXf& P, float X, float Y, float Rad, float Sx = 1.f, int32 N = 28) { return Arc(P, X, Y, Rad, 0.f, 360.f, N, Sx); }
inline FLine RBox(const FXf& P, float X0, float Y0, float X1, float Y1, float Rad, int32 N = 4)
{
	FLine L;
	const float Q[4][3] = {{X1 - Rad, Y0 + Rad, -90.f}, {X1 - Rad, Y1 - Rad, 0.f}, {X0 + Rad, Y1 - Rad, 90.f}, {X0 + Rad, Y0 + Rad, 180.f}};
	for (const auto& C : Q)
		for (int32 K = 0; K <= N; ++K)
		{
			const float A = FMath::DegreesToRadians(C[2] + K * 90.f / N);
			L.Add(P(C[0] + FMath::Cos(A) * Rad, C[1] + FMath::Sin(A) * Rad));
		}
	const FVector2f First = L[0];
	L.Add(First);
	return L;
}
inline FLine Bez(FVector2f A, FVector2f B, FVector2f C, int32 N = 18)
{
	FLine L;
	for (int32 I = 0; I <= N; ++I) { const float T = I / (float)N, U = 1.f - T; L.Add(A * (U * U) + B * (2.f * U * T) + C * (T * T)); }
	return L;
}
inline void Add(FLines& Out, const FLines& In) { Out.Append(In); }

// ---- 가림: 앞 물체(닫힌 윤곽)에 덮이는 선 조각을 지운다
inline bool Inside(FVector2f Pt, const FLine& Poly)
{
	bool bIn = false;
	for (int32 I = 0; I + 1 < Poly.Num(); ++I)
	{
		const FVector2f A = Poly[I], B = Poly[I + 1];
		if ((A.Y > Pt.Y) != (B.Y > Pt.Y) && Pt.X < A.X + (Pt.Y - A.Y) * (B.X - A.X) / (B.Y - A.Y)) bIn = !bIn;
	}
	return bIn;
}
inline FLines Hide(const FLines& Lines, const FLines& Fronts, float Step = 0.6f)
{
	FLines Out;
	for (const FLine& Ln : Lines)
	{
		FLine Cur;
		for (int32 I = 0; I + 1 < Ln.Num(); ++I)
		{
			const FVector2f A0 = Ln[I], B0 = Ln[I + 1];
			const int32 N = FMath::Max(1, (int32)(FVector2f::Distance(A0, B0) / Step));
			for (int32 K = 0; K < N; ++K)
			{
				const FVector2f A = A0 + (B0 - A0) * (K / (float)N), B = A0 + (B0 - A0) * ((K + 1) / (float)N);
				bool bHid = false;
				for (const FLine& F : Fronts) bHid = bHid || Inside((A + B) * 0.5f, F);
				if (bHid) { if (Cur.Num() > 1) Out.Add(Cur); Cur.Reset(); }
				else { if (!Cur.Num()) Cur.Add(A); Cur.Add(B); }
			}
		}
		if (Cur.Num() > 1) Out.Add(Cur);
	}
	return Out;
}
struct FGroup { FLines Lines; FLine Sil; }; // 뒤→앞 순서
inline FLines Stack(const TArray<FGroup>& G)
{
	FLines Out;
	for (int32 I = 0; I < G.Num(); ++I)
	{
		FLines Fronts;
		for (int32 J = I + 1; J < G.Num(); ++J) Fronts.Add(G[J].Sil);
		Add(Out, Fronts.Num() ? Hide(G[I].Lines, Fronts) : G[I].Lines);
	}
	return Out;
}

// ---- 부품
inline FLines Sword(const FXf& P) // 칼날 윤곽 + 홈 + 코등이 + 손잡이 + 끝 구슬 (위가 칼끝)
{
	return {Poly(P, {{0.f, -1.f}, {0.17f, -0.78f}, {0.17f, 0.3f}, {-0.17f, 0.3f}, {-0.17f, -0.78f}}), Seg(P, 0.f, -0.72f, 0.f, 0.22f),
		RBox(P, -0.48f, 0.3f, 0.48f, 0.44f, 0.07f), Poly(P, {{-0.08f, 0.44f}, {0.08f, 0.44f}, {0.08f, 0.78f}, {-0.08f, 0.78f}}), Arc(P, 0.f, 0.9f, 0.12f, 0.f, 360.f, 12)};
}
inline FLines Arrow(const FXf& P) // 날렵한 화살촉 + 살 + 깃 (오른쪽으로)
{
	return {Poly(P, {{1.f, 0.f}, {0.45f, -0.38f}, {0.55f, -0.1f}, {-0.55f, -0.1f}, {-0.55f, 0.1f}, {0.55f, 0.1f}, {0.45f, 0.38f}}),
		Poly(P, {{-0.55f, -0.1f}, {-0.85f, -0.38f}, {-1.f, -0.38f}, {-0.75f, -0.1f}}, false), Poly(P, {{-0.55f, 0.1f}, {-0.85f, 0.38f}, {-1.f, 0.38f}, {-0.75f, 0.1f}}, false)};
}
inline FLine Bolt(const FXf& P) { return Poly(P, {{0.22f, -1.f}, {-0.5f, 0.12f}, {-0.06f, 0.12f}, {-0.3f, 1.f}, {0.5f, -0.18f}, {0.06f, -0.18f}, {0.32f, -1.f}}); }
inline FLine Heart(const FXf& P, float K = 1.f)
{
	FLine L;
	for (int32 I = 0; I <= 360; I += 8)
	{
		const float T = FMath::DegreesToRadians((float)I), St = FMath::Sin(T);
		const float X = 16.f * St * St * St, Y = -(13.f * FMath::Cos(T) - 5.f * FMath::Cos(2.f * T) - 2.f * FMath::Cos(3.f * T) - FMath::Cos(4.f * T));
		L.Add(P(X / 17.f * K, (Y + 2.f) / 17.f * K));
	}
	return L;
}
inline FLines Card(const FXf& P, bool bPip = true)
{
	FLines L = {RBox(P, -0.4f, -0.62f, 0.4f, 0.62f, 0.1f)};
	if (bPip) L.Add(Poly(P, {{0.f, -0.22f}, {0.16f, 0.f}, {0.f, 0.22f}, {-0.16f, 0.f}}));
	return L;
}
inline FLines Cards(const TArray<FXf>& Specs, const TArray<bool>& Pips) // 뒤→앞
{
	TArray<FGroup> G;
	for (int32 I = 0; I < Specs.Num(); ++I) { FLines L = Card(Specs[I], Pips[I]); G.Add({L, L[0]}); }
	return Stack(G);
}
inline FLine Burst(const FXf& P, int32 N = 8, float Ro = 1.f, float Ri = 0.55f, float Rot = 0.f)
{
	FLine L;
	for (int32 K = 0; K < N * 2; ++K)
	{
		const float A = FMath::DegreesToRadians(Rot + K * 180.f / N - 90.f), Rr = K % 2 == 0 ? Ro : Ri;
		L.Add(P(FMath::Cos(A) * Rr, FMath::Sin(A) * Rr));
	}
	const FVector2f First = L[0];
	L.Add(First);
	return L;
}
inline FLines Person(const FXf& P, float X, float Y, float S)
{
	FLine Body = Arc(P, X, Y + 0.45f * S, 0.42f * S, 180.f, 360.f, 14);
	Body.Add(P(X - 0.42f * S, Y + 0.45f * S));
	return {Ring(P, X, Y - 0.32f * S, 0.24f * S, 1.f, 18), Body};
}
inline FLines PersonSil(const FXf& P, float X, float Y, float S)
{
	FLine Body = Arc(P, X, Y + 0.45f * S, 0.5f * S, 180.f, 360.f, 14);
	Body.Add(P(X + 0.5f * S, Y + 0.6f * S)); Body.Add(P(X - 0.5f * S, Y + 0.6f * S)); Body.Add(P(X - 0.5f * S, Y + 0.45f * S));
	return {Ring(P, X, Y - 0.32f * S, 0.32f * S, 1.f, 18), Body};
}
inline FLines Scope(const FXf& P, float Rad, float Gap)
{
	return {Ring(P, 0.f, 0.f, Rad), Seg(P, -1.f, 0.f, -Gap, 0.f), Seg(P, Gap, 0.f, 1.f, 0.f), Seg(P, 0.f, -1.f, 0.f, -Gap), Seg(P, 0.f, Gap, 0.f, 1.f)};
}

// 붓이 따로 그릴 것: 없음 / 광대 모자 / 네 무늬 (잡식가). 둘 다 HatC·HatR 에 중심·크기
enum class EExtra : uint8_t { None, Hat, Suits };
struct FIcon
{
	FLines Lines;
	EExtra Extra = EExtra::None;
	FVector2f HatC = FVector2f::ZeroVector;
	float HatR = 0.f;
};

// EPDStat 번호: 1 SuitDmg 2 SuitRate 3 SuitRange 4 SuitSplash 5 SuitSlow 6 HandDmg 7 HandRate 8 HandSplash 9 AllDmg 10 ChainAdd
// 11 FuseAdd 12 FusedDmg 13 GoldPct 14 HandMax 15 Draw 16 Lives 19 JokerDmg 20 JokerAdd 21 AllRate 22 CritChance 23 CritMult
inline FIcon StatIcon(int32 St, float CX, float CY, float R)
{
	FIcon I;
	FLines& L = I.Lines;
	const FXf P(CX, CY, R);
	switch (St)
	{
	case 1: case 6: L = Sword(FXf(CX, CY, R, 45.f)); break;                                                   // 공격: 칼
	case 9: L = Sword(FXf(CX, CY, R, 45.f, 0.f, 0.f, 0.95f)); Add(L, Sword(FXf(CX, CY, R, -45.f, 0.f, 0.f, 0.95f))); break; // 모든 공격: 엇갈린 칼
	case 12:                                                                                                   // 합성 공격: 쌓인 판 위에 칼
	{
		TArray<FGroup> G;
		for (int32 K = 2; K >= 0; --K)
		{
			const float Y = 0.25f + K * 0.25f;
			const FLine D = Poly(P, {{-0.9f, Y}, {0.f, Y - 0.32f}, {0.9f, Y}, {0.f, Y + 0.32f}});
			G.Add({{D}, D});
		}
		L = Stack(G);
		Add(L, Sword(FXf(CX, CY, R, 0.f, 0.f, -0.35f, 0.62f)));
		break;
	}
	case 19: L = Sword(FXf(CX, CY, R, 45.f, -0.05f, 0.05f, 0.9f)); L.Add(Ring(P, 0.68f, -0.68f, 0.2f, 1.f, 16)); L.Add(Seg(P, 0.55f, -0.68f, 0.81f, -0.68f)); break; // 조커 공격: 칼 + 방울
	case 2: case 7: L = Arrow(FXf(CX, CY, R, -30.f)); break;                                                  // 연사: 화살
	case 21: L = Arrow(FXf(CX, CY, R, -30.f, 0.2f, -0.32f, 0.8f)); Add(L, Arrow(FXf(CX, CY, R, -30.f, -0.2f, 0.32f, 0.8f))); break; // 모든 연사: 화살 둘
	case 3: L = Scope(P, 0.72f, 0.42f); L.Add(Ring(P, 0.f, 0.f, 0.12f, 1.f, 12)); break;                      // 사거리: 조준경
	case 4: case 8: L = {Burst(P, 8, 1.f, 0.58f, 22.5f), Ring(P, 0.f, 0.f, 0.28f, 1.f, 16)}; break;           // 범위: 터지는 별
	case 5:                                                                                                    // 둔화: 눈꽃
	{
		FLine Hex;
		for (int32 K = 0; K <= 6; ++K) { const float A = FMath::DegreesToRadians(30.f + 60.f * K); Hex.Add(P(FMath::Cos(A) * 0.22f, FMath::Sin(A) * 0.22f)); }
		L.Add(Hex);
		for (int32 K = 0; K < 6; ++K)
		{
			const float A = FMath::DegreesToRadians(-90.f + 60.f * K), Ux = FMath::Cos(A), Uy = FMath::Sin(A);
			L.Add(Seg(P, Ux * 0.22f, Uy * 0.22f, Ux, Uy));
			for (float Sg : {1.f, -1.f})
			{
				const float B = A + Sg * FMath::DegreesToRadians(40.f);
				L.Add(Seg(P, Ux * 0.6f, Uy * 0.6f, Ux * 0.6f + FMath::Cos(B) * 0.3f, Uy * 0.6f + FMath::Sin(B) * 0.3f));
			}
		}
		break;
	}
	case 10: { const FXf Q(CX, CY, R, -45.f); L = {RBox(Q, -1.f, -0.32f, 0.15f, 0.32f, 0.3f), RBox(Q, -0.15f, -0.32f, 1.f, 0.32f, 0.3f)}; break; } // 연쇄: 사슬
	case 11:                                                                                                   // 합성 배율: 작은 칸 둘 → 큰 칸
		L = {RBox(P, -1.f, 0.35f, -0.55f, 0.8f, 0.1f), RBox(P, 0.55f, 0.35f, 1.f, 0.8f, 0.1f), RBox(P, -0.42f, -0.95f, 0.42f, -0.11f, 0.14f),
			Burst(FXf(CX, CY, R, 0.f, 0.f, -0.53f, 0.3f), 5, 1.f, 0.45f, 0.f),
			Bez(P(-0.78f, 0.35f), P(-0.75f, -0.05f), P(-0.48f, -0.2f), 8), Poly(P, {{-0.62f, -0.32f}, {-0.46f, -0.2f}, {-0.6f, -0.06f}}, false),
			Bez(P(0.78f, 0.35f), P(0.75f, -0.05f), P(0.48f, -0.2f), 8), Poly(P, {{0.62f, -0.32f}, {0.46f, -0.2f}, {0.6f, -0.06f}}, false)};
		break;
	case 13:                                                                                                   // 골드: 쌓인 동전
	{
		TArray<FGroup> G;
		for (int32 K = 0; K < 3; ++K)
		{
			const float Y = 0.5f - K * 0.38f;
			FLine Side = {P(-0.78f, Y - 0.12f), P(-0.78f, Y + 0.06f)};
			Side.Append(Arc(P, 0.f, Y + 0.06f, 0.26f, 180.f, 0.f, 12, 3.f));
			Side.Add(P(0.78f, Y - 0.12f));
			FLine Sil = Arc(P, 0.f, Y - 0.12f, 0.26f, 180.f, 360.f, 12, 3.f);
			Sil.Add(P(0.78f, Y + 0.06f));
			FLine Low = Arc(P, 0.f, Y + 0.06f, 0.26f, 0.f, 180.f, 12, 3.f);
			Low.RemoveAt(0);
			Sil.Append(Low);
			Sil.Add(P(-0.78f, Y - 0.12f));
			G.Add({{Ring(P, 0.f, Y - 0.12f, 0.26f, 3.f, 24), Side}, Sil});
		}
		L = Stack(G);
		break;
	}
	case 14: L = Cards({FXf(CX, CY, R, -24.f, -0.42f, 0.2f, 0.95f), FXf(CX, CY, R, 24.f, 0.42f, 0.2f, 0.95f), FXf(CX, CY, R, 0.f, 0.f, 0.12f, 0.95f)}, {false, false, true}); break; // 손패 칸
	case 15:                                                                                                   // 드로우: 카드 + 굽은 화살
		L = Card(FXf(CX, CY, R, -8.f, -0.3f, 0.15f, 0.95f));
		L.Add(Bez(P(0.25f, 0.7f), P(0.95f, 0.55f), P(0.75f, -0.6f), 12));
		L.Add(Poly(P, {{0.5f, -0.45f}, {0.78f, -0.85f}, {0.98f, -0.4f}}, false));
		break;
	case 16: L = {Heart(P), Arc(P, -0.38f, -0.25f, 0.22f, 200.f, 280.f, 8)}; break;                           // 목숨: 하트
	case 20:                                                                                                   // 조커 +1: 광대 모자 + 더하기
		I.Extra = EExtra::Hat; I.HatC = FVector2f(CX - R * 0.12f, CY + R * 0.55f); I.HatR = R * 0.95f;
		L = {Seg(P, 0.5f, -0.78f, 1.f, -0.78f), Seg(P, 0.75f, -1.03f, 0.75f, -0.53f)};
		break;
	case 22: L = {Bolt(P)}; break;                                                                             // 치명 확률: 번개
	case 23: L = {Burst(P, 10, 1.f, 0.78f), Bolt(FXf(CX, CY, R, 0.f, 0.f, 0.f, 0.6f))}; break;                // 치명 배율: 별 안에 번개
	default: L = {Ring(P, 0.f, 0.f, 0.4f)}; break;
	}
	return I;
}

// EPDKeystone 순서: 외골수, 잡식가, 큰 판, 작은 판, 대장장이, 광대의 길, 고독한 저격수, 군중
inline FIcon KeystoneIcon(int32 K, float CX, float CY, float R)
{
	FIcon I;
	FLines& L = I.Lines;
	const FXf P(CX, CY, R);
	switch (K)
	{
	case 0: // 외골수: 깎은 보석
		L = {Poly(P, {{-0.9f, -0.3f}, {-0.5f, -0.8f}, {0.5f, -0.8f}, {0.9f, -0.3f}, {0.f, 0.95f}}), Seg(P, -0.9f, -0.3f, 0.9f, -0.3f),
			Poly(P, {{-0.5f, -0.8f}, {-0.25f, -0.3f}, {0.f, 0.95f}}, false), Poly(P, {{0.5f, -0.8f}, {0.25f, -0.3f}, {0.f, 0.95f}}, false),
			Poly(P, {{-0.25f, -0.3f}, {0.f, -0.8f}, {0.25f, -0.3f}}, false)};
		break;
	case 1: I.Extra = EExtra::Suits; I.HatC = FVector2f(CX, CY); I.HatR = R; break; // 잡식가: 네 무늬 (붓이 그림)
	case 2: // 큰 판: 다섯 장 부채
	{
		TArray<FXf> S;
		for (int32 J : {-2, 2, -1, 1, 0}) S.Add(FXf(CX, CY, R, J * 17.f, J * 0.3f, 0.1f + FMath::Abs(J) * 0.06f, 0.82f));
		L = Cards(S, {true, true, true, true, true});
		break;
	}
	case 3: L = Cards({FXf(CX, CY, R, -14.f, -0.3f, 0.f, 0.95f), FXf(CX, CY, R, 14.f, 0.3f, 0.f, 0.95f)}, {true, true}); break; // 작은 판: 두 장
	case 4: // 대장장이: 모루 + 망치
	{
		const FXf H(CX, CY, R, -35.f, 0.1f, -0.38f);
		L = {Poly(P, {{-0.95f, 0.05f}, {0.75f, 0.05f}, {0.75f, 0.25f}, {0.35f, 0.4f}, {0.35f, 0.65f}, {0.6f, 0.9f}, {-0.6f, 0.9f}, {-0.35f, 0.65f}, {-0.35f, 0.4f}, {-0.7f, 0.25f}}),
			RBox(H, -0.5f, -0.2f, 0.15f, 0.2f, 0.06f), Seg(H, 0.15f, 0.f, 0.9f, 0.f)};
		break;
	}
	case 5: I.Extra = EExtra::Hat; I.HatC = FVector2f(CX, CY + R * 0.5f); I.HatR = R * 0.95f; break; // 광대의 길: 광대 모자
	case 6: L = Scope(P, 0.92f, 0.62f); Add(L, Person(P, 0.f, 0.05f, 0.85f)); break;                       // 고독한 저격수: 조준경 속 한 사람
	case 7: // 군중: 세 사람 (양옆이 앞)
	{
		FLines Fronts = PersonSil(P, -0.55f, 0.25f, 0.75f);
		Add(Fronts, PersonSil(P, 0.55f, 0.25f, 0.75f));
		L = Hide(Person(P, 0.f, -0.1f, 0.9f), Fronts);
		Add(L, Person(P, -0.55f, 0.25f, 0.75f));
		Add(L, Person(P, 0.55f, 0.25f, 0.75f));
		break;
	}
	default: break;
	}
	return I;
}
} // namespace PDIcon
