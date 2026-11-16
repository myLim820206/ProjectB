// 채널 TV (D-316, D-318): 오른쪽 위 흑백 TV (D-279 CRT 시안 D). 판 위력 숫자 · 타워 순위는 뺐고, 다이얼로 채널을 바꾼다.
// CH1 다음 웨이브 · CH2 포커 중계 · CH3 지직 · 숨은 채널(다이얼을 빠르게 7번). 모든 채널 아래에 방송 자막 띠(남은 적 · 자동 시작 · 처치 · 놓침)
#include "PDBoardDraw.h"

void SPDBoardWidget::TvDial()
{
	TvDialClicks.Add(Clock);
	TvDialClicks.RemoveAll([this](float T) { return Clock - T > 2.5f; });
	if (TvDialClicks.Num() >= 7) // 숨은 채널: 2.5초 안에 7번 (D-316)
	{
		TvChannel = 3;
		TvDialClicks.Reset();
		if (!bTvSecret) { bTvSecret = true; SaveProgress(); }
	}
	else TvChannel = TvChannel >= 2 ? 0 : TvChannel + 1;
	TvSwitchAt = Clock;
	UiSound(EPDSound::Move); // 딸깍
	SaveSettings();
}

void SPDBoardWidget::PaintTv(FCanvas& C) const
{
	const FPDGame& G = Game;
	const FLinearColor LavP = Hex(TEXT("a896e1")), Mono = Hex(TEXT("dee8f5"));
	auto MonoA = [&](float A) { return FLinearColor(Mono.R, Mono.G, Mono.B, A); };
	auto Frame2 = [&](float X, float Y, float W, float H, float R, const FLinearColor& Col, float B)
	{
		const TArray<FVector2f> Ro = FCanvas::RoundRectPoints(X, Y, W, H, R), Ri = FCanvas::RoundRectPoints(X + 5.f, Y + 5.f, W - 10.f, H - 10.f, FMath::Max(2.f, R - 5.f));
		C.Glow(Ro, FLinearColor(Col.R * B, Col.G * B, Col.B * B, 1.f), 2.f, 1.f * B);
		C.Lines(Ro, FLinearColor(1.f, 1.f, 1.f, 0.3f * B), 0.6f);
		C.Glow(Ri, FLinearColor(Col.R * B * 0.8f, Col.G * B * 0.8f, Col.B * B * 0.8f, 1.f), 1.1f, 0.45f * B);
	};
	// TV 빛 (D-303 ~ D-305)
	const float TvFl = 0.9f + 0.06f * FMath::Sin(Clock * 7.3f) + 0.04f * FMath::Sin(Clock * 23.1f);
	const FLinearColor TvLight = Hex(TEXT("9cc6ff"));
	C.Spill(FCanvas::RoundRectPoints(RX + 8.f, FY + 8.f, PW - 16.f, PowCardH - 16.f, 16.f), TvLight, 60.f, 0.5f * TvFl);
	C.SoftDisc(FVector2f(RX + PW * 0.45f, FY + PowCardH * 0.5f), PW * 0.95f, PowCardH * 0.85f, TvLight, 0.3f * TvFl);
	// 틀 · 안테나 · 스피커 · 손잡이(위 = 채널 다이얼)
	C.Box(RX, FY, PW, PowCardH, Hex(TEXT("06060b"), 0.85f));
	Frame2(RX, FY, PW, PowCardH, 16.f, LavP, 0.8f);
	const float SX = RX + 12.f, SY = FY + 12.f, SW = PW - 52.f, SH = PowCardH - 24.f;
	C.Box(SX + 4.f, SY + 4.f, SW - 8.f, SH - 8.f, Hex(TEXT("0b0d14"), 1.f));
	C.SoftDisc(FVector2f(SX + SW * 0.5f, SY + SH * 0.5f), SW * 0.55f, SH * 0.55f, TvLight, 0.42f * TvFl);
	C.Glow(FCanvas::RoundRectPoints(SX + 4.f, SY + 4.f, SW - 8.f, SH - 8.f, 18.f), WithAlpha(TvLight, 0.4f * TvFl), 2.f, 0.9f * TvFl);
	C.Glow(FCanvas::RoundRectPoints(SX, SY, SW, SH, 22.f), WithAlpha(LavP, 0.85f), 1.5f, 0.5f);
	{
		const float BX = RX + PW - 54.f, BY = FY;
		C.Glow({FVector2f(BX - 10.f, BY), FVector2f(BX + 10.f, BY), FVector2f(BX + 6.f, BY - 6.f), FVector2f(BX - 6.f, BY - 6.f), FVector2f(BX - 10.f, BY)}, LavP, 1.3f, 0.5f);
		C.Glow({FVector2f(BX - 3.f, BY - 6.f), FVector2f(BX - 18.f, BY - 22.f)}, LavP, 1.1f, 0.5f);
		C.Glow({FVector2f(BX + 3.f, BY - 6.f), FVector2f(BX + 20.f, BY - 21.f)}, LavP, 1.1f, 0.5f);
	}
	for (int32 K = 0; K < 4; ++K) C.Lines({FVector2f(RX + PW - 30.f + K * 5.f, FY + 30.f), FVector2f(RX + PW - 30.f + K * 5.f, FY + 70.f)}, WithAlpha(LavP, 0.4f), 1.f);
	C.TextCentered(TEXT("CH"), RX + PW - 22.f, FY + 88.f, WithAlpha(LavP, 0.8f), 7.f, true);
	C.TextCentered(TvChannel == 3 ? FString(TEXT("??")) : FString::Printf(TEXT("%02d"), TvChannel + 1), RX + PW - 22.f, FY + 100.f, Mono, 9.f, true);
	{
		const FVector2f Kc = TvDialPos();
		const bool bHov = FVector2f::Distance(Mouse, Kc) <= TvDialR + 3.f;
		C.BtnFill(Kc.X - TvDialR, Kc.Y - TvDialR, TvDialR * 2.f, TvDialR * 2.f, TvDialR, LavP, bHov); // 누르는 다이얼 (D-328 ②)
		C.Glow(FCanvas::CirclePoints(Kc, TvDialR - 3.f, 20), bHov ? FLinearColor::White : LavP, bHov ? 1.6f : 1.2f, bHov ? 0.8f : 0.4f);
		const float A = FMath::DegreesToRadians(-150.f + TvChannel * 60.f); // 채널마다 돌아간 다이얼
		C.Lines({Kc, Kc + FVector2f(FMath::Cos(A), FMath::Sin(A)) * (TvDialR - 4.f)}, bHov ? FLinearColor::White : LavP, 1.4f);
		const FVector2f K2 = Kc + FVector2f(0.f, 26.f); // 아래 손잡이 = 장식 (소리 크기)
		C.Glow(FCanvas::CirclePoints(K2, 8.f, 20), LavP, 1.2f, 0.4f);
		C.Lines({K2, K2 + FVector2f(FMath::Cos(0.8f), FMath::Sin(0.8f)) * 6.f}, LavP, 1.2f);
	}
	// 방송 자막 띠 (D-318): 모든 채널 아래 같은 띠. 전구 16개(남은 비율 / 자동 시작까지 초록으로 차오름) + 글 + 처치 · 놓침
	const float TkH = 34.f, TkY = SY + SH - 6.f - TkH, CX0 = SX + 10.f, CW = SW - 20.f, ContentBot = TkY - 4.f;
	auto Ticker = [&]()
	{
		C.Box(SX + 6.f, TkY, SW - 12.f, TkH, FLinearColor(0.f, 0.f, 0.f, 0.45f));
		C.Lines({FVector2f(SX + 8.f, TkY), FVector2f(SX + SW - 8.f, TkY)}, MonoA(0.35f), 1.f);
		const bool bWave = G.Phase == EPDPhase::Wave, bAuto = G.Phase == EPDPhase::Build && G.AutoStartT >= 0.f;
		float Frac = 0.f;
		FString Msg;
		FLinearColor MsgCol = Mono;
		if (bWave) { const int32 Left = G.WaveLeft(), Tot = FMath::Max(1, G.WaveTotal); Frac = FMath::Clamp(Left / (float)Tot, 0.f, 1.f); Msg = TrF(TEXT("남은 적 %d/%d"), Left, G.WaveTotal); }
		else if (bAuto) { Frac = 1.f - FMath::Clamp(G.AutoStartT / AutoStartDelayFor(G.Wave + 1), 0.f, 1.f); Msg = TrF(TEXT("%d초 뒤 자동"), FMath::CeilToInt(G.AutoStartT)); MsgCol = Hex(TEXT("9cffb0")); }
		else Msg = TrF(TEXT("웨이브 %d 준비"), G.Wave + 1);
		C.Text(Msg, CX0, TkY + 13.f, MsgCol, 10.f, 0, true, CW * 0.55f);
		C.Text(TrF(TEXT("처치 %d · 놓침 %d"), G.Kills, G.Leaks), CX0 + CW, TkY + 13.f, MonoA(0.7f), 9.f, 2, false, CW * 0.45f);
		const int32 NBulb = 16, On = FMath::RoundToInt(NBulb * Frac);
		const float Step = CW / NBulb;
		for (int32 I = 0; I < NBulb; ++I) Bulb(C, FVector2f(CX0 + Step * (I + 0.5f), TkY + 25.f), 2.6f, I < On, bAuto ? Hex(TEXT("9cffb0")) : Hex(TEXT("ffe196")));
	};
	// 채널 표시 (오른쪽 위)
	auto ChTag = [&](const FString& S) { C.Text(S, SX + SW - 12.f, SY + 20.f, MonoA(0.8f), 9.f, 2, true); };
	const float Sw = Clock - TvSwitchAt;
	if (Sw >= 0.f && Sw < 0.2f) // 채널을 바꾸는 0.2초: 화면 전체 지직
	{
		FRandomStream Ns(FMath::FloorToInt(Clock * 60.f));
		for (int32 K = 0; K < 260; ++K) C.Box(Ns.FRandRange(SX + 6.f, SX + SW - 8.f), Ns.FRandRange(SY + 6.f, SY + SH - 8.f), Ns.FRandRange(1.f, 3.f), 1.f, MonoA(Ns.FRandRange(0.2f, 0.7f)));
	}
	else if (TvChannel == 0) // CH1 다음 화 예고 (사용자 2026-11-16, D-344): TV 드라마처럼 "다음 화 예고 · 제N화" + 제목(한 글자씩 찍힘) + 나오는 적
	{
		const int32 NW = G.Phase == EPDPhase::Wave ? G.Wave : G.Wave + 1;
		const bool bLast = G.Goal() > 0 && NW == G.Goal(), bNow = G.Phase == EPDPhase::Wave;
		const FString Ep = bLast ? FString(Tr(TEXT("최종화"))) : TrF(TEXT("제%d화"), NW);
		const FLinearColor Gold = Hex(TEXT("ffd26e"));
		C.Text(bNow ? TrF(TEXT("지금 방송 중 · %s"), *Ep) : TrF(TEXT("다음 화 예고 · %s"), *Ep), CX0 + (bNow ? 9.f : 0.f), SY + 20.f, bNow ? Hex(TEXT("ff8a8a")) : Gold, 9.f, 0, true, CW - 49.f);
		if (bNow && FMath::Frac(Clock * 1.2f) < 0.6f) C.Lines(FCanvas::CirclePoints(FVector2f(CX0 + 2.5f, SY + 16.5f), 2.f, 8), Hex(TEXT("ff6a6a")), 2.f); // 방송 중 빨간 점
		ChTag(TEXT("CH1"));
		const int32 NShow = FMath::Clamp(FMath::FloorToInt((Clock - TvTitleAt) * 14.f), 0, TvTitle.Len()); // 한 글자씩
		const FString Ttl = TvTitle.Left(NShow) + ((NShow < TvTitle.Len() && FMath::Frac(Clock * 4.f) < 0.5f) ? TEXT("_") : TEXT(""));
		const bool bBossEp = TvMix.Num() && EnemyDef(TvMix[0].T).bBoss;
		C.Text(FString(TEXT("“")) + Ttl + (NShow >= TvTitle.Len() ? TEXT("”") : TEXT("")), CX0 + CW / 2.f, SY + 42.f, bBossEp ? FLinearColor::White : Mono, 13.f, 1, true, CW);
		C.Lines({FVector2f(CX0 + CW * 0.2f, SY + 50.f), FVector2f(CX0 + CW * 0.8f, SY + 50.f)}, MonoA(0.35f), 1.f);
		float Y = SY + 58.f;
		for (int32 I = 0; I < TvMix.Num() && Y < ContentBot - 12.f; ++I)
		{
			const FTvMix& M = TvMix[I];
			const bool bBoss = EnemyDef(M.T).bBoss;
			C.CardSoldier(SoldierShape(M.T), FVector2f(CX0 + 10.f, Y + 8.f), bBoss ? 9.f : 7.f, FMath::Frac(Clock * 1.5f), MonoA(bBoss ? 1.f : 0.85f));
			C.Text(EnemyName(M.T), CX0 + 26.f, Y + 4.f, MonoA(bBoss ? 1.f : 0.88f), 10.f, 0, bBoss, CW * 0.6f);
			C.Text(FString::Printf(TEXT("×%d"), M.N), CX0 + CW, Y + 4.f, MonoA(0.9f), 10.f, 2, true);
			if (bBoss) { C.Text(BossSkillText(M.T), CX0 + 26.f, Y + 17.f, MonoA(0.65f), 8.5f, 0, false, CW - 30.f); Y += 13.f; }
			Y += 18.f;
		}
		Ticker();
	}
	else if (TvChannel == 1) // CH2 포커 중계: 카드 다섯 장이 차례로 뒤집히고 자막, 가끔 화면이 흔들린다
	{
		static const TCHAR* Hands[] = {TEXT("ROYAL FLUSH!"), TEXT("FOUR OF A KIND"), TEXT("FULL HOUSE"), TEXT("STRAIGHT"), TEXT("FLUSH")};
		static const TCHAR* Ranks[5][5] = {{TEXT("10"), TEXT("J"), TEXT("Q"), TEXT("K"), TEXT("A")}, {TEXT("9"), TEXT("9"), TEXT("9"), TEXT("9"), TEXT("4")}, {TEXT("K"), TEXT("K"), TEXT("K"), TEXT("7"), TEXT("7")}, {TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9")}, {TEXT("2"), TEXT("6"), TEXT("8"), TEXT("J"), TEXT("A")}};
		const float Cyc = 6.f, U = FMath::Fmod(Clock, Cyc);
		const int32 Hi = FMath::FloorToInt(Clock / Cyc) % 5;
		const float Jit = FMath::Fmod(Clock, 9.3f) < 0.15f ? FMath::Sin(Clock * 90.f) * 2.f : 0.f; // 가끔 흔들림
		C.Text(TEXT("LIVE · LATE NIGHT POKER"), CX0 + Jit, SY + 20.f, Mono, 9.f, 0, true, CW - 40.f);
		ChTag(TEXT("CH2"));
		const float CdW = 24.f, CdH = 34.f, Gp = (CW - 5.f * CdW) / 4.f, CY0 = SY + 40.f;
		for (int32 K = 0; K < 5; ++K)
		{
			const float Flip = FMath::Clamp((U - 0.4f - K * 0.45f) / 0.25f, 0.f, 1.f), Wk = FMath::Abs(FMath::Cos(Flip * PI)) * CdW;
			const float X = CX0 + K * (CdW + Gp) + (CdW - Wk) / 2.f + Jit;
			C.Lines(FCanvas::RoundRectPoints(X, CY0, FMath::Max(2.f, Wk), CdH, 3.f), MonoA(0.85f), 1.f);
			if (Flip >= 0.5f) { C.TextCentered(Ranks[Hi][K], X + Wk / 2.f, CY0 + 12.f, Mono, 10.f, true); C.Shape(Hi == 4 ? EPDSuit::Spade : (EPDSuit)(K % 4), X + Wk / 2.f, CY0 + 25.f, 4.f, Mono, 1.f, 0.2f); }
			else for (int32 L = 1; L < 4; ++L) C.Lines({FVector2f(X + 3.f, CY0 + L * CdH / 4.f), FVector2f(X + Wk - 3.f, CY0 + L * CdH / 4.f)}, MonoA(0.3f), 1.f); // 뒷면 무늬
		}
		if (U > 3.f) { const float A = FMath::Min(1.f, (U - 3.f) * 3.f); NeonText(C, Hands[Hi], CX0 + CW / 2.f + Jit, CY0 + CdH + 28.f, Mono, 15.f, 1, A); }
		Ticker();
	}
	else if (TvChannel == 2) // CH3 지직
	{
		FRandomStream Ns(FMath::FloorToInt(Clock * 24.f));
		for (int32 K = 0; K < 420; ++K) C.Box(Ns.FRandRange(SX + 6.f, SX + SW - 8.f), Ns.FRandRange(SY + 6.f, ContentBot), Ns.FRandRange(1.f, 2.5f), 1.f, MonoA(Ns.FRandRange(0.1f, 0.55f)));
		ChTag(TEXT("CH3"));
		Ticker();
	}
	else // 숨은 채널: 늦은 밤 로고 방송
	{
		PaintLogoBadge(C, SX + SW / 2.f - 2.f, SY + 22.f, 0.38f); // 배치 C 는 옆으로 넓어서 조금 작게 (D-346)
		C.TextCentered(TEXT("LATE NIGHT POKER"), SX + SW / 2.f, ContentBot - 10.f, MonoA(0.85f), 9.f, true, CW);
		ChTag(TEXT("CH ??"));
		Ticker();
	}
	// 흑백 TV 결: 주사선 → 굴러가는 밝은 띠 → 잡음 점 → 가장자리 어둡게
	for (float Ly = SY + 6.f; Ly < SY + SH - 6.f; Ly += 3.f) C.Box(SX + 6.f, Ly, SW - 12.f, 1.f, FLinearColor(0.f, 0.f, 0.f, 0.32f));
	const float BandY = SY + FMath::Fmod(Clock * 22.f, SH - 20.f) + 6.f;
	for (int32 K = 0; K < 12; ++K) C.Box(SX + 6.f, BandY + K, SW - 12.f, 1.f, MonoA(0.035f * (1.f - FMath::Abs(K - 6) / 6.f)));
	FRandomStream Ns(FMath::FloorToInt(Clock * 14.f));
	for (int32 K = 0; K < 36; ++K) C.Box(Ns.FRandRange(SX + 8.f, SX + SW - 8.f), Ns.FRandRange(SY + 8.f, SY + SH - 8.f), 1.f, 1.f, MonoA(Ns.FRandRange(0.12f, 0.35f)));
	for (int32 K = 0; K < 6; ++K) C.Lines(FCanvas::RoundRectPoints(SX + 2.f + K, SY + 2.f + K, SW - 4.f - 2.f * K, SH - 4.f - 2.f * K, 20.f), FLinearColor(0.f, 0.f, 0.f, 0.12f * (6 - K) / 6.f), 1.f);
}

// 주크박스 (D-319)
void SPDBoardWidget::JukeStep(int32 Dir)
{
	JukeTrack = ((JukeTrack + Dir) % JazzTrackCount + JazzTrackCount) % JazzTrackCount;
	JukeSwitchAt = Clock; // 레코드 바꾸는 움직임
	JukeSwitchDir = Dir;
	UiSound(EPDSound::Move);
	SaveSettings();
}

// 주크박스 (D-319 → 실제 주크박스, 사용자 2026-11-16): 본체 하나 안에 다 있다.
// 위 아치: 무지개 네온 튜브 셋 + 둘레를 도는 전구 / 유리창: 도는 레코드 + NOW PLAYING + 흐르는 곡 이름 / 선곡 버튼: ◀ 번호 ▶ (누름)
// 양옆 거품 튜브(올라가는 방울), 스피커 그릴(박자에 맞춰 떨림), 받침. 곡을 바꾸면 0.7초 동안 레코드가 옆으로 빠졌다 새 레코드가 들어오고 집게 팔이 움직인다
void SPDBoardWidget::PaintJukebox(FCanvas& C) const
{
	const FLinearColor Gold = Hex(TEXT("ffd26e")), Pink = Hex(TEXT("ff6aaa")), Cyan = Hex(TEXT("6edcff")), Lav = Hex(TEXT("a896e1")), Fg = Hex(TEXT("dce4f0")), Bulbc = Hex(TEXT("ffe196")), Bg = Hex(TEXT("06060b"), 0.92f);
	const bool bBoss = WantBossMusic();
	const float Bpm = bBoss ? 132.f : JazzTrackBpm(JukeTrack), BeatN = Clock * Bpm / 60.f, Beat = FMath::Max(0.f, FMath::Cos(BeatN * TwoPi));
	const float X0 = JukeX0, W = JukeW, Y0 = JukeY0, CX = X0 + W / 2.f, Bot = Y0 + JukeH;
	const float ArchH = 26.f, ArchY = Y0 + ArchH; // 위 아치(타원) 밑선
	auto Arch = [&](float In, float Down) { TArray<FVector2f> P; const float Rx = W / 2.f - In, Ry = ArchH - In; P.Add(FVector2f(CX - Rx, ArchY + Down)); for (int32 A = 0; A <= 24; ++A) { const float T = PI + A * PI / 24.f; P.Add(FVector2f(CX + FMath::Cos(T) * Rx, ArchY + FMath::Sin(T) * Ry)); } P.Add(FVector2f(CX + Rx, ArchY + Down)); return P; };
	// 몸통 (속 어둡게) + 테두리 + 받침
	C.Box(X0, ArchY, W, Bot - 6.f - ArchY, Bg);
	for (float Yy = 0.f; Yy < ArchH; Yy += 2.f) { const float Rx = W / 2.f * FMath::Sqrt(FMath::Max(0.f, 1.f - FMath::Square((ArchH - Yy) / ArchH))); C.Box(CX - Rx, Y0 + Yy, Rx * 2.f, 2.f, Bg); }
	TArray<FVector2f> Body = Arch(0.f, Bot - 6.f - ArchY);
	Body.Add(Body[0]);
	C.Glow(Body, Lav, 1.6f, 0.8f);
	C.Glow({FVector2f(X0 - 3.f, Bot - 6.f), FVector2f(X0 + W + 3.f, Bot - 6.f), FVector2f(X0 + W + 1.f, Bot), FVector2f(X0 - 1.f, Bot), FVector2f(X0 - 3.f, Bot - 6.f)}, Lav, 1.3f, 0.6f);
	// 무지개 튜브 셋 (박자에 맞춰 차례로 밝아짐) — 양옆으로 내려와 몸통 옆 기둥이 된다
	const FLinearColor Tube[3] = {Pink, Gold, Cyan};
	for (int32 I = 0; I < 3; ++I)
	{
		const float Br = 0.7f + 0.3f * FMath::Max(0.f, FMath::Cos((BeatN - I * 0.33f) * TwoPi));
		C.Glow(Arch(4.f + I * 3.5f, Bot - 14.f - ArchY), WithAlpha(Tube[I], Br), 1.3f, 0.85f * Br);
	}
	// 아치 둘레를 도는 전구 (보스 웨이브는 빨갛게)
	for (int32 I = 0; I < 11; ++I)
	{
		const float T = PI + (I + 0.5f) * PI / 11.f;
		const bool bOn = (I + FMath::FloorToInt(BeatN * 2.f)) % 4 == 0;
		Bulb(C, FVector2f(CX + FMath::Cos(T) * (W / 2.f + 3.f), ArchY + FMath::Sin(T) * (ArchH + 3.f)), 1.7f, bOn, bBoss ? Hex(TEXT("ff6a6a")) : Bulbc);
	}
	// 양옆 거품 튜브: 방울이 올라간다
	for (const float S : {-1.f, 1.f})
	{
		const float TX = CX + S * (W / 2.f - 17.f);
		for (int32 K = 0; K < 4; ++K)
		{
			const float U = FMath::Frac(Clock * 0.6f + K * 0.25f + (S > 0.f ? 0.12f : 0.f));
			C.Lines(FCanvas::CirclePoints(FVector2f(TX, Bot - 16.f - U * 44.f), 1.3f + 0.6f * U, 8), WithAlpha(S < 0.f ? Pink : Cyan, 0.9f * (1.f - U)), 1.f);
		}
	}
	// 유리창: 도는 레코드 + 곡 정보 (곡을 바꾸면 레코드가 옆으로 빠졌다 새 레코드가 들어온다)
	const float GX = X0 + 24.f, GY = ArchY + 2.f, GW = W - 48.f, GH = 30.f;
	C.Box(GX, GY, GW, GH, Hex(TEXT("0b0d14"), 1.f));
	C.Glow(FCanvas::RoundRectPoints(GX, GY, GW, GH, 6.f), Fg, 1.1f, 0.45f);
	const float Sw = Clock - JukeSwitchAt, SwU = FMath::Clamp(Sw / 0.7f, 0.f, 1.f);
	const float RecK = Sw < 0.7f ? (SwU < 0.5f ? SwU * 2.f : (1.f - SwU) * 2.f) : 0.f; // 0 → 1 → 0: 레코드가 유리창 밑으로 완전히 내려갔다 새 레코드가 올라옴 (사용자 2026-11-16)
	const float RecEase = RecK * RecK * (3.f - 2.f * RecK), ClipY = GY + GH - 2.f;
	const FVector2f RC(GX + 12.f, GY + GH / 2.f + RecEase * (GH / 2.f + 11.f));
	auto RecLines = [&](const TArray<FVector2f>& P, const FLinearColor& Col) // 유리창 아래로 나간 부분은 자른다
	{
		TArray<FVector2f> Run;
		for (int32 I = 0; I + 1 < P.Num(); ++I)
		{
			FVector2f A = P[I], B = P[I + 1];
			if (A.Y > ClipY && B.Y > ClipY) { if (Run.Num() > 1) C.Lines(Run, Col, 1.f); Run.Reset(); continue; }
			if (A.Y > ClipY) A = A + (B - A) * ((A.Y - ClipY) / (A.Y - B.Y));
			if (B.Y > ClipY) B = A + (B - A) * ((ClipY - A.Y) / (B.Y - A.Y));
			if (Run.Num() == 0 || FMath::Abs(Run.Last().X - A.X) + FMath::Abs(Run.Last().Y - A.Y) > 0.01f) { if (Run.Num() > 1) C.Lines(Run, Col, 1.f); Run.Reset(); Run.Add(A); }
			Run.Add(B);
		}
		if (Run.Num() > 1) C.Lines(Run, Col, 1.f);
	};
	RecLines(FCanvas::CirclePoints(RC, 9.f, 20), Fg);
	RecLines(FCanvas::CirclePoints(RC, 3.f, 10), Pink);
	const float RA = Clock * (Bpm / 60.f) * PI * 0.5f;
	RecLines({RC + FVector2f(FMath::Cos(RA), FMath::Sin(RA)) * 4.f, RC + FVector2f(FMath::Cos(RA), FMath::Sin(RA)) * 8.f}, WithAlpha(Fg, 0.7f));
	const float ArmA = FMath::DegreesToRadians(Sw < 0.7f ? 70.f - 50.f * (1.f - FMath::Abs(SwU * 2.f - 1.f)) : 40.f); // 집게 팔: 바꿀 때 들렸다 내려옴
	const FVector2f Pv(GX + 26.f, GY + 4.f);
	C.Lines({Pv, Pv + FVector2f(-FMath::Cos(ArmA), FMath::Sin(ArmA)) * 12.f}, Gold, 1.1f);
	const float TxX = GX + 30.f, TxW = GW - 34.f;
	C.Text(bBoss ? TEXT("BOSS THEME") : TEXT("NOW PLAYING"), TxX, GY + 9.f, bBoss ? Hex(TEXT("ff6a6a")) : Gold, 7.f, 0, true, TxW);
	const FString Nm = bBoss ? FString(Tr(TEXT("보스 웨이브"))) : FString(UTF8_TO_TCHAR(JazzTrackName(JukeTrack)));
	const float NmW = FCanvas::Measure(Nm, FCanvas::Font(9.f, true)).X, NmY = GY + 22.f;
	if (Sw < 0.35f) {} // 레코드가 빠지는 동안 이름은 비움
	else if (NmW <= TxW) C.Text(Nm, TxX, NmY, Fg, 9.f, 0, true);
	else // 길면 흐른다 (유리창 밖은 덮개로 가림)
	{
		const float Loop = NmW + 24.f, Off = FMath::Fmod(FMath::Max(0.f, Sw - 1.2f) * 22.f, Loop); // 곡을 바꾸면 처음부터, 1초쯤 멈췄다 흐름
		for (int32 K = 0; K < 2; ++K) C.Text(Nm, TxX - Off + K * Loop, NmY, Fg, 9.f, 0, true);
		C.Box(GX + 22.f, NmY - 10.f, 8.f, 13.f, Hex(TEXT("0b0d14"), 1.f));
		C.Box(GX + GW - 2.f, NmY - 10.f, 26.f, 13.f, Bg);
		C.Box(GX - 26.f, NmY - 10.f, 26.f, 13.f, Bg);
	}
	// 선곡 버튼 줄: ◀ 번호 ▶ (누르는 버튼은 눌린 듯 살짝 내려감)
	for (int32 Dir = -1; Dir <= 1; Dir += 2)
	{
		const float BtX = JukeBtnX(Dir);
		const bool bHov = InRect(Mouse, BtX, JukeBtnY, JukeBtnS, JukeBtnS), bPress = Sw < 0.15f && JukeSwitchDir == Dir;
		const float Dy = bPress ? 1.5f : 0.f;
		C.BtnFill(BtX, JukeBtnY + 2.f + Dy, JukeBtnS, JukeBtnS - 4.f, RadPill(JukeBtnS - 4.f), Pink, bHov || bPress);
		C.Glow(FCanvas::RoundRectPoints(BtX, JukeBtnY + 2.f + Dy, JukeBtnS, JukeBtnS - 4.f, RadPill(JukeBtnS - 4.f)), bHov ? Pink : WithAlpha(Pink, 0.7f), bHov ? 1.4f : 1.1f, bHov ? 0.9f : 0.4f);
		const FVector2f Cc(BtX + JukeBtnS / 2.f, JukeBtnY + JukeBtnS / 2.f + Dy);
		C.Lines({Cc + FVector2f(Dir * 3.f, 0.f), Cc + FVector2f(-Dir * 2.f, -3.5f), Cc + FVector2f(-Dir * 2.f, 3.5f), Cc + FVector2f(Dir * 3.f, 0.f)}, bHov ? FLinearColor::White : Fg, 1.1f);
	}
	for (int32 I = 0; I < JazzTrackCount; ++I) // 곡 번호 칸 (지금 곡이 금색으로 켜짐)
	{
		const float KX = CX - (JazzTrackCount - 1) * 4.5f + I * 9.f;
		C.Glow(FCanvas::RoundRectPoints(KX - 3.f, JukeBtnY + 5.f, 6.f, 6.f, 1.5f), I == JukeTrack ? Gold : WithAlpha(Lav, 0.5f), 1.f, I == JukeTrack ? 0.9f : 0.2f);
	}
	// 스피커 그릴: 박자에 맞춰 살짝 떨림
	for (int32 I = 0; I < 9; ++I)
	{
		const float SX = CX - 32.f + I * 8.f, Lh = 8.f + 3.f * Beat * (I % 2 ? 1.f : 0.6f);
		C.Lines({FVector2f(SX, Bot - 11.f - Lh), FVector2f(SX, Bot - 11.f)}, WithAlpha(Lav, 0.5f + 0.3f * Beat), 1.f);
	}
}

// 도전 과제 알림 (B8): 화면 위 가운데에 금색 알약이 내려왔다 3초 뒤 올라간다
void SPDBoardWidget::PaintAchToast(FCanvas& C) const
{
	if (!AchToast.Num()) return;
	const float T = AchToastT, In = FMath::Clamp(T / 0.3f, 0.f, 1.f), Out = FMath::Clamp((T - 2.8f) / 0.4f, 0.f, 1.f);
	const float Y = -50.f + 70.f * (1.f - (1.f - In) * (1.f - In)) - 70.f * Out * Out, W = 380.f, H = 44.f, X = DesignW / 2.f - W / 2.f;
	const FLinearColor Gold = Hex(TEXT("ffd26e"));
	C.Box(X + 6.f, Y + 3.f, W - 12.f, H - 6.f, Hex(TEXT("06060b"), 0.92f));
	C.BtnFill(X, Y, W, H, RadPill(H), Gold, false);
	C.Glow(FCanvas::RoundRectPoints(X, Y, W, H, RadPill(H)), Gold, 2.f, 1.f);
	C.Shape(EPDSuit::Wild, X + 26.f, Y + H / 2.f, 8.f, Gold, 1.4f, 0.8f);
	C.Text(Tr(TEXT("도전 과제 달성")), X + 46.f, Y + 18.f, Gold, 10.f, 0, true);
	C.Text(AchName(AchToast[0]), X + 46.f, Y + 34.f, FLinearColor::White, 13.f, 0, true, W - 70.f);
}
