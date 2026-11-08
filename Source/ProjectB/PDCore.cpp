#include "PDCore.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace PD
{
// ---------------------------------------------------------------- 표
static const FPDSuitDef GPDSuits[4] = {
	{1.6f, 0.8f, 3.0f, 0.f, 0.f},  // ♠ 저격
	{0.9f, 0.9f, 2.2f, 0.8f, 0.f}, // ♥ 폭발
	{0.45f, 2.6f, 2.2f, 0.f, 0.f}, // ♦ 연사
	{0.7f, 1.0f, 2.4f, 0.f, 0.4f}, // ♣ 둔화
};
const FPDSuitDef& SuitDef(EPDSuit S) { return GPDSuits[(int)S]; }

static const int GPDHandVal[(int)EPDHand::Count] = {100, 90, 80, 30, 28, 60, 45, 20};
int HandBaseValue(EPDHand H) { return GPDHandVal[(int)H]; }
int HandValue(EPDHand H, int N) { return GPDHandVal[(int)H] + ((H == EPDHand::Flush || H == EPDHand::Straight) ? (N - 3) * 22 : 0); }

std::vector<EPDHand> ClassifyHand(const std::vector<FPDCard>& Cards)
{
	const int N = (int)Cards.size();
	std::map<int, int> Cnt;
	for (const FPDCard& C : Cards) Cnt[C.Rank]++;
	std::vector<int> Vals;
	for (auto& KV : Cnt) Vals.push_back(KV.second);
	std::sort(Vals.rbegin(), Vals.rend());
	bool bSameSuit = true;
	for (const FPDCard& C : Cards) bSameSuit = bSameSuit && C.Suit == Cards[0].Suit;
	bool bStraight = false;
	if ((int)Vals.size() == N && N >= 2)
	{
		std::vector<int> Hi, Lo;
		for (const FPDCard& C : Cards) { Hi.push_back(C.Rank); Lo.push_back(C.Rank == 14 ? 1 : C.Rank); }
		std::sort(Hi.begin(), Hi.end());
		std::sort(Lo.begin(), Lo.end());
		auto Consec = [](const std::vector<int>& V) { for (size_t I = 1; I < V.size(); ++I) if (V[I] != V[I - 1] + 1) return false; return true; };
		bStraight = Consec(Hi) || Consec(Lo);
	}
	std::vector<EPDHand> Out;
	if (N == 2 && Vals[0] == 2) Out.push_back(EPDHand::Pair);
	if (N == 3 && Vals[0] == 3) Out.push_back(EPDHand::Triple);
	if (N == 4 && Vals[0] == 4) Out.push_back(EPDHand::Four);
	if (N == 4 && Vals.size() == 2 && Vals[0] == 2 && Vals[1] == 2) Out.push_back(EPDHand::TwoPair);
	if (N == 5 && Vals.size() == 2 && Vals[0] == 3 && Vals[1] == 2) Out.push_back(EPDHand::FullHouse);
	if (N == 5 && bSameSuit && bStraight) Out.push_back(EPDHand::StraightFlush);
	if (N >= 3 && bSameSuit) Out.push_back(EPDHand::Flush);
	if (N >= 2 && bStraight) Out.push_back(EPDHand::Straight); // 이어진 숫자는 2장부터 (페어와 맞춤)
	return Out;
}

// ---------------------------------------------------------------- 시너지
static const int GPDN4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

// ESU 방식으로 연결 묶음(2~KMax칸)을 한 번씩만 만든다: 시작점보다 번호가 큰 칸으로만 넓힌다
static void PDExtend(const std::vector<std::vector<int>>& Nb, std::vector<int>& Sub, const std::vector<int>& Ext, int Root, int KMax, std::vector<std::vector<int>>& Out)
{
	if (Sub.size() >= 2) Out.push_back(Sub);
	if ((int)Sub.size() == KMax) return;
	std::vector<int> E = Ext;
	while (!E.empty())
	{
		const int W = E.back();
		E.pop_back();
		std::set<int> Near;
		for (int U : Sub) for (int Z : Nb[U]) Near.insert(Z);
		std::vector<int> Next = E;
		for (int U : Nb[W])
			if (U > Root && std::find(Sub.begin(), Sub.end(), U) == Sub.end() && U != W && !Near.count(U) && std::find(E.begin(), E.end(), U) == E.end())
				Next.push_back(U);
		Sub.push_back(W);
		PDExtend(Nb, Sub, Next, Root, KMax, Out);
		Sub.pop_back();
	}
}

std::vector<FPDSynergy> ComputeSynergies(const FPDTowerMap& Towers)
{
	std::vector<FPDSynergy> Picked;
	std::set<int> Seen;
	for (const auto& KV : Towers)
	{
		if (Seen.count(KV.first)) continue;
		// 덩어리 찾기
		std::vector<int> Comp, Stack{KV.first};
		Seen.insert(KV.first);
		while (!Stack.empty())
		{
			const int K = Stack.back();
			Stack.pop_back();
			Comp.push_back(K);
			const int X = K % Cols, Y = K / Cols;
			for (const auto& D : GPDN4)
			{
				const int NX = X + D[0], NY = Y + D[1];
				if (NX < 0 || NY < 0 || NX >= Cols || NY >= Rows) continue;
				const int NK = CellKey(NX, NY);
				if (Towers.count(NK) && !Seen.count(NK)) { Seen.insert(NK); Stack.push_back(NK); }
			}
		}
		if (Comp.size() < 2) continue;
		std::sort(Comp.begin(), Comp.end());
		std::map<int, int> Idx;
		for (int I = 0; I < (int)Comp.size(); ++I) Idx[Comp[I]] = I;
		std::vector<std::vector<int>> Nb(Comp.size());
		for (int I = 0; I < (int)Comp.size(); ++I)
		{
			const int X = Comp[I] % Cols, Y = Comp[I] / Cols;
			for (const auto& D : GPDN4)
			{
				const int NX = X + D[0], NY = Y + D[1];
				if (NX < 0 || NY < 0 || NX >= Cols || NY >= Rows) continue;
				auto It = Idx.find(CellKey(NX, NY));
				if (It != Idx.end()) Nb[I].push_back(It->second);
			}
		}
		std::vector<std::vector<int>> Subs;
		for (int V = 0; V < (int)Comp.size(); ++V)
		{
			std::vector<int> Sub{V}, Ext;
			for (int U : Nb[V]) if (U > V) Ext.push_back(U);
			PDExtend(Nb, Sub, Ext, V, 5, Subs);
		}
		struct FCand { EPDHand Kind; int N, Value; std::vector<int> Cells; };
		std::vector<FCand> Cands;
		for (const auto& Sub : Subs)
		{
			std::vector<FPDCard> Cards;
			std::vector<int> Cells;
			for (int I : Sub) { Cells.push_back(Comp[I]); Cards.push_back(Towers.at(Comp[I]).Card); }
			for (EPDHand H : ClassifyHand(Cards)) Cands.push_back({H, (int)Sub.size(), HandValue(H, (int)Sub.size()), Cells});
		}
		std::stable_sort(Cands.begin(), Cands.end(), [](const FCand& A, const FCand& B) { return A.Value != B.Value ? A.Value > B.Value : A.N > B.N; });
		std::set<int> Used;
		for (const FCand& C : Cands)
		{
			bool bFree = true;
			for (int K : C.Cells) bFree = bFree && !Used.count(K);
			if (!bFree) continue;
			for (int K : C.Cells) Used.insert(K);
			FPDSynergy S;
			S.Kind = C.Kind;
			S.N = C.N;
			S.Value = C.Value;
			S.Cells = C.Cells;
			Picked.push_back(S);
		}
	}
	// 연쇄: 족보끼리 맞닿아 있으면 하나의 연쇄
	std::map<int, int> Owner;
	for (int I = 0; I < (int)Picked.size(); ++I) for (int K : Picked[I].Cells) Owner[K] = I;
	std::vector<std::set<int>> Adj(Picked.size());
	for (int I = 0; I < (int)Picked.size(); ++I)
		for (int K : Picked[I].Cells)
		{
			const int X = K % Cols, Y = K / Cols;
			for (const auto& D : GPDN4)
			{
				const int NX = X + D[0], NY = Y + D[1];
				if (NX < 0 || NY < 0 || NX >= Cols || NY >= Rows) continue;
				auto It = Owner.find(CellKey(NX, NY));
				if (It != Owner.end() && It->second != I) Adj[I].insert(It->second);
			}
		}
	int Cid = 0;
	for (int I = 0; I < (int)Picked.size(); ++I)
	{
		if (Picked[I].ChainId >= 0) continue;
		std::vector<int> Mem, St{I};
		Picked[I].ChainId = Cid;
		while (!St.empty())
		{
			const int K = St.back();
			St.pop_back();
			Mem.push_back(K);
			for (int J : Adj[K]) if (Picked[J].ChainId < 0) { Picked[J].ChainId = Cid; St.push_back(J); }
		}
		for (int K : Mem) Picked[K].Chain = (int)Mem.size();
		Cid++;
	}
	for (int I = 0; I < (int)Picked.size(); ++I) Picked[I].Links.assign(Adj[I].begin(), Adj[I].end());
	return Picked;
}

std::vector<FPDSynergy> ApplyEffects(FPDTowerMap& Towers)
{
	std::vector<FPDSynergy> Syn = ComputeSynergies(Towers);
	for (auto& KV : Towers) KV.second.Eff = FPDEffects{};
	for (int I = 0; I < (int)Syn.size(); ++I)
		for (int K : Syn[I].Cells)
		{
			FPDEffects& E = Towers[K].Eff;
			E.Hand = I;
			const int N = Syn[I].N;
			switch (Syn[I].Kind)
			{
			case EPDHand::Pair: E.Dmg *= 1.5f; break;
			case EPDHand::TwoPair: E.Dmg *= 1.5f; E.Range *= 1.25f; break;
			case EPDHand::Triple: E.Dmg *= 2.f; break;
			case EPDHand::Straight: E.Rate *= 1.f + 0.25f * (N - 1); break;
			case EPDHand::Flush: E.Splash = std::max(E.Splash, 0.5f + 0.25f * N); break;
			case EPDHand::FullHouse: E.Dmg *= 1.5f; E.Rate *= 1.5f; E.Splash = std::max(E.Splash, 1.5f); break;
			case EPDHand::Four: E.Dmg *= 3.5f; break;
			case EPDHand::StraightFlush: E.Dmg *= 2.f; E.Rate *= 2.f; E.Splash = std::max(E.Splash, 2.f); break;
			default: break;
			}
			if (Syn[I].Chain >= 2) { E.Dmg *= 1.f + ChainStep * (Syn[I].Chain - 1); E.Chain = Syn[I].Chain; }
		}
	return Syn;
}

FPDTowerStats TowerStats(const FPDTower& T)
{
	const FPDSuitDef& S = SuitDef(T.Card.Suit);
	return {(3.f + T.Card.Rank) * S.Dmg * T.Eff.Dmg, S.Rate * T.Eff.Rate, S.Range * T.Eff.Range, std::max(S.Splash, T.Eff.Splash), S.Slow};
}

// ---------------------------------------------------------------- 길
void FPDPath::Build(const std::vector<std::array<int, 2>>& InCorners)
{
	Corners = InCorners;
	Points.clear();
	SegStart.clear();
	Length = 0.f;
	OnPath.assign(Cols * Rows, 0);
	for (const auto& C : Corners) Points.push_back({C[0] + 0.5f, C[1] + 0.5f});
	for (size_t I = 1; I < Points.size(); ++I)
	{
		SegStart.push_back(Length);
		Length += std::hypot(Points[I].X - Points[I - 1].X, Points[I].Y - Points[I - 1].Y);
	}
	for (size_t I = 1; I < Corners.size(); ++I)
	{
		int X = Corners[I - 1][0], Y = Corners[I - 1][1];
		const int BX = Corners[I][0], BY = Corners[I][1];
		const int DX = (BX > X) - (BX < X), DY = (BY > Y) - (BY < Y);
		while (true)
		{
			if (X >= 0 && X < Cols && Y >= 0 && Y < Rows) OnPath[CellKey(X, Y)] = 1;
			if (X == BX && Y == BY) break;
			X += DX;
			Y += DY;
		}
	}
}

FPDPoint FPDPath::At(float D) const
{
	for (size_t I = 1; I < Points.size(); ++I)
	{
		const float L = std::hypot(Points[I].X - Points[I - 1].X, Points[I].Y - Points[I - 1].Y);
		if (D <= SegStart[I - 1] + L)
		{
			const float U = L > 0.f ? (D - SegStart[I - 1]) / L : 0.f;
			return {Points[I - 1].X + (Points[I].X - Points[I - 1].X) * U, Points[I - 1].Y + (Points[I].Y - Points[I - 1].Y) * U};
		}
	}
	return Points.empty() ? FPDPoint{} : Points.back();
}

int FPDPath::CellCount() const
{
	int N = 0;
	for (size_t I = 1; I < Corners.size(); ++I) N += std::abs(Corners[I][0] - Corners[I - 1][0]) + std::abs(Corners[I][1] - Corners[I - 1][1]);
	return N;
}

std::vector<std::array<int, 2>> GeneratePath(std::mt19937& Rng)
{
	auto Ri = [&](int A, int B) { return std::uniform_int_distribution<int>(A, B)(Rng); };
	for (int Tries = 0; Tries < 500; ++Tries)
	{
		int X = -1, Y = Ri(1, Rows - 2);
		std::vector<std::array<int, 2>> Pts{{X, Y}};
		while (true)
		{
			const int NX = X + Ri(2, 4);
			if (NX >= Cols - 2) { Pts.push_back({Cols, Y}); break; }
			Pts.push_back({NX, Y});
			X = NX;
			int NY;
			do NY = Ri(1, Rows - 2); while (std::abs(NY - Y) < 2);
			Pts.push_back({X, NY});
			Y = NY;
		}
		int Cells = 0;
		for (size_t I = 1; I < Pts.size(); ++I) Cells += std::abs(Pts[I][0] - Pts[I - 1][0]) + std::abs(Pts[I][1] - Pts[I - 1][1]);
		if (Cells >= PathMin && Cells <= PathMax) return Pts;
	}
	return {{-1, 1}, {3, 1}, {3, 7}, {7, 7}, {7, 2}, {11, 2}, {11, 8}, {14, 8}, {14, 4}, {16, 4}};
}

// ---------------------------------------------------------------- 진행
FPDGame::FPDGame(uint32_t Seed) : Rng(Seed ? Seed : std::random_device{}()) { NewGame(false); }

void FPDGame::NewGame(bool bStart)
{
	Phase = bStart ? EPDPhase::Build : EPDPhase::Title;
	Wave = 0;
	Lives = StartLives;
	Kills = Leaks = Discarded = 0;
	Time = 0.f;
	Towers.clear();
	Synergies.clear();
	Enemies.clear();
	Pending.clear();
	Hand.clear();
	Deck.clear();
	Events.clear();
	for (int S = 0; S < 4; ++S) for (int R = 2; R <= 14; ++R) Deck.push_back({R, (EPDSuit)S});
	std::shuffle(Deck.begin(), Deck.end(), Rng);
	Path.Build(GeneratePath(Rng));
	Draw(StartHand);
}

void FPDGame::Draw(int N, int* OutGot)
{
	int Got = 0;
	while (Got < N && (int)Hand.size() < HandMax && !Deck.empty()) { Hand.push_back(Deck.back()); Deck.pop_back(); Got++; }
	if (OutGot) *OutGot = Got;
}

bool FPDGame::Buildable(int X, int Y) const { return X >= 0 && Y >= 0 && X < Cols && Y < Rows && !Path.IsPath(X, Y); }

std::vector<FPDGame::FSpawn> FPDGame::WaveSpec(int W) const
{
	std::vector<FSpawn> List;
	const float Hp = 26.f * std::pow(1.39f, (float)(W - 1));
	const int N = 8 + W * 2;
	for (int I = 0; I < N; ++I)
	{
		EPDEnemy T = EPDEnemy::Normal;
		if (W >= 3 && I % 4 == 3) T = EPDEnemy::Fast;
		if (W >= 5 && I % 6 == 5) T = EPDEnemy::Tank;
		List.push_back({T, 0.f, 0.f});
	}
	if (W % 5 == 0) List.push_back({EPDEnemy::Boss, 0.f, 0.f});
	for (FSpawn& S : List)
	{
		static const float HpK[4] = {1.f, 0.6f, 2.6f, 14.f}, Sp[4] = {1.1f, 1.9f, 0.7f, 0.55f};
		S.Hp = Hp * HpK[(int)S.Type];
		S.Speed = Sp[(int)S.Type];
	}
	return List;
}

void FPDGame::StartWave()
{
	if (Phase != EPDPhase::Build) return;
	Wave++;
	Phase = EPDPhase::Wave;
	Pending = WaveSpec(Wave);
	SpawnT = 0.f;
	FPDEvent E;
	E.Kind = FPDEvent::EKind::WaveStart;
	E.A = Wave;
	E.B = Wave % 5 == 0;
	Ev(E);
	Sfx(EPDSound::Start);
}

void FPDGame::EndWave()
{
	if (Wave >= NumWaves) { Phase = EPDPhase::Win; FPDEvent E; E.Kind = FPDEvent::EKind::Win; Ev(E); Sfx(EPDSound::Win); return; }
	int Got = 0;
	Draw(DrawPerWave, &Got);
	Phase = EPDPhase::Build;
	FPDEvent E;
	E.Kind = FPDEvent::EKind::WaveClear;
	E.A = Got;
	Ev(E);
	Sfx(EPDSound::Clear);
}

std::vector<FPDSynergy> FPDGame::PreviewSynergies(int HandIdx, int X, int Y, FPDTowerMap* OutTowers) const
{
	std::vector<FPDSynergy> Out;
	if (HandIdx < 0 || HandIdx >= (int)Hand.size() || !Buildable(X, Y)) return Out;
	FPDTowerMap Copy = Towers;
	FPDTower T;
	T.Card = Hand[HandIdx];
	T.X = X;
	T.Y = Y;
	Copy[CellKey(X, Y)] = T;
	std::vector<FPDSynergy> All = ApplyEffects(Copy);
	std::set<int> Chains;
	for (const FPDSynergy& S : All) if (std::find(S.Cells.begin(), S.Cells.end(), CellKey(X, Y)) != S.Cells.end()) Chains.insert(S.ChainId);
	for (const FPDSynergy& S : All) if (Chains.count(S.ChainId)) Out.push_back(S);
	if (OutTowers) *OutTowers = std::move(Copy);
	return Out;
}

bool FPDGame::Place(int HandIdx, int X, int Y)
{
	if (Phase == EPDPhase::Title || Phase == EPDPhase::Over || Phase == EPDPhase::Win) return false;
	if (HandIdx < 0 || HandIdx >= (int)Hand.size() || !Buildable(X, Y)) return false;
	const int Key = CellKey(X, Y);
	auto Sig = [](const FPDSynergy& S) { std::vector<int> V = S.Cells; std::sort(V.begin(), V.end()); V.push_back((int)S.Kind); return V; };
	std::set<std::vector<int>> Before;
	for (const FPDSynergy& S : Synergies) Before.insert(Sig(S));
	if (Towers.count(Key)) Discarded++;
	FPDTower T;
	T.Card = Hand[HandIdx];
	T.X = X;
	T.Y = Y;
	Hand.erase(Hand.begin() + HandIdx);
	Towers[Key] = T;
	Synergies = ApplyEffects(Towers);
	FPDEvent P;
	P.Kind = FPDEvent::EKind::Place;
	P.X = X + 0.5f;
	P.Y = Y + 0.5f;
	P.Suit = T.Card.Suit;
	Ev(P);
	// 새로 생긴 족보 중 가장 큰 것
	const FPDSynergy* Best = nullptr;
	for (const FPDSynergy& S : Synergies)
		if (!Before.count(Sig(S)) && std::find(S.Cells.begin(), S.Cells.end(), Key) != S.Cells.end() && (!Best || S.Value > Best->Value)) Best = &S;
	if (Best)
	{
		FPDEvent E;
		E.Kind = FPDEvent::EKind::Synergy;
		E.Hand = Best->Kind;
		E.A = Best->N;
		E.B = Best->Chain;
		float CX = 0.f, CY = 0.f;
		for (int K : Best->Cells) { CX += K % Cols + 0.5f; CY += K / Cols + 0.5f; }
		E.X = CX / Best->Cells.size();
		E.Y = CY / Best->Cells.size();
		Ev(E);
		Sfx(EPDSound::Burst, std::min(8, std::max(1, Best->Value / 15 + Best->Chain - 1)), Best->Value >= 80 ? 2 : Best->Value >= 50 ? 1 : 0);
		if (Best->Kind == EPDHand::Flush || Best->Kind == EPDHand::StraightFlush) Sfx(EPDSound::Flush);
		if (Best->Kind == EPDHand::Straight && Best->N >= 3) Sfx(EPDSound::Straight, Best->N);
	}
	else
		Sfx(EPDSound::Lock);
	return true;
}

void FPDGame::Discard(int HandIdx)
{
	if (HandIdx < 0 || HandIdx >= (int)Hand.size()) return;
	Hand.erase(Hand.begin() + HandIdx);
	Discarded++;
	FPDEvent E;
	E.Kind = FPDEvent::EKind::Discard;
	Ev(E);
	Sfx(EPDSound::HardDrop);
}

void FPDGame::Hit(FPDEnemy& E, float Dmg, float Slow)
{
	E.Hp -= Dmg;
	if (Slow > 0.f) { E.SlowT = 1.2f; E.SlowAmt = std::max(E.SlowAmt, Slow); }
	if (E.Hp <= 0.f && !E.bDead)
	{
		E.bDead = true;
		Kills++;
		const FPDPoint P = Path.At(E.D);
		FPDEvent K;
		K.Kind = FPDEvent::EKind::Kill;
		K.X = P.X;
		K.Y = P.Y;
		K.Enemy = E.Type;
		Ev(K);
		if (KillSfxT <= 0.f || E.Type == EPDEnemy::Boss) { Sfx(E.Type == EPDEnemy::Boss ? EPDSound::Clear : EPDSound::Rotate); KillSfxT = 0.08f; }
	}
}

void FPDGame::Update(float Dt)
{
	Time += Dt;
	for (auto& KV : Towers) KV.second.Age += Dt;
	KillSfxT -= Dt;
	if (Phase != EPDPhase::Wave) return;
	SpawnT -= Dt;
	if (!Pending.empty() && SpawnT <= 0.f)
	{
		const FSpawn S = Pending.front();
		Pending.erase(Pending.begin());
		FPDEnemy E;
		E.Type = S.Type;
		E.Hp = E.MaxHp = S.Hp;
		E.Speed = S.Speed;
		E.Id = NextId++;
		Enemies.push_back(E);
		SpawnT = S.Type == EPDEnemy::Boss ? 1.2f : 0.65f;
	}
	for (FPDEnemy& E : Enemies)
	{
		E.SlowT = std::max(0.f, E.SlowT - Dt);
		E.D += E.Speed * (E.SlowT > 0.f ? 1.f - E.SlowAmt : 1.f) * Dt;
		if (E.D >= Path.Length && !E.bDead)
		{
			E.bDead = true;
			const int Loss = E.Type == EPDEnemy::Boss ? 5 : 1;
			Lives -= Loss;
			Leaks++;
			FPDEvent L;
			L.Kind = FPDEvent::EKind::Leak;
			L.A = Loss;
			Ev(L);
			Sfx(EPDSound::Denied);
		}
	}
	for (auto& KV : Towers)
	{
		FPDTower& T = KV.second;
		T.Cd -= Dt;
		if (T.Cd > 0.f) continue;
		const FPDTowerStats St = TowerStats(T);
		const float TX = T.X + 0.5f, TY = T.Y + 0.5f;
		FPDEnemy* Target = nullptr;
		for (FPDEnemy& E : Enemies)
		{
			if (E.bDead) continue;
			const FPDPoint P = Path.At(E.D);
			if (std::hypot(P.X - TX, P.Y - TY) <= St.Range && (!Target || E.D > Target->D)) Target = &E;
		}
		if (!Target) continue;
		T.Cd = 1.f / St.Rate;
		const FPDPoint P = Path.At(Target->D);
		FPDEvent Sh;
		Sh.Kind = FPDEvent::EKind::Shot;
		Sh.X = TX;
		Sh.Y = TY;
		Sh.X2 = P.X;
		Sh.Y2 = P.Y;
		Sh.Suit = T.Card.Suit;
		Sh.A = T.Eff.Hand >= 0;
		Ev(Sh);
		Hit(*Target, St.Dmg, St.Slow);
		if (St.Splash > 0.f)
		{
			for (FPDEnemy& O : Enemies)
			{
				if (&O == Target || O.bDead) continue;
				const FPDPoint Q = Path.At(O.D);
				if (std::hypot(Q.X - P.X, Q.Y - P.Y) <= St.Splash) Hit(O, St.Dmg * 0.6f, St.Slow);
			}
			FPDEvent Sp;
			Sp.Kind = FPDEvent::EKind::Splash;
			Sp.X = P.X;
			Sp.Y = P.Y;
			Sp.R = St.Splash;
			Sp.Suit = T.Card.Suit;
			Ev(Sp);
		}
	}
	Enemies.erase(std::remove_if(Enemies.begin(), Enemies.end(), [](const FPDEnemy& E) { return E.bDead; }), Enemies.end());
	if (Lives <= 0)
	{
		Lives = 0;
		Phase = EPDPhase::Over;
		FPDEvent G;
		G.Kind = FPDEvent::EKind::GameOver;
		Ev(G);
		Sfx(EPDSound::GameOver);
		return;
	}
	if (Pending.empty() && Enemies.empty()) EndWave();
}

void FPDGame::Tick(float Dt)
{
	Dt = std::min(Dt, 0.05f);
	for (int I = 0; I < Speed; ++I) Update(Dt);
}
} // namespace PD
