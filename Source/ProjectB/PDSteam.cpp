// 스팀 연결부 (B8): 아직 App ID 가 없어 빈 구현. 연결할 때:
//  1) ProjectB.Build.cs 에 "OnlineSubsystem", "OnlineSubsystemSteam" (또는 Steamworks SDK 직접), DefaultEngine.ini 에 [OnlineSubsystemSteam] bEnabled=true, SteamDevAppId=App ID
//  2) 아래 함수에서 IOnlineSubsystem::Get()->GetAchievementsInterface()->WriteAchievements(...) 또는 SteamUserStats()->SetAchievement(ApiName) + StoreStats()
//  3) Steamworks 관리 페이지에 PD::AchApiName 의 이름 27개를 등록 (D-341) (목록 DOC/design/23-achievements.md)
#include "PDSteam.h"

namespace PDSteam
{
void UnlockAchievement(const char* ApiName) { (void)ApiName; }
void SyncAchievements(uint32 Bits) { (void)Bits; }
bool IsAvailable() { return false; }
}
