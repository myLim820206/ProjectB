// QA 39 사본 규칙 시험: 발사체 출처 칸, 상위 합치기 경험치, 레시피 카드 공유
#include "../../Source/ProjectB/PDCore.h"
#include <cstdio>
#include <cmath>
using namespace PD;
static void Put(FPDTowerMap& M, int X, int Y, FPDCard Card) { FPDTower T; T.Card = Card; T.X = X; T.Y = Y; M[CellKey(X, Y)] = T; }
static FPDTower Joker(int X, int Y, EPDJoker K, int Lv = 1, float Xp = 0.f) { FPDTower T; T.Card = {0, EPDSuit::Wild}; T.X = X; T.Y = Y; T.Joker = (int)K; T.Mult = 4.f; T.JLevel = Lv; T.JXp = Xp; T.Cd = 100.f; return T; }
// 날아가는 발사체 하나 (출처 Src) 를 두고, 그 사이 판을 바꾼 뒤 한 틱
static float FlightKill(bool bReplaceSrcWithJoker, bool bMoveJokerAway, float* SrcXp)
{
	FPDGame G(5);
	G.NewGame(true);
	G.bMuted = true;
	G.Towers.clear();
	G.Phase = EPDPhase::Wave;
	const int Src = CellKey(1, 1), Other = CellKey(7, 5);
	if (bMoveJokerAway) G.Towers[Src] = Joker(1, 1, EPDJoker::Jack);
	else { Put(G.Towers, 1, 1, {9, EPDSuit::Heart}); G.Towers[Src].Cd = 100.f; }
	G.Towers[Other] = Joker(7, 5, EPDJoker::Bell);
	G.Enemies.clear();
	FPDEnemy E = G.SpawnTestEnemy(EPDEnemy::Normal, 1.f, 3);
	E.D = 3.f; E.Id = 777;
	G.Enemies.push_back(E);
	const FPDPoint P = G.Path.At(E.D);
	FPDProjectile Pr; Pr.Kind = EPDShot::Missile; Pr.X = P.X; Pr.Y = P.Y; Pr.TX = P.X; Pr.TY = P.Y; Pr.Target = 777; Pr.Dmg = 1e6f; Pr.Src = Src;
	G.Projectiles.push_back(Pr);
	if (bReplaceSrcWithJoker) G.Towers[Src] = Joker(1, 1, EPDJoker::Jack); // 날아가는 동안 그 칸 카드로 조커를 만듦 (웨이브 중에도 된다, InPlay)
	if (bMoveJokerAway) { G.Towers.erase(Src); G.Towers[CellKey(2, 1)] = Joker(2, 1, EPDJoker::Clown); } // 날아가는 동안 쏜 조커가 옆 칸 상위 조커 재료로 들어감
	G.Update(0.016f);
	bool bDead = true; for (auto& X : G.Enemies) if (X.Id == 777 && !X.bDead) bDead = false;
	if (SrcXp) *SrcXp = G.Towers.count(Src) ? G.Towers.at(Src).JXp : -1.f;
	std::printf("  enemy dead=%d kills=%d\n", (int)bDead, G.Kills);
	return G.Towers.at(Other).JXp;
}
int main()
{
	float SX = 0;
	float A = FlightKill(false, false, &SX);
	std::printf("A 카드 발사체가 잡음 (판 그대로): 다른 조커 경험치 %g (기대 0)\n", A);
	float B = FlightKill(true, false, &SX);
	std::printf("B 카드 발사체가 날아가는 동안 그 칸이 조커가 됨: 다른 조커 경험치 %g, 새 조커 %g (기대 0 = 카드가 잡은 것)\n", B, SX);
	float C2 = FlightKill(false, true, &SX);
	std::printf("C 조커 발사체가 날아가는 동안 쏜 조커가 다른 칸으로 합쳐짐: 다른 조커 경험치 %g (조커가 잡았으니 기대 %g)\n", C2, JokerKillXp);
	// 상위 합치기: 경험치
	{
		FPDGame G(5); G.NewGame(true); G.bMuted = true; G.Towers.clear();
		int U = -1, AJ = -1, BJ = -1;
		for (int a = 0; a < (int)EPDJoker::Count && U < 0; ++a) for (int b = 0; b < (int)EPDJoker::Count; ++b) { if (a == b) continue; int u = UpperOf((EPDJoker)a, (EPDJoker)b); if (u >= 0 && UpperDef(u).Special == EPDUpperSpecial::None && G.UpperOpen(u)) { U = u; AJ = a; BJ = b; break; } }
		G.Towers[CellKey(3, 3)] = Joker(3, 3, (EPDJoker)AJ, 6, JokerXpNeed(6) - 1.f);
		G.Towers[CellKey(4, 3)] = Joker(4, 3, (EPDJoker)BJ, 4, 30.f);
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto Ms = G.JokerMerges(); int Mi = -1; for (int i = 0; i < (int)Ms.size(); ++i) if (Ms[i].Upper == U) Mi = i;
		const bool bOk = Mi >= 0 && G.MergeJokers(Mi, CellKey(3, 3));
		const FPDTower& J = G.Towers.at(CellKey(3, 3));
		std::printf("D 상위 합치기 (%d+%d → 상위 %d) ok=%d: 레벨 %d 경험치 %g (재료: 레벨 6 경험치 %g, 레벨 4 경험치 30)\n", AJ, BJ, U, (int)bOk, J.JLevel, J.JXp, JokerXpNeed(6) - 1.f);
	}
	{
		FPDGame G(5); G.NewGame(true); G.bMuted = true; G.Towers.clear();
		G.Towers[CellKey(3, 3)] = Joker(3, 3, EPDJoker::Clown, 6, 10.f);
		G.Towers[CellKey(4, 3)] = Joker(4, 3, EPDJoker::Clown, 6, 25.f);
		G.Synergies = ApplyEffects(G.Towers, 0);
		const bool bOk = G.MergeJokers(0, CellKey(3, 3));
		const FPDTower& J = G.Towers.at(CellKey(3, 3));
		std::printf("E 같은 조커 합치기 ok=%d: 레벨 %d 경험치 %g (재료 레벨 6 경험치 10 · 25)\n", (int)bOk, J.JLevel, J.JXp);
	}
	return 0;
}
