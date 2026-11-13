// 효과음 합성기
// 사운드 파일 없이 코드로 효과음을 만든다 (네온 화풍에 맞춘 신스 소리).
// 언리얼 의존성 없는 순수 C++. 언리얼에서는 이 PCM을 USoundWaveProcedural로 재생하고,
// Tests/render_sounds.cpp 로 소리를 WAV 로 뽑아 들어 볼 수 있다. 이름은 처음 만든 테트리스 포커 때 것이 남아 있다.
#pragma once

#include <cstdint>
#include <vector>

namespace PD
{
enum class EPDSound : uint8_t
{
	Move,       // 카드 고르기, 합성 위치 고르기 시작
	Rotate,     // 적 처치, 골드 강화, 트리 노드 찍기
	HardDrop,   // 카드 버리기
	Lock,       // 족보 없이 카드 설치
	Hold,       // (지금은 안 씀)
	Denied,     // 안 됨 (설치 불가, 골드 부족), 적이 빠져나감
	Burst,      // 족보 생김·합성·먹이기·보상. A = 세기 단계(1~8), B = 크기(0 작음, 1 중간, 2 큼)
	Straight,   // 3장 이상 스트레이트. A = 장수
	Flush,      // 플러시·스트레이트 플러시
	Clear,      // 웨이브 통과, 보스 처치, 합성, 조커 레벨 업
	GameOver,   // 게임 오버
	Win,        // 조커 합성, 키스톤·조커 노드 찍기
	Start,      // 게임 시작, 웨이브 시작
	Shot,       // 발사 (D-155). A = 무늬 (EPDSuit, 4 = 조커·와일드), B = 1 이면 족보 타워 (조금 밝게)
	Hit,        // 타격 (D-155). A = 무늬, B = 1 이면 치명타
	Count
};

struct FPDSoundEvent
{
	EPDSound Sound = EPDSound::Move;
	int A = 0;
	int B = 0;
	float Volume = 1.f;
};

// 소리 하나를 합성한다. 모노 16비트 PCM
std::vector<int16_t> SynthesizeSound(EPDSound Sound, int A, int B, int SampleRate = 44100);
// 배경 음악 (D-117): 80년대 신스웨이브. 104 BPM, A 단조, Am-F-C-G 16마디(약 37초), 끝과 처음이 이어지는 반복용 PCM
std::vector<int16_t> SynthesizeMusic(int SampleRate = 44100);
// 캐시 키
inline int SoundKey(EPDSound Sound, int A, int B) { return ((int)Sound * 64 + (A & 63)) * 8 + (B & 7); }
} // namespace PD
