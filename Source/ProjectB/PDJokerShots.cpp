// 조커 10종 + 상위 조커 30종 발사체: 조커마다 다른 실루엣 · 색 · 움직임 · 착탄 (D-488)
// 시안: tools/neon/joker_shots_mock.py (같은 모양 · 색 · 움직임). 화풍 DOC/art/15-art-style.md 10절.
// 그림만 바뀐다: 속도 · 피해 · 유도 같은 규칙은 PDCore 그대로. 조커 번호(Fx)는 쏜 타워에서 읽는다 (0~9 조커, 10~39 상위 조커)
#include "PDBoardDraw.h"

namespace PDJokerShotsImpl
{
using FJsLn = TArray<FVector2f>;
using FJsLns = TArray<FJsLn>;

enum class EJsFam : uint8 { Lob, Boomerang, Spin, Twin, Fan, Wave, Slash, Instant, Ring, Comet };
enum class EJsImp : uint8 { Confetti, Poof, Crescent, Cross, Shards, Sparkle, XCut, Spiral, Rings, Rays, Paint, Feathers, Smoke, Mirror, Splash, Crumbs, Tick, Crack, Tears, Frost, Bubbles, Petals, Hearts, Cards, Squares, Zzz };
enum class EJsExtra : uint8 { None, Ghost, Blink, Mirror, Paint, Bounce };

struct FJokerShotDef
{
	const TCHAR* Col;
	EJsFam Fam;
	EJsImp Imp;
	EJsExtra Extra = EJsExtra::None;
	const TCHAR* Col2 = nullptr; // 둘째 모양 색 (희비극)
};

// 순서: EPDJoker 10종, 그다음 상위 조커 30종 (PDCore GPDUpper 순서). 무리 = 시안 당시 첫 재료 조커의 움직임
const FJokerShotDef GJsDefs[40] = {
	{TEXT("ffa45a"), EJsFam::Lob, EJsImp::Confetti},                       // 광대: 서로 도는 저글링 공 셋
	{TEXT("4ff0c8"), EJsFam::Boomerang, EJsImp::Poof},                     // 모자 장수: 실크해트
	{TEXT("f2ff6a"), EJsFam::Spin, EJsImp::Crescent},                      // 웃는 가면: 눈가리개 가면
	{TEXT("ff9ad5"), EJsFam::Twin, EJsImp::Cross, EJsExtra::None, TEXT("7fd6ff")}, // 희비극: 웃는 얼굴(분홍) + 우는 얼굴(하늘)
	{TEXT("ff5ad2"), EJsFam::Fan, EJsImp::Shards},                         // 할리퀸: 마름모 셋
	{TEXT("a6ff7a"), EJsFam::Wave, EJsImp::Sparkle},                       // 지휘봉: 8분음표
	{TEXT("6a8cff"), EJsFam::Slash, EJsImp::XCut},                         // 잭: 초승달 칼날
	{TEXT("b77aff"), EJsFam::Instant, EJsImp::Spiral},                     // 체셔: 즉시 광선 + 이빨 미소
	{TEXT("ffd25a"), EJsFam::Ring, EJsImp::Rings},                         // 방울: 종 + 소리 고리
	{TEXT("ffffff"), EJsFam::Comet, EJsImp::Rays},                         // 스타: 큰 별 혜성
	{TEXT("ff6a5a"), EJsFam::Lob, EJsImp::Paint, EJsExtra::Paint},           // 장미 칠하는 정원사: 장미
	{TEXT("ffe45a"), EJsFam::Lob, EJsImp::Confetti, EJsExtra::Bounce},       // 트위들덤과 트위들디: 똑같은 얼굴 둘
	{TEXT("e6b27a"), EJsFam::Lob, EJsImp::Feathers},                       // 도도새: 깃털
	{TEXT("5ab4ff"), EJsFam::Lob, EJsImp::Smoke},                          // 애벌레: 연기 고리 셋
	{TEXT("ffb4c8"), EJsFam::Lob, EJsImp::Poof},                           // 공작부인: 아기 돼지
	{TEXT("dfe6ff"), EJsFam::Lob, EJsImp::Mirror, EJsExtra::Mirror},         // 거울 나라: 손거울 + 거울상
	{TEXT("c8ff5a"), EJsFam::Boomerang, EJsImp::Splash},                   // 3월 토끼: 찻잔
	{TEXT("ffbe5a"), EJsFam::Boomerang, EJsImp::Rays},                     // 하트 왕: 왕관 + 하트
	{TEXT("ff9a7a"), EJsFam::Boomerang, EJsImp::Crumbs},                   // 하트 잭: 타르트
	{TEXT("6affb0"), EJsFam::Boomerang, EJsImp::Splash},                   // 미친 다과회: 찻주전자
	{TEXT("ffffff"), EJsFam::Boomerang, EJsImp::Tick},                     // 흰 토끼: 회중시계
	{TEXT("fff0b0"), EJsFam::Spin, EJsImp::Crack},                         // 험프티 덤프티: 금 간 알
	{TEXT("ff3a4a"), EJsFam::Spin, EJsImp::XCut, EJsExtra::Ghost},           // 붉은 여왕: 체스 퀸 + 잔상
	{TEXT("b8d870"), EJsFam::Spin, EJsImp::Tears},                         // 가짜 거북: 거북
	{TEXT("e0b860"), EJsFam::Spin, EJsImp::Feathers},                      // 그리핀: 날개
	{TEXT("e070ff"), EJsFam::Spin, EJsImp::Spiral, EJsExtra::Blink},         // 웃음만 남은 고양이: 눈 없는 미소
	{TEXT("cfeaff"), EJsFam::Twin, EJsImp::Frost},                         // 하얀 여왕: 눈송이 둘
	{TEXT("4ab8e8"), EJsFam::Twin, EJsImp::Bubbles},                       // 바다코끼리와 목수: 굴 껍데기 둘
	{TEXT("a8b8ff"), EJsFam::Twin, EJsImp::XCut, EJsExtra::None, TEXT("a8b8ff")}, // 하얀 기사: 랜스 + 방패
	{TEXT("d8a0ff"), EJsFam::Twin, EJsImp::Petals},                        // 말하는 꽃들: 데이지 둘
	{TEXT("ff4a7a"), EJsFam::Fan, EJsImp::Hearts},                         // 하트 여왕: 하트 셋
	{TEXT("ff6aa0"), EJsFam::Fan, EJsImp::Shards},                         // 홍학 크로케: 가시 공 셋
	{TEXT("c8d0e0"), EJsFam::Fan, EJsImp::Cards},                          // 트럼프 근위대: 카드 셋
	{TEXT("9fb0d0"), EJsFam::Wave, EJsImp::Squares},                       // 체스판: 체크무늬
	{TEXT("b0a0ff"), EJsFam::Wave, EJsImp::Zzz},                           // 겨울잠쥐: Z z z
	{TEXT("ffc0f0"), EJsFam::Wave, EJsImp::Sparkle},                       // 유니콘: 나선 뿔
	{TEXT("6aff5a"), EJsFam::Slash, EJsImp::XCut},                         // 재버워키: 발톱 세 줄
	{TEXT("ffc83a"), EJsFam::Slash, EJsImp::Rays},                         // 사자: 갈기
	{TEXT("ff7a9a"), EJsFam::Instant, EJsImp::Zzz},                        // 꿈꾸는 붉은 왕: 즉시 광선 + 꿈 구름
	{TEXT("8a6aff"), EJsFam::Ring, EJsImp::Sparkle},                       // 반짝반짝 작은 박쥐: 박쥐 + 초음파
};

const FJokerShotDef& JsDef(int32 Fx) { return GJsDefs[FMath::Clamp(Fx, 0, 39)]; }

// ---------------------------------------------------------------- 도형 도우미 (단위 좌표: 1 = 발사체 반지름, x = 앞)
FVector2f JsV(float X, float Y) { return FVector2f(X, Y); }
FJsLn JsCirc(float X, float Y, float R, int32 N = 16, float SX = 1.f, float SY = 1.f)
{
	FJsLn P;
	for (int32 K = 0; K <= N; ++K) P.Add(JsV(X + FMath::Cos(K * TwoPi / N) * R * SX, Y + FMath::Sin(K * TwoPi / N) * R * SY));
	return P;
}
FJsLn JsArc(float X, float Y, float R, float A0, float A1, int32 N = 10) // 각은 도
{
	FJsLn P;
	for (int32 K = 0; K <= N; ++K)
	{
		const float A = FMath::DegreesToRadians(A0 + (A1 - A0) * K / N);
		P.Add(JsV(X + FMath::Cos(A) * R, Y + FMath::Sin(A) * R));
	}
	return P;
}
FJsLn JsStar(float X, float Y, float R, float Ri, int32 N)
{
	FJsLn P;
	for (int32 K = 0; K <= 2 * N; ++K)
	{
		const float A = -PI / 2.f + K * PI / N, Rr = (K % 2) ? R * Ri : R;
		P.Add(JsV(X + FMath::Cos(A) * Rr, Y + FMath::Sin(A) * Rr));
	}
	return P;
}
FJsLn JsHeart(float X, float Y, float R, bool bFlip = false)
{
	FJsLn P;
	for (int32 I = 0; I <= 24; ++I)
	{
		const float T = I * TwoPi / 24.f, S = FMath::Sin(T);
		const float Hx = 16.f * S * S * S, Hy = -(13.f * FMath::Cos(T) - 5.f * FMath::Cos(2.f * T) - 2.f * FMath::Cos(3.f * T) - FMath::Cos(4.f * T));
		P.Add(JsV(X + Hx / 17.f * R, Y + (Hy / 17.f - 0.1f) * R * (bFlip ? -1.f : 1.f)));
	}
	return P;
}
FJsLn JsBez(FVector2f A, FVector2f B, FVector2f C, int32 N = 8)
{
	FJsLn P;
	for (int32 I = 0; I <= N; ++I)
	{
		const float T = (float)I / N;
		P.Add(A * ((1.f - T) * (1.f - T)) + B * (2.f * (1.f - T) * T) + C * (T * T));
	}
	return P;
}
void JsAppend(FJsLn& A, const FJsLn& B, int32 From = 1) { for (int32 I = From; I < B.Num(); ++I) A.Add(B[I]); }
FJsLns JsXf(const FJsLns& L, FVector2f O, float Ang, float S) // 옮기기 · 돌리기 · 키우기
{
	const float C = FMath::Cos(Ang), Sn = FMath::Sin(Ang);
	FJsLns R;
	for (const FJsLn& Ln : L)
	{
		FJsLn Q;
		for (const FVector2f& P : Ln) Q.Add(O + FVector2f(P.X * C - P.Y * Sn, P.X * Sn + P.Y * C) * S);
		R.Add(MoveTemp(Q));
	}
	return R;
}
FJsLn JsSeg(float X0, float Y0, float X1, float Y1) { return FJsLn{JsV(X0, Y0), JsV(X1, Y1)}; }

FJsLns JsGrin(bool bEyes)
{
	const float W = 0.9f;
	FJsLn G = JsBez(JsV(-W, 0.f), JsV(0.f, 0.8f), JsV(W, 0.f));
	JsAppend(G, JsBez(JsV(W, 0.f), JsV(0.f, 0.35f), JsV(-W, 0.f)));
	FJsLns L{G};
	for (float X : {-0.4f, -0.2f, 0.f, 0.2f, 0.4f}) L.Add(JsSeg(X, 0.12f, X, 0.32f - FMath::Abs(X) * 0.3f));
	if (bEyes) { L.Add(JsSeg(-0.55f, -0.45f, -0.25f, -0.35f)); L.Add(JsSeg(0.55f, -0.45f, 0.25f, -0.35f)); }
	return L;
}
FJsLn JsFeather() { return FJsLn{JsV(1.1f, 0.f), JsV(0.35f, -0.45f), JsV(-0.55f, -0.35f), JsV(-1.f, 0.f), JsV(-0.55f, 0.35f), JsV(0.35f, 0.45f), JsV(1.1f, 0.f)}; }
FJsLns JsSnow()
{
	FJsLns L;
	for (int32 K = 0; K < 3; ++K)
	{
		const float A = K * TwoPi / 6.f + PI / 2.f;
		L.Add(JsSeg(FMath::Cos(A) * 0.85f, FMath::Sin(A) * 0.85f, -FMath::Cos(A) * 0.85f, -FMath::Sin(A) * 0.85f));
	}
	for (int32 K = 0; K < 6; ++K)
	{
		const float A = K * TwoPi / 6.f + PI / 2.f, Mx = FMath::Cos(A) * 0.5f, My = FMath::Sin(A) * 0.5f;
		for (float D : {-0.6f, 0.6f}) L.Add(JsSeg(Mx, My, Mx + FMath::Cos(A + D) * 0.3f, My + FMath::Sin(A + D) * 0.3f));
	}
	return L;
}

// 조커 Fx 의 실루엣 (T = 날아간 시간, bSecond = 둘째 모양: 희비극 우는 얼굴, 하얀 기사 방패)
FJsLns JsShape(int32 Fx, float T, bool bSecond)
{
	switch (Fx)
	{
	case 0: // 광대: 서로 도는 공 셋
	{
		FJsLns L;
		for (int32 K = 0; K < 3; ++K) L.Add(JsCirc(FMath::Cos(T * 9.f + K * TwoPi / 3.f) * 0.55f, FMath::Sin(T * 9.f + K * TwoPi / 3.f) * 0.55f, 0.32f, 10));
		return L;
	}
	case 1: // 모자 장수: 실크해트
		return {JsSeg(-1.1f, 0.55f, 1.1f, 0.55f), FJsLn{JsV(-0.65f, 0.55f), JsV(-0.6f, -0.9f), JsV(0.6f, -0.9f), JsV(0.65f, 0.55f)}, JsSeg(-0.63f, 0.2f, 0.63f, 0.2f)};
	case 2: // 웃는 가면: 눈가리개 + 미소
		return {FJsLn{JsV(-1.f, -0.25f), JsV(-0.55f, -0.6f), JsV(0.f, -0.35f), JsV(0.55f, -0.6f), JsV(1.f, -0.25f), JsV(0.7f, 0.2f), JsV(0.f, 0.02f), JsV(-0.7f, 0.2f), JsV(-1.f, -0.25f)},
			JsCirc(-0.45f, -0.18f, 0.16f, 8, 1.3f), JsCirc(0.45f, -0.18f, 0.16f, 8, 1.3f), JsArc(0.f, 0.25f, 0.5f, 25.f, 155.f, 8)};
	case 3: // 희비극: 웃는 얼굴 / 우는 얼굴
		if (bSecond) return {JsCirc(0.f, 0.f, 0.6f, 16), JsArc(0.f, 0.42f, 0.3f, 200.f, 340.f, 6), JsSeg(-0.22f, -0.15f, -0.1f, -0.2f), JsSeg(0.1f, -0.2f, 0.22f, -0.15f)};
		return {JsCirc(0.f, 0.f, 0.6f, 16), JsArc(0.f, 0.05f, 0.32f, 20.f, 160.f, 6), JsSeg(-0.22f, -0.18f, -0.1f, -0.18f), JsSeg(0.1f, -0.18f, 0.22f, -0.18f)};
	case 4: // 할리퀸: 길쭉한 마름모
		return {FJsLn{JsV(1.1f, 0.f), JsV(0.f, 0.55f), JsV(-1.1f, 0.f), JsV(0.f, -0.55f), JsV(1.1f, 0.f)}};
	case 5: // 지휘봉: 8분음표
		return {JsCirc(-0.3f, 0.55f, 0.3f, 12, 1.35f), FJsLn{JsV(0.08f, 0.5f), JsV(0.08f, -0.95f), JsV(0.6f, -0.55f), JsV(0.55f, -0.25f)}};
	case 6: // 잭: 초승달 칼날 + 자루
	{
		FJsLn B = JsArc(-0.7f, 0.f, 1.4f, -65.f, 65.f, 14);
		JsAppend(B, JsArc(-1.f, 0.f, 1.15f, 55.f, -55.f, 12), 0);
		B.Add(B[0]);
		return {B, JsSeg(-1.f, 0.f, -1.6f, 0.f), JsSeg(-1.1f, -0.3f, -1.1f, 0.3f)};
	}
	case 7: return JsGrin(true); // 체셔
	case 8: // 방울: 흔들리는 종 + 추
	{
		FJsLn Body = JsBez(JsV(-0.65f, 0.5f), JsV(-0.7f, -0.95f), JsV(0.f, -0.85f));
		JsAppend(Body, JsBez(JsV(0.f, -0.85f), JsV(0.7f, -0.95f), JsV(0.65f, 0.5f)));
		Body.Add(JsV(-0.65f, 0.5f));
		return JsXf({Body, JsCirc(0.f, 0.72f, 0.16f, 8), JsSeg(-0.85f, 0.5f, 0.85f, 0.5f)}, JsV(0.f, 0.f), FMath::Sin(T * 14.f) * 0.25f, 1.f);
	}
	case 9: return {JsStar(0.f, 0.f, 1.05f, 0.45f, 5)}; // 스타
	case 10: // 장미: 원 + 나선 + 잎
	{
		FJsLn Sp;
		for (int32 K = 0; K < 28; ++K) { const float A = K * 0.35f; Sp.Add(JsV(FMath::Cos(A) * A / 9.5f * 0.7f, FMath::Sin(A) * A / 9.5f * 0.7f)); }
		FJsLn Leaf = JsBez(JsV(-0.2f, 0.75f), JsV(-0.8f, 1.1f), JsV(-0.9f, 0.65f));
		JsAppend(Leaf, JsBez(JsV(-0.9f, 0.65f), JsV(-0.5f, 0.55f), JsV(-0.2f, 0.75f)));
		return {JsCirc(0.f, 0.f, 0.75f, 18), Sp, Leaf};
	}
	case 11: // 트위들덤과 트위들디: 맞붙은 똑같은 얼굴 둘
	{
		FJsLns L;
		for (float Cx : {-0.5f, 0.5f})
		{
			L.Add(JsCirc(Cx, 0.f, 0.48f, 14));
			L.Add(JsSeg(Cx - 0.18f, -0.1f, Cx - 0.06f, -0.1f));
			L.Add(JsSeg(Cx + 0.06f, -0.1f, Cx + 0.18f, -0.1f));
			L.Add(JsArc(Cx, 0.02f, 0.25f, 20.f, 160.f, 5));
			L.Add(JsSeg(Cx, -0.48f, Cx, -0.7f));
		}
		return L;
	}
	case 12: // 도도새: 깃털
		return {JsFeather(), JsSeg(-1.4f, 0.f, 1.f, 0.f), JsSeg(0.2f, 0.f, -0.1f, -0.38f), JsSeg(-0.3f, 0.f, -0.6f, -0.3f), JsSeg(0.2f, 0.f, -0.1f, 0.38f), JsSeg(-0.3f, 0.f, -0.6f, 0.3f)};
	case 13: // 애벌레: 연기 고리 셋
		return {JsCirc(0.7f, 0.f, 0.28f, 12, 0.55f), JsCirc(0.f, 0.f, 0.45f, 14, 0.55f), JsCirc(-0.85f, 0.f, 0.62f, 16, 0.55f)};
	case 14: // 공작부인: 아기 돼지 얼굴
		return {JsCirc(0.f, 0.05f, 0.72f, 18), JsCirc(0.f, 0.22f, 0.28f, 10, 1.35f), JsCirc(-0.12f, 0.22f, 0.05f, 4), JsCirc(0.12f, 0.22f, 0.05f, 4),
			FJsLn{JsV(-0.55f, -0.45f), JsV(-0.6f, -0.95f), JsV(-0.2f, -0.65f)}, FJsLn{JsV(0.55f, -0.45f), JsV(0.6f, -0.95f), JsV(0.2f, -0.65f)}, JsSeg(-0.3f, -0.15f, -0.2f, -0.15f), JsSeg(0.2f, -0.15f, 0.3f, -0.15f)};
	case 15: // 거울 나라: 손거울
		return {JsCirc(0.f, -0.25f, 0.62f, 18, 0.78f), JsCirc(0.f, -0.25f, 0.45f, 14, 0.78f), JsSeg(0.f, 0.37f, 0.f, 1.05f), JsSeg(-0.18f, 0.62f, 0.18f, 0.62f)};
	case 16: // 3월 토끼: 찻잔 + 김
	{
		FJsLns L{FJsLn{JsV(-0.75f, -0.3f), JsV(-0.5f, 0.45f), JsV(0.5f, 0.45f), JsV(0.75f, -0.3f), JsV(-0.75f, -0.3f)}, JsArc(0.72f, 0.02f, 0.24f, -80.f, 100.f, 6), JsSeg(-0.6f, 0.62f, 0.6f, 0.62f)};
		for (float X : {-0.2f, 0.2f})
		{
			FJsLn St;
			for (int32 K = 0; K < 5; ++K) St.Add(JsV(X + FMath::Sin(K * 1.2f + T * 10.f) * 0.1f, -0.5f - K * 0.14f));
			L.Add(St);
		}
		return L;
	}
	case 17: // 하트 왕: 왕관 + 하트
		return {FJsLn{JsV(-0.85f, 0.1f), JsV(-0.95f, -0.75f), JsV(-0.45f, -0.3f), JsV(0.f, -0.95f), JsV(0.45f, -0.3f), JsV(0.95f, -0.75f), JsV(0.85f, 0.1f), JsV(-0.85f, 0.1f)}, JsHeart(0.f, 0.62f, 0.38f)};
	case 18: // 하트 잭: 격자 타르트
	{
		FJsLns L{JsCirc(0.f, 0.f, 0.8f, 20), JsCirc(0.f, 0.f, 0.58f, 16), JsSeg(-0.4f, -0.4f, 0.4f, 0.4f), JsSeg(-0.4f, 0.4f, 0.4f, -0.4f)};
		for (int32 K = 0; K < 8; ++K) { const float A = K * TwoPi / 8.f; L.Add(JsSeg(FMath::Cos(A) * 0.58f, FMath::Sin(A) * 0.58f, FMath::Cos(A) * 0.8f, FMath::Sin(A) * 0.8f)); }
		return L;
	}
	case 19: // 미친 다과회: 찻주전자
		return {JsCirc(0.f, 0.1f, 0.6f, 18, 1.15f), FJsLn{JsV(0.65f, 0.f), JsV(1.15f, -0.45f), JsV(1.25f, -0.4f)}, JsArc(-0.72f, 0.1f, 0.3f, 100.f, 260.f, 6), JsArc(0.f, -0.48f, 0.3f, 200.f, 340.f, 6), JsCirc(0.f, -0.65f, 0.1f, 6)};
	case 20: // 흰 토끼: 바늘 도는 회중시계
	{
		FJsLns L{JsCirc(0.f, 0.f, 0.82f, 22), JsCirc(0.f, -0.98f, 0.14f, 8)};
		for (int32 K = 0; K < 4; ++K) { const float A = K * TwoPi / 4.f; L.Add(JsSeg(FMath::Cos(A) * 0.62f, FMath::Sin(A) * 0.62f, FMath::Cos(A) * 0.74f, FMath::Sin(A) * 0.74f)); }
		L.Add(JsSeg(0.f, 0.f, FMath::Cos(T * 9.f - PI / 2.f) * 0.6f, FMath::Sin(T * 9.f - PI / 2.f) * 0.6f));
		L.Add(JsSeg(0.f, 0.f, FMath::Cos(T * 1.5f) * 0.38f, FMath::Sin(T * 1.5f) * 0.38f));
		return L;
	}
	case 21: // 험프티 덤프티: 금 간 알
		return {JsCirc(0.f, 0.f, 0.62f, 20, 1.f, 1.3f), FJsLn{JsV(-0.6f, -0.05f), JsV(-0.35f, 0.15f), JsV(-0.15f, -0.1f), JsV(0.1f, 0.15f), JsV(0.32f, -0.1f), JsV(0.6f, 0.1f)}};
	case 22: // 붉은 여왕: 체스 퀸
		return {JsSeg(-0.65f, 0.9f, 0.65f, 0.9f), FJsLn{JsV(-0.48f, 0.9f), JsV(-0.25f, -0.2f), JsV(0.25f, -0.2f), JsV(0.48f, 0.9f)},
			FJsLn{JsV(-0.38f, -0.2f), JsV(-0.48f, -0.62f), JsV(-0.2f, -0.38f), JsV(0.f, -0.78f), JsV(0.2f, -0.38f), JsV(0.48f, -0.62f), JsV(0.38f, -0.2f)}, JsCirc(0.f, -0.95f, 0.12f, 6), JsSeg(-0.35f, 0.4f, 0.35f, 0.4f)};
	case 23: // 가짜 거북: 등껍질 + 머리 + 지느러미
	{
		FJsLn Hx;
		for (int32 K = 0; K <= 6; ++K) Hx.Add(JsV(FMath::Cos(K * TwoPi / 6.f) * 0.32f, FMath::Sin(K * TwoPi / 6.f) * 0.32f));
		const float Fl = FMath::Sin(T * 16.f) * 0.15f;
		return {JsCirc(0.f, 0.f, 0.8f, 20, 1.f, 0.68f), Hx, JsCirc(1.f, 0.f, 0.2f, 8), JsSeg(0.4f, -0.5f, 0.6f, -0.9f - Fl), JsSeg(0.4f, 0.5f, 0.6f, 0.9f + Fl), JsSeg(-0.6f, -0.42f, -0.85f, -0.7f), JsSeg(-0.6f, 0.42f, -0.85f, 0.7f)};
	}
	case 24: // 그리핀: 퍼덕이는 날개 + 부리
	{
		const float F = 0.55f + 0.45f * FMath::Sin(T * 18.f);
		FJsLns L;
		for (float S : {-1.f, 1.f})
		{
			FJsLn W = JsBez(JsV(0.f, 0.f), JsV(S * 0.6f, -1.f * F), JsV(S * 1.15f, -0.55f * F));
			W.Append(FJsLn{JsV(S * 0.95f, -0.15f * F), JsV(S * 0.8f, -0.05f), JsV(S * 0.6f, 0.1f), JsV(0.f, 0.1f)});
			L.Add(W);
		}
		L.Add(FJsLn{JsV(-0.18f, -0.05f), JsV(0.f, -0.45f), JsV(0.18f, -0.05f)});
		return L;
	}
	case 25: return JsGrin(false); // 웃음만 남은 고양이
	case 26: return JsSnow(); // 하얀 여왕
	case 27: // 바다코끼리와 목수: 굴 껍데기
	{
		FJsLn Sh = JsArc(0.f, 0.35f, 0.85f, 200.f, 340.f, 12);
		Sh.Add(JsV(0.f, 0.55f));
		Sh.Add(Sh[0]);
		FJsLns L{Sh};
		for (float A : {230.f, 255.f, 285.f, 310.f}) { const float R = FMath::DegreesToRadians(A); L.Add(JsSeg(0.f, 0.55f, FMath::Cos(R) * 0.75f, 0.35f + FMath::Sin(R) * 0.75f)); }
		return L;
	}
	case 28: // 하얀 기사: 랜스 / 방패
		if (bSecond) return {FJsLn{JsV(-0.55f, -0.6f), JsV(0.55f, -0.6f), JsV(0.5f, 0.2f), JsV(0.f, 0.75f), JsV(-0.5f, 0.2f), JsV(-0.55f, -0.6f)}, JsSeg(0.f, -0.6f, 0.f, 0.75f), JsSeg(-0.52f, -0.15f, 0.52f, -0.15f)};
		return {JsSeg(-1.25f, 0.f, 0.6f, 0.f), FJsLn{JsV(0.5f, -0.2f), JsV(1.35f, 0.f), JsV(0.5f, 0.2f), JsV(0.5f, -0.2f)}, FJsLn{JsV(-0.55f, -0.38f), JsV(-0.15f, 0.f), JsV(-0.55f, 0.38f)}};
	case 29: // 말하는 꽃들: 도는 데이지
	{
		FJsLns L{JsCirc(0.f, 0.f, 0.22f, 8)};
		for (int32 K = 0; K < 6; ++K)
		{
			const float A = K * TwoPi / 6.f + T * 3.f;
			L.Append(JsXf({JsCirc(0.f, 0.f, 1.f, 10, 1.f, 0.5f)}, JsV(FMath::Cos(A) * 0.58f, FMath::Sin(A) * 0.58f), A, 0.32f));
		}
		return L;
	}
	case 30: return {JsHeart(0.f, 0.f, 0.85f)}; // 하트 여왕
	case 31: // 홍학 크로케: 도는 가시 공
	{
		FJsLns L{JsCirc(0.f, 0.f, 0.5f, 14)};
		for (int32 K = 0; K < 9; ++K) { const float A = K * TwoPi / 9.f + T * 8.f; L.Add(JsSeg(FMath::Cos(A) * 0.5f, FMath::Sin(A) * 0.5f, FMath::Cos(A) * 0.9f, FMath::Sin(A) * 0.9f)); }
		return L;
	}
	case 32: // 트럼프 근위대: 스페이드 카드
	{
		const float W = 0.55f, H = 0.8f, R = 0.15f;
		FJsLn Box = JsArc(W - R, -H + R, R, -90.f, 0.f, 3);
		JsAppend(Box, JsArc(W - R, H - R, R, 0.f, 90.f, 3), 0);
		JsAppend(Box, JsArc(-W + R, H - R, R, 90.f, 180.f, 3), 0);
		JsAppend(Box, JsArc(-W + R, -H + R, R, 180.f, 270.f, 3), 0);
		Box.Add(Box[0]);
		return {Box, JsHeart(0.f, -0.02f, 0.3f, true), FJsLn{JsV(0.f, 0.15f), JsV(0.12f, 0.32f), JsV(-0.12f, 0.32f), JsV(0.f, 0.15f)}};
	}
	case 33: // 체스판: 2×2 체크 (두 칸 빗금)
	{
		FJsLns L{FJsLn{JsV(-0.75f, -0.75f), JsV(0.75f, -0.75f), JsV(0.75f, 0.75f), JsV(-0.75f, 0.75f), JsV(-0.75f, -0.75f)}, JsSeg(0.f, -0.75f, 0.f, 0.75f), JsSeg(-0.75f, 0.f, 0.75f, 0.f)};
		for (const FVector2f& O : {JsV(-0.75f, -0.75f), JsV(0.f, 0.f)})
			for (float K : {0.25f, 0.5f}) { L.Add(JsSeg(O.X + K, O.Y, O.X, O.Y + K)); L.Add(JsSeg(O.X + 0.75f, O.Y + K, O.X + K, O.Y + 0.75f)); }
		return L;
	}
	case 34: // 겨울잠쥐: Z z z
	{
		FJsLns L;
		const float Zs[3][3] = {{-0.55f, 0.35f, 0.5f}, {0.15f, -0.05f, 0.36f}, {0.65f, -0.5f, 0.26f}};
		for (int32 K = 0; K < 3; ++K)
		{
			const float X = Zs[K][0], Y = Zs[K][1] + FMath::Sin(T * 6.f + K) * 0.08f, S = Zs[K][2];
			L.Add(FJsLn{JsV(X - S, Y - S), JsV(X + S, Y - S), JsV(X - S, Y + S), JsV(X + S, Y + S)});
		}
		return L;
	}
	case 35: // 유니콘: 나선 뿔
	{
		FJsLns L{FJsLn{JsV(-0.9f, -0.32f), JsV(1.2f, 0.f), JsV(-0.9f, 0.32f), JsV(-0.9f, -0.32f)}};
		const float Ph = FMath::Fmod(T * 4.f, 1.f);
		for (int32 K = 0; K < 4; ++K)
		{
			const float X = -0.75f + (K + Ph) * 0.45f, H = 0.32f * (1.2f - X) / 2.1f;
			if (X < 1.05f) L.Add(JsSeg(X - 0.12f, -H, X + 0.12f, H));
		}
		return L;
	}
	case 36: return {JsArc(-0.9f, -0.5f, 1.3f, -50.f, 50.f, 10), JsArc(-0.9f, 0.f, 1.3f, -50.f, 50.f, 10), JsArc(-0.9f, 0.5f, 1.3f, -50.f, 50.f, 10)}; // 재버워키: 발톱 세 줄
	case 37: // 사자: 갈기 + 얼굴
	{
		FJsLn Mane;
		for (int32 K = 0; K <= 22; ++K) { const float R = (K % 2) ? 0.95f : 0.68f; Mane.Add(JsV(R * FMath::Cos(K * TwoPi / 22.f), R * FMath::Sin(K * TwoPi / 22.f))); }
		return {Mane, JsCirc(0.f, 0.f, 0.45f, 14), JsCirc(-0.16f, -0.08f, 0.05f, 4), JsCirc(0.16f, -0.08f, 0.05f, 4), FJsLn{JsV(-0.12f, 0.18f), JsV(0.f, 0.26f), JsV(0.12f, 0.18f)}};
	}
	case 38: // 꿈꾸는 붉은 왕: 꿈 구름 + 작은 왕관
	{
		FJsLn C = JsArc(-0.55f, 0.1f, 0.38f, 90.f, 250.f, 6);
		JsAppend(C, JsArc(-0.1f, -0.15f, 0.45f, 200.f, 330.f, 6), 0);
		JsAppend(C, JsArc(0.5f, 0.05f, 0.4f, 250.f, 450.f, 8), 0);
		C.Add(JsV(-0.55f, 0.48f));
		return {C, FJsLn{JsV(-0.3f, -0.6f), JsV(-0.35f, -0.95f), JsV(-0.15f, -0.75f), JsV(0.f, -1.f), JsV(0.15f, -0.75f), JsV(0.35f, -0.95f), JsV(0.3f, -0.6f), JsV(-0.3f, -0.6f)}};
	}
	case 39: // 반짝반짝 작은 박쥐: 퍼덕이는 박쥐
	{
		const float F = 0.6f + 0.4f * FMath::Sin(T * 20.f);
		FJsLns L{JsCirc(0.f, 0.f, 0.22f, 8, 1.f, 1.4f), JsSeg(-0.12f, -0.28f, -0.18f, -0.48f), JsSeg(0.12f, -0.28f, 0.18f, -0.48f)};
		for (float S : {-1.f, 1.f})
			L.Add(FJsLn{JsV(S * 0.2f, -0.1f), JsV(S * 0.65f, -0.55f * F), JsV(S * 1.2f, -0.35f * F), JsV(S * 1.f, 0.1f * F), JsV(S * 0.8f, -0.02f), JsV(S * 0.62f, 0.22f * F), JsV(S * 0.42f, 0.02f), JsV(S * 0.2f, 0.2f)});
		return L;
	}
	default: return {JsCirc(0.f, 0.f, 0.6f, 12)};
	}
}

// 무리마다 몸통의 방향: 진행 방향 / 빙글 / 천천히 돎 / 늘 똑바로
float JsBodyAngle(EJsFam Fam, float DirAng, float T, int32 Copy, float U)
{
	switch (Fam)
	{
	case EJsFam::Fan: return DirAng + (Copy - 1) * 0.35f * FMath::Cos(PI * U * 0.9f);
	case EJsFam::Slash: return DirAng;
	case EJsFam::Boomerang: return T * 14.f;
	case EJsFam::Spin: return T * 11.f;
	case EJsFam::Comet: return T * 3.f;
	default: return 0.f;
	}
}

int32 JsCopies(EJsFam Fam) { return Fam == EJsFam::Fan ? 3 : Fam == EJsFam::Twin ? 2 : 1; }

} // namespace PDJokerShotsImpl

using namespace PDJokerShotsImpl;

void SPDBoardWidget::DrawJokerShape(FCanvas& F, const TArray<TArray<FVector2f>>& L, const FLinearColor& Col, bool bWhiteCore)
{
	for (int32 I = 0; I < L.Num(); ++I)
	{
		if (I < 2) F.Glow(L[I], Col, 1.4f, 1.f); // 바깥 모양 둘만 번짐 (선 수 = 그리기 부담, D-138)
		else F.Lines(L[I], Col, 1.2f);
		if (bWhiteCore) F.Lines(L[I], WithAlpha(FLinearColor::White, Col.A * 0.8f), 0.6f);
	}
}

namespace PDJokerShotsImpl
{
// 타격이 몰리는 프레임에는 파편을 줄인다 (PDBoardFx 의 CountHit 와 같은 생각, D-136)
float GJsHitClock = -1.f;
int32 GJsHits = 0;

constexpr float JsShotR = 11.f; // 발사체 반지름 (화면 px, 예전 3~7)
} // namespace PDJokerShotsImpl

FLinearColor SPDBoardWidget::JokerFxColor(int32 Fx) { return Hex(JsDef(Fx).Col); }

int32 SPDBoardWidget::JokerFxAt(int32 Key) const
{
	auto It = Game.Towers.find(Key);
	if (It == Game.Towers.end()) return -1;
	const int32 J = It->second.Joker;
	return J >= 0 && J < 40 ? J : -1;
}

int32 SPDBoardWidget::JokerFxOf(const FPDProjectile& Pr) const
{
	const int32 Fx = JokerFxAt(Pr.Src);
	if (Fx >= 0) return Fx;
	return Pr.Kind >= EPDShot::Juggle ? (int32)Pr.Kind - (int32)EPDShot::Juggle : -1; // 쏜 타워가 사라졌으면 발사체 종류의 기본 조커
}

void SPDBoardWidget::SnapShots()
{
	ShotSnaps.Reset();
	for (const FPDProjectile& Pr : Game.Projectiles)
		if (Pr.Kind >= EPDShot::Juggle) ShotSnaps.Add({Pr.Kind, Pr.X, Pr.Y, Pr.VX, Pr.VY, JokerFxOf(Pr)});
}

// 닿음 사건에는 쏜 타워가 없어서, 틱 직전에 찍어 둔 발사체 중 같은 종류 · 같은 속도(없으면 가장 가까운 것)를 찾는다
int32 SPDBoardWidget::JokerFxOfImpact(const FPDEvent& E) const
{
	const EPDShot K = (EPDShot)(int)E.R;
	int32 Best = -1;
	float BestD = 1e9f;
	for (const FShotSnap& S : ShotSnaps)
	{
		if (S.Kind != K) continue;
		if (S.VX == E.X2 && S.VY == E.Y2) return S.Fx;
		const float D = FMath::Square(S.X - E.X) + FMath::Square(S.Y - E.Y);
		if (D < BestD) { BestD = D; Best = S.Fx; }
	}
	return Best >= 0 ? Best : (K >= EPDShot::Juggle ? (int32)K - (int32)EPDShot::Juggle : 0);
}

// 날아가는 조커 발사체의 화면 위치: 규칙상 위치(직선) + 무리마다 다른 그림 오프셋 (포물선 · 휘기 · 물결 · 부채꼴 · 감기). 닿는 순간 오프셋은 0
FVector2f SPDBoardWidget::JokerShotPos(const FPDProjectile& Pr, int32 Fx, int32 Copy, float& OutU, float& OutAng) const
{
	const FJokerShotDef& D = JsDef(Fx);
	const FVector2f At0 = CellPx(Pr.X, Pr.Y);
	const float VL = FMath::Max(0.001f, FMath::Sqrt(Pr.VX * Pr.VX + Pr.VY * Pr.VY));
	const FVector2f Dir(Pr.VX / VL, Pr.VY / VL), Nrm(-Dir.Y, Dir.X);
	const float Spd = FMath::Max(0.1f, ShotSpeed(Pr.Kind));
	const float Rem = FMath::Sqrt(FMath::Square(Pr.TX - Pr.X) + FMath::Square(Pr.TY - Pr.Y)) / Spd;
	const float T = FMath::Max(0.f, Pr.Life), U = FMath::Clamp(T / FMath::Max(0.001f, T + Rem), 0.f, 1.f);
	const float SinU = FMath::Sin(PI * U);
	FVector2f Off(0.f, 0.f);
	switch (D.Fam)
	{
	case EJsFam::Lob:
		Off.Y = -SinU * 30.f;
		if (D.Extra == EJsExtra::Bounce) Off.Y -= FMath::Abs(FMath::Sin(U * PI * 3.f)) * 6.f;
		break;
	case EJsFam::Boomerang: Off = Nrm * SinU * 26.f; break;
	case EJsFam::Twin: Off = Nrm * (FMath::Sin(T * 16.f) * 7.f * (Copy ? -1.f : 1.f) * FMath::Sin(PI * FMath::Min(1.f, U * 1.2f + 0.1f))); break;
	case EJsFam::Fan: Off = Nrm * ((Copy - 1) * SinU * 16.f) - Dir * (FMath::Abs(Copy - 1) * 6.f * (1.f - U)); break;
	case EJsFam::Wave: Off = Nrm * FMath::Sin(T * 14.f) * 7.f; break;
	default: break;
	}
	OutU = U;
	OutAng = JsBodyAngle(D.Fam == EJsFam::Instant ? EJsFam::Spin : D.Fam, FMath::Atan2(Dir.Y, Dir.X), T, Copy, U);
	return At0 + Off;
}

void SPDBoardWidget::PaintJokerShot(FCanvas& F, const FPDProjectile& Pr, int32 Fx) const
{
	const FJokerShotDef& D = JsDef(Fx);
	const FLinearColor Col = Hex(D.Col), Col2 = D.Col2 ? Hex(D.Col2) : Col;
	const float VL = FMath::Max(0.001f, FMath::Sqrt(Pr.VX * Pr.VX + Pr.VY * Pr.VY));
	const FVector2f Dir(Pr.VX / VL, Pr.VY / VL), Nrm(-Dir.Y, Dir.X);
	const float DirAng = FMath::Atan2(Dir.Y, Dir.X), T = Pr.Life;
	const float A = D.Extra == EJsExtra::Blink ? 0.35f + 0.65f * (0.5f + 0.5f * FMath::Cos(T * 28.f)) : 1.f;
	for (int32 C = 0; C < JsCopies(D.Fam); ++C)
	{
		float U = 0.f, Ang = 0.f;
		const FVector2f P = JokerShotPos(Pr, Fx, C, U, Ang);
		const bool bSecond = C == 1 && (Fx == 3 || Fx == 28);
		const FLinearColor Cl = WithAlpha(C == 1 ? Col2 : Col, A);
		// 꼬리: 혜성 줄 셋, 소리 고리, 잔상 (점 꼬리는 UpdateFx 의 입자)
		if (D.Fam == EJsFam::Comet)
			for (int32 S = -1; S <= 1; ++S) F.Lines({P - Dir * 4.f + Nrm * S * 1.5f, P - Dir * 34.f + Nrm * S * 7.f}, WithAlpha(Col, 0.55f), 1.2f);
		if (D.Fam == EJsFam::Ring)
			for (int32 K = 0; K < 3; ++K) F.Lines(ArcPts(P - Dir * (6.f + K * 7.f), 7.f + K * 4.f, DirAng, 0.95f, 8), WithAlpha(Col, 0.9f - K * 0.25f), 1.2f);
		if (D.Extra == EJsExtra::Ghost)
			for (int32 K = 1; K <= 2; ++K) DrawJokerShape(F, JsXf(JsShape(Fx, T - 0.05f * K, false), P - Dir * 10.f * K, Ang, JsShotR), WithAlpha(Col, 0.4f - 0.15f * K), false);
		DrawJokerShape(F, JsXf(JsShape(Fx, T, bSecond), P, Ang, JsShotR), Cl, D.Fam == EJsFam::Comet);
		if (D.Extra == EJsExtra::Mirror) // 거울상: 길(직선) 건너편에 좌우 반전, 옅게
		{
			const FVector2f At0 = CellPx(Pr.X, Pr.Y), M = At0 * 2.f - P;
			FJsLns L = JsShape(Fx, T, false);
			for (FJsLn& Ln : L) for (FVector2f& Q : Ln) Q.X = -Q.X;
			DrawJokerShape(F, JsXf(L, M, 0.f, JsShotR), WithAlpha(Col, 0.4f), false);
		}
	}
}

// 날아가는 동안 뒤에 남는 입자: 점 꼬리 (무리 색), 장미는 떨어지는 페인트 방울
void SPDBoardWidget::JokerTrail(const FPDProjectile& Pr, int32 Fx)
{
	const FJokerShotDef& D = JsDef(Fx);
	if (D.Fam == EJsFam::Comet || D.Fam == EJsFam::Ring || D.Extra == EJsExtra::Ghost) return; // 그림에서 꼬리를 그린다
	const FLinearColor Col = Hex(D.Col), Col2 = D.Col2 ? Hex(D.Col2) : Col;
	for (int32 C = 0; C < JsCopies(D.Fam) && Particles.Num() < 2600; ++C)
	{
		float U = 0.f, Ang = 0.f;
		const FVector2f P = JokerShotPos(Pr, Fx, C, U, Ang);
		const FVector2f Jit(FMath::FRandRange(-3.f, 3.f), FMath::FRandRange(-3.f, 3.f));
		Particles.Add({P, Jit, 0.25f, 0.25f, 1.8f, 2.f, 0.f, WithAlpha(C == 1 ? Col2 : Col, 0.7f), false});
		if (D.Extra == EJsExtra::Paint && FMath::FRand() < 0.4f) Particles.Add({P + FVector2f(-2.f, 4.f), FVector2f(0.f, 20.f), 0.5f, 0.5f, 2.2f, 0.5f, 160.f, WithAlpha(Col, 0.85f), false});
	}
}

// 닿음: 자기 실루엣 도장 (커지며 사라짐) + 무리마다 다른 파편 + 월드 층
void SPDBoardWidget::JokerImpactFx(int32 Fx, FVector2f P, FVector2f Dir, float Power)
{
	if (Clock != GJsHitClock) { GJsHitClock = Clock; GJsHits = 0; }
	const int32 HitN = ++GJsHits;
	const FJokerShotDef& D = JsDef(Fx);
	const FLinearColor Col = Hex(D.Col), Col2 = D.Col2 ? Hex(D.Col2) : Col, White = FLinearColor::White;
	const float Pw = FMath::Clamp(Power, 1.f, 6.f), Ang = FMath::Atan2(Dir.Y, Dir.X), Rnd = FMath::FRandRange(0.f, TwoPi);
	const float BodyAng = D.Fam == EJsFam::Fan || D.Fam == EJsFam::Slash ? Ang : 0.f;
	const bool bInstant = D.Fam == EJsFam::Instant;
	if (HitN <= 16)
	{
		Stamps.Add({P, 0.f, bInstant ? 0.6f : 0.45f, BodyAng, bInstant ? 1.6f : 1.f, bInstant ? 1.4f : 2.3f, Fx, false});
		if (Fx == 3 || Fx == 28) Stamps.Add({P + FVector2f(8.f, 0.f), 0.f, 0.45f, 0.f, 0.8f, 1.8f, Fx, true});
	}
	PushWorldFx(bInstant || D.Fam == EJsFam::Ring ? EWorldFx::ImpZap : D.Fam == EJsFam::Slash ? EWorldFx::ImpSpark : D.Fam == EJsFam::Comet ? EWorldFx::ImpStarburst : EWorldFx::ImpGlitter, P, Col, Pw, Dir);
	if (HitN > 24) return; // 몰리면 도장 · 월드 층만
	const float Q = (Particles.Num() > 1800 || HitN > 8) ? 0.35f : 1.f;
	auto Nq = [&](float N) { return FMath::Max(1, FMath::RoundToInt(N * Q)); };
	auto Wave = [&](EWave S, int32 N, float R0, float R1, float Delay, float Life, float Th, const FLinearColor& Cl, float Rot, float Spin) {
		if (HitN > 12 && Delay > 0.f) return;
		Rings.Add({P, R0, R1, -Delay, Life, Th, Cl, S, N, Rot, Spin});
	};
	Wave(EWave::Circle, 0, 3.f, 12.f, 0.f, 0.2f, 1.f, Col, 0.f, 0.f); // 공통: 작은 고리
	switch (D.Imp)
	{
	case EJsImp::Confetti: // 위로 튀었다 떨어지는 색종이 (흰색 섞음)
		BurstCone(P, FVector2f(0.f, -1.f), 1.2f, Nq(6), Col, 90.f, 220.f, 0.8f, 2.4f, false, 480.f);
		BurstCone(P, FVector2f(0.f, -1.f), 1.2f, Nq(4), White, 90.f, 220.f, 0.8f, 2.2f, false, 480.f);
		break;
	case EJsImp::Poof: // 펑 연기
		Wave(EWave::Circle, 0, 4.f, 18.f, 0.f, 0.35f, 1.4f, Col, 0.f, 0.f);
		Burst(P, Nq(8), Hex(TEXT("8a8090"), 0.45f), 20.f, 70.f, 0.7f, 3.6f, false, -30.f);
		break;
	case EJsImp::Crescent: Wave(EWave::Broken, 2, 6.f, 22.f, 0.f, 0.3f, 1.5f, Col, Rnd, 9.f); break; // 웃는 호 둘이 돌며 퍼짐
	case EJsImp::Cross: // 분홍 · 하늘 삼각형이 엇갈려 육각별
		Wave(EWave::Poly, 3, 5.f, 19.f, 0.f, 0.32f, 1.5f, Col, -PI / 2.f, 2.f);
		Wave(EWave::Poly, 3, 5.f, 19.f, 0.f, 0.32f, 1.5f, Col2, PI / 2.f, -2.f);
		break;
	case EJsImp::Shards: AddShards(P, Nq(8), Col, 100.f, 220.f, 4.f, 7.f, 0.55f); break; // 깨져 흩어지는 조각
	case EJsImp::Sparkle: Burst(P, Nq(10), Col, 20.f, 70.f, 0.8f, 1.8f, false, -140.f); break; // 떠오르는 반짝이
	case EJsImp::XCut: // X 자 베기 + 흰 줄기
		Beams.Add({P + FVector2f(-18.f, -13.f), P + FVector2f(18.f, 13.f), 0.f, 0.22f, 2.4f, Col});
		Beams.Add({P + FVector2f(18.f, -13.f), P + FVector2f(-18.f, 13.f), 0.f, 0.22f, 2.4f, Col});
		Burst(P, Nq(10), White, 120.f, 300.f, 0.22f, 1.5f, true);
		Shake = FMath::Max(Shake, 0.06f);
		break;
	case EJsImp::Spiral: Wave(EWave::Broken, 3, 26.f, 6.f, 0.f, 0.4f, 1.5f, Col, Rnd, 7.f); break; // 안으로 감겨 듦
	case EJsImp::Rings: for (int32 J = 0; J < 4; ++J) Wave(EWave::Broken, 4, 4.f, 20.f + J * 8.f, 0.06f * J, 0.3f, 1.8f - J * 0.3f, Col, Ang + PI / 4.f, 0.f); break; // 차례로 퍼지는 소리 호
	case EJsImp::Rays: // 빛살
		Wave(EWave::Rays, 8, 8.f, 34.f, 0.f, 0.3f, 1.4f, Col, Rnd, 0.f);
		Flares.Add({P, 0.f, 0.25f, 22.f, Rnd, Col, 4});
		break;
	case EJsImp::Paint: Burst(P, Nq(10), Col, 60.f, 160.f, 0.6f, 3.f, false, 260.f); break; // 튀는 페인트 방울
	case EJsImp::Feathers: // 천천히 흩날리는 깃털
		AddShards(P, Nq(6), Col, 30.f, 90.f, 6.f, 9.f, 0.9f);
		break;
	case EJsImp::Smoke: for (int32 J = 0; J < 3; ++J) Wave(EWave::Circle, 0, 4.f + J * 2.f, 16.f + J * 5.f, 0.07f * J, 0.45f, 1.2f, Col, 0.f, 0.f); break; // 커지는 연기 고리
	case EJsImp::Mirror: // 네모 거울 + 흰 반사 줄기
		Wave(EWave::Poly, 4, 4.f, 20.f, 0.f, 0.3f, 1.4f, Col, PI / 4.f, 0.f);
		Burst(P, Nq(6), White, 120.f, 260.f, 0.2f, 1.4f, true);
		break;
	case EJsImp::Splash: // 찻물이 튐 (떨어짐)
		BurstCone(P, FVector2f(0.f, -1.f), 0.9f, Nq(12), Col, 80.f, 200.f, 0.6f, 2.f, false, 520.f);
		break;
	case EJsImp::Crumbs: Burst(P, Nq(12), Col, 50.f, 140.f, 0.7f, 2.4f, false, 380.f); break; // 부스러기
	case EJsImp::Tick: Wave(EWave::Rays, 12, 12.f, 22.f, 0.f, 0.3f, 1.2f, Col, 0.f, 0.f); break; // 째깍: 시계 눈금 열둘
	case EJsImp::Crack: // 알이 깨짐: 반쪽 호 둘 + 껍데기 조각
		Wave(EWave::Broken, 2, 7.f, 18.f, 0.f, 0.35f, 1.5f, Col, 0.f, 0.f);
		AddShards(P, Nq(6), Col, 80.f, 180.f, 4.f, 7.f, 0.6f);
		break;
	case EJsImp::Tears: Burst(P, Nq(6), Col, 10.f, 40.f, 0.7f, 2.f, false, 300.f); break; // 떨어지는 눈물
	case EJsImp::Frost: // 눈송이 결정 + 흰 눈
		Wave(EWave::Star, 6, 6.f, 22.f, 0.f, 0.35f, 1.3f, Col, Rnd, 2.f);
		Burst(P, Nq(10), White, 30.f, 90.f, 0.6f, 1.8f, false, 40.f);
		break;
	case EJsImp::Bubbles: // 떠오르는 거품
		for (int32 J = 0; J < 3; ++J) Rings.Add({P + FVector2f(FMath::FRandRange(-10.f, 10.f), FMath::FRandRange(-6.f, 2.f)), 1.f, FMath::FRandRange(3.f, 6.f), -0.05f * J, 0.4f, 1.f, Col});
		Burst(P, Nq(6), Col, 10.f, 40.f, 0.8f, 1.6f, false, -120.f);
		break;
	case EJsImp::Petals: AddShards(P, Nq(8), Col, 60.f, 160.f, 4.f, 6.f, 0.7f); break; // 꽃잎
	case EJsImp::Hearts: // 퍼지는 하트 + X
		for (int32 J = 0; J < 3; ++J) Wave(EWave::Suit, (int32)EPDSuit::Heart, 3.f, 12.f + J * 6.f, 0.05f * J, 0.28f, 1.4f, Col, 0.f, 0.f);
		Beams.Add({P + FVector2f(-14.f, -14.f), P + FVector2f(14.f, 14.f), 0.f, 0.2f, 2.f, Col});
		Beams.Add({P + FVector2f(14.f, -14.f), P + FVector2f(-14.f, 14.f), 0.f, 0.2f, 2.f, Col});
		break;
	case EJsImp::Cards: // 날리는 카드 조각 + 도는 네모
		AddShards(P, Nq(6), Col, 70.f, 170.f, 6.f, 8.f, 0.8f);
		Wave(EWave::Poly, 4, 4.f, 16.f, 0.f, 0.3f, 1.3f, Col, Rnd, 6.f);
		break;
	case EJsImp::Squares: for (int32 J = 0; J < 2; ++J) Wave(EWave::Poly, 4, 4.f + J * 4.f, 16.f + J * 8.f, 0.08f * J, 0.3f, 1.4f, Col, PI / 4.f * J, 0.f); break; // 체크 네모
	case EJsImp::Zzz: // 떠오르는 Zz
		if (Floats.Num() < 50) Floats.Add({TEXT("Zz"), P + FVector2f(10.f, -12.f), 0.f, 0.7f, 12.f, Col});
		Wave(EWave::Circle, 0, 4.f, 18.f, 0.f, 0.4f, 1.3f, Col, 0.f, 0.f);
		break;
	}
	if (Pw >= 5.f) Shake = FMath::Max(Shake, 0.04f + 0.02f * (Pw - 5.f));
}

void SPDBoardWidget::PaintStamps(FCanvas& F) const
{
	for (const FStamp& S : Stamps)
	{
		if (S.T < 0.f) continue;
		const float U = FMath::Clamp(S.T / S.MaxT, 0.f, 1.f), E = 1.f - (1.f - U) * (1.f - U) * (1.f - U);
		const bool bInstant = JsDef(S.Fx).Fam == EJsFam::Instant;
		const float K = S.S0 + (S.S1 - S.S0) * (bInstant ? U : E), A = bInstant ? 1.f - U * U : 1.f - U;
		const FJokerShotDef& D = JsDef(S.Fx);
		const FLinearColor Col = WithAlpha(Hex(S.bSecond && D.Col2 ? D.Col2 : D.Col), A);
		DrawJokerShape(F, JsXf(JsShape(S.Fx, S.T, S.bSecond), S.P, S.Ang, JsShotR * K), Col, false);
	}
}

void SPDBoardWidget::UpdateStamps(float Dt)
{
	for (FStamp& S : Stamps) S.T += Dt;
	Stamps.RemoveAll([](const FStamp& S) { return S.T >= S.MaxT; });
	if (Stamps.Num() > 60) Stamps.RemoveAt(0, Stamps.Num() - 60, EAllowShrinking::No);
}
