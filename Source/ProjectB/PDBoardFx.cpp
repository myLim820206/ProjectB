// 포커 디펜스 화면 이펙트: 게임 사건(FPDEvent)을 Slate 파티클·파동·별빛과 월드 이펙트 요청(나이아가라, D-092)으로 바꾼다
#include "PDBoardDraw.h"

void SPDBoardWidget::Burst(FVector2f P, int N, FLinearColor Col, float SpeedMin, float SpeedMax, float Life, float Size, bool bStreak, float Gravity)
{
	for (int I = 0; I < N && Particles.Num() < 2600; ++I)
	{
		const float A = FMath::FRandRange(0.f, TwoPi), Sp = FMath::FRandRange(SpeedMin, SpeedMax);
		FParticle Pa;
		Pa.P = P;
		Pa.V = FVector2f(FMath::Cos(A), FMath::Sin(A)) * Sp;
		Pa.MaxLife = Pa.Life = Life * FMath::FRandRange(0.6f, 1.2f);
		Pa.Size = Size * FMath::FRandRange(0.7f, 1.3f);
		Pa.Drag = 3.f;
		Pa.Gravity = Gravity;
		Pa.Col = Col;
		Pa.bStreak = bStreak;
		Particles.Add(Pa);
	}
}

namespace
{
// 맞을 때 이펙트 세기 (1 약 ~ 6 강): 레벨 + 족보·조커 타워 + 족보·조커 발사체 + 치명타 (3번까지)
float ShotPower(EPDShot K, int32 Lv, bool bHandTower, int32 Crits)
{
	return Lv + (bHandTower ? 1.f : 0.f) + (IsJokerShot(K) || IsHandShot(K) ? 1.5f : 0.f) + FMath::Min(Crits, 3);
}
} // namespace

void SPDBoardWidget::SuckIn(FVector2f P, int PerCell, float Jitter, float Life, float SizeMin, float SizeMax, const FLinearColor& ColA, const FLinearColor& ColB, int Every)
{
	for (const FVector2f& From : PendingFuseCells)
		for (int I = 0; I < PerCell; ++I)
		{
			FParticle Pa;
			Pa.P = From + FVector2f(FMath::FRandRange(-Jitter, Jitter), FMath::FRandRange(-Jitter, Jitter));
			Pa.V = (P - Pa.P) / Life;
			Pa.MaxLife = Pa.Life = Life;
			Pa.Size = FMath::FRandRange(SizeMin, SizeMax);
			Pa.Drag = 0.f;
			Pa.Gravity = 0.f;
			Pa.Col = I % Every ? ColA : ColB;
			Pa.bStreak = true;
			Particles.Add(Pa);
		}
	PendingFuseCells.Reset();
}

void SPDBoardWidget::HandleEvents()
{
	for (const FPDEvent& E : Game.Events)
	{
		const FVector2f P = CellPx(E.X, E.Y);
		switch (E.Kind)
		{
		case FPDEvent::EKind::Shot:
		{
			const EPDShot K = (EPDShot)(int)E.R;
			const FLinearColor Col = ShotColor(K, E.Suit);
			const int32 Lv = FMath::Clamp(E.Lv, 1, MaxLevel);
			const FVector2f To = CellPx(E.X2, E.Y2), Dir = (To - P).GetSafeNormal();
			const float W = (E.A ? 2.6f : 1.6f) + (E.B > 0 ? 1.4f : 0.f);
			// 총구: 고리 + 쏘는 쪽으로 튀는 섬광 (레벨이 높을수록 크게)
			Rings.Add({P, 4.f, 12.f + 3.f * Lv, 0.f, 0.15f, 1.5f, Col});
			Burst(P + Dir * 10.f, 1 + Lv, Col, 40.f, 110.f, 0.14f, 1.6f, true);
			PushWorldFx(EWorldFx::Muzzle, P + Dir * 10.f, Col, (float)Lv, Dir);
			switch (K) // 즉시 발사체는 쏘는 순간 빛줄기가 닿는다 (날아가는 것은 Impact 에서)
			{
			case EPDShot::Laser: Beams.Add({P, To, 0.f, 0.12f + 0.03f * Lv, W + 0.5f * (Lv - 1), Col, Lv >= 3 ? 1 : 0, Lv}); break;
			case EPDShot::JokerLaser: Beams.Add({P, To, 0.f, 0.14f, W + 1.f, Col}); break;
			case EPDShot::Arc: Beams.Add({P, To, 0.f, 0.18f, W, Col, 2, Lv}); break;
			case EPDShot::Crown: Beams.Add({P, To, 0.f, 0.3f, W + 2.f, Col, 3, Lv}); break;
			case EPDShot::Grin: Beams.Add({P, To, 0.f, 0.3f, W + 0.5f, Col, 4, Lv}); break;
			default: break;
			}
			if (ShotSpeed(K) <= 0.f)
			{
				const float Pw = ShotPower(K, Lv, E.A != 0, E.B);
				ImpactFx(K, To, Dir, Col, Lv, Pw);
				HitFx(To, Pw, E.B, E.Dmg);
				if (E.B > 0) CritFx(To, E.B);
			}
			break;
		}
		case FPDEvent::EKind::Impact: // 발사체가 닿음: 종류마다 다른 타격 효과 + 공통(번쩍임·숫자)
		{
			const EPDShot K = (EPDShot)(int)E.R;
			const FLinearColor Col = ShotColor(K, E.Suit);
			const int32 Lv = FMath::Clamp(E.Lv, 1, MaxLevel);
			const FVector2f Dir = FVector2f(E.X2, E.Y2).GetSafeNormal();
			const float Pw = ShotPower(K, Lv, E.A != 0, E.B);
			ImpactFx(K, P, Dir, Col, Lv, Pw);
			HitFx(P, Pw, E.B, E.Dmg);
			if (E.B > 0) CritFx(P, E.B);
			break;
		}
		case FPDEvent::EKind::Splash:
		{
			const FLinearColor Col = SuitColor(E.Suit);
			Rings.Add({P, 6.f, E.R * CS, 0.f, 0.35f, 2.f, Col, EWave::Broken, 10, FMath::FRandRange(0.f, TwoPi), 2.f}); // 범위: 도는 점선 고리
			Burst(P, 10, Col, 60.f, 200.f, 0.35f, 2.2f, true);
			PushWorldFx(EWorldFx::Splash, P, Col, FMath::Max(0.5f, E.R));
			break;
		}
		case FPDEvent::EKind::Kill:
		{
			DeathFx(P, E.Enemy);
			if (EnemyDef(E.Enemy).bBoss) Floats.Add({FString::Printf(TEXT("+%d 골드"), E.A), P - FVector2f(0.f, 40.f), 0.f, 1.4f, 22.f, Hex(TEXT("ffd96a"))});
			break;
		}
		case FPDEvent::EKind::Leak:
			Shake = FMath::Max(Shake, 0.35f);
			FlashT = 0.4f;
			FlashCol = Hex(TEXT("ff3a4a"));
			PushWorldFx(EWorldFx::Leak, P, Hex(TEXT("ff3a4a")), 1.f);
			Floats.Add({FString::Printf(TEXT("목숨 -%d"), E.A), FVector2f(FX + Cols * CS - 60.f, FY + 40.f), 0.f, 1.2f, 20.f, Hex(TEXT("ff6a6a"))});
			break;
		case FPDEvent::EKind::Place:
		{
			const FLinearColor Col = SuitColor(E.Suit);
			Rings.Add({P, 8.f, 46.f, 0.f, 0.4f, 2.f, Col});
			Burst(P, 14, Col, 40.f, 160.f, 0.45f, 2.2f);
			PushWorldFx(EWorldFx::Place, P, Col, 1.f);
			break;
		}
		case FPDEvent::EKind::Synergy:
		{
			const FLinearColor Col = HandColor(E.Hand);
			const int Val = HandValue(E.Hand, E.A);
			// 이 족보의 칸들을 찾아 칸마다 빛과 상승 입자
			for (const FPDSynergy& S : Game.Synergies)
			{
				if (S.Kind != E.Hand) continue;
				float CX = 0.f, CY = 0.f;
				for (int K : S.Cells) { CX += K % Cols + 0.5f; CY += K / Cols + 0.5f; }
				if (FMath::Abs(CX / S.Cells.size() - E.X) > 0.01f || FMath::Abs(CY / S.Cells.size() - E.Y) > 0.01f) continue;
				for (int K : S.Cells)
				{
					CellFlashes.Add({K, 0.6f, Col});
					for (int I = 0; I < 10; ++I)
					{
						FParticle Pa;
						Pa.P = CellCenter(K) + FVector2f(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-20.f, 20.f));
						Pa.V = FVector2f(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-160.f, -60.f));
						Pa.MaxLife = Pa.Life = FMath::FRandRange(0.5f, 1.0f);
						Pa.Size = FMath::FRandRange(1.5f, 3.f);
						Pa.Drag = 1.2f;
						Pa.Gravity = 0.f;
						Pa.Col = Col;
						Pa.bStreak = true;
						Particles.Add(Pa);
					}
				}
				break;
			}
			Rings.Add({P, 10.f, 70.f + Val * 0.6f, 0.f, 0.55f, 3.f, Col});
			Burst(P, 18 + Val / 3, Col, 80.f, 260.f, 0.7f, 2.6f, true);
			PushWorldFx(EWorldFx::Synergy, P, Col, 1.f + Val / 40.f);
			FString Label = HandName(E.Hand);
			if (E.Hand == EPDHand::Flush || E.Hand == EPDHand::Straight) Label += FString::FromInt(E.A);
			Floats.Add({Label + TEXT("!"), P - FVector2f(0.f, 24.f), 0.f, 1.4f, Val >= 80 ? 30.f : 22.f, Col});
			if (E.B >= 2)
			{
				Floats.Add({FString::Printf(TEXT("연쇄 ×%s"), *FString::SanitizeFloat(1.f + ChainStep * (E.B - 1))), P + FVector2f(0.f, 6.f), 0.f, 1.4f, 18.f, FLinearColor::White});
				Shake = FMath::Max(Shake, 0.12f + 0.05f * E.B);
			}
			if (Val >= 80) { FlashT = 0.3f; FlashCol = Col; Shake = FMath::Max(Shake, 0.3f); }
			break;
		}
		case FPDEvent::EKind::WaveStart:
			PowerRef = Game.Power(); // 위쪽 띠의 판 위력 ±% 기준 (D-119)
			BannerText = E.B ? FString::Printf(TEXT("BOSS WAVE %d"), E.A) : FString::Printf(TEXT("WAVE %d"), E.A);
			BannerSub = E.B ? FString::Printf(TEXT("보스: %s · %s"), BossName(E.Enemy), BossSkillText(E.Enemy)) : (E.A > Game.BestWave && Game.BestWave > 0 ? FString(TEXT("최고 기록 갱신 중!")) : FString::Printf(TEXT("최고 %d웨이브"), Game.BestWave));
			BannerCol = E.B ? Hex(TEXT("ff6a6a")) : Hex(TEXT("7fd6ff"));
			BannerT = BannerMax = 1.6f;
			if (E.B) { FlashT = 0.25f; FlashCol = BannerCol; }
			break;
		case FPDEvent::EKind::WaveClear:
			BannerText = TEXT("WAVE CLEAR");
			BannerSub = FString::Printf(TEXT("카드 +%d%s"), E.A, E.A < DrawPerWave ? TEXT(" (손패 가득)") : TEXT(""));
			BannerCol = Hex(TEXT("9cffb0"));
			BannerT = BannerMax = 1.4f;
			break;
		case FPDEvent::EKind::GameOver:
			Shake = 0.8f;
			FlashT = 0.6f;
			FlashCol = Hex(TEXT("ff3a4a"));
			break;
		case FPDEvent::EKind::Discard: break;
		case FPDEvent::EKind::JokerFuse:
		{
			SuckIn(P, 10, 18.f, 0.45f, 1.8f, 3.2f, JokerPink, FLinearColor::White, 3);
			Rings.Add({P, 110.f, 6.f, 0.f, 0.45f, 3.f, JokerPink});
			Rings.Add({P, 10.f, 160.f, 0.3f, 1.1f, 3.4f, JokerPink});
			Rings.Add({P, 10.f, 240.f, 0.4f, 1.4f, 2.f, FLinearColor::White});
			Burst(P, 90, JokerPink, 100.f, 380.f, 1.1f, 3.2f, true);
			PushWorldFx(EWorldFx::JokerFuse, P, JokerPink, 1.f);
			Burst(P, 40, FLinearColor::White, 60.f, 240.f, 0.9f, 2.4f, false, 60.f);
			Floats.Add({FString::Printf(TEXT("조커: %s!"), JokerName(E.A)), P - FVector2f(0.f, 36.f), 0.f, 2.f, 30.f, JokerPink});
			if (E.B) Floats.Add({TEXT("새로 발견! (J: 도감)"), P + FVector2f(0.f, 2.f), 0.f, 2.f, 16.f, FLinearColor::White});
			FlashT = 0.45f;
			FlashCol = JokerPink;
			Shake = FMath::Max(Shake, 0.55f);
			BannerText = JokerName(E.A);
			BannerSub = JokerAbility(E.A);
			BannerCol = JokerPink;
			BannerT = BannerMax = 1.8f;
			break;
		}
		case FPDEvent::EKind::Feed: // 족보 먹이기: 먹인 칸에서 조커로 빨려 드는 입자, 레벨이 오르면 고리와 글자
		{
			SuckIn(P, 8, 14.f, 0.4f, 1.6f, 2.8f, JokerPink, FLinearColor::White, 3);
			Rings.Add({P, 60.f, 6.f, 0.f, 0.4f, 2.4f, JokerPink});
			if (E.B > 0)
			{
				Rings.Add({P, 10.f, 110.f, 0.3f, 0.9f, 3.f, JokerPink});
				Burst(P, 40, JokerPink, 80.f, 260.f, 0.8f, 2.6f, true);
				PushWorldFx(EWorldFx::Synergy, P, JokerPink, 2.f);
				Floats.Add({FString::Printf(TEXT("레벨 %d!"), E.A), P - FVector2f(0.f, 30.f), 0.f, 1.6f, 22.f, JokerPink});
				Shake = FMath::Max(Shake, 0.25f);
			}
			else Floats.Add({FString::Printf(TEXT("경험치 +%.0f"), E.R), P - FVector2f(0.f, 26.f), 0.f, 1.2f, 14.f, JokerPink});
			break;
		}
		case FPDEvent::EKind::JokerXp: // 조커 처치 경험치 (D-125): 작은 "+1 경험치" + 별빛, 레벨이 오르면 고리·입자·큰 글자
		{
			if (E.B > 0) // 레벨업 (D-127): 칸 번쩍 + 고리 여러 겹(별·빛살·조각) + 빛살 줄기 + 별빛 + 분수처럼 솟는 금빛 + 큰 글자
			{
				const FLinearColor Gold = Hex(TEXT("ffd96a")), White = FLinearColor::White;
				const float R0 = FMath::FRandRange(0.f, TwoPi);
				CellFlashes.Add({CellKey((int)E.X, (int)E.Y), 0.6f, JokerPink});
				FlashT = FMath::Max(FlashT, 0.12f); FlashCol = JokerPink;
				Rings.Add({P, 4.f, 60.f, 0.f, 0.35f, 3.f, White});
				Rings.Add({P, 10.f, 140.f, 0.f, 0.9f, 3.f, JokerPink});
				Rings.Add({P, 8.f, 100.f, 0.05f, 0.8f, 2.4f, Gold, EWave::Star, 6, R0, 2.5f});
				Rings.Add({P, 14.f, 120.f, 0.1f, 0.9f, 2.f, WithAlpha(White, 0.9f), EWave::Rays, 12, R0 + 0.26f, -1.5f});
				Rings.Add({P, 30.f, 170.f, 0.2f, 1.1f, 2.f, WithAlpha(JokerPink, 0.8f), EWave::Broken, 10, R0, 1.f});
				for (int32 K = 0; K < 8; ++K) // 사방으로 뻗는 빛살 줄기
				{
					const FVector2f D(FMath::Cos(R0 + K * TwoPi / 8.f), FMath::Sin(R0 + K * TwoPi / 8.f));
					Beams.Add({P + D * 14.f, P + D * (K % 2 ? 60.f : 90.f), 0.f, 0.35f, K % 2 ? 1.6f : 2.4f, K % 2 ? Gold : JokerPink});
				}
				Flares.Add({P, 0.f, 0.7f, 70.f, R0, White, 4});
				for (int32 K = 0; K < 6; ++K) // 둘레에 차례로 터지는 작은 별빛
				{
					const float A = R0 + K * TwoPi / 6.f;
					Flares.Add({P + FVector2f(FMath::Cos(A), FMath::Sin(A)) * 46.f, -0.06f * (K + 1), 0.45f, 26.f, A, K % 2 ? Gold : JokerPink, 4});
				}
				Burst(P, 50, JokerPink, 90.f, 280.f, 0.9f, 2.6f, true);
				Burst(P, 18, White, 40.f, 160.f, 0.6f, 2.f, false);
				BurstCone(P, FVector2f(0.f, -1.f), 0.6f, 28, Gold, 160.f, 320.f, 1.1f, 2.4f, true, 380.f); // 금빛 분수
				AddShards(P, 12, JokerPink, 80.f, 200.f, 6.f, 12.f, 0.7f);
				PushWorldFx(EWorldFx::JokerFuse, P, JokerPink, 1.4f);
				PushWorldFx(EWorldFx::Synergy, P, Gold, 1.8f);
				Floats.Add({TEXT("LEVEL UP!"), P - FVector2f(0.f, 46.f), 0.f, 1.8f, 26.f, Gold});
				Floats.Add({E.B > 1 ? FString::Printf(TEXT("Lv%d  (+%d)"), E.A, E.B) : FString::Printf(TEXT("Lv%d"), E.A), P - FVector2f(0.f, 22.f), 0.f, 1.8f, 18.f, JokerPink});
				if (E.A >= JokerUpLevel && E.A - E.B < JokerUpLevel) Floats.Add({TEXT("조커 강화!"), P + FVector2f(0.f, 30.f), 0.f, 2.f, 18.f, White}); // 레벨 10: 트리 강화와 같은 효과
				Shake = FMath::Max(Shake, 0.25f);
			}
			else
			{
				Burst(P + FVector2f(FMath::FRandRange(-8.f, 8.f), -CS * 0.3f), 3, JokerPink, 20.f, 60.f, 0.35f, 1.6f, false);
				Floats.Add({FString::Printf(TEXT("+%.0f 경험치"), E.R), P + FVector2f(FMath::FRandRange(-10.f, 10.f), -CS * 0.45f), 0.f, 0.7f, E.R >= 10.f ? 16.f : 11.f, WithAlpha(JokerPink, 0.9f)});
			}
			break;
		}
		case FPDEvent::EKind::JokerMerge: // 같은 조커끼리 합치기 (D-114)
		{
			SuckIn(P, 12, 14.f, 0.4f, 1.8f, 3.f, JokerPink, FLinearColor::White, 3);
			Rings.Add({P, 60.f, 6.f, 0.f, 0.4f, 2.4f, JokerPink});
			Rings.Add({P, 8.f, 90.f, 0.25f, 0.7f, 3.f, JokerPink, EWave::Star, 4, FMath::FRandRange(0.f, TwoPi), 2.f});
			Burst(P, 40, JokerPink, 80.f, 260.f, 0.8f, 2.6f, true);
			PushWorldFx(EWorldFx::JokerFuse, P, JokerPink, 1.f);
			Floats.Add({FString::Printf(TEXT("%s %d개 합침!"), JokerName(E.A), E.B), P - FVector2f(0.f, 30.f), 0.f, 1.6f, 22.f, JokerPink});
			Shake = FMath::Max(Shake, 0.25f);
			break;
		}
		case FPDEvent::EKind::Bell:
			Rings.Add({P, 10.f, E.R * CS, 0.f, 0.6f, 3.f, Hex(TEXT("cfe4ff"))});
			Rings.Add({P, 10.f, E.R * CS * 0.7f, 0.1f, 0.6f, 2.f, JokerPink});
			Burst(P, 20, Hex(TEXT("cfe4ff")), 60.f, 200.f, 0.5f, 2.f, true);
			PushWorldFx(EWorldFx::Splash, P, Hex(TEXT("cfe4ff")), FMath::Max(0.5f, E.R));
			break;
		case FPDEvent::EKind::BossSkill: // 보스 능력이 터지는 순간 (D-112)
		{
			const FLinearColor Col = EnemyColor(E.Enemy);
			const float Rnd = FMath::FRandRange(0.f, TwoPi);
			switch (E.Enemy)
			{
			case EPDEnemy::Charger: Rings.Add({P, 8.f, 40.f, 0.f, 0.3f, 2.f, Col, EWave::Poly, 3, 0.f, 0.f}); Burst(P, 10, Col, 80.f, 220.f, 0.3f, 2.f, true); break;
			case EPDEnemy::Summoner: Rings.Add({P, 30.f, 6.f, 0.f, 0.35f, 2.f, Col}); Burst(P, 12, Col, 40.f, 140.f, 0.5f, 2.f); break;
			case EPDEnemy::Phantom: Rings.Add({P, 30.f, 4.f, 0.f, 0.3f, 1.6f, Col, EWave::Broken, 6, Rnd, 4.f}); break;
			case EPDEnemy::Blinker: Rings.Add({P, 4.f, 30.f, 0.f, 0.3f, 2.f, Col, EWave::Poly, 4, 0.f, 3.f}); Flares.Add({P, 0.f, 0.2f, 20.f, Rnd, Col, 4}); break;
			case EPDEnemy::Warden: Rings.Add({P, 10.f, 34.f, 0.f, 0.4f, 2.4f, Col, EWave::Poly, 6, PI / 6.f, 0.f}); break;
			case EPDEnemy::Breaker:
			{
				const FVector2f T = CellPx(E.X2, E.Y2);
				Beams.Add({P, T, 0.f, 0.25f, 2.4f, Col, 2, 1});
				Rings.Add({T, 4.f, 26.f, 0.f, 0.3f, 2.f, Col, EWave::Star, 4, Rnd, 0.f});
				Floats.Add({TEXT("기절!"), T - FVector2f(0.f, 24.f), 0.f, 1.f, 14.f, Col});
				Shake = FMath::Max(Shake, 0.12f);
				break;
			}
			default: Rings.Add({P, 6.f, 30.f, 0.f, 0.3f, 2.f, Col}); break;
			}
			break;
		}
		case FPDEvent::EKind::Star:
			for (const FPDEnemy& En : Game.Enemies)
			{
				const FPDPoint Q = Game.Path.At(En.D);
				Beams.Add({P, CellPx(Q.X, Q.Y), 0.f, 0.18f, 1.6f, FLinearColor::White});
				Burst(CellPx(Q.X, Q.Y), 3, JokerPink, 40.f, 120.f, 0.25f, 2.f, true);
				PushWorldFx(EWorldFx::ImpGlitter, CellPx(Q.X, Q.Y), JokerPink, 1.f);
			}
			Rings.Add({P, 6.f, 40.f, 0.f, 0.3f, 2.f, FLinearColor::White});
			break;
		case FPDEvent::EKind::Fuse:
		{
			const FLinearColor Col = SuitColor(E.Suit), HC = HandColor(E.Hand);
			SuckIn(P, 14, 18.f, 0.35f, 1.6f, 3.f, Col, HC, 2); // 족보 칸들에서 합쳐질 칸으로
			Rings.Add({P, 70.f, 6.f, 0.f, 0.35f, 2.4f, HC});   // 안으로 모이는 고리
			Rings.Add({P, 8.f, 90.f, 0.15f, 0.75f, 3.f, Col}); // 터져 나가는 고리
			Burst(P, 50, Col, 80.f, 300.f, 0.8f, 2.8f, true);
			PushWorldFx(EWorldFx::Fuse, P, Col, (float)E.A);
			Floats.Add({FString::Printf(TEXT("레벨 %d!"), E.A), P - FVector2f(0.f, 30.f), 0.f, 1.5f, 28.f, Hex(TEXT("ffd96a"))});
			FlashT = FMath::Max(FlashT, 0.2f);
			FlashCol = Col;
			Shake = FMath::Max(Shake, 0.25f + 0.08f * E.A);
			break;
		}
		case FPDEvent::EKind::TreeBuy:
			TreeBuyId = E.A;
			TreeBuyT = 0.8f;
			break;
		case FPDEvent::EKind::Reward:
		{
			const FString Name = E.A ? CharmName(E.B) : InstantName(E.B);
			Floats.Add({Name + TEXT("!"), FVector2f(FX + Cols * CS / 2.f, FY + 90.f), 0.f, 1.4f, 26.f, Hex(TEXT("ffd96a"))});
			Burst(FVector2f(FX + Cols * CS / 2.f, FY + 90.f), 40, Hex(TEXT("ffd96a")), 60.f, 260.f, 0.8f, 2.6f, true);
			break;
		}
		}
	}
	Game.Events.clear();
}

void SPDBoardWidget::BurstCone(FVector2f P, FVector2f Dir, float Spread, int N, FLinearColor Col, float SpeedMin, float SpeedMax, float Life, float Size, bool bStreak, float Gravity)
{
	const float Base = FMath::Atan2(Dir.Y, Dir.X);
	for (int I = 0; I < N && Particles.Num() < 2600; ++I)
	{
		const float A = Base + FMath::FRandRange(-Spread, Spread), Sp = FMath::FRandRange(SpeedMin, SpeedMax);
		const float L = Life * FMath::FRandRange(0.7f, 1.2f);
		Particles.Add({P, FVector2f(FMath::Cos(A), FMath::Sin(A)) * Sp, L, L, Size, 3.f, Gravity, Col, bStreak});
	}
}

void SPDBoardWidget::AddShards(FVector2f P, int N, FLinearColor Col, float SpeedMin, float SpeedMax, float LenMin, float LenMax, float Life)
{
	for (int I = 0; I < N && Shards.Num() < 700; ++I)
	{
		const float A = FMath::FRandRange(0.f, TwoPi), L = Life * FMath::FRandRange(0.7f, 1.2f);
		Shards.Add({P, FVector2f(FMath::Cos(A), FMath::Sin(A)) * FMath::FRandRange(SpeedMin, SpeedMax), A, FMath::FRandRange(-16.f, 16.f), FMath::FRandRange(LenMin, LenMax), L, L, Col});
	}
}

// 발사체 → 월드 타격 이펙트 무리 (D-094): 방향 불꽃, 폭발, 서리, 전기, 반짝이, 별빛 터짐
static SPDBoardWidget::EWorldFx ImpactWorldFx(EPDShot K)
{
	using W = SPDBoardWidget::EWorldFx;
	switch (K)
	{
	case EPDShot::Laser: case EPDShot::Bullet: case EPDShot::Arrow: case EPDShot::Blade: return W::ImpSpark;
	case EPDShot::Missile: case EPDShot::Cluster: case EPDShot::Shell: return W::ImpBlast;
	case EPDShot::Orb: return W::ImpFrost;
	case EPDShot::Arc: case EPDShot::JokerLaser: case EPDShot::Grin: case EPDShot::Crescent: return W::ImpZap;
	case EPDShot::Comet: case EPDShot::StarComet: case EPDShot::Nova: case EPDShot::Wave: return W::ImpStarburst;
	default: return W::ImpGlitter; // 별, 로열, 광대, 모자 장수, 희비극, 할리퀸, 지휘봉
	}
}

// 발사체마다 다른 타격 효과 (Power: 1 약 ~ 6 강, 레벨·족보·조커·치명타로 커진다)
void SPDBoardWidget::ImpactFx(EPDShot K, FVector2f P, FVector2f Dir, FLinearColor Col, int32 Lv, float Power)
{
	const float Pw = FMath::Clamp(Power, 1.f, 6.f), Ang = FMath::Atan2(Dir.Y, Dir.X);
	const bool bBusy = Particles.Num() > 1800; // 적이 많을 때는 가볍게
	const float Q = bBusy ? 0.4f : 1.f;        // 파티클 수 배율
	auto Nq = [&](float N) { return FMath::Max(1, FMath::RoundToInt(N * Q)); };
	const FLinearColor White = FLinearColor::White, Gold = Hex(TEXT("ffd96a")), Smoke = Hex(TEXT("8a8090"), 0.45f), Cyan = Hex(TEXT("7fd6ff"));
	const FVector2f Nrm(-Dir.Y, Dir.X);
	const float Rnd = FMath::FRandRange(0.f, TwoPi);
	// 파동 하나 더하기: 모양, 갈래/변 수, 시작 크기 → 끝 크기, 늦음, 길이, 굵기, 색, 시작 각, 회전
	auto Wave = [&](EWave S, int32 N, float R0, float R1, float Delay, float Life, float Th, const FLinearColor& Cl, float Rot, float Spin) {
		Rings.Add({P, R0, R1, -Delay, Life, Th, Cl, S, N, Rot, Spin});
	};
	switch (K)
	{
	case EPDShot::Laser: // ♠ 관통: 꿰뚫는 빛줄 + 퍼지는 스페이드 + 앞쪽 빛줄기
		Flares.Add({P, 0.f, 0.14f, 16.f + 5.f * Lv, Ang, Col, 2});
		Flares.Add({P, 0.f, 0.1f, 7.f + 2.f * Lv, Ang + PI / 2.f, White, 2});
		Wave(EWave::Suit, (int32)EPDSuit::Spade, 4.f, 14.f + 3.f * Lv, 0.f, 0.22f, 1.5f, Col, 0.f, 0.f);
		BurstCone(P, Dir, 0.35f, Nq(4 + 2 * Lv), Col, 200.f, 380.f, 0.2f, 1.4f, true);
		if (Lv >= 3) Wave(EWave::Rays, 6, 10.f, 30.f, 0.03f, 0.2f, 1.2f, Col, Ang, 0.f);
		if (Lv >= 4) Wave(EWave::Poly, 4, 6.f, 36.f, 0.05f, 0.3f, 2.f, Col, Ang + PI / 4.f, 3.f);
		break;
	case EPDShot::JokerLaser:
		Flares.Add({P, 0.f, 0.16f, 18.f, Rnd, Col});
		Wave(EWave::Star, 4, 4.f, 22.f, 0.f, 0.25f, 1.6f, Col, Rnd, 4.f);
		break;
	case EPDShot::Missile: // ♥ 불덩이: 퍼지는 하트 + 끊어진 충격파 + 불똥·연기
		Flares.Add({P, 0.f, 0.3f + 0.04f * Lv, 20.f + 7.f * Lv, 0.f, Hex(TEXT("ff8a5a")), 0});
		Flares.Add({P, 0.f, 0.12f, 10.f + 3.f * Lv, Rnd, Gold, 6});
		Wave(EWave::Suit, (int32)EPDSuit::Heart, 5.f, 16.f + 5.f * Lv, 0.f, 0.32f, 2.f, Col, 0.f, 0.f);
		Wave(EWave::Broken, 5, 8.f, 26.f + 6.f * Lv, 0.04f, 0.35f, 1.6f, Gold, Rnd, 2.5f);
		Burst(P, Nq(10 + 4 * Lv), Col, 60.f, 180.f + 25.f * Lv, 0.5f, 2.6f, false, 90.f);
		Burst(P, Nq(6 + 2 * Lv), Gold, 80.f, 220.f, 0.3f, 1.6f, true);
		Burst(P, Nq(5), Smoke, 10.f, 40.f, 0.9f, 4.f, false, -50.f);
		if (Lv >= 4) { Wave(EWave::Poly, 8, 10.f, 56.f, 0.06f, 0.45f, 1.4f, Gold, Rnd, 1.f); Shake = FMath::Max(Shake, 0.1f); }
		break;
	case EPDShot::Bullet: // ♦ 도탄: 작은 마름모가 튀고, 반대쪽 불티
		Flares.Add({P, 0.f, 0.08f, 6.f + 2.f * Lv, Rnd, Col});
		Wave(EWave::Suit, (int32)EPDSuit::Diamond, 3.f, 9.f + 2.f * Lv, 0.f, 0.14f, 1.3f, Col, 0.f, 0.f);
		BurstCone(P, -Dir, 0.7f, Nq(3 + Lv), Col, 160.f, 320.f, 0.14f, 1.2f, true, 200.f);
		if (Lv >= 3) BurstCone(P, Dir, 0.25f, Nq(3), White, 250.f, 400.f, 0.1f, 1.f, true);
		if (FMath::FRand() < 0.4f) AddShards(P, 1, Hex(TEXT("b0a070")), 40.f, 90.f, 2.f, 3.f, 0.5f);
		if (Lv >= 4) Wave(EWave::Poly, 4, 2.f, 16.f, 0.02f, 0.16f, 1.2f, White, Rnd, 8.f);
		break;
	case EPDShot::Orb: // ♣ 얼음: 육각 서리가 천천히 돌며 퍼지고, 눈꽃·얼음 조각·눈
		Flares.Add({P, 0.f, 0.4f, 14.f + 4.f * Lv, Rnd, Hex(TEXT("cfffe8")), 6});
		Wave(EWave::Poly, 6, 6.f, 18.f + 4.f * Lv, 0.f, 0.6f, 1.6f, Col, Rnd, 0.8f);
		Wave(EWave::Suit, (int32)EPDSuit::Club, 3.f, 10.f + 2.f * Lv, 0.05f, 0.35f, 1.2f, Hex(TEXT("eafff6")), 0.f, 0.f);
		AddShards(P, Nq(4 + Lv), Col, 60.f, 150.f, 3.f, 6.f, 0.5f);
		Burst(P, Nq(6), Hex(TEXT("eafff6")), 10.f, 40.f, 0.9f, 1.6f, false, 30.f);
		break;
	case EPDShot::Star: // ★ 반짝임: 도는 별 테두리 + 내려앉는 반짝이
		Flares.Add({P, 0.f, 0.3f, 12.f + 4.f * Lv, Rnd, White, 5});
		Wave(EWave::Star, 5, 4.f, 16.f + 4.f * Lv, 0.f, 0.3f, 1.4f, White, Rnd, 5.f);
		Burst(P, Nq(5 + 2 * Lv), White, 20.f, 80.f, 0.8f, 1.5f, false, 25.f);
		if (Lv >= 3) for (int32 J = 0; J < Lv - 1; ++J) Flares.Add({P + Rot2(FVector2f(14.f, 0.f), J * TwoPi / (Lv - 1)), -0.05f * J, 0.2f, 6.f, 0.f, White, 4});
		break;
	case EPDShot::Arrow: // 스트레이트: 박히는 화살 셋 + 앞으로 퍼지는 삼각형
		for (int32 J = -1; J <= 1; ++J) Flares.Add({P + Nrm * J * 6.f, -0.03f * (J + 1), 0.14f, 12.f, Ang, Col, 2});
		Wave(EWave::Poly, 3, 4.f, 20.f, 0.f, 0.22f, 1.5f, Col, Ang, 0.f);
		BurstCone(P, Dir, 0.3f, Nq(8), Col, 180.f, 320.f, 0.18f, 1.3f, true);
		break;
	case EPDShot::Arc: // 플러시: 갈라지는 잔번개 + 지직거리는 끊어진 호
		for (int32 J = 0; J < 4; ++J) Beams.Add({P, P + Rot2(FVector2f(FMath::FRandRange(18.f, 34.f), 0.f), FMath::FRandRange(0.f, TwoPi)), 0.f, 0.16f, 1.4f, Col, 2, 1});
		Flares.Add({P, 0.f, 0.12f, 14.f, Rnd, White, 8});
		Wave(EWave::Broken, 7, 8.f, 24.f, 0.f, 0.2f, 1.3f, Col, Rnd, -9.f);
		Burst(P, Nq(10), Col, 120.f, 300.f, 0.12f, 1.2f, true);
		break;
	case EPDShot::Cluster: // 풀하우스: 차례로 터지는 작은 폭발 다섯 (각자 도는 네모)
		for (int32 J = 0; J < 5; ++J)
		{
			const FVector2f Qp = P + Rot2(FVector2f(16.f, 0.f), J * TwoPi / 5.f + FMath::FRandRange(-0.3f, 0.3f));
			Flares.Add({Qp, -0.06f * J, 0.22f, 14.f, 0.f, Col, 0});
			Flares.Add({Qp, -0.06f * J, 0.12f, 8.f, Rnd, Gold, 4});
			Rings.Add({Qp, 2.f, 12.f, -0.06f * J, 0.24f, 1.6f, Col, EWave::Poly, 4, Rnd + J, 6.f});
		}
		Burst(P, Nq(16), Col, 80.f, 220.f, 0.5f, 2.f, false, 80.f);
		break;
	case EPDShot::Shell: // 포카드: 거대한 폭발 (흰 섬광, 팔각 충격파, 사방 빛줄기, 끊어진 호, 파편, 먼지)
		Flares.Add({P, 0.f, 0.18f, 46.f, Rnd, White, 4});
		Flares.Add({P, 0.f, 0.45f, 60.f, 0.f, Col, 0});
		Wave(EWave::Poly, 8, 6.f, 46.f, 0.f, 0.32f, 3.2f, Col, Rnd, 0.5f);
		Wave(EWave::Rays, 12, 14.f, 70.f, 0.03f, 0.3f, 1.6f, White, Rnd, 0.f);
		Wave(EWave::Broken, 6, 12.f, 96.f, 0.1f, 0.6f, 1.2f, WithAlpha(Col, 0.7f), Rnd, 1.5f);
		AddShards(P, Nq(10), Col, 120.f, 300.f, 5.f, 10.f, 0.7f);
		Burst(P, Nq(18), Smoke, 20.f, 90.f, 1.1f, 4.5f, false, -30.f);
		Burst(P, Nq(14), White, 150.f, 380.f, 0.2f, 1.5f, true);
		Shake = FMath::Max(Shake, 0.2f);
		break;
	case EPDShot::Comet: // 스트레이트 플러시: 여덟 뿔 별이 돌며 퍼지고 긴 빛살
		Flares.Add({P, 0.f, 0.3f, 30.f, Rnd, Col, 8});
		Wave(EWave::Star, 8, 6.f, 34.f, 0.f, 0.32f, 1.8f, Col, Rnd, 3.f);
		Burst(P, Nq(20), Col, 160.f, 420.f, 0.32f, 1.6f, true);
		break;
	case EPDShot::Nova: // 파이브 카드: 안으로 빨려 들었다가 오각형이 차례로 터져 나감
		Wave(EWave::Circle, 0, 34.f, 4.f, 0.f, 0.14f, 1.6f, Col, 0.f, 0.f);
		for (int32 J = 0; J < 4; ++J) Wave(EWave::Poly, 5, 6.f + J * 4.f, 26.f + J * 14.f, 0.14f + 0.05f * J, 0.35f, 2.4f - J * 0.4f, Col, Rnd + J * 0.3f, 1.5f);
		Flares.Add({P, -0.14f, 0.4f, 34.f, 0.f, Col, 0});
		Burst(P, Nq(12), Gold, 40.f, 140.f, 0.7f, 1.6f, false, -20.f);
		break;
	case EPDShot::Crown: // 로열: 금빛 빛줄기 + 여덟 뿔 별 + 솟았다 떨어지는 금가루
		Flares.Add({P, 0.f, 0.4f, 40.f, Rnd, Col, 8});
		Flares.Add({P, 0.f, 0.2f, 18.f, 0.f, White, 4});
		Wave(EWave::Rays, 16, 10.f, 50.f, 0.f, 0.4f, 1.6f, Col, Rnd, 0.6f);
		Wave(EWave::Star, 8, 6.f, 30.f, 0.06f, 0.4f, 2.f, White, Rnd, -2.f);
		BurstCone(P, FVector2f(0.f, -1.f), 0.9f, Nq(20), Gold, 120.f, 300.f, 0.9f, 1.8f, false, 420.f);
		Burst(P, Nq(10), White, 30.f, 120.f, 0.7f, 1.4f, false, -40.f);
		break;
	case EPDShot::Grin: // 체셔: 보랏빛 덩어리 + 안으로 감겨 드는 끊어진 호
		Flares.Add({P, 0.f, 0.4f, 26.f, 0.f, Col, 0});
		Wave(EWave::Broken, 3, 28.f, 6.f, 0.f, 0.35f, 1.6f, Col, Rnd, 6.f);
		Burst(P, Nq(10), Col, 30.f, 120.f, 0.6f, 1.6f, false, -30.f);
		break;
	case EPDShot::Juggle: // 광대: 네 무늬가 사방으로 퍼짐 + 색종이
		for (int32 J = 0; J < 4; ++J) Rings.Add({P + Rot2(FVector2f(6.f, 0.f), J * PI / 2.f + Rnd), 2.f, 9.f, -0.03f * J, 0.3f, 1.3f, SuitColor((EPDSuit)J), EWave::Suit, J, 0.f, 0.f});
		for (int32 J = 0; J < 4; ++J) BurstCone(P, FVector2f(0.f, -1.f), 1.2f, Nq(4), SuitColor((EPDSuit)J), 100.f, 240.f, 0.8f, 2.f, false, 500.f);
		break;
	case EPDShot::Hat: // 모자 장수: 도는 네모 + 펑 연기 + 날리는 카드 조각
		Wave(EWave::Poly, 4, 4.f, 20.f, 0.f, 0.3f, 1.5f, Col, Rnd, 7.f);
		Burst(P, Nq(8), Smoke, 20.f, 70.f, 0.7f, 4.f, false, -40.f);
		AddShards(P, Nq(5), White, 60.f, 140.f, 5.f, 7.f, 0.8f);
		Flares.Add({P, 0.f, 0.2f, 16.f, Rnd, Col, 4});
		break;
	case EPDShot::Crescent: // 웃는 가면: 남아서 웃는 초승달 + 반쪽 호 둘
		Beams.Add({P, P, 0.f, 0.35f, 1.6f, Col, 4, 1});
		Wave(EWave::Broken, 2, 6.f, 22.f, 0.f, 0.28f, 1.5f, Col, Rnd, 10.f);
		Burst(P, Nq(8), Col, 60.f, 180.f, 0.3f, 1.6f, true);
		break;
	case EPDShot::Twin: // 희비극: 분홍·파랑 삼각형이 엇갈려 육각별
		Wave(EWave::Poly, 3, 4.f, 20.f, 0.f, 0.3f, 1.6f, Col, -PI / 2.f, 2.f);
		Wave(EWave::Poly, 3, 4.f, 20.f, 0.f, 0.3f, 1.6f, Cyan, PI / 2.f, -2.f);
		Flares.Add({P + Nrm * 7.f, 0.f, 0.22f, 14.f, 0.f, Col, 4});
		Flares.Add({P - Nrm * 7.f, 0.f, 0.22f, 14.f, PI / 4.f, Cyan, 4});
		break;
	case EPDShot::Shards: // 할리퀸: 마름모 셋이 차례로 퍼지고 조각이 깨져 흩어짐
		for (int32 J = 0; J < 3; ++J) Wave(EWave::Suit, (int32)EPDSuit::Diamond, 3.f, 12.f + J * 6.f, 0.05f * J, 0.25f, 1.4f, J == 1 ? White : Col, 0.f, 0.f);
		AddShards(P, Nq(10), Col, 100.f, 240.f, 4.f, 8.f, 0.6f);
		AddShards(P, Nq(4), White, 100.f, 240.f, 4.f, 7.f, 0.5f);
		break;
	case EPDShot::Note: // 지휘봉: 박자 맞춰 퍼지는 오각형 셋 + 떠오르는 반짝이
		for (int32 J = 0; J < 3; ++J) Wave(EWave::Poly, 5, 3.f, 16.f, 0.1f * J, 0.2f, 1.6f, Col, Rnd + J, 3.f);
		Burst(P, Nq(10), Col, 20.f, 70.f, 0.8f, 1.8f, false, -140.f);
		break;
	case EPDShot::Blade: // 잭: X 자 베기 + 날카로운 빛줄기 + 흔들림
		Beams.Add({P + FVector2f(-16.f, -12.f), P + FVector2f(16.f, 12.f), 0.f, 0.2f, 2.6f, Col});
		Beams.Add({P + FVector2f(16.f, -12.f), P + FVector2f(-16.f, 12.f), 0.f, 0.2f, 2.6f, Col});
		Flares.Add({P, 0.f, 0.14f, 20.f, PI / 4.f, White, 4});
		Wave(EWave::Rays, 8, 8.f, 30.f, 0.02f, 0.2f, 1.4f, White, Rnd, 0.f);
		Burst(P, Nq(14), White, 120.f, 300.f, 0.22f, 1.6f, true);
		Shake = FMath::Max(Shake, 0.08f);
		break;
	case EPDShot::Wave: // 방울: 차례로 퍼지는 끊어진 소리 호 넷 (파티클 없음)
		for (int32 J = 0; J < 4; ++J) Wave(EWave::Broken, 4, 4.f, 20.f + J * 9.f, 0.06f * J, 0.3f, 1.8f - J * 0.3f, Col, Ang + PI / 4.f, 0.f);
		break;
	case EPDShot::StarComet: // 스타: 큰 다섯 뿔 별이 돌며 퍼지고 빛살
		Flares.Add({P, 0.f, 0.35f, 32.f, Rnd, Col, 5});
		Flares.Add({P, 0.f, 0.18f, 16.f, 0.f, White, 5});
		Wave(EWave::Star, 5, 6.f, 36.f, 0.f, 0.35f, 2.f, Col, Rnd, 4.f);
		Burst(P, Nq(16), Col, 120.f, 320.f, 0.35f, 1.8f, true);
		break;
	default:
		Flares.Add({P, 0.f, 0.14f, 12.f, Rnd, Col});
		break;
	}
	if (Pw >= 5.f) Shake = FMath::Max(Shake, 0.04f + 0.02f * (Pw - 5.f));
	// 월드 층: 무리별 나이아가라 (♦ 도탄은 맞은 반대쪽으로 튄다)
	PushWorldFx(ImpactWorldFx(K), P, Col, Pw, K == EPDShot::Bullet ? -Dir : Dir);
}

// 월드 이펙트 요청을 쌓는다 (D-092). 게임 모드가 매 프레임 꺼내 나이아가라로 재생
void SPDBoardWidget::PushWorldFx(EWorldFx Kind, FVector2f DesignPos, const FLinearColor& Col, float Scale, FVector2f Dir)
{
	if (!Settings.bWorldFx) return; // 설정: 월드 이펙트 끔 (D-110)
	if (WorldFx.Num() >= 64) return; // 한 프레임에 너무 많이 쌓이면 버린다 (나이아가라 폭주 방지)
	// 판 좌표 → 화면 가운데를 0으로 한 판 좌표. 판은 창 가운데에 맞춰 그리므로 (Offset - 창/2) / 배율 만큼 민다
	const FVector2f Center = (ViewOffset - ViewSize * 0.5f) / FMath::Max(0.01f, ViewScale);
	const FVector2f D = Dir.GetSafeNormal();
	WorldFx.Add({Kind, DesignPos + Center, Col, Scale, D.IsNearlyZero() ? FVector2f(0.f, -1.f) : D});
}

// 맞는 순간 공통: 작은 흰 심 + 맞은 적 번쩍임 + 피해 숫자
void SPDBoardWidget::HitFx(FVector2f P, float Power, int32 Crits, float Dmg)
{
	PushWorldFx(EWorldFx::Hit, P, FLinearColor::White, FMath::Clamp(Power, 1.f, 6.f));
	Flares.Add({P, 0.f, 0.07f, 5.f + 1.5f * FMath::Clamp(Power, 1.f, 6.f), 0.f, FLinearColor::White, 0});
	// 맞은 적: 가장 가까운 적을 잠깐 하얗게
	int32 BestId = -1;
	float BestD = CS * 0.8f;
	for (const FPDEnemy& E : Game.Enemies)
	{
		const FPDPoint Q = Game.Path.At(E.D);
		const float D = FVector2f::Distance(CellPx(Q.X, Q.Y), P);
		if (D < BestD) { BestD = D; BestId = E.Id; }
	}
	if (BestId >= 0) HitFlash.Add(BestId, 0.14f);
	// 피해 숫자: 치명타는 금색으로 크게 튀어 오른다
	if (Dmg > 0.f && Floats.Num() < 70)
	{
		const bool bCrit = Crits > 0;
		const FVector2f At = P + FVector2f(FMath::FRandRange(-10.f, 10.f), -12.f);
		Floats.Add({bCrit ? ShortNum(Dmg) + TEXT("!") : ShortNum(Dmg), At, 0.f, bCrit ? 0.9f : 0.6f, bCrit ? 15.f + 2.f * FMath::Min(Crits, 4) : 11.f,
			bCrit ? Hex(TEXT("ffd96a")) : WithAlpha(FLinearColor::White, 0.85f)});
	}
}

// 죽는 순간: 하얀 폭발 심 + 큰 별빛 + 겹겹 충격파 + 몸 조각(도는 선) + 불씨 + 흔들림. 보스는 화면이 번쩍
void SPDBoardWidget::DeathFx(FVector2f P, EPDEnemy Type)
{
	const FLinearColor Col = EnemyColor(Type), White = FLinearColor::White;
	const bool bBoss = EnemyDef(Type).bBoss, bTank = Type == EPDEnemy::Tank || Type == EPDEnemy::Armored, bMini = Type == EPDEnemy::Mini;
	const float S = bBoss ? 4.f : bTank ? 1.8f : bMini ? 0.7f : 1.f; // 크기
	PushWorldFx(EWorldFx::Death, P, Col, S);
	const bool bBusy = Particles.Num() > 1800 || Shards.Num() > 500;
	// 빛: 흰 심 + 적 색 큰 별빛
	Flares.Add({P, 0.f, 0.12f + 0.05f * S, 22.f * S, FMath::FRandRange(0.f, PI), White});
	Flares.Add({P, 0.f, 0.28f + 0.08f * S, 40.f * S, FMath::FRandRange(0.f, PI), Col});
	// 충격파 셋: 적의 몸 모양 그대로 퍼진다 (빠른 적 삼각형, 탱크 육각형, 재생자 마름모, 보스 다각형, 나머지 동그라미)
	EWave BodyShape = EWave::Circle;
	int32 BodyN = 0;
	switch (Type)
	{
	case EPDEnemy::Fast: BodyShape = EWave::Poly; BodyN = 3; break;
	case EPDEnemy::Tank: BodyShape = EWave::Poly; BodyN = 6; break;
	case EPDEnemy::Regen: case EPDEnemy::RegenBoss: BodyShape = EWave::Poly; BodyN = Type == EPDEnemy::RegenBoss ? 6 : 4; break;
	case EPDEnemy::ShieldBoss: BodyShape = EWave::Poly; BodyN = 4; break;
	case EPDEnemy::SplitBoss: BodyShape = EWave::Poly; BodyN = 8; break;
	case EPDEnemy::Charger: BodyShape = EWave::Poly; BodyN = 3; break;
	case EPDEnemy::Warlord: BodyShape = EWave::Star; BodyN = 5; break;
	case EPDEnemy::Berserker: BodyShape = EWave::Star; BodyN = 12; break;
	case EPDEnemy::Blinker: case EPDEnemy::Breaker: BodyShape = EWave::Poly; BodyN = 4; break;
	case EPDEnemy::Warden: BodyShape = EWave::Poly; BodyN = 6; break;
	case EPDEnemy::Devourer: BodyShape = EWave::Broken; BodyN = 6; break;
	default: break;
	}
	const float Rot0 = FMath::FRandRange(0.f, TwoPi);
	Rings.Add({P, 2.f, 18.f * S, 0.f, 0.14f, 3.f, White, EWave::Rays, bBoss ? 16 : 8, Rot0, 0.f});
	Rings.Add({P, 6.f, 34.f * S, 0.f, 0.32f + 0.05f * S, 2.4f, Col, BodyShape, BodyN, Rot0, 2.f});
	Rings.Add({P, 10.f, 56.f * S, -0.06f, 0.5f + 0.08f * S, 1.2f, WithAlpha(Col, 0.6f), EWave::Broken, bBoss ? 8 : 5, Rot0, -1.5f});
	// 몸 조각: 돌며 흩어지는 선
	const int32 NS = bBusy ? 4 : FMath::RoundToInt(8.f * FMath::Min(S, 3.f));
	for (int32 I = 0; I < NS; ++I)
	{
		const float A = I * TwoPi / NS + FMath::FRandRange(-0.3f, 0.3f), Sp = FMath::FRandRange(90.f, 240.f) * (bBoss ? 1.6f : 1.f);
		Shards.Add({P + FVector2f(FMath::Cos(A), FMath::Sin(A)) * 6.f * S, FVector2f(FMath::Cos(A), FMath::Sin(A)) * Sp, A, FMath::FRandRange(-14.f, 14.f), FMath::FRandRange(5.f, 10.f) * FMath::Min(S, 2.5f),
			0.f, FMath::FRandRange(0.45f, 0.8f) * (bBoss ? 1.6f : 1.f), Col});
		Shards.Last().Life = Shards.Last().MaxLife;
	}
	// 빛살 + 불씨 (떨어짐) + 흰 불꽃
	Burst(P, bBusy ? 6 : FMath::RoundToInt(16 * S), Col, 120.f, 300.f * FMath::Min(S, 2.f), 0.35f, 1.8f, true);
	if (!bBusy) Burst(P, FMath::RoundToInt(10 * S), Hex(TEXT("ffd96a")), 40.f, 160.f * FMath::Min(S, 2.f), 0.9f, 2.f, false, 160.f);
	Burst(P, bBusy ? 3 : FMath::RoundToInt(8 * S), White, 160.f, 380.f, 0.2f, 1.4f, true);
	Shake = FMath::Max(Shake, bBoss ? 0.7f : bTank ? 0.18f : 0.06f);
	if (bBoss) { FlashT = 0.45f; FlashCol = Col; }
}

void SPDBoardWidget::CritFx(FVector2f P, int32 N)
{
	// 치명타: 흰 불꽃과 고리 (여러 번이면 더 크게)
	Burst(P, 6 + 3 * FMath::Min(N, 4), FLinearColor::White, 80.f, 240.f, 0.3f, 2.2f, true);
	PushWorldFx(EWorldFx::Crit, P, FLinearColor::White, (float)FMath::Min(N, 4));
	Rings.Add({P, 4.f, 18.f + 6.f * FMath::Min(N, 4), 0.f, 0.22f, 2.f, FLinearColor::White, EWave::Star, 4, FMath::FRandRange(0.f, TwoPi), 6.f}); // 도는 네 뿔 별
}

void SPDBoardWidget::UpdateFx(float Dt)
{
	// 발사체 꼬리 (게임이 멈춰 있으면 안 남김): 미사일 연기, 서리, 혜성 불꽃, 포탄 연기, 분열탄 불똥, 음표 반짝이, 레벨 3 이상은 무늬 색 잔상
	if (!bTree && !bUpg && !bMenu)
		for (const FPDProjectile& Pr : Game.Projectiles)
		{
			if (Particles.Num() >= 2600) break;
			const FVector2f At = CellPx(Pr.X, Pr.Y);
			const float VL = FMath::Max(0.001f, FMath::Sqrt(Pr.VX * Pr.VX + Pr.VY * Pr.VY));
			const FVector2f Back(-Pr.VX / VL, -Pr.VY / VL);
			const FVector2f Jit(FMath::FRandRange(-12.f, 12.f), FMath::FRandRange(-12.f, 12.f));
			const FLinearColor Col = ShotColor(Pr.Kind, Pr.Suit);
			switch (Pr.Kind)
			{
			case EPDShot::Missile: Particles.Add({At, Jit, 0.35f, 0.35f, 2.4f + 0.3f * (Pr.Level - 1), 2.f, 0.f, Hex(TEXT("ff9a8a"), 0.6f), false}); break;
			case EPDShot::Orb: Particles.Add({At, Jit, 0.3f, 0.3f, 1.8f, 2.f, 0.f, Hex(TEXT("bfffe0"), 0.5f), false}); break;
			case EPDShot::Comet: case EPDShot::StarComet: Particles.Add({At, Back * 70.f + Jit, 0.3f, 0.3f, 1.6f, 3.f, 0.f, WithAlpha(Col, 0.8f), true}); break;
			case EPDShot::Shell: Particles.Add({At, Jit * 0.5f, 0.5f, 0.5f, 3.2f, 1.5f, 0.f, Hex(TEXT("ff8a7a"), 0.45f), false}); break;
			case EPDShot::Cluster: Particles.Add({At, Back * 40.f + Jit * 2.f, 0.25f, 0.25f, 1.4f, 3.f, 0.f, WithAlpha(Col, 0.8f), true}); break;
			case EPDShot::Note: case EPDShot::Hat: case EPDShot::Twin: Particles.Add({At, Jit, 0.35f, 0.35f, 1.4f, 2.f, -30.f, WithAlpha(Col, 0.6f), false}); break;
			default:
				if (Pr.Level >= 3) Particles.Add({At, Back * 30.f + Jit * 0.5f, 0.2f, 0.2f, 1.4f, 3.f, 0.f, WithAlpha(Col, 0.55f), false});
				break;
			}
		}
	for (FParticle& Pa : Particles)
	{
		Pa.P += Pa.V * Dt;
		Pa.V *= FMath::Max(0.f, 1.f - Pa.Drag * Dt);
		Pa.V.Y += Pa.Gravity * Dt;
		Pa.Life -= Dt;
	}
	Particles.RemoveAll([](const FParticle& Pa) { return Pa.Life <= 0.f; });
	for (FFlare& Fl : Flares) Fl.T += Dt;
	Flares.RemoveAll([](const FFlare& Fl) { return Fl.T >= Fl.MaxT; });
	for (FShard& Sh : Shards)
	{
		Sh.P += Sh.V * Dt;
		Sh.V *= FMath::Max(0.f, 1.f - 2.5f * Dt);
		Sh.V.Y += 60.f * Dt;
		Sh.A += Sh.AV * Dt;
		Sh.Life -= Dt;
	}
	Shards.RemoveAll([](const FShard& Sh) { return Sh.Life <= 0.f; });
	for (auto It = HitFlash.CreateIterator(); It; ++It) { It.Value() -= Dt; if (It.Value() <= 0.f) It.RemoveCurrent(); }
	{ // 체력 막대: 깎인 부분이 흰색으로 남았다가 천천히 따라 내려간다
		TSet<int32> Live;
		for (const FPDEnemy& E : Game.Enemies)
		{
			Live.Add(E.Id);
			const float Now = FMath::Max(0.f, E.Hp / FMath::Max(1e-3f, E.MaxHp));
			float& Shown = HpShown.FindOrAdd(E.Id, Now);
			Shown = Now >= Shown ? Now : FMath::Max(Now, Shown - Dt * 0.9f);
		}
		for (auto It = HpShown.CreateIterator(); It; ++It) if (!Live.Contains(It.Key())) It.RemoveCurrent();
	}
	for (FBeam& B : Beams) B.T += Dt;
	Beams.RemoveAll([](const FBeam& B) { return B.T >= B.MaxT; });
	for (FRing& R : Rings) R.T += Dt;
	Rings.RemoveAll([](const FRing& R) { return R.T >= R.MaxT; });
	for (FFloatText& F : Floats) F.T += Dt;
	Floats.RemoveAll([](const FFloatText& F) { return F.T >= F.MaxT; });
	for (FCellFlash& F : CellFlashes) F.T -= Dt;
	CellFlashes.RemoveAll([](const FCellFlash& F) { return F.T <= 0.f; });
	Shake = FMath::Max(0.f, Shake - Dt);
	FlashT = FMath::Max(0.f, FlashT - Dt);
	BannerT = FMath::Max(0.f, BannerT - Dt);
	// 빠른 적과 보스 잔상
	TSet<int32> Alive;
	for (const FPDEnemy& E : Game.Enemies)
	{
		Alive.Add(E.Id);
		if (E.Type != EPDEnemy::Fast && !EnemyDef(E.Type).bBoss) continue;
		const FPDPoint Pt = Game.Path.At(E.D);
		TArray<FVector2f>& T = Trails.FindOrAdd(E.Id);
		T.Insert(CellPx(Pt.X, Pt.Y), 0);
		if (T.Num() > 8) T.SetNum(8);
	}
	for (auto It = Trails.CreateIterator(); It; ++It) if (!Alive.Contains(It.Key())) It.RemoveCurrent();
}
