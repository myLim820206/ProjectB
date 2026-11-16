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
	else if (TvChannel == 0) // CH1 다음 웨이브: 적 그림 · 이름 · 마리 수, 보스면 이름 · 능력 한 줄
	{
		const int32 NW = G.Phase == EPDPhase::Wave ? G.Wave : G.Wave + 1;
		C.Text(G.Phase == EPDPhase::Wave ? TrF(TEXT("지금 웨이브 %d"), NW) : TrF(TEXT("다음 웨이브 %d"), NW), CX0, SY + 20.f, Mono, 11.f, 0, true, CW - 40.f);
		ChTag(TEXT("CH1"));
		float Y = SY + 40.f;
		for (int32 I = 0; I < TvMix.Num() && Y < ContentBot - 12.f; ++I)
		{
			const FTvMix& M = TvMix[I];
			const bool bBoss = EnemyDef(M.T).bBoss;
			C.CardSoldier(SoldierShape(M.T), FVector2f(CX0 + 10.f, Y + 9.f), bBoss ? 9.f : 7.f, FMath::Frac(Clock * 1.5f), MonoA(bBoss ? 1.f : 0.85f));
			C.Text(EnemyName(M.T), CX0 + 26.f, Y + 4.f, MonoA(bBoss ? 1.f : 0.88f), 10.f, 0, bBoss, CW * 0.6f);
			C.Text(FString::Printf(TEXT("×%d"), M.N), CX0 + CW, Y + 4.f, MonoA(0.9f), 10.f, 2, true);
			if (bBoss) { C.Text(BossSkillText(M.T), CX0 + 26.f, Y + 18.f, MonoA(0.65f), 8.5f, 0, false, CW - 30.f); Y += 14.f; }
			Y += 20.f;
		}
		if (G.Goal() > 0 && NW == G.Goal()) C.Text(Tr(TEXT("마지막 웨이브 · 최종 보스")), CX0, ContentBot - 2.f, MonoA(0.95f), 9.f, 0, true, CW);
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
		PaintLogoBadge(C, SX + SW / 2.f - 10.f, SY + 18.f, 0.42f);
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
	UiSound(EPDSound::Move);
	SaveSettings();
}

// 주크박스 간판 (D-319): 천장 줄에 매단 두 겹 튜브 간판(예전 남은 적 간판 자리). 'NOW PLAYING' + 흐르는 곡 이름 + ◀ ▶ + 박자에 맞춰 깜빡이는 전구 줄
void SPDBoardWidget::PaintJukebox(FCanvas& C) const
{
	const FLinearColor Gold = Hex(TEXT("ffd26e")), Pink = Hex(TEXT("ff6aaa")), Lav = Hex(TEXT("a896e1")), Fg = Hex(TEXT("dce4f0")), Wire = Hex(TEXT("5f6878"), 0.5f);
	const float X = SignEnX, Y = SignEnY, W = SignEnW, H = SignEnH;
	for (const float Wx : {X + 16.f, X + W - 16.f}) C.Lines({FVector2f(Wx, 0.f), FVector2f(Wx, Y)}, Wire, 1.f);
	C.Box(X, Y, W, H, Hex(TEXT("06060b"), 0.85f));
	const TArray<FVector2f> Ro = FCanvas::RoundRectPoints(X, Y, W, H, RadSmall + 2.f), Ri = FCanvas::RoundRectPoints(X + 5.f, Y + 5.f, W - 10.f, H - 10.f, RadSmall - 3.f);
	C.Glow(Ro, WithAlpha(Lav, 0.85f), 2.2f, 0.95f);
	C.Lines(Ro, FLinearColor(1.f, 1.f, 1.f, 0.3f), 0.6f);
	C.Glow(Ri, WithAlpha(Lav, 0.6f), 1.1f, 0.4f);
	const bool bBoss = WantBossMusic();
	C.TextCentered(bBoss ? TEXT("BOSS THEME") : TEXT("NOW PLAYING"), X + W / 2.f, Y + 15.f, bBoss ? Hex(TEXT("ff6a6a")) : Gold, 9.f, true);
	// 곡 이름: 칸보다 길면 흐른다 (가운데 칸 안만 보이게 양쪽 끝을 간판 바탕색으로 덮음)
	const FString Nm = bBoss ? FString(Tr(TEXT("보스 웨이브"))) : FString::Printf(TEXT("%d. %s"), JukeTrack + 1, UTF8_TO_TCHAR(JazzTrackName(JukeTrack)));
	const float NX0 = X + 10.f + JukeBtnS, NW = W - 20.f - 2.f * JukeBtnS, TW = FCanvas::Measure(Nm, FCanvas::Font(11.f, true)).X;
	if (TW <= NW - 6.f) C.TextCentered(Nm, X + W / 2.f, JukeBtnY + JukeBtnS / 2.f, Fg, 11.f, true);
	else
	{
		const float Loop = TW + 40.f, Off = FMath::Fmod(Clock * 28.f, Loop);
		for (int32 K = 0; K < 2; ++K) C.Text(Nm, NX0 + 4.f - Off + K * Loop, JukeBtnY + JukeBtnS / 2.f + 4.f, Fg, 11.f, 0, true);
		C.Box(X + 6.f, JukeBtnY - 2.f, NX0 - X - 6.f, JukeBtnS + 4.f, Hex(TEXT("06060b"), 1.f));
		C.Box(NX0 + NW, JukeBtnY - 2.f, X + W - 6.f - NX0 - NW, JukeBtnS + 4.f, Hex(TEXT("06060b"), 1.f));
	}
	for (int32 Dir = -1; Dir <= 1; Dir += 2) // ◀ ▶ (눌리는 버튼, D-328 ②)
	{
		const float BX = JukeBtnX(Dir);
		const bool bHov = InRect(Mouse, BX, JukeBtnY, JukeBtnS, JukeBtnS);
		C.BtnFill(BX, JukeBtnY, JukeBtnS, JukeBtnS, RadPill(JukeBtnS), Pink, bHov);
		C.Glow(FCanvas::RoundRectPoints(BX, JukeBtnY, JukeBtnS, JukeBtnS, RadPill(JukeBtnS)), bHov ? Pink : Lav, bHov ? 1.5f : 1.1f, bHov ? 0.9f : 0.4f);
		const FVector2f Cc(BX + JukeBtnS / 2.f, JukeBtnY + JukeBtnS / 2.f);
		C.Glow({Cc + FVector2f(Dir * 4.f, 0.f), Cc + FVector2f(-Dir * 3.f, -5.f), Cc + FVector2f(-Dir * 3.f, 5.f), Cc + FVector2f(Dir * 4.f, 0.f)}, bHov ? FLinearColor::White : Fg, 1.2f, 0.4f);
	}
	// 박자 전구: 4분 박마다 하나씩 옮겨 가며 켜짐 (보스 웨이브는 빨간 빠른 깜빡임)
	const float Bpm = bBoss ? 132.f : JazzTrackBpm(JukeTrack), BeatN = Clock * Bpm / 60.f;
	const int32 NB = 12, Cur = FMath::FloorToInt(BeatN) % 4;
	const float Pulse = 1.f - FMath::Frac(BeatN);
	for (int32 K = 0; K < NB; ++K)
	{
		const bool bOn = K % 4 == Cur && Pulse > 0.35f;
		Bulb(C, FVector2f(X + 18.f + K * (W - 36.f) / (NB - 1), Y + H - 9.f), 2.6f, bOn, bBoss ? Hex(TEXT("ff6a6a")) : Hex(TEXT("ffe196")));
	}
}
