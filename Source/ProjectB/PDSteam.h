// 스팀 연결부 (B8): 도전 과제 · 클라우드 저장. App ID 가 생기고 Steamworks(OnlineSubsystemSteam) 를 켜면 PDSteam.cpp 만 채운다.
// 지금은 아무것도 안 한다 (게임은 진행 파일에 도전 과제를 따로 남긴다). 클라우드 저장은 Steam Auto-Cloud 설정만으로 (docs/03 2절)
#pragma once
#include "CoreMinimal.h"

namespace PDSteam
{
void UnlockAchievement(const char* ApiName); // 도전 과제 하나 (Steamworks 에 같은 이름으로 등록, PD::AchApiName)
void SyncAchievements(uint32 Bits);          // 이미 이룬 것 모두 다시 알리기 (오프라인에서 이룬 것)
bool IsAvailable();                          // 스팀이 연결되어 있나
}
