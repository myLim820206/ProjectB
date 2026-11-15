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
	Zap,        // 전기 (D-156): 번개 발사체(플러시). A = 0 쏠 때(지지직 + 웅), 1 맞을 때(짧게 탁탁)
	Shake,      // 칵테일 셰이커 흔들기 (D-306): 쉐킷쉐킷 — 쇠통 속 얼음이 오가며 부딪는 소리 약 1.2초
	Pour,       // 칵테일 따르기 (D-306): 조로록 — 잔에 술이 떨어지며 보글거리는 소리 약 1.1초
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
// 메뉴 음악 (D-264): 칵테일바 라운지 스윙. 88 BPM F 장조 16마디(약 44초), 로즈 · 워킹 베이스 · 라이드 · 브러시 · 비브라폰
std::vector<int16_t> SynthesizeMenuMusic(int SampleRate = 44100);
// 인게임 음악 (D-117 → D-264): 재즈 펑크 + 신스. 104 BPM A 단조 Am9-D9-Fmaj7-E7#9 16마디(약 37초), 끝과 처음이 이어지는 반복용 PCM
std::vector<int16_t> SynthesizeMusic(int SampleRate = 44100);
// 보스 웨이브 음악 (D-188 → D-270): 느와르 위압. 66 BPM D 단조 8마디(약 29초), 쿵 · 팀파니 · 찌그러진 낮은 금관 덩어리 · 심장 박동 킥 · 반음 베이스 · 어두운 합창 · 비브라폰 증4도
std::vector<int16_t> SynthesizeBossMusic(int SampleRate = 44100);
// 보스 음악 시안 (D-266): 무겁게. 0 느와르 기어가기 72 BPM, 1 느린 빅밴드 위협 92 BPM 스윙, 2 둠 펑크 84 BPM
std::vector<int16_t> SynthesizeBossMusicAlt(int Variant, int SampleRate = 44100);
// 캐시 키
inline int SoundKey(EPDSound Sound, int A, int B) { return ((int)Sound * 64 + (A & 63)) * 8 + (B & 7); }
} // namespace PD
