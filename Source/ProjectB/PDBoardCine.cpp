// 완성 연출 (D-314, D-315): 화면 앞 Slate 층에서 아틀라스 선 스프라이트로 그린다 (나이아가라는 판 뒤라 쓰지 않는다, D-092).
// 꽃가루 · 폭죽은 시간으로 위치를 정하는 입자라 상태가 없다. 도는 동안 게임은 멈추고(SPDBoardWidget::Tick), 클릭하면 바로 건너뛴다.
// 설정 '완성 연출': 전부 / 조커만(일반 합성은 끔) / 끔.
#include "PDBoardDraw.h"

namespace
{
const FLinearColor CGold = FLinearColor(FColor::FromHex(TEXT("ffd96a")));
float CineEase(float U) { U = FMath::Clamp(U, 0.f, 1.f); return 1.f - (1.f - U) * (1.f - U) * (1.f - U); }
float CineSeg(float U, float A, float B) { return FMath::Clamp((U - A) / FMath::Max(0.0001f, B - A), 0.f, 1.f); } // U 가 A~B 사이 어디쯤
uint32 CineHash(uint32 X) { X ^= X >> 16; X *= 0x7feb352du; X ^= X >> 15; X *= 0x846ca68bu; X ^= X >> 16; return X; }
float CineRnd(uint32 Seed, int32 I, int32 K) { return (CineHash(Seed * 977u + (uint32)I * 131u + (uint32)K * 7919u) & 0xFFFF) / 65535.f; }
}

void SPDBoardWidget::QueueCine(const FPDEvent& E)
{
	if (Settings.Cine >= 2 || (Game.Phase != EPDPhase::Build && Game.Phase != EPDPhase::Wave)) return; // 끔, 판 밖
	if (E.Kind != FPDEvent::EKind::Fuse && E.Kind != FPDEvent::EKind::JokerFuse && E.Kind != FPDEvent::EKind::JokerMerge && E.Kind != FPDEvent::EKind::Feed && E.Kind != FPDEvent::EKind::JokerXp) return; // 발사 · 맞음 같은 많은 사건은 바로 넘긴다
	if (E.Kind == FPDEvent::EKind::JokerXp && E.B <= 0) return;
	const int32 Cell = CellKey((int32)E.X, (int32)E.Y);
	auto It = Game.Towers.find(Cell);
	if (It == Game.Towers.end()) return;
	const FPDTower& T = It->second;
	FCine Cn;
	Cn.Cell = Cell;
	Cn.Card = T.Card;
	Cn.Level = T.IsJoker() ? T.JLevel : T.Level;
	Cn.Joker = T.Joker;
	Cn.Seed = CineHash((uint32)(Clock * 1000.f) + (uint32)Cell * 31u);
	Cn.Mats = PendingFuseCells;
	switch (E.Kind)
	{
	case FPDEvent::EKind::Fuse: // 합성: 카드가 튀어나와 돌다 펑 + 꽃가루, 레벨이 오를수록 크게 (D-314 ①)
		if (Settings.Cine == 1) return; // 조커만
		Cn.Kind = ECine::Fuse; Cn.Level = E.A; Cn.Dur = 0.8f + 0.1f * FMath::Clamp(E.A - 2, 0, 2);
		break;
	case FPDEvent::EKind::JokerFuse: // 조커 / 상위 조커 만들기 (D-314 ② ③)
		Cn.bNew = E.B != 0;
		if (IsUpperJoker(E.A)) { Cn.Kind = ECine::Upper; Cn.Dur = 2.5f + (Cn.bNew ? 0.5f : 0.f); }
		else { Cn.Kind = ECine::Joker; Cn.Dur = 1.4f; }
		Cn.Joker = E.A;
		break;
	case FPDEvent::EKind::JokerMerge: // 같은 조커 합치기 (D-315): 짧게. 상위 조커끼리는 왕관이 부딪히는 2초
		Cn.Kind = IsUpperJoker(E.A) ? ECine::Upper : ECine::Merge;
		Cn.bSame = IsUpperJoker(E.A);
		Cn.Dur = IsUpperJoker(E.A) ? 2.f : 1.2f;
		Cn.Joker = E.A;
		break;
	case FPDEvent::EKind::Feed:
	case FPDEvent::EKind::JokerXp: // 조커 레벨 10 (기본) · 10레벨마다 (상위): 작은 멈춤 연출 약 1초 (D-315)
	{
		const int32 NewLv = E.A, OldLv = E.A - E.B;
		if (E.B <= 0) return;
		if (IsUpperJoker(T.Joker)) { if (NewLv / 10 == OldLv / 10) return; Cn.Kind = ECine::UpperLv; }
		else { if (!(OldLv < JokerUpLevel && NewLv >= JokerUpLevel)) return; Cn.Kind = ECine::JokerLv; }
		Cn.Level = NewLv; Cn.Dur = 1.f; Cn.Mats.Reset();
		break;
	}
	default: return;
	}
	if (Cines.Num() >= 4) return; // 10배속에서 쌓이지 않게
	Cines.Add(MoveTemp(Cn));
}

void SPDBoardWidget::UpdateCineTails(float Dt)
{
	for (FCine& Tl : CineTails) Tl.T += Dt;
	CineTails.RemoveAll([](const FCine& Tl) { return Tl.T > Tl.Dur + 2.5f; });
}

void SPDBoardWidget::UpdateCine(float Dt)
{
	if (!Cines.Num()) return;
	FCine& Cn = Cines[0];
	const float T0 = Cn.T;
	Cn.T += Dt;
	auto At = [&](float S) { return T0 < S && Cn.T >= S; };
	switch (Cn.Kind) // 소리: 터지는 순간
	{
	case ECine::Fuse: if (At(Cn.Dur * 0.6f)) UiSound(EPDSound::Clear); break;
	case ECine::Joker: case ECine::Merge: if (At(Cn.Dur * 0.55f)) UiSound(EPDSound::Win); break;
	case ECine::Upper: if (At(Cn.Dur * 0.4f) || At(Cn.Dur * 0.62f)) UiSound(EPDSound::Win); break;
	default: if (At(0.3f)) UiSound(EPDSound::Clear); break;
	}
	if (Cn.T >= Cn.Dur || !Game.Towers.count(Cn.Cell)) { if (Cn.T >= Cn.Dur && CineTails.Num() < 4) CineTails.Add(Cn); Cines.RemoveAt(0); } // 끝나면 꼬리로 (파티클만 마저)
}

void SPDBoardWidget::PaintCine(FCanvas& C) const
{
	for (const FCine& Tl : CineTails) PaintCineOne(C, Tl, true); // 끝난 연출의 꽃가루 · 폭죽 꼬리
	if (Cines.Num()) PaintCineOne(C, Cines[0], false);
}

void SPDBoardWidget::PaintCineOne(FCanvas& C, const FCine& Cn, bool bTail) const
{
	const float U = FMath::Clamp(Cn.T / Cn.Dur, 0.f, 1.f);
	const FVector2f Cell = CellCenter(Cn.Cell), Ctr(FX + Cols * CS / 2.f, FY + Rows * CS / 2.f - 10.f);
	const FLinearColor White = FLinearColor::White;
	// 꽃가루 / 폭죽: Seed 로 정한 입자가 시간 S(초)만큼 날아간 자리. Gold = 금색만
	// 꽃가루 · 금가루 (화면 전체 기준, 사용자 2026-11-16): 터진 자리에서 화면 끝까지 퍼지는 몫 + 화면 위 전체에서 쏟아져 내리는 몫. S = 터진 뒤 초
	auto Confetti = [&](FVector2f O, float S, int32 N, float Speed, bool bGoldOnly, const FLinearColor& Col)
	{
		const float Life = 2.2f;
		if (S <= 0.f || S > Life) return;
		static const FLinearColor Pal[5] = {FLinearColor(FColor::FromHex(TEXT("ff6aaa"))), FLinearColor(FColor::FromHex(TEXT("6edcff"))), FLinearColor(FColor::FromHex(TEXT("ffd96a"))), FLinearColor(FColor::FromHex(TEXT("8affc4"))), FLinearColor(FColor::FromHex(TEXT("b48cff")))};
		const float Fade = FMath::Min(1.f, (Life - S) / 0.6f);
		const float Far = FMath::Max(DesignW, DesignH) * (0.55f + Speed / 1200.f); // 화면 끝까지 닿는 거리
		auto Piece = [&](FVector2f P, int32 I)
		{
			const float L = 4.f + 4.f * CineRnd(Cn.Seed, I, 3), R = CineRnd(Cn.Seed, I, 2) * 12.f * S;
			const FLinearColor Pc = bGoldOnly ? CGold : I % 3 == 0 ? Col : Pal[I % 5];
			C.Glow({P - FVector2f(FMath::Cos(R), FMath::Sin(R)) * L, P + FVector2f(FMath::Cos(R), FMath::Sin(R)) * L}, WithAlpha(Pc, Fade), 1.8f, 0.8f * Fade);
		};
		for (int32 I = 0; I < N; ++I) // 터지는 몫: 처음엔 빠르고 점점 느려지며 화면 끝까지, 그다음 떨어짐
		{
			const float A = CineRnd(Cn.Seed, I, 0) * TwoPi, D = Far * (0.3f + 0.7f * CineRnd(Cn.Seed, I, 1)) * (1.f - FMath::Exp(-S * 3.f));
			Piece(O + FVector2f(FMath::Cos(A), FMath::Sin(A)) * D + FVector2f(0.f, 90.f * S * S), I);
		}
		for (int32 I = 0; I < N; ++I) // 쏟아지는 몫: 화면 위 전체 폭에서 흔들리며 내려온다
		{
			const float X0 = CineRnd(Cn.Seed, I + 500, 4) * DesignW, Y0 = -30.f - CineRnd(Cn.Seed, I + 500, 5) * 260.f;
			const float V = 260.f + 220.f * CineRnd(Cn.Seed, I + 500, 6), Sway = 26.f * FMath::Sin(S * 4.f + I);
			Piece(FVector2f(X0 + Sway, Y0 + V * S), I + 500);
		}
	};
	auto Firework = [&](FVector2f O, float S, int32 Rays, float Rad, const FLinearColor& Col)
	{
		if (S <= 0.f || S > 1.1f) return;
		const float E = CineEase(S / 1.1f), Fade = 1.f - S / 1.1f;
		for (int32 K = 0; K < Rays; ++K)
		{
			const float A = TwoPi * K / Rays;
			const FVector2f D(FMath::Cos(A), FMath::Sin(A));
			C.Glow({O + D * Rad * E * 0.55f, O + D * Rad * E + FVector2f(0.f, 18.f * S * S)}, WithAlpha(Col, Fade), 1.8f, 1.1f * Fade);
		}
		if (S < 0.15f) C.SoftDisc(O, Rad * 0.5f, Rad * 0.5f, White, 0.6f * (1.f - S / 0.15f));
	};
	// 연출 중 판 덮개 (D-348): 다른 비활성 덮개처럼 짙게 깔아 튀어나온 카드가 판 위 카드들에 묻히지 않게. 처음 0.12초 동안 스며들듯 짙어진다
	auto Darken = [&](float A) { A *= FMath::Min(1.f, Cn.T / 0.12f); if (A > 0.f && !bTail) C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), A)); };
	auto Crown = [&](FVector2f O, float R, float A) // 금색 왕관
	{
		static const float X[] = {-1.f, -1.f, -0.5f, 0.f, 0.5f, 1.f, 1.f, -1.f}, Y[] = {0.f, -0.8f, -0.4f, -1.f, -0.4f, -0.8f, 0.f, 0.f};
		TArray<FVector2f> P;
		for (int32 K = 0; K < 8; ++K) P.Add(O + FVector2f(X[K], Y[K]) * R);
		C.Glow(P, WithAlpha(CGold, A), 2.2f, 1.2f * A);
	};
	if (bTail) // 끝난 연출: 꽃가루 · 금가루 · 폭죽만 마저 (본 연출과 같은 씨앗 · 시간이라 끊김 없이 이어진다)
	{
		switch (Cn.Kind)
		{
		case ECine::Fuse: Confetti(Ctr, Cn.T - Cn.Dur * 0.6f, 30 + 15 * Cn.Level, 260.f + 60.f * Cn.Level, Cn.Level >= MaxLevel, SuitColor(Cn.Card.Suit)); break;
		case ECine::Joker:
			for (int32 K = 0; K < 5; ++K) Firework(FVector2f(120.f + CineRnd(Cn.Seed, K, 5) * (DesignW - 240.f), 90.f + CineRnd(Cn.Seed, K, 6) * (DesignH * 0.6f - 90.f)), Cn.T - Cn.Dur * (0.58f + 0.08f * K), 16, 130.f, K == 1 ? CGold : JokerPink);
			break;
		case ECine::Upper:
			for (int32 K = 0; K < 8; ++K) Firework(FVector2f(100.f + CineRnd(Cn.Seed, K, 8) * (DesignW - 200.f), 70.f + CineRnd(Cn.Seed, K, 9) * (DesignH * 0.65f - 70.f)), Cn.T - Cn.Dur * (0.55f + 0.04f * K), 18, 160.f, CGold);
			Confetti(Ctr, Cn.T - Cn.Dur * 0.62f, 90, 600.f, true, CGold);
			break;
		case ECine::JokerLv: case ECine::UpperLv: Confetti(Cell, Cn.T - 0.3f, 30, 200.f, true, CGold); break;
		default: break;
		}
		return;
	}
	switch (Cn.Kind)
	{
	case ECine::Fuse:
	{
		const bool bGold = Cn.Level >= MaxLevel;
		const float Big = 2.6f + 0.4f * FMath::Clamp(Cn.Level - 2, 0, 2), Out = CineSeg(U, 0.f, 0.25f), Back = CineSeg(U, 0.78f, 1.f);
		Darken(0.82f * (1.f - Back));
		const FVector2f Mid = FMath::Lerp(Cell, Ctr, CineEase(Out));
		const FVector2f P = FMath::Lerp(Mid, Cell, CineEase(Back));
		const float R = CS * 0.42f * FMath::Lerp(FMath::Lerp(1.f, Big, CineEase(Out)), 1.f, CineEase(Back));
		const float SpinU = CineSeg(U, 0.2f, 0.6f), W = FMath::Abs(FMath::Cos(SpinU * PI * 4.f)); // 빙글빙글
		const FPDCard Cd = Cn.Card;
		if (U < 0.6f && W < 0.5f) C.Glow(FCanvas::RoundRectPoints(P.X - R * W, P.Y - R, FMath::Max(2.f, 2.f * R * W), 2.f * R, R * 0.22f), SuitColor(Cd.Suit), 2.f, 1.f);
		else C.CardShape(Cd, P.X, P.Y, R, 1.f, true, 0.5f + 0.5f * FMath::Sin(Clock * 10.f));
		const float Pop = Cn.T - Cn.Dur * 0.6f;
		if (Pop > 0.f) // 펑: 섬광 + 고리 + 꽃가루 (Lv4 는 금색)
		{
			if (Pop < 0.18f) C.SoftDisc(Ctr, R * 2.2f, R * 2.2f, bGold ? CGold : White, 0.7f * (1.f - Pop / 0.18f));
			C.Glow(FCanvas::CirclePoints(Ctr, R * (1.2f + Pop * 6.f), 40), WithAlpha(bGold ? CGold : SuitColor(Cd.Suit), FMath::Max(0.f, 1.f - Pop * 3.f)), 2.2f, 1.f);
			Confetti(Ctr, Pop, 30 + 15 * Cn.Level, 260.f + 60.f * Cn.Level, bGold, SuitColor(Cd.Suit));
			if (Back < 0.5f) NeonText(C, FString::Printf(TEXT("Lv%d"), Cn.Level), Ctr.X, Ctr.Y + R + 26.f, bGold ? CGold : White, 22.f, 1, 1.f - Back * 2.f);
		}
		break;
	}
	case ECine::Joker:
	case ECine::Merge:
	{
		// 재료 카드들이 화면 앞으로 나와(둘레에 펼쳐짐) 가운데로 모여 하얀 빛과 함께 조커, 폭죽 (D-314 ②)
		const float Gather = CineSeg(U, 0.12f, 0.5f), Back = CineSeg(U, 0.85f, 1.f), Show = CineSeg(U, 0.55f, 0.65f);
		Darken(0.82f * (1.f - Back));
		const int32 N = FMath::Max(2, Cn.Mats.Num());
		for (int32 I = 0; I < Cn.Mats.Num() && U < 0.55f; ++I)
		{
			const float A = TwoPi * I / N - PI / 2.f + Gather * 2.2f;
			const FVector2f Ring = Ctr + FVector2f(FMath::Cos(A), FMath::Sin(A)) * 120.f * (1.f - CineEase(Gather));
			const FVector2f P = FMath::Lerp(Cn.Mats[I], Ring, CineEase(CineSeg(U, 0.f, 0.15f)));
			C.Glow(FCanvas::RoundRectPoints(P.X - 14.f, P.Y - 18.f, 28.f, 36.f, 5.f), Cn.Kind == ECine::Merge ? JokerPink : White, 1.8f, 0.9f);
		}
		const float Fl = Cn.T - Cn.Dur * 0.55f;
		if (Fl > 0.f && Fl < 0.25f) C.SoftDisc(Ctr, 150.f, 150.f, White, 0.85f * (1.f - Fl / 0.25f)); // 하얀 빛
		if (Show > 0.f)
		{
			const FVector2f P = FMath::Lerp(Ctr, Cell, CineEase(Back));
			const float R = FMath::Lerp(46.f, CS * 0.42f, CineEase(Back)) * (0.6f + 0.4f * CineEase(Show));
			C.JokerTower(Cn.Joker, P.X, P.Y, R, 1.f, 0.5f + 0.5f * FMath::Sin(Clock * 8.f));
			if (Back < 0.3f) C.TextCentered(JokerName(Cn.Joker), Ctr.X, Ctr.Y + 70.f, JokerPink, 18.f, true);
			if (Cn.bNew && Back < 0.3f) C.Badge(TEXT("NEW"), Ctr.X + 54.f, Ctr.Y - 54.f, CGold, 1.f);
		}
		if (Cn.Kind == ECine::Joker) // 폭죽 셋
		{
			for (int32 K = 0; K < 5; ++K)
			{
				const FVector2f O(120.f + CineRnd(Cn.Seed, K, 5) * (DesignW - 240.f), 90.f + CineRnd(Cn.Seed, K, 6) * (DesignH * 0.6f - 90.f)); // 화면 전체 어디서나
				Firework(O, Cn.T - Cn.Dur * (0.58f + 0.08f * K), 16, 130.f, K == 1 ? CGold : JokerPink);
			}
		}
		else for (int32 K = 0; K < 2; ++K) Firework(FVector2f(200.f + CineRnd(Cn.Seed, K, 7) * (DesignW - 400.f), 100.f + CineRnd(Cn.Seed, K, 8) * 260.f), Fl - 0.1f * K, 12, 110.f, JokerPink);
		break;
	}
	case ECine::Upper:
	{
		// 상위 조커 (D-314 ③): 판이 어두워짐 → 재료 조커 두 장이 돌며 빨라짐 → 부딪혀 금빛 + 왕관이 내려와 앉음 → 네온 간판에 이름이 전구 하나씩 → 금색 폭죽 · 금가루 → 칸으로
		const float Orbit = CineSeg(U, 0.08f, 0.4f), Crash = U - 0.4f, Back = CineSeg(U, 0.9f, 1.f);
		Darken(0.85f * (1.f - Back));
		const int32 Ui = Cn.Joker - UpperBase;
		if (U < 0.4f)
		{
			const FPDUpperDef& D = UpperDef(FMath::Clamp(Ui, 0, UpperJokerCount - 1));
			const float A = Orbit * Orbit * 14.f, Rad = 130.f * (1.f - Orbit * 0.85f);
			for (int32 K = 0; K < 2; ++K)
			{
				const FVector2f P = Ctr + FVector2f(FMath::Cos(A + K * PI), FMath::Sin(A + K * PI) * 0.6f) * Rad;
				C.JokerTower(Cn.bSame ? Cn.Joker : (K == 0 ? (int32)D.A : (int32)D.B), P.X, P.Y, 30.f, 1.f, 0.6f);
			}
		}
		if (Crash > 0.f && Crash < 0.08f) C.SoftDisc(Ctr, 220.f, 220.f, CGold, 0.9f * (1.f - Crash / 0.08f)); // 금빛 번쩍
		if (U >= 0.4f)
		{
			const FVector2f P = FMath::Lerp(Ctr, Cell, CineEase(Back));
			const float R = FMath::Lerp(52.f, CS * 0.42f, CineEase(Back));
			C.JokerTower(Cn.Joker, P.X, P.Y, R, 1.f, 0.5f + 0.5f * FMath::Sin(Clock * 6.f));
			const float Drop = CineEase(CineSeg(U, 0.42f, 0.55f));
			if (Back < 0.2f) Crown(Ctr + FVector2f(0.f, -R - 8.f - 120.f * (1.f - Drop)), 22.f, 1.f);
			// 대표 11종 소품 (D-315): 둘레를 도는 작은 그림
			const EPDUpperSpecial Sp = JokerSpecial(Cn.Joker);
			if (Sp != EPDUpperSpecial::None && Back < 0.2f)
				for (int32 K = 0; K < 3; ++K)
				{
					const float A = Clock * 1.4f + K * TwoPi / 3.f;
					const FVector2f Q = Ctr + FVector2f(FMath::Cos(A) * 96.f, FMath::Sin(A) * 60.f);
					switch (Sp)
					{
					case EPDUpperSpecial::MadTea: C.Glow({Q + FVector2f(-7, -4), Q + FVector2f(7, -4), Q + FVector2f(5, 5), Q + FVector2f(-5, 5), Q + FVector2f(-7, -4)}, CGold, 1.4f, 0.7f); C.Glow(FCanvas::CirclePoints(Q + FVector2f(9, 0), 3.f, 10), CGold, 1.2f, 0.6f); break; // 찻잔
					case EPDUpperSpecial::WhiteRabbit: C.Glow(FCanvas::CirclePoints(Q, 7.f, 18), White, 1.4f, 0.7f); C.Lines({Q, Q + FVector2f(FMath::Cos(Clock * 6.f), FMath::Sin(Clock * 6.f)) * 5.f}, White, 1.2f); break; // 회중시계
					case EPDUpperSpecial::QueenHearts: C.Shape(EPDSuit::Heart, Q.X, Q.Y, 5.f, FLinearColor(FColor::FromHex(TEXT("ff4a6a"))), 1.3f, 0.7f); break; // 장미 꽃잎
					case EPDUpperSpecial::GrinOnly: { TArray<FVector2f> Arc; for (int32 J = 0; J <= 8; ++J) { const float B = PI * (0.15f + 0.7f * J / 8.f); Arc.Add(Q + FVector2f(FMath::Cos(B) * 9.f, FMath::Sin(B) * 5.f)); } C.Glow(Arc, JokerPink, 1.5f, 0.8f); break; } // 웃는 입
					case EPDUpperSpecial::Dormouse: C.TextCentered(TEXT("z"), Q.X, Q.Y, White, 12.f, true); break;
					case EPDUpperSpecial::Jabberwock: for (int32 J = -1; J <= 1; ++J) C.Glow({Q + FVector2f(J * 4.f - 3.f, -6.f), Q + FVector2f(J * 4.f + 3.f, 6.f)}, CGold, 1.4f, 0.7f); break; // 발톱 자국
					case EPDUpperSpecial::Tweedle: C.Glow(FCanvas::CirclePoints(Q + FVector2f(-5, 0), 4.f, 12), White, 1.2f, 0.6f); C.Glow(FCanvas::CirclePoints(Q + FVector2f(5, 0), 4.f, 12), White, 1.2f, 0.6f); break;
					case EPDUpperSpecial::Mirror: C.Glow(FCanvas::RoundRectPoints(Q.X - 5.f, Q.Y - 8.f, 10.f, 16.f, 3.f), White, 1.3f, 0.7f); C.Lines({Q + FVector2f(-3, 4), Q + FVector2f(3, -4)}, White, 1.f); break; // 거울
					case EPDUpperSpecial::RedQueen: Crown(Q + FVector2f(0.f, 4.f), 6.f, 0.9f); break;
					case EPDUpperSpecial::MockTurtle: { TArray<FVector2f> Sh; for (int32 J = 0; J <= 10; ++J) { const float B = PI * J / 10.f; Sh.Add(Q + FVector2f(-FMath::Cos(B) * 9.f, -FMath::Sin(B) * 6.f)); } C.Glow(Sh, Hex(TEXT("8affc4")), 1.4f, 0.7f); break; } // 등껍질
					case EPDUpperSpecial::Walrus: // 벌어진 굴 껍데기 + 진주 (D-482, tools/neon/neon.py oyster)
					{
						const float OR = 9.f, LA = FMath::DegreesToRadians(32.f);
						TArray<FVector2f> Bowl, Lid;
						for (int32 J = 0; J <= 12; ++J)
						{
							const float B = PI * J / 12.f;
							Bowl.Add(Q + FVector2f(FMath::Cos(B) * OR, FMath::Sin(B) * OR * 0.5f));
							const float LX = -OR + FMath::Cos(B) * OR, LY = -FMath::Sin(B) * OR * 0.3f;
							Lid.Add(Q + FVector2f(OR + LX * FMath::Cos(LA) - LY * FMath::Sin(LA), LX * FMath::Sin(LA) + LY * FMath::Cos(LA)));
						}
						Bowl.Add(Bowl[0]);
						const FLinearColor Ice = Hex(TEXT("dce8ff"));
						C.Glow(Bowl, Ice, 1.4f, 0.7f);
						C.Glow(Lid, Ice, 1.4f, 0.7f);
						C.Glow(FCanvas::CirclePoints(Q + FVector2f(0.f, -OR * 0.08f), OR * 0.2f, 10), Ice, 1.2f, 0.6f);
						break;
					}
					default: break;
					}
				}
			// 네온 간판에 이름이 전구 하나씩 (D-314 ③)
			if (Back < 0.2f && IsUpperJoker(Cn.Joker))
			{
				const FString Nm = UpperName(Ui);
				TArray<float> LB;
				const float Lit = CineSeg(U, 0.55f, 0.75f) * Nm.Len();
				for (int32 K = 0; K < Nm.Len(); ++K) LB.Add(K < Lit ? (FMath::Abs(Lit - K - 0.6f) < 0.3f ? 0.3f : 1.f) : 0.08f); // 켜지며 한 번 지직
				BulbSign(C, Ctr.X, Ctr.Y + 78.f, FMath::Max(260.f, Nm.Len() * 26.f + 60.f), 56.f, Nm, CGold, 24.f, Clock, &LB, false);
				if (Cn.bNew) C.Badge(TEXT("NEW"), Ctr.X + 70.f, Ctr.Y - 70.f, CGold, 1.f);
			}
			for (int32 K = 0; K < 8; ++K) // 금색 폭죽 여러 발 (화면 전체)
			{
				const FVector2f O(100.f + CineRnd(Cn.Seed, K, 8) * (DesignW - 200.f), 70.f + CineRnd(Cn.Seed, K, 9) * (DesignH * 0.65f - 70.f)); // 화면 전체
				Firework(O, Cn.T - Cn.Dur * (0.55f + 0.04f * K), 18, 160.f, CGold);
			}
			Confetti(Ctr, Cn.T - Cn.Dur * 0.62f, 90, 600.f, true, CGold); // 금가루 (화면 전체)
		}
		break;
	}
	case ECine::JokerLv:
	case ECine::UpperLv:
	{
		// 조커 레벨 10 (기본: 금색 고리 한 바퀴 + '강화!' + 열린 능력 한 줄) · 상위 조커 10레벨마다 (간판에 'LvN') (D-315)
		const float Up = CineEase(CineSeg(U, 0.f, 0.2f)), Back = CineSeg(U, 0.85f, 1.f);
		Darken(0.82f * (1.f - Back));
		const FVector2f P = Cell + FVector2f(0.f, -26.f * Up * (1.f - Back));
		const float R = CS * 0.42f * (1.f + 0.9f * Up * (1.f - Back));
		C.JokerTower(Cn.Joker, P.X, P.Y, R, 1.f, 0.8f);
		const float Sweep = CineSeg(U, 0.15f, 0.65f);
		if (Sweep > 0.f && Sweep < 1.f) { TArray<FVector2f> Arc; for (int32 J = 0; J <= 24; ++J) Arc.Add(P + FVector2f(FMath::Cos(-PI / 2.f + TwoPi * Sweep * J / 24.f), FMath::Sin(-PI / 2.f + TwoPi * Sweep * J / 24.f)) * (R + 10.f)); C.Glow(Arc, CGold, 2.4f, 1.2f); }
		if (Cn.Kind == ECine::JokerLv)
		{
			if (U > 0.3f && Back < 0.5f) { NeonText(C, Tr(TEXT("강화!")), P.X, P.Y - R - 18.f, CGold, 20.f, 1, 1.f - Back * 2.f); C.TextCentered(JokerAbility(Cn.Joker), P.X, P.Y + R + 22.f, White, 11.f, true, 360.f); }
		}
		else if (U > 0.3f && Back < 0.5f) BulbSign(C, P.X, P.Y - R - 60.f, 140.f, 44.f, FString::Printf(TEXT("Lv%d"), Cn.Level), CGold, 20.f, Clock, nullptr, false, 1.f - Back * 2.f);
		Confetti(P, Cn.T - 0.3f, 30, 200.f, true, CGold);
		break;
	}
	}
	C.TextCentered(TEXT("클릭: 건너뛰기"), Ctr.X, FY + Rows * CS - 14.f, FLinearColor(1.f, 1.f, 1.f, 0.35f), 10.f, false);
}
