// 룰 로그 (D-358): 쉬핑 빌드에서도 남는 파일 로그. UE_LOG 는 쉬핑에서 꺼지므로 직접 파일에 쓴다.
// 위치: Saved/Logs/PDRules.log (쉬핑 = %LOCALAPPDATA%/ProjectB/Saved/Logs). 실행할 때마다 새로 쓰고, 지난 실행은 PDRules-prev.log 로.
// 남기는 것: 룰 사건(놓기 · 족보 · 합성 · 조커 · 먹이기 · 판매 · 웨이브 · 보상 · 레벨 · 보스 기술), 거절 소리, 조커 만들기 고르기 과정.
// 버그를 잡는 동안만 쓰는 장치라 나중에 빼거나 끈다
#include "PDBoardDraw.h"

namespace
{
FString PDRuleLogPath() { return FPaths::ProjectSavedDir() / TEXT("Logs") / TEXT("PDRules.log"); }
const TCHAR* PDPhaseName(EPDPhase P)
{
	switch (P)
	{
	case EPDPhase::Title: return TEXT("Title");
	case EPDPhase::Build: return TEXT("Build");
	case EPDPhase::Wave: return TEXT("Wave");
	case EPDPhase::Reward: return TEXT("Reward");
	case EPDPhase::Over: return TEXT("Over");
	case EPDPhase::Clear: return TEXT("Clear");
	default: return TEXT("Other");
	}
}
FString PDCellS(int K) { return FString::Printf(TEXT("(%d,%d)"), K % Cols, K / Cols); }
}

void SPDBoardWidget::RuleLog(const FString& S) const
{
	if (!bRuleLogOpen) // 이번 실행 첫 줄: 지난 파일은 -prev 로 옮기고 머리말
	{
		bRuleLogOpen = true;
		const FString P = PDRuleLogPath();
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(P), true);
		IFileManager::Get().Move(*(FPaths::GetPath(P) / TEXT("PDRules-prev.log")), *P, true, true, false, true);
		RuleBuf.Add(FString::Printf(TEXT("==== PDRules %s · build %s %s ===="), *FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")), TEXT(__DATE__), TEXT(__TIME__)));
	}
	const FString Line = FString::Printf(TEXT("[%s w%d %s] %s"), *FDateTime::Now().ToString(TEXT("%H:%M:%S")), Game.Wave, PDPhaseName(Game.Phase), *S);
	RuleBuf.Add(Line);
	UE_LOG(LogTemp, Log, TEXT("PDRULE %s"), *Line);
}

void SPDBoardWidget::FlushRuleLog() const
{
	if (RuleBuf.Num() == 0) return;
	FString All = FString::Join(RuleBuf, TEXT("\r\n")) + TEXT("\r\n");
	RuleBuf.Reset();
	FFileHelper::SaveStringToFile(All, *PDRuleLogPath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
}

void SPDBoardWidget::RuleLogNodes(const TCHAR* Why) const
{
	const std::vector<FPDJokerNode> Ns = Game.JokerNodes();
	RuleLog(FString::Printf(TEXT("%s: craftJoker=%d craftUpper=%d picks=%d nodes=%d"), Why, CraftJoker, CraftUpper, (int32)CraftPicks.size(), (int32)Ns.size()));
	for (int32 I = 0; I < (int32)Ns.size(); ++I)
	{
		FString Cs;
		for (int K : Ns[I].Cells)
		{
			const FPDTower* T = Game.Towers.count(K) ? &Game.Towers.at(K) : nullptr;
			Cs += FString::Printf(TEXT("%s%d%s/L%d "), *PDCellS(K), T ? T->Card.Rank : -1, T ? SuitName(T->Card.Suit) : TEXT("?"), T ? T->Level : -1);
		}
		RuleLog(FString::Printf(TEXT("  node %d %s syn=%d fused=%d : %s"), I, HandName(Ns[I].Kind), Ns[I].Syn, Ns[I].Fused, *Cs));
	}
	for (const std::vector<int>& Pk : CraftPicks) { FString Cs; for (int K : Pk) Cs += PDCellS(K) + TEXT(" "); RuleLog(TEXT("  pick ") + Cs); }
	FString Mk;
	for (const FJokerEntry& E : JokerList) if (E.bMake) Mk += E.Upper >= 0 ? FString::Printf(TEXT("U%d "), E.Upper) : FString::Printf(TEXT("J%d "), E.J);
	RuleLog(TEXT("  makeable: ") + Mk);
}
