// 효과음 합성기
// 사운드 파일 없이 코드로 효과음을 만든다 (네온 화풍에 맞춘 신스 소리).
// 언리얼 의존성 없는 순수 C++. 언리얼에서는 이 PCM을 USoundWaveProcedural로 재생하고,
// 웹 프로토타입용 소리(prototype/tetris-poker/sounds.js)도 같은 코드로 뽑는다.
#pragma once

#include <cstdint>
#include <vector>

namespace TP
{
enum class ETPSound : uint8_t
{
	Move,       // 좌우 이동
	Rotate,     // 회전
	HardDrop,   // 즉시 떨어뜨리기
	Lock,       // 자연스럽게 내려앉음 (터짐 없음)
	Hold,       // 보관
	Denied,     // 보관 금지 등 거부
	Burst,      // 족보 터짐. A = 연쇄 단계(1~), B = 크기(0 트리플·투 페어, 1 풀하우스·포카드, 2 파이브 카드)
	Straight,   // 연쇄 스트레이트. A = 이어진 단계 수
	Flush,      // 플러시
	Clear,      // 라운드 통과
	GameOver,   // 실패
	Win,        // 8라운드 전부 통과
	Start,      // 시작, 다음 라운드
	Count
};

struct FTPSoundEvent
{
	ETPSound Sound = ETPSound::Move;
	int A = 0;
	int B = 0;
	float Volume = 1.f;
};

// 소리 하나를 합성한다. 모노 16비트 PCM
std::vector<int16_t> SynthesizeSound(ETPSound Sound, int A, int B, int SampleRate = 44100);
// 캐시 키
inline int SoundKey(ETPSound Sound, int A, int B) { return ((int)Sound * 64 + (A & 63)) * 8 + (B & 7); }
} // namespace TP
