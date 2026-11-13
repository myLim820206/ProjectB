// 부하 측정 (D-134): F4 치트와 같은 판(길 둘레 2칸을 무작위 카드로, 9칸마다 조커)에서
// 화면이 매 프레임 부르는 코어 함수가 얼마나 걸리는지 잰다.  빌드: build\b.bat → build\bench.exe
#include "../Source/ProjectB/PDCore.h"

#include <chrono>
#include <cstdio>
#include <random>

using namespace PD;

static FPDGame MakeStress()
{
	FPDGame G(7);
	G.NewGame(true);
	G.bMuted = true;
	std::mt19937 R(1234);
	G.Towers.clear();
	int N = 0;
	for (int Y = 0; Y < Rows; ++Y)
		for (int X = 0; X < Cols; ++X)
		{
			if (!G.Buildable(X, Y)) continue;
			bool bNear = false;
			for (int DY = -2; DY <= 2 && !bNear; ++DY)
				for (int DX = -2; DX <= 2 && !bNear; ++DX)
				{
					const int NX = X + DX, NY = Y + DY;
					bNear = NX >= 0 && NY >= 0 && NX < Cols && NY < Rows && G.Path.IsPath(NX, NY);
				}
			if (!bNear) continue;
			FPDTower T;
			T.X = X;
			T.Y = Y;
			T.Card = {2 + (int)(R() % 13), (EPDSuit)(R() % 4)};
			T.Mult = 6.f;
			if (++N % 9 == 0) { T.Joker = (int)(R() % (int)EPDJoker::Count); T.JLevel = 15; T.Mult = 30.f; T.Card = {0, EPDSuit::Wild}; }
			G.Towers[CellKey(X, Y)] = T;
		}
	G.Synergies = ApplyEffects(G.Towers, G.CharmMask(), &G.Mods);
	G.Wave = 19;
	G.Lives = 999;
	G.Phase = EPDPhase::Build;
	return G;
}

template <class F> static double Ms(F&& Fn, int Reps)
{
	const auto A = std::chrono::high_resolution_clock::now();
	for (int I = 0; I < Reps; ++I) Fn();
	const auto B = std::chrono::high_resolution_clock::now();
	return std::chrono::duration<double, std::milli>(B - A).count() / Reps;
}

int main()
{
	FPDGame G = MakeStress();
	std::printf("타워 %d · 족보 %d\n", (int)G.Towers.size(), (int)G.Synergies.size());
	volatile size_t Sink = 0;
	std::printf("ApplyEffects  %.3f ms\n", Ms([&] { FPDTowerMap T = G.Towers; Sink += ApplyEffects(T, G.CharmMask(), &G.Mods).size(); }, 20));
	std::printf("Compute(only) %.3f ms\n", Ms([&] { Sink += ComputeSynergies(G.Towers).size(); }, 20));
	std::printf("JokerGroups   %.3f ms\n", Ms([&] { Sink += G.JokerGroups().size(); }, 20));
	std::printf("FeedTargets   %.3f ms\n", Ms([&] { Sink += G.FeedTargets().size(); }, 20));
	std::printf("JokerMerges   %.3f ms\n", Ms([&] { Sink += G.JokerMerges().size(); }, 20));
	std::printf("TowerPowers   %.3f ms\n", Ms([&] { Sink += G.TowerPowers().size(); }, 20));
	std::printf("SynergyGain   %.3f ms\n", Ms([&] { Sink += (size_t)G.SynergyGain(); }, 20));
	std::printf("Power         %.3f ms\n", Ms([&] { Sink += (size_t)G.Power(); }, 20));
	int Fu = 0;
	std::printf("CanFuse x all %.3f ms\n", Ms([&] { for (int I = 0; I < (int)G.Synergies.size(); ++I) Fu += G.CanFuse(I); }, 20));
	G.StartWave();
	std::printf("Tick(1/60)    %.3f ms (적 나오는 중)\n", Ms([&] { G.Tick(1.f / 60.f); G.Events.clear(); G.Sounds.clear(); }, 300));
	std::printf("적 %d · 발사체 %d\n", (int)G.Enemies.size(), (int)G.Projectiles.size());
	return (int)(Sink & 1) + (Fu < 0);
}
