// Alice 시안 오른쪽 기둥 (D-456): 예전에 시안 그림 조각(tv.png · rpanel.png)으로 찍던 TV 몸통 · 아래 판을 코드 네온 선으로.
// 좌표는 시안 그림(docs/images/ui-1120/reference.webp, 2000 × 1125)에서 잰 값 그대로 쓰고 × 0.64 로 게임 좌표 (그림 조각과 같은 자리).
// 색은 시안에서 잰 관 심 · 번짐 · 바탕 색
#include "PDBoardDraw.h"

namespace
{
	constexpr float RefK = 0.64f;
	FVector2f RP(float X, float Y) { return FVector2f(X * RefK, Y * RefK); }
	void AddArc(TArray<FVector2f>& P, float Cx, float Cy, float R, float A0, float A1, int32 N = 8) // 각도(도), 화면 좌표(아래가 +y), 시안 좌표
	{
		for (int32 I = 0; I <= N; ++I)
		{
			const float A = FMath::DegreesToRadians(FMath::Lerp(A0, A1, I / (float)N));
			P.Add(RP(Cx + FMath::Cos(A) * R, Cy + FMath::Sin(A) * R));
		}
	}
	TArray<FVector2f> HLine(float X0, float X1, float Y) { return {RP(X0, Y), RP(X1, Y)}; }
}

void SPDBoardWidget::PaintAliceTvBody(FCanvas& C) const
{
	auto RR = [](float X0, float Y0, float X1, float Y1, float Rad) { return FCanvas::RoundRectPoints(X0 * RefK, Y0 * RefK, (X1 - X0) * RefK, (Y1 - Y0) * RefK, Rad * RefK); };
	const FLinearColor PurGlow = Hex(TEXT("62388e")), LavCore = Hex(TEXT("8e78a8")); // 시안: 회보랏빛 심 + 차분한 보라 번짐
	auto Tube = [&](const TArray<FVector2f>& P, float Th) { C.Glow(P, PurGlow, Th * 1.2f, 1.f); C.Lines(P, LavCore, Th * 0.85f); };
	// 몸통: 바깥 관(1621 ~ 1982 × 134 ~ 624, 모서리 16) 안을 짙은 보라로, 관 둘레 보랏빛
	C.FillPoly(RR(1621.f, 134.f, 1982.f, 624.f, 16.f), Hex(TEXT("1b0731")));
	C.Spill(RR(1621.f, 134.f, 1982.f, 624.f, 16.f), PurGlow, 40.f * RefK, 0.42f);
	C.Spill(RR(1644.f, 157.f, 1958.f, 514.f, 12.f), PurGlow, 34.f * RefK, 0.36f);
	// 바깥 관은 위(1653 ~ 1669) · 아래(1941 ~ 1957)가 끊겨 있다
	{
		const float X0 = 1621.f, Y0 = 134.f, X1 = 1982.f, Y1 = 624.f, Rd = 16.f;
		TArray<FVector2f> A, B;
		A.Add(RP(1669.f, Y0));
		AddArc(A, X1 - Rd, Y0 + Rd, Rd, -90.f, 0.f);
		AddArc(A, X1 - Rd, Y1 - Rd, Rd, 0.f, 90.f);
		A.Add(RP(1957.f, Y1));
		B.Add(RP(1941.f, Y1));
		AddArc(B, X0 + Rd, Y1 - Rd, Rd, 90.f, 180.f);
		AddArc(B, X0 + Rd, Y0 + Rd, Rd, 180.f, 270.f);
		B.Add(RP(1653.f, Y0));
		Tube(A, 2.f);
		Tube(B, 2.f);
	}
	Tube(RR(1644.f, 157.f, 1958.f, 514.f, 12.f), 2.f); // 화면 테
	// 안테나: 몸통 위 반원 손잡이(가운데 1858, 134 · 반지름 20) + 두 가닥
	{
		TArray<FVector2f> Knob;
		AddArc(Knob, 1858.f, 134.f, 20.f, 180.f, 360.f, 12);
		Tube(Knob, 2.f);
		Tube({RP(1853.f, 115.f), RP(1849.f, 48.f)}, 1.9f);
		Tube({RP(1871.f, 116.f), RP(1906.f, 9.f)}, 1.9f);
	}
	// 배속 버튼 두 개 (D-457: 시안 확대해서 다시 맞춤): 1646 ~ 1700 · 1714 ~ 1768 × 536 ~ 604, 밝은 회색 바깥 테 + 4 안쪽 어두운 회녹 테,
	// 'x2' 글자는 가운데 559, ▶▶ 는 그 아래(573 ~ 588)
	for (int32 K = 0; K < 2; ++K)
	{
		const float X0 = 1646.f + K * 68.f, X1 = X0 + 54.f, Cx = X0 + 27.f;
		const TArray<FVector2f> Ro = RR(X0, 536.f, X1, 604.f, 6.f);
		C.FillPoly(Ro, Hex(TEXT("37413e")));
		C.Glow(Ro, Hex(TEXT("8c9a95")), 1.6f, 0.4f);
		C.Lines(Ro, Hex(TEXT("aab4b0")), 1.4f);
		C.Lines(RR(X0 + 4.f, 540.f, X1 - 4.f, 600.f, 3.f), Hex(TEXT("5f6e69")), 1.f);
		const FLinearColor Tc = Hex(TEXT("aab1ae"));
		// D-459: 시안 확대본에서 글자는 553 ~ 565 (가운데 559, 폭 20), ▶▶ 는 573 ~ 588 에 폭 9 삼각형 둘을 8 간격으로 겹쳐 폭 17
		C.TextCentered(K == 0 ? TEXT("x2") : TEXT("x4"), Cx * RefK, 557.8f * RefK, Tc, 12.5f, true); // 시안 글자는 굵은 편
		for (int32 J = 0; J < 2; ++J) // ▶▶
		{
			const float Ax = Cx - 8.5f + J * 8.f;
			C.FillPoly({RP(Ax, 573.f), RP(Ax + 9.f, 580.5f), RP(Ax, 588.f)}, Tc);
		}
	}
	// 채널 다이얼 (D-458: 시안 픽셀을 각도별로 재서 다시 맞춤): 가운데 (1837, 571), 바깥 고리 34 · 안 고리 29 둘 다 거의 온 고리,
	// 왼쪽 위 187° ~ 207° 만 트여 있고 바늘(210°)이 그 끝에 닿아 고리 밖 45 까지 나온다. 안은 자줏빛으로 채우고 가운데가 조금 밝다
	{
		const FLinearColor PkGlow = Hex(TEXT("a4246c")), PkCore = Hex(TEXT("e295c2")), PkInner = Hex(TEXT("a2447a")); // 심은 시안처럼 희뿌연 분홍
		C.SoftDisc(RP(1837.f, 571.f), 48.f * RefK, 48.f * RefK, PkGlow, 0.6f); // 고리 밖으로 번지는 분홍 빛
		C.FillPoly(FCanvas::CirclePoints(RP(1837.f, 571.f), 34.f * RefK, 40), Hex(TEXT("641243")));
		C.SoftDisc(RP(1837.f, 571.f), 22.f * RefK, 22.f * RefK, Hex(TEXT("7e1c54")), 0.8f);
		C.SoftDisc(RP(1837.f, 571.f), 6.f * RefK, 6.f * RefK, Hex(TEXT("a83270")), 0.4f); // 바늘 뿌리의 밝은 점
		TArray<FVector2f> O, I;
		AddArc(O, 1837.f, 571.f, 34.f, 207.f, 547.f, 40);
		AddArc(I, 1837.f, 571.f, 29.f, 209.f, 545.f, 36);
		C.Glow(O, PkGlow, 2.2f, 0.8f); C.Lines(O, PkCore, 1.4f);
		C.Lines(I, PkInner, 0.9f);
		const float Na = FMath::DegreesToRadians(210.f);
		const TArray<FVector2f> Needle = {RP(1837.f + FMath::Cos(Na) * 45.f, 571.f + FMath::Sin(Na) * 45.f), RP(1837.f, 571.f)};
		C.Glow(Needle, PkGlow, 2.f, 0.8f); C.Lines(Needle, PkCore, 1.4f);
	}
	// 스피커 줄 다섯 (1905 ~ 1959)
	for (const float Y : {538.f, 554.f, 570.5f, 587.f, 603.5f}) Tube(HLine(1905.f, 1959.f, Y), 2.6f);
}

void SPDBoardWidget::PaintAliceRPanel(FCanvas& C) const
{
	auto RR = [](float X0, float Y0, float X1, float Y1, float Rad) { return FCanvas::RoundRectPoints(X0 * RefK, Y0 * RefK, (X1 - X0) * RefK, (Y1 - Y0) * RefK, Rad * RefK); };
	auto NeonLine = [&](float X0, float X1, float Y, const TCHAR* Col, float Th, float Str)
	{
		const FLinearColor Cl = Hex(Col);
		C.Glow(HLine(X0, X1, Y), Cl, Th, Str);
		C.Lines(HLine(X0, X1, Y), FLinearColor(FMath::Min(1.f, Cl.R * 1.25f + 0.05f), FMath::Min(1.f, Cl.G * 1.25f + 0.05f), FMath::Min(1.f, Cl.B * 1.25f + 0.05f), 1.f), Th * 0.6f);
	};
	// 위 줄 셋 (보라 · 파랑 · 연보라 = 칵테일 칸 윗선)
	NeonLine(1620.f, 1983.f, 643.5f, TEXT("8e43a3"), 2.4f, 0.7f);
	NeonLine(1620.f, 1983.f, 653.5f, TEXT("4454c0"), 2.8f, 0.7f);
	// 칵테일 칸 바탕 (짙은 자줏빛)
	C.Box(1621.f * RefK, 666.f * RefK, 362.f * RefK, 185.f * RefK, Hex(TEXT("1c0d23")));
	NeonLine(1621.f, 1983.f, 664.5f, TEXT("715281"), 2.f, 0.5f);
	// 머리글 양옆 선 · 잔 밑 파란 선 · 칸 아랫선
	C.Lines(HLine(1644.f, 1758.f, 694.f), Hex(TEXT("4a3359")), 3.f);
	C.Lines(HLine(1846.f, 1960.f, 694.f), Hex(TEXT("4a3359")), 3.f);
	NeonLine(1636.f, 1967.f, 836.f, TEXT("324180"), 1.8f, 0.5f);
	NeonLine(1621.f, 1983.f, 852.f, TEXT("715281"), 2.f, 0.5f);
	// 아래 줄 셋 (하늘 · 분홍 · 청록)
	NeonLine(1620.f, 1984.f, 862.5f, TEXT("7080f0"), 3.f, 0.8f);
	NeonLine(1620.f, 1984.f, 874.f, TEXT("c46ae6"), 2.6f, 0.8f);
	NeonLine(1620.f, 1983.f, 884.5f, TEXT("3f9c9b"), 2.2f, 0.6f);
	// 스킬 버튼: 짙은 남색 판 + 굵은 파란 관 + 안쪽 가는 파란 테 + 갈래 기호
	{
		const FLinearColor Bl = Hex(TEXT("1e8cff")), BlIn = Hex(TEXT("2a7fe6")), Ic = Hex(TEXT("8bceff"));
		const TArray<FVector2f> Ro = RR(1622.f, 900.f, 1982.f, 978.f, 8.f);
		C.FillPoly(Ro, Hex(TEXT("00285a")));
		C.Glow(Ro, Bl, 3.2f, 1.f);
		C.Lines(Ro, Hex(TEXT("5ab0ff")), 1.8f);
		C.Lines(RR(1634.f, 911.f, 1970.f, 967.f, 4.f), BlIn, 1.2f);
		const FVector2f Ctr = RP(1666.f, 937.f);
		for (const FVector2f Q : {RP(1648.f, 926.f), RP(1689.f, 919.f), RP(1648.f, 950.f), RP(1689.f, 953.f)})
		{
			C.Lines({Ctr, Q}, Ic, 2.f);
			C.FillPoly(FCanvas::CirclePoints(Q, 5.5f * RefK, 14), Ic);
		}
		C.FillPoly(FCanvas::CirclePoints(Ctr, 7.f * RefK, 16), Ic);
	}
	// 시작 버튼: 짙은 초록 판(가로 주사선) + 굵은 초록 관 + 안쪽 연두 가는 테 + 재생 삼각형
	{
		const FLinearColor Gr = Hex(TEXT("3fd23f")), GrIn = Hex(TEXT("a8d070")), Tri = Hex(TEXT("7ce687"));
		const TArray<FVector2f> Ro = RR(1622.f, 993.f, 1982.f, 1106.f, 10.f);
		C.FillPoly(Ro, Hex(TEXT("1c461d")));
		for (float Y = 1004.f; Y < 1096.f; Y += 4.f) C.Lines(HLine(1636.f, 1968.f, Y), FLinearColor(0.f, 0.f, 0.f, ((int32)Y / 4) % 3 == 0 ? 0.22f : 0.1f), 1.f);
		C.Glow(Ro, Gr, 3.4f, 1.f);
		C.Lines(Ro, Hex(TEXT("6cf06c")), 1.8f);
		C.Lines(RR(1631.f, 1002.f, 1972.f, 1098.f, 4.f), GrIn, 1.2f);
		const TArray<FVector2f> T = {RP(1652.f, 1027.f), RP(1681.f, 1050.f), RP(1652.f, 1074.f)};
		C.FillPoly(T, Tri);
		C.Glow({T[0], T[1], T[2], T[0]}, Gr, 1.2f, 0.4f);
	}
}
