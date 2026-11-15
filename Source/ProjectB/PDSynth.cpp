#include "PDSynth.h"

#include <algorithm>
#include <cmath>

namespace PD
{
namespace
{
constexpr float SynthTwoPi = 6.2831853f;

enum class EWave : uint8_t { Sine, Triangle, Square, Saw };

float MidiHz(float Midi) { return 440.f * std::pow(2.f, (Midi - 69.f) / 12.f); }

struct FSynthBuf
{
	int Rate;
	std::vector<float> S;
	explicit FSynthBuf(int InRate) : Rate(InRate) {}
	void Ensure(size_t N)
	{
		if (S.size() < N) S.resize(N, 0.f);
	}

	// 음 하나: 시작 시각, 길이, 시작·끝 주파수(지수 변화), 파형, 크기, 어택, 감쇠 속도(클수록 빨리 줄어듦)
	void Tone(float T0, float Dur, float F0, float F1, EWave W, float Amp, float Attack = 0.004f, float DecayK = 4.f, float TremHz = 0.f)
	{
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)(Dur * Rate);
		Ensure(I0 + N);
		float Phase = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate, U = T / Dur;
			const float F = F0 * std::pow(F1 / F0, U);
			Phase += F / Rate;
			Phase -= std::floor(Phase);
			float V = 0.f;
			switch (W)
			{
			case EWave::Sine: V = std::sin(SynthTwoPi * Phase); break;
			case EWave::Triangle: V = 4.f * std::fabs(Phase - 0.5f) - 1.f; break;
			case EWave::Square: V = std::tanh(3.f * std::sin(SynthTwoPi * Phase)) * 0.8f; break; // 부드러운 사각파
			case EWave::Saw: V = (2.f * Phase - 1.f) * 0.7f; break;
			}
			float Env = T < Attack ? T / Attack : std::exp(-DecayK * (T - Attack) / Dur);
			const float Tail = Dur - T;
			if (Tail < 0.006f) Env *= Tail / 0.006f; // 끝 딸깍 소리 방지
			if (TremHz > 0.f) Env *= 0.7f + 0.3f * std::sin(SynthTwoPi * TremHz * T);
			S[I0 + I] += V * Env * Amp;
		}
	}
	void Note(float T0, float Dur, float Midi, EWave W, float Amp, float DecayK = 4.f) { Tone(T0, Dur, MidiHz(Midi), MidiHz(Midi), W, Amp, 0.004f, DecayK); }

	// 잡음 (Lowpass 0~1: 작을수록 둔한 소리)
	void Noise(float T0, float Dur, float Amp, float Lowpass, uint32_t Seed)
	{
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)(Dur * Rate);
		Ensure(I0 + N);
		uint32_t X = Seed * 2654435761u + 1u;
		float Lp = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			X ^= X << 13;
			X ^= X >> 17;
			X ^= X << 5;
			const float R = (float)(X & 0xFFFF) / 32768.f - 1.f;
			Lp += (R - Lp) * Lowpass;
			const float U = (float)I / N;
			S[I0 + I] += Lp * Amp * (1.f - U) * (1.f - U);
		}
	}

	// 짧은 메아리 (공간감)
	void Echo(float Delay, float Feedback, int Repeats)
	{
		const size_t D = (size_t)(Delay * Rate);
		const size_t Orig = S.size();
		Ensure(Orig + D * Repeats);
		for (size_t I = D; I < S.size(); ++I) S[I] += S[I - D] * Feedback;
	}

	std::vector<int16_t> ToPcm(float Gain) const
	{
		std::vector<int16_t> Out(S.size());
		for (size_t I = 0; I < S.size(); ++I)
		{
			const float V = std::tanh(S[I] * Gain); // 부드러운 클리핑
			Out[I] = (int16_t)std::lround(V * 30000.f);
		}
		return Out;
	}
};

// 연쇄 단계가 오를수록 음이 올라간다 (장조 5음계)
float BurstRoot(int Chain)
{
	static const int Pent[5] = {0, 2, 4, 7, 9};
	const int C = std::max(1, Chain) - 1;
	return 72.f + Pent[C % 5] + 12.f * (C / 5);
}
} // namespace

std::vector<int16_t> SynthesizeSound(EPDSound Sound, int A, int B, int Rate)
{
	FSynthBuf Buf(Rate);
	float Gain = 1.f;
	switch (Sound)
	{
	case EPDSound::Move:
		Buf.Tone(0.f, 0.022f, 1250.f, 1100.f, EWave::Square, 0.10f, 0.001f, 6.f);
		break;
	case EPDSound::Rotate:
		Buf.Tone(0.f, 0.05f, 700.f, 1080.f, EWave::Triangle, 0.20f, 0.002f, 3.f);
		break;
	case EPDSound::HardDrop:
		Buf.Tone(0.f, 0.16f, 190.f, 52.f, EWave::Sine, 0.65f, 0.002f, 3.f);
		Buf.Noise(0.f, 0.05f, 0.30f, 0.25f, 11);
		Buf.Tone(0.f, 0.03f, 1800.f, 900.f, EWave::Triangle, 0.06f, 0.001f, 5.f);
		break;
	case EPDSound::Lock:
		Buf.Tone(0.f, 0.06f, 340.f, 300.f, EWave::Triangle, 0.22f, 0.002f, 5.f);
		break;
	case EPDSound::Hold:
		Buf.Note(0.f, 0.05f, 79.f, EWave::Triangle, 0.18f);
		Buf.Note(0.05f, 0.06f, 74.f, EWave::Triangle, 0.18f);
		break;
	case EPDSound::Denied:
		Buf.Tone(0.f, 0.15f, 150.f, 140.f, EWave::Square, 0.16f, 0.003f, 2.f);
		Buf.Tone(0.f, 0.15f, 156.f, 146.f, EWave::Square, 0.16f, 0.003f, 2.f);
		break;
	case EPDSound::Burst:
	{
		// 화음을 빠르게 펼친다. 연쇄가 깊을수록 높게, 족보가 클수록 음이 많고 길게
		const float Root = BurstRoot(A);
		const float Dur = 0.38f + 0.12f * B;
		std::vector<float> Notes = {0.f, 7.f, 12.f};
		if (B >= 1) Notes.push_back(16.f);
		if (B >= 2) Notes.push_back(19.f);
		for (size_t I = 0; I < Notes.size(); ++I)
		{
			const float T = 0.032f * I;
			Buf.Note(T, Dur, Root + Notes[I], EWave::Triangle, 0.22f, 4.5f);
			Buf.Note(T, Dur, Root + Notes[I] + 12.f, EWave::Sine, 0.08f, 6.f);
		}
		Buf.Note(0.f, Dur * 0.8f, Root - 12.f, EWave::Sine, 0.20f, 5.f); // 아래 받침음
		Buf.Noise(0.f, 0.07f, 0.10f, 0.9f, 23 + A);                       // 반짝이는 잡음
		Buf.Echo(0.11f, 0.28f, 3);
		Gain = 1.1f;
		break;
	}
	case EPDSound::Straight:
	{
		const int N = std::max(3, A) + 1;
		static const int Pent[5] = {0, 2, 4, 7, 9};
		for (int I = 0; I < N; ++I)
			Buf.Note(0.055f * I, 0.16f, 79.f + Pent[I % 5] + 12.f * (I / 5), EWave::Square, 0.17f, 4.f);
		Buf.Echo(0.09f, 0.3f, 3);
		break;
	}
	case EPDSound::Flush:
		Buf.Tone(0.f, 0.5f, 1568.f, 1568.f, EWave::Sine, 0.10f, 0.02f, 2.5f, 17.f);
		Buf.Tone(0.f, 0.5f, 1577.f, 1577.f, EWave::Sine, 0.10f, 0.02f, 2.5f, 13.f);
		Buf.Tone(0.03f, 0.45f, 2093.f, 2093.f, EWave::Sine, 0.07f, 0.02f, 3.f, 19.f);
		Buf.Echo(0.13f, 0.3f, 3);
		break;
	case EPDSound::Clear:
	{
		const float Ns[5] = {72.f, 76.f, 79.f, 84.f, 88.f};
		for (int I = 0; I < 5; ++I) Buf.Note(0.09f * I, I == 4 ? 0.6f : 0.2f, Ns[I], EWave::Triangle, 0.22f, I == 4 ? 3.f : 4.f);
		Buf.Note(0.36f, 0.6f, 60.f, EWave::Sine, 0.18f, 3.f);
		Buf.Echo(0.12f, 0.3f, 3);
		break;
	}
	case EPDSound::GameOver:
	{
		const float Ns[4] = {67.f, 63.f, 60.f, 55.f};
		for (int I = 0; I < 4; ++I) Buf.Note(0.18f * I, I == 3 ? 0.9f : 0.24f, Ns[I], EWave::Square, 0.14f, I == 3 ? 2.5f : 3.f);
		Buf.Note(0.54f, 0.9f, 43.f, EWave::Sine, 0.25f, 2.5f);
		Buf.Echo(0.15f, 0.25f, 3);
		break;
	}
	case EPDSound::Win:
	{
		const float Ns[4] = {72.f, 76.f, 79.f, 84.f};
		for (int I = 0; I < 4; ++I) Buf.Note(0.11f * I, 0.22f, Ns[I], EWave::Triangle, 0.22f);
		const float Chord[4] = {84.f, 88.f, 91.f, 96.f};
		for (float M : Chord) Buf.Note(0.46f, 1.0f, M, EWave::Triangle, 0.13f, 2.f);
		Buf.Note(0.46f, 1.0f, 60.f, EWave::Sine, 0.22f, 2.f);
		Buf.Echo(0.14f, 0.3f, 4);
		break;
	}
	case EPDSound::Start:
		Buf.Tone(0.f, 0.26f, 300.f, 900.f, EWave::Sine, 0.18f, 0.01f, 1.5f);
		Buf.Tone(0.f, 0.26f, 600.f, 1800.f, EWave::Triangle, 0.06f, 0.01f, 2.f);
		Buf.Echo(0.1f, 0.25f, 2);
		break;
	// 발사·타격 (D-155): 아주 짧고 작게 (자주 나서 귀가 피곤하지 않게). 무늬마다 공격 방식에 맞는 소리
	case EPDSound::Shot:
	{
		const float K = B ? 1.15f : 1.f;
		switch (A)
		{
		case 0: // ♠ 저격 레이저: 높은 곳에서 빠르게 떨어지는 '퓻'
			Buf.Tone(0.f, 0.07f, 2600.f, 700.f, EWave::Saw, 0.07f * K, 0.001f, 5.f);
			Buf.Tone(0.f, 0.05f, 1300.f, 500.f, EWave::Sine, 0.06f * K, 0.001f, 4.f);
			break;
		case 1: // ♥ 폭발 미사일: 낮은 '슉' (잡음 + 낮게 미끄러지는 음)
			Buf.Noise(0.f, 0.12f, 0.10f * K, 0.18f, 21);
			Buf.Tone(0.f, 0.12f, 260.f, 170.f, EWave::Triangle, 0.07f * K, 0.004f, 3.f);
			break;
		case 2: // ♦ 연사 탄환: 짧은 '틱'
			Buf.Tone(0.f, 0.035f, 1700.f, 900.f, EWave::Square, 0.05f * K, 0.0005f, 7.f);
			Buf.Noise(0.f, 0.02f, 0.06f * K, 0.6f, 22);
			break;
		case 3: // ♣ 서리 구체: 올라가는 맑은 '띵'
			Buf.Tone(0.f, 0.09f, 1800.f, 2300.f, EWave::Sine, 0.05f * K, 0.002f, 4.f);
			Buf.Tone(0.02f, 0.08f, 2800.f, 2800.f, EWave::Sine, 0.03f * K, 0.002f, 5.f);
			break;
		default: // 조커·와일드 별: 두 음이 반짝
			Buf.Tone(0.f, 0.08f, 1200.f, 1800.f, EWave::Triangle, 0.06f * K, 0.002f, 4.f);
			Buf.Tone(0.03f, 0.08f, 1800.f, 2400.f, EWave::Sine, 0.04f * K, 0.002f, 4.f);
			break;
		}
		break;
	}
	case EPDSound::Hit:
		Gain = 2.6f; // 타격이 너무 작다 (D-156): 전체를 키운다 (부드러운 클리핑이라 찢어지지 않는다)
		switch (A)
		{
		case 0: // ♠ 지직
			Buf.Tone(0.f, 0.045f, 1500.f, 600.f, EWave::Triangle, 0.07f, 0.001f, 5.f);
			Buf.Noise(0.f, 0.03f, 0.07f, 0.7f, 31);
			break;
		case 1: // ♥ 쿵 (작은 폭발)
			Buf.Tone(0.f, 0.15f, 150.f, 45.f, EWave::Sine, 0.22f, 0.002f, 3.f);
			Buf.Noise(0.f, 0.12f, 0.16f, 0.12f, 32);
			break;
		case 2: // ♦ 탁
			Buf.Tone(0.f, 0.035f, 900.f, 450.f, EWave::Square, 0.05f, 0.0005f, 7.f);
			Buf.Noise(0.f, 0.025f, 0.08f, 0.35f, 33);
			break;
		case 3: // ♣ 얼음 깨지는 소리
			Buf.Noise(0.f, 0.06f, 0.08f, 0.85f, 34);
			Buf.Tone(0.f, 0.07f, 2600.f, 1800.f, EWave::Sine, 0.04f, 0.001f, 5.f);
			break;
		default:
			Buf.Tone(0.f, 0.06f, 700.f, 1000.f, EWave::Triangle, 0.06f, 0.001f, 4.f);
			Buf.Noise(0.f, 0.04f, 0.06f, 0.4f, 35);
			break;
		}
		if (B) Buf.Tone(0.f, 0.1f, 1800.f, 2700.f, EWave::Triangle, 0.08f, 0.001f, 3.f); // 치명타: 높은 '핑'
		break;
	case EPDSound::Zap: // 전기 (D-156): 떨리는 낮은 웅 + 불규칙한 지지직 + 높은 치익
	{
		const bool bShot = A == 0;
		const float Len = bShot ? 0.2f : 0.1f;
		Gain = 1.8f;
		Buf.Tone(0.f, Len, 130.f, 90.f, EWave::Square, bShot ? 0.10f : 0.07f, 0.001f, 2.f, 46.f);
		Buf.Tone(0.f, Len * 0.8f, 3400.f, 2200.f, EWave::Saw, 0.035f, 0.001f, 3.f);
		uint32_t X = bShot ? 0x9e3779b9u : 0x7f4a7c15u;
		const int N = bShot ? 9 : 5;
		for (int K = 0; K < N; ++K)
		{
			X ^= X << 13; X ^= X >> 17; X ^= X << 5;
			const float T = (float)(X % 1000) / 1000.f * Len * 0.85f;
			Buf.Noise(T, 0.008f + 0.008f * (float)((X >> 10) % 3), 0.32f, 0.92f, 40 + K);
		}
		break;
	}
	default:
		break;
	}
	return Buf.ToPcm(Gain);
}

// ---------------------------------------------------------------- 배경 음악 (D-117)
// 80년대 신스웨이브: 네 박자마다 킥, 2·4박 게이트 리버브 스네어, 8분 하이햇, 8분으로 뛰는 톱니 베이스(킥에 눌림),
// 디튠 톱니 패드, 16분 아르페지오(메아리), 마지막 4마디 리드 멜로디. 소리 파일 없이 합성
namespace
{
struct FMusic
{
	int Rate;
	std::vector<float> Bus[3]; // 0 드럼(눌림 없음), 1 눌리는 소리(베이스·패드), 2 메아리 보낼 소리(아르페지오·리드)
	uint32_t Rng = 0x1234567u;
	explicit FMusic(int InRate, size_t N) : Rate(InRate) { for (auto& B : Bus) B.assign(N, 0.f); }
	float Rand() { Rng ^= Rng << 13; Rng ^= Rng >> 17; Rng ^= Rng << 5; return (float)(Rng & 0xFFFF) / 32768.f - 1.f; }
	static float Saw(float P) { return 2.f * (P - std::floor(P)) - 1.f; }
	static float Pulse(float P, float W) { return (P - std::floor(P)) < W ? 1.f : -1.f; }
	void Add(int B, size_t I, float V) { if (I < Bus[B].size()) Bus[B][I] += V; }

	void Kick(float T0, float Amp = 1.f)
	{
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)(0.42f * Rate);
		float Ph = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			const float F = 45.f + 105.f * std::exp(-T / 0.035f);
			Ph += F / Rate;
			const float Env = std::exp(-T / 0.16f);
			const float Click = T < 0.004f ? (1.f - T / 0.004f) * 0.4f : 0.f;
			Add(0, I0 + I, (std::sin(SynthTwoPi * Ph) * Env + Click) * 0.9f * Amp);
		}
	}
	void Snare(float T0, float Amp = 1.f)
	{
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)(0.34f * Rate);
		float Lp = 0.f, Prev = 0.f, Ph = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			const float R = Rand();
			Lp += (R - Lp) * 0.6f;
			const float Hp = Lp - Prev; // 높은 소리만
			Prev = Lp;
			// 게이트 리버브: 몸통이 길게 이어지다 0.26초에 뚝 끊긴다
			const float Body = T < 0.26f ? 0.55f * std::exp(-T / 0.09f) + 0.22f : 0.22f * std::max(0.f, 1.f - (T - 0.26f) / 0.02f);
			Ph += 185.f / Rate;
			const float Tone = std::sin(SynthTwoPi * Ph) * std::exp(-T / 0.05f) * 0.35f;
			Add(0, I0 + I, (Hp * 1.6f * Body + Tone) * 0.55f * Amp);
		}
	}
	void Tom(float T0, float F0, float Amp) // 탐 (D-188): 음높이가 살짝 내려가는 사인 + 짧은 잡음
	{
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)(0.3f * Rate);
		float Ph = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			Ph += F0 * (1.f + 0.5f * std::exp(-T / 0.04f)) / Rate;
			Add(0, I0 + I, (std::sin(SynthTwoPi * Ph) + 0.15f * Rand() * std::exp(-T / 0.01f)) * std::exp(-T / 0.12f) * Amp);
		}
	}
	void Hat(float T0, float Len, float Amp)
	{
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)(Len * Rate);
		float Prev = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			const float R = Rand();
			const float Hp = R - Prev;
			Prev = R * 0.9f;
			Add(0, I0 + I, Hp * std::exp(-T / (Len * 0.3f)) * Amp);
		}
	}
	// 톱니 소리 하나: 디튠 두 겹 + 1극 저역 통과(엔벨로프로 열림), 어택/릴리스
	void SawNote(int B, float T0, float Dur, float Midi, float Amp, float Cut0, float Cut1, float CutDecay, float Attack, float Release, float Detune, float Vib = 0.f)
	{
		const float F = MidiHz(Midi);
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)((Dur + Release) * Rate);
		float P1 = Rand() * 0.5f + 0.5f, P2 = Rand() * 0.5f + 0.5f, Lp = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			const float V = Vib > 0.f && T > 0.2f ? 1.f + Vib * std::sin(SynthTwoPi * 5.5f * T) : 1.f;
			P1 += F * V * (1.f + Detune) / Rate;
			P2 += F * V * (1.f - Detune) / Rate;
			const float X = 0.5f * (Saw(P1) + Saw(P2));
			const float Cut = Cut1 + (Cut0 - Cut1) * std::exp(-T / CutDecay);
			const float A = 1.f - std::exp(-SynthTwoPi * Cut / Rate);
			Lp += (X - Lp) * A;
			float Env = T < Attack ? T / Attack : 1.f;
			if (T > Dur) Env *= std::max(0.f, 1.f - (T - Dur) / Release);
			Add(B, I0 + I, Lp * Env * Amp);
		}
	}
	void PulseNote(int B, float T0, float Dur, float Midi, float Amp)
	{
		const float F = MidiHz(Midi);
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)(Dur * Rate);
		float P = 0.f, Lp = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			P += F / Rate;
			Lp += (Pulse(P, 0.25f) - Lp) * 0.35f;
			const float Env = std::exp(-T / (Dur * 0.45f)) * std::min(1.f, (Dur - T) / 0.005f);
			Add(B, I0 + I, Lp * Env * Amp);
		}
	}
	// ---------------------------------------------------------------- 재즈 악기 (D-264): 칵테일바 재즈 + 신스 혼합
	// 로즈 전자피아노: FM (1:1 변조 = 따뜻한 몸통, 세기가 금방 줄어듦) + 14배 쇳소리(톡) + 느린 트레몰로
	void Rhodes(int B, float T0, float Dur, float Midi, float Amp)
	{
		const float F = MidiHz(Midi), Rel = 0.3f;
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)((Dur + Rel) * Rate);
		float Pc = 0.f, Pt = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			Pc += F / Rate;
			Pt += F * 14.f / Rate;
			const float Index = 1.5f * std::exp(-T / 0.22f) + 0.22f;
			const float V = std::sin(SynthTwoPi * Pc + Index * std::sin(SynthTwoPi * Pc)) * std::exp(-T / 1.8f);
			const float Tine = std::sin(SynthTwoPi * Pt) * 0.16f * std::exp(-T / 0.025f);
			float Env = T < 0.002f ? T / 0.002f : 1.f;
			if (T > Dur) Env *= std::max(0.f, 1.f - (T - Dur) / Rel);
			const float Trem = 1.f + 0.12f * std::sin(SynthTwoPi * 4.5f * T);
			Add(B, I0 + I, (V + Tine) * Env * Trem * Amp);
		}
	}
	// 업라이트 베이스: 사인 + 약한 배음, 손가락 튕김(짧은 잡음), 금방 줄어드는 울림
	void Upright(int B, float T0, float Dur, float Midi, float Amp)
	{
		const float F = MidiHz(Midi), Rel = 0.06f;
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)((Dur + Rel) * Rate);
		float P = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			P += F / Rate;
			const float V = std::sin(SynthTwoPi * P) + 0.35f * std::sin(2.f * SynthTwoPi * P) * std::exp(-T / 0.15f) + 0.1f * std::sin(3.f * SynthTwoPi * P) * std::exp(-T / 0.06f);
			float Env = T < 0.004f ? T / 0.004f : std::exp(-T / 0.6f);
			if (T > Dur) Env *= std::max(0.f, 1.f - (T - Dur) / Rel);
			Add(B, I0 + I, (V * Env * 0.8f + Rand() * std::exp(-T / 0.006f) * 0.2f) * Amp);
		}
	}
	// 비브라폰: 사인 기음 + 4배 · 10배 부분음(금방 사라짐), 모터 떨림, 긴 울림 (Dur 뒤에는 막아서 줄임)
	void Vibes(int B, float T0, float Dur, float Midi, float Amp)
	{
		const float F = MidiHz(Midi), Rel = 0.35f;
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)((Dur + Rel) * Rate);
		float P = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			P += F / Rate;
			const float V = std::sin(SynthTwoPi * P) + 0.22f * std::sin(4.f * SynthTwoPi * P) * std::exp(-T / 0.35f) + 0.07f * std::sin(10.f * SynthTwoPi * P) * std::exp(-T / 0.04f);
			float Env = (T < 0.0015f ? T / 0.0015f : 1.f) * std::exp(-T / 1.5f);
			if (T > Dur) Env *= std::exp(-(T - Dur) / (Rel * 0.4f));
			const float Trem = 0.78f + 0.22f * std::sin(SynthTwoPi * 5.5f * T);
			Add(B, I0 + I, V * Env * Trem * Amp);
		}
	}
	// 라이드 심벌: 비배음 사각파 여섯 개 + 고역 통과(금속성) + 짧은 '땡'
	void Ride(float T0, float Amp, float Len = 0.9f)
	{
		static const float Fs[6] = {3150.f, 3810.f, 4470.f, 5230.f, 6120.f, 7350.f};
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)(Len * Rate);
		float Ph[6] = {0.1f, 0.3f, 0.5f, 0.7f, 0.2f, 0.9f}, Prev = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			float X = 0.f;
			for (int K = 0; K < 6; ++K) { Ph[K] += Fs[K] / Rate; X += Pulse(Ph[K], 0.5f); }
			X = X / 6.f + Rand() * 0.25f;
			const float Hp = X - Prev;
			Prev = X * 0.9f;
			const float Ping = std::sin(SynthTwoPi * 5200.f * T) * std::exp(-T / 0.06f) * 0.25f;
			Add(0, I0 + I, (Hp * std::exp(-T / (Len * 0.33f)) + Ping) * Amp);
		}
	}
	// 브러시: 걸러진 잡음. bTap = 짧게 탁, 아니면 마디 길이로 천천히 쓸기
	void Brush(float T0, float Len, float Amp, bool bTap)
	{
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)(Len * Rate);
		float Lp = 0.f, Prev = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			const float R = Rand();
			Lp += (R - Lp) * 0.35f;
			const float Hp = Lp - Prev * 0.7f;
			Prev = Lp;
			const float Env = bTap ? (T < 0.002f ? T / 0.002f : std::exp(-T / 0.07f)) : std::sin(3.1415926f * T / Len) * (0.7f + 0.3f * std::sin(SynthTwoPi * 2.f * T / Len));
			Add(0, I0 + I, Hp * Env * Amp);
		}
	}
	// 신스 금관 찌르기: 디튠 톱니 세 겹, 필터가 확 열렸다 닫힘, 끝에서 음이 살짝 떨어짐(폴)
	void Brass(int B, float T0, float Dur, float Midi, float Amp, bool bFall = false)
	{
		const float F = MidiHz(Midi), Rel = 0.06f;
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)((Dur + Rel) * Rate);
		float P[3] = {Rand() * 0.5f + 0.5f, Rand() * 0.5f + 0.5f, Rand() * 0.5f + 0.5f}, L1 = 0.f, L2 = 0.f;
		static const float Dt[3] = {-0.007f, 0.f, 0.006f};
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			float Fk = 1.f;
			if (bFall && T > Dur * 0.55f) { const float U = (T - Dur * 0.55f) / (Dur * 0.45f + Rel); Fk = 1.f - 0.08f * U * U; }
			float X = 0.f;
			for (int K = 0; K < 3; ++K) { P[K] += F * Fk * (1.f + Dt[K]) / Rate; X += Saw(P[K]); }
			X /= 3.f;
			const float Swell = std::min(1.f, T / 0.025f);
			const float Cut = 350.f + 4200.f * Swell * std::exp(-T / 0.12f);
			const float A = 1.f - std::exp(-SynthTwoPi * Cut / Rate);
			L1 += (X - L1) * A;
			L2 += (L1 - L2) * A;
			float Env = Swell;
			if (T > Dur) Env *= std::max(0.f, 1.f - (T - Dur) / Rel);
			Add(B, I0 + I, L2 * Env * Amp);
		}
	}
	// 약음 트럼펫 느낌 리드: 펄스 + 톱니를 두 번 거른 소리, 숨처럼 부푸는 어택, 늦은 비브라토
	void MuteLead(int B, float T0, float Dur, float Midi, float Amp)
	{
		const float F = MidiHz(Midi), Rel = 0.09f;
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)((Dur + Rel) * Rate);
		float P = 0.f, L1 = 0.f, L2 = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			const float Vib = T > 0.22f ? 1.f + 0.007f * std::sin(SynthTwoPi * 5.2f * T) * std::min(1.f, (T - 0.22f) / 0.2f) : 1.f;
			P += F * Vib / Rate;
			const float X = 0.6f * Pulse(P, 0.3f) + 0.4f * Saw(P) + 0.06f * Rand() * std::exp(-T / 0.05f);
			const float Swell = std::min(1.f, T / 0.04f);
			const float Cut = 700.f + 1500.f * Swell;
			const float A = 1.f - std::exp(-SynthTwoPi * Cut / Rate);
			L1 += (X - L1) * A;
			L2 += (L1 - L2) * A;
			float Env = Swell;
			if (T > Dur) Env *= std::max(0.f, 1.f - (T - Dur) / Rel);
			Add(B, I0 + I, L2 * Env * Amp);
		}
	}
	// 레코드판 지글 (메뉴): 가끔 톡 튀는 소리 + 아주 작은 쉬익
	void Crackle(size_t N, float Amp)
	{
		float Lp = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float R = Rand();
			Lp += (R - Lp) * 0.08f;
			float V = Lp * 0.25f;
			if (std::fabs(Rand()) > 0.9996f) V += Rand() * 2.5f;
			Add(0, I, V * Amp);
		}
	}
};
} // namespace

// 섞기 (D-264): 킥 눌림(사이드체인) + 점 8분 메아리 + 작은 방 잔향 + 끝에서 넘친 꼬리를 처음에 더해 이음매 없이 반복
struct FMixOpt { float Duck = 0.65f; float EchoFb = 0.35f; float EchoMix = 0.6f; float Room = 0.f; float Gain = 0.9f; };
static std::vector<int16_t> MixDown(FMusic& M, const std::vector<float>& Kicks, float Beat, size_t LoopN, size_t TailN, int SampleRate, const FMixOpt& O)
{
	std::vector<float>& Duck = M.Bus[1];
	size_t KI = 0;
	for (size_t I = 0; I < Duck.size(); ++I)
	{
		const float T = (float)I / SampleRate;
		while (KI + 1 < Kicks.size() && Kicks[KI + 1] <= T) ++KI;
		const float Since = Kicks.empty() || Kicks[KI] > T ? 1.f : T - Kicks[KI];
		Duck[I] *= 1.f - O.Duck * std::exp(-Since / 0.11f);
	}
	std::vector<float>& Send = M.Bus[2];
	const size_t D = (size_t)(Beat * 0.75f * SampleRate);
	std::vector<float> Wet(Send.size(), 0.f);
	for (size_t I = D; I < Send.size(); ++I) Wet[I] = (Send[I - D] + Wet[I - D]) * O.EchoFb;
	// 작은 방 잔향 (슈뢰더: 빗살 넷 + 전역 통과 둘)
	std::vector<float> Rev(Send.size(), 0.f);
	if (O.Room > 0.f)
	{
		std::vector<float> In(Send.size());
		for (size_t I = 0; I < In.size(); ++I) In[I] = Send[I] + 0.5f * M.Bus[1][I] + 0.25f * M.Bus[0][I];
		static const float Combs[4] = {0.0297f, 0.0371f, 0.0411f, 0.0437f};
		for (float Cd : Combs)
		{
			const size_t CD = (size_t)(Cd * SampleRate);
			std::vector<float> Y(In.size(), 0.f);
			float Lp = 0.f;
			for (size_t I = 0; I < In.size(); ++I)
			{
				const float Back = I >= CD ? Y[I - CD] : 0.f;
				Lp += (Back - Lp) * 0.45f;
				Y[I] = In[I] + Lp * 0.8f;
				Rev[I] += Y[I] * 0.25f;
			}
		}
		for (const float Ad : {0.005f, 0.0017f})
		{
			const size_t AD = (size_t)(Ad * SampleRate);
			std::vector<float> X = Rev;
			for (size_t I = 0; I < Rev.size(); ++I) Rev[I] = -0.7f * X[I] + (I >= AD ? X[I - AD] + 0.7f * Rev[I - AD] : 0.f);
		}
	}
	std::vector<float> Mix(LoopN, 0.f);
	for (size_t I = 0; I < LoopN + TailN; ++I)
	{
		const float V = M.Bus[0][I] * 0.8f + M.Bus[1][I] + Send[I] + Wet[I] * O.EchoMix + Rev[I] * O.Room;
		Mix[I % LoopN] += V;
	}
	std::vector<int16_t> Out(LoopN);
	for (size_t I = 0; I < LoopN; ++I) Out[I] = (int16_t)std::lround(std::tanh(Mix[I] * O.Gain) * 28000.f);
	return Out;
}

// 코드 한 마디: 근음(베이스), 3도·7도 간격, 손 화음 네 음
struct FJazzChord { float Root; float Third; float Seventh; float Voice[4]; };
// 워킹 베이스 한 마디 (4분 넷): 근음 → 3도 → 5도 → 다음 근음 반음 위/아래 (Alt 로 번갈아)
static void WalkBar(FMusic& M, float T, float Beat, const FJazzChord& C, float NextRoot, bool Alt, float Amp, bool bSynth)
{
	float Nr = NextRoot;
	while (Nr - C.Root > 6.f) Nr -= 12.f;
	while (C.Root - Nr > 6.f) Nr += 12.f;
	const float Notes[4] = {C.Root, C.Root + C.Third, C.Root + 7.f, Nr + (Alt ? 1.f : -1.f)};
	for (int K = 0; K < 4; ++K)
	{
		M.Upright(1, T + K * Beat, Beat * 0.88f, Notes[K], Amp);
		if (bSynth) M.SawNote(1, T + K * Beat, Beat * 0.6f, Notes[K], Amp * 0.35f, 700.f, 220.f, 0.05f, 0.003f, 0.03f, 0.003f);
	}
}

struct FLead { int Bar; float At, Len, Midi; };

// ---------------------------------------------------------------- 1. 메뉴: 칵테일바 라운지 스윙 (D-264)
// 88 BPM 스윙, F 장조 16마디 (I-vi-ii-V 돌고 IV 로 갔다가 돌아옴). 로즈 찰스턴 반주, 워킹 업라이트 베이스, 라이드 · 브러시,
// 5~12마디 비브라폰 선율, 13~16마디 약음 트럼펫 느낌 리드, 레코드판 지글, 작은 방 잔향. 신스는 아주 옅은 패드만
std::vector<int16_t> SynthesizeMenuMusic(int SampleRate)
{
	const float Bpm = 88.f, Beat = 60.f / Bpm, Bar = 4.f * Beat, Sw = 2.f / 3.f; // 스윙 8분: 뒷박이 2/3 자리
	const int Bars = 16;
	const size_t LoopN = (size_t)(Bars * Bar * SampleRate), TailN = (size_t)(2.5f * SampleRate);
	FMusic M(SampleRate, LoopN + TailN);
	M.Rng = 0x51ed27u;
	const FJazzChord Ch[16] = {
		{41.f, 4.f, 11.f, {57.f, 60.f, 64.f, 67.f}}, {38.f, 3.f, 10.f, {53.f, 57.f, 60.f, 64.f}}, {43.f, 3.f, 10.f, {53.f, 57.f, 58.f, 62.f}}, {36.f, 4.f, 10.f, {52.f, 57.f, 58.f, 62.f}},
		{45.f, 3.f, 10.f, {55.f, 60.f, 64.f, 67.f}}, {38.f, 4.f, 10.f, {54.f, 59.f, 60.f, 64.f}}, {43.f, 3.f, 10.f, {53.f, 57.f, 58.f, 62.f}}, {36.f, 4.f, 10.f, {52.f, 57.f, 58.f, 62.f}},
		{41.f, 4.f, 11.f, {57.f, 60.f, 64.f, 67.f}}, {41.f, 4.f, 10.f, {57.f, 60.f, 63.f, 67.f}}, {46.f, 4.f, 11.f, {57.f, 60.f, 62.f, 65.f}}, {46.f, 3.f, 9.f, {53.f, 55.f, 58.f, 61.f}},
		{45.f, 3.f, 10.f, {55.f, 60.f, 64.f, 67.f}}, {38.f, 4.f, 10.f, {54.f, 60.f, 63.f, 66.f}}, {43.f, 3.f, 10.f, {53.f, 57.f, 58.f, 62.f}}, {36.f, 4.f, 10.f, {52.f, 58.f, 61.f, 64.f}}};
	for (int B = 0; B < Bars; ++B)
	{
		const float T = B * Bar;
		const FJazzChord& C = Ch[B];
		// 드럼: 라이드 '딩 딩-다 딩 딩-다', 2·4박 발 하이햇, 마디 내내 브러시 쓸기 + 2·4박 탁, 1·3박 아주 작은 킥
		for (const float At : {0.f, 1.f, 1.f + Sw, 2.f, 3.f, 3.f + Sw}) M.Ride(T + At * Beat, At == 1.f + Sw || At == 3.f + Sw ? 0.06f : 0.1f);
		for (const float At : {1.f, 3.f}) { M.Hat(T + At * Beat, 0.03f, 0.07f); M.Brush(T + At * Beat, 0.2f, 0.16f, true); }
		M.Brush(T, Bar, 0.06f, false);
		for (const float At : {0.f, 2.f}) M.Kick(T + At * Beat, 0.25f);
		// 베이스: 워킹
		WalkBar(M, T, Beat, C, Ch[(B + 1) % Bars].Root, B % 2 == 1, 0.42f, false);
		// 로즈: 찰스턴 (1박 짧게 + 2박 뒷박 길게), 홀수 마디는 2박 · 3박 뒷박
		const bool bOdd = B % 2 == 1;
		const float Hits[2][2] = {{0.f, 0.6f}, {1.f + Sw, 1.2f}}, HitsO[2][2] = {{1.f, 0.5f}, {2.f + Sw, 1.1f}};
		for (int H = 0; H < 2; ++H)
		{
			const float At = bOdd ? HitsO[H][0] : Hits[H][0], Len = bOdd ? HitsO[H][1] : Hits[H][1];
			for (float N : C.Voice) M.Rhodes(1, T + At * Beat, Len * Beat, N, 0.07f);
		}
		// 옅은 신스 패드 (혼합 맛)
		for (int K = 0; K < 3; ++K) M.SawNote(1, T, Bar * 0.98f, C.Voice[K + 1] - 12.f, 0.018f, 300.f, 700.f, 1.f, 0.4f, 0.4f, 0.006f);
	}
	// 비브라폰 (5~12마디)
	const FLead Vib[] = {
		{4, 0.f, 1.f, 72.f}, {4, 1.f, Sw, 76.f}, {4, 1.f + Sw, 1.f + 1.f / 3.f, 79.f}, {4, 3.f, 1.f, 76.f},
		{5, 0.f, 1.5f, 78.f}, {5, 1.f + Sw, 1.f / 3.f, 76.f}, {5, 2.f, 2.f, 74.f},
		{6, 0.f, Sw, 74.f}, {6, Sw, 1.f / 3.f, 77.f}, {6, 1.f, 1.f, 81.f}, {6, 2.f, 1.f, 79.f}, {6, 3.f, 1.f, 77.f},
		{7, 0.f, 2.f, 76.f}, {7, 2.f + Sw, 1.f / 3.f, 74.f}, {7, 3.f, 1.f, 72.f},
		{8, 0.f, 1.f, 69.f}, {8, 1.f, Sw, 72.f}, {8, 1.f + Sw, 1.f / 3.f, 76.f}, {8, 2.f, 2.f, 79.f},
		{9, 0.f, 1.f, 81.f}, {9, 1.f, 1.f, 79.f}, {9, 2.f, 1.f, 75.f}, {9, 3.f, 1.f, 72.f},
		{10, 0.f, 3.f, 74.f}, {10, 3.f, 1.f, 77.f},
		{11, 0.f, 2.f, 73.f}, {11, 2.f, 1.f, 72.f}, {11, 3.f, 1.f, 70.f}};
	for (const FLead& L : Vib) M.Vibes(2, L.Bar * Bar + L.At * Beat, L.Len * Beat, L.Midi, 0.16f);
	// 약음 리드 (13~16마디)
	const FLead Mute[] = {
		{12, 0.f, 1.f, 76.f}, {12, 1.f, 1.f, 72.f}, {12, 2.f, Sw, 74.f}, {12, 2.f + Sw, 1.f / 3.f, 76.f}, {12, 3.f, 1.f, 79.f},
		{13, 0.f, 2.f, 78.f}, {13, 2.f, 1.f, 75.f}, {13, 3.f, 1.f, 72.f},
		{14, 0.f, 1.f, 70.f}, {14, 1.f, 1.f, 74.f}, {14, 2.f, 1.f, 77.f}, {14, 3.f, 1.f, 81.f},
		{15, 0.f, 2.f, 79.f}, {15, 2.f, 1.f, 76.f}, {15, 3.f, 1.f, 73.f}};
	for (const FLead& L : Mute) M.MuteLead(2, L.Bar * Bar + L.At * Beat, L.Len * Beat * 0.92f, L.Midi, 0.13f);
	M.Crackle(LoopN + TailN, 0.05f);
	FMixOpt O;
	O.Duck = 0.f; O.EchoFb = 0.2f; O.EchoMix = 0.25f; O.Room = 0.35f; O.Gain = 1.1f;
	return MixDown(M, {}, Beat, LoopN, TailN, SampleRate, O);
}

// ---------------------------------------------------------------- 2. 인게임: 재즈 펑크 + 신스 (D-264)
// 104 BPM, A 단조 Am9 - D9 - Fmaj7 - E7#9 16마디. 16분 하이햇(살짝 스윙), 킥 1 · 2박 뒷박 · 3박 뒷박, 2·4박 스네어 + 고스트,
// 로즈 당김 반주, 신스 베이스 + 업라이트 겹친 펑크 베이스, 옅은 신스 패드, 9~16마디 펄스 아르페지오, 9~12 비브라폰, 13~16 약음 리드
std::vector<int16_t> SynthesizeMusic(int SampleRate)
{
	const float Bpm = 104.f, Beat = 60.f / Bpm, Bar = 4.f * Beat, S16 = Beat / 4.f;
	auto Sw16 = [&](int K) { return K * S16 + (K % 2 ? S16 * 0.16f : 0.f); }; // 16분 뒷박을 조금 늦게 (스윙 감)
	const int Bars = 16;
	const size_t LoopN = (size_t)(Bars * Bar * SampleRate), TailN = (size_t)(2.f * SampleRate);
	FMusic M(SampleRate, LoopN + TailN);
	const FJazzChord Ch[4] = {{45.f, 3.f, 10.f, {60.f, 64.f, 67.f, 71.f}}, {38.f, 4.f, 10.f, {54.f, 60.f, 64.f, 69.f}}, {41.f, 4.f, 11.f, {57.f, 60.f, 64.f, 67.f}}, {40.f, 4.f, 10.f, {56.f, 62.f, 67.f, 71.f}}};
	std::vector<float> Kicks;
	for (int B = 0; B < Bars; ++B)
	{
		const float T = B * Bar;
		const FJazzChord& C = Ch[B % 4];
		const bool bHalf2 = B >= 8;
		for (const int K : {0, 6, 10}) { M.Kick(T + Sw16(K), 0.85f); Kicks.push_back(T + Sw16(K)); }
		for (const int K : {4, 12}) M.Snare(T + Sw16(K), 0.8f);
		for (const int K : {7, 15}) M.Brush(T + Sw16(K), 0.12f, 0.12f, true); // 고스트
		for (int K = 0; K < 16; ++K) M.Hat(T + Sw16(K), 0.03f, K % 2 ? 0.05f : 0.1f);
		if (B % 4 == 3) M.Hat(T + Sw16(14), 0.2f, 0.11f);
		// 펑크 베이스 (16분 자리): 근음 · 근음 · 옥타브 · 근음 · 7도 · 옥타브 · 다음 근음 반음 아래
		float Nr = Ch[(B + 1) % 4].Root;
		while (Nr - C.Root > 6.f) Nr -= 12.f;
		while (C.Root - Nr > 6.f) Nr += 12.f;
		const int BPos[7] = {0, 3, 6, 8, 10, 11, 14};
		const float BNote[7] = {C.Root, C.Root, C.Root + 12.f, C.Root, C.Root + C.Seventh, C.Root + 12.f, Nr - 1.f};
		for (int K = 0; K < 7; ++K)
		{
			const float Len = (K == 3 ? 2.f : 1.5f) * S16;
			M.SawNote(1, T + Sw16(BPos[K]), Len, BNote[K], 0.3f, 1100.f, 240.f, 0.05f, 0.002f, 0.02f, 0.004f);
			M.Upright(1, T + Sw16(BPos[K]), Len, BNote[K], 0.25f);
		}
		// 로즈 당김 반주: 16분 3 · 6 · 11 자리 (짝수 마디는 2 · 7 · 12)
		const int RPos[2][3] = {{3, 6, 11}, {2, 7, 12}};
		for (int K = 0; K < 3; ++K) for (float N : C.Voice) M.Rhodes(1, T + Sw16(RPos[B % 2][K]), S16 * (K == 2 ? 3.f : 1.6f), N, 0.055f);
		// 옅은 신스 패드
		for (float N : C.Voice) M.SawNote(1, T, Bar * 0.98f, N - 12.f, 0.03f, 400.f, 1200.f, 0.8f, 0.3f, 0.35f, 0.006f);
		// 펄스 아르페지오 (9~16마디, 신스 맛)
		if (bHalf2)
			for (int K = 0; K < 16; ++K) M.PulseNote(2, T + Sw16(K), S16 * 0.8f, C.Voice[(K * 3) % 4] + 12.f, 0.03f);
	}
	const FLead Vib[] = {
		{8, 0.f, 0.75f, 76.f}, {8, 0.75f, 0.25f, 79.f}, {8, 1.f, 1.f, 81.f}, {8, 2.5f, 0.5f, 79.f}, {8, 3.f, 1.f, 76.f},
		{9, 0.f, 0.5f, 74.f}, {9, 0.5f, 0.5f, 76.f}, {9, 1.f, 1.5f, 78.f}, {9, 2.75f, 0.25f, 76.f}, {9, 3.f, 1.f, 74.f},
		{10, 0.f, 1.f, 72.f}, {10, 1.f, 0.5f, 76.f}, {10, 1.5f, 0.5f, 79.f}, {10, 2.f, 2.f, 81.f},
		{11, 0.f, 1.f, 80.f}, {11, 1.f, 1.f, 79.f}, {11, 2.f, 1.f, 76.f}, {11, 3.f, 1.f, 74.f}};
	for (const FLead& L : Vib) M.Vibes(2, L.Bar * Bar + L.At * Beat, L.Len * Beat, L.Midi, 0.13f);
	const FLead Mute[] = {
		{12, 0.f, 1.5f, 69.f}, {12, 1.5f, 0.5f, 72.f}, {12, 2.f, 1.f, 76.f}, {12, 3.f, 0.5f, 74.f}, {12, 3.5f, 0.5f, 72.f},
		{13, 0.f, 2.f, 74.f}, {13, 2.f, 1.f, 78.f}, {13, 3.f, 1.f, 81.f},
		{14, 0.f, 1.f, 79.f}, {14, 1.f, 1.f, 76.f}, {14, 2.f, 1.f, 72.f}, {14, 3.f, 1.f, 71.f},
		{15, 0.f, 1.5f, 68.f}, {15, 1.5f, 0.5f, 71.f}, {15, 2.f, 2.f, 76.f}};
	for (const FLead& L : Mute) M.MuteLead(2, L.Bar * Bar + L.At * Beat, L.Len * Beat * 0.92f, L.Midi, 0.12f);
	FMixOpt O;
	O.Duck = 0.35f; O.EchoFb = 0.28f; O.EchoMix = 0.4f; O.Room = 0.18f; O.Gain = 1.f;
	return MixDown(M, Kicks, Beat, LoopN, TailN, SampleRate, O);
}

// ---------------------------------------------------------------- 3. 보스: 신스 + 빅밴드 금관 (D-188 → D-264)
// 140 BPM D 단조 Dm - B♭ - Gm - A7 16마디. 4박 킥 + 마디 끝 두 번, 게이트 스네어, 16분 하이햇 + 4분 라이드, 4마디마다 탐 채움,
// 8분 워킹 베이스(신스 + 업라이트), 금관 찌르기(1박 · 2박 뒷박, 채움 마디는 3 · 3½ · 4박 몰아치기), 반음 경보 동기, 9마디부터 약음 리드
std::vector<int16_t> SynthesizeBossMusic(int SampleRate)
{
	const float Bpm = 140.f, Beat = 60.f / Bpm, Bar = 4.f * Beat, S16 = Beat / 4.f;
	const int Bars = 16;
	const size_t LoopN = (size_t)(Bars * Bar * SampleRate), TailN = (size_t)(2.f * SampleRate);
	FMusic M(SampleRate, LoopN + TailN);
	M.Rng = 0x9e3779b9u;
	const FJazzChord Ch[4] = {{38.f, 3.f, 10.f, {62.f, 65.f, 69.f, 74.f}}, {34.f, 4.f, 11.f, {62.f, 65.f, 70.f, 74.f}}, {43.f, 3.f, 10.f, {62.f, 67.f, 70.f, 74.f}}, {45.f, 4.f, 10.f, {61.f, 64.f, 67.f, 69.f}}};
	std::vector<float> Kicks;
	for (int B = 0; B < Bars; ++B)
	{
		const float T = B * Bar;
		const FJazzChord& C = Ch[B % 4];
		const bool bFill = B % 4 == 3, bLead = B >= 8;
		for (int K = 0; K < 4; ++K) { M.Kick(T + K * Beat); Kicks.push_back(T + K * Beat); }
		if (!bFill) { M.Kick(T + 3.5f * Beat); Kicks.push_back(T + 3.5f * Beat); }
		M.Snare(T + Beat);
		if (!bFill) M.Snare(T + 3.f * Beat);
		for (int K = 0; K < 16; ++K) M.Hat(T + K * S16, 0.03f, K % 4 == 2 ? 0.12f : 0.05f);
		for (int K = 0; K < 4; ++K) M.Ride(T + K * Beat, 0.06f, 0.6f);
		if (bFill) for (int K = 0; K < 8; ++K) M.Tom(T + 2.f * Beat + K * S16, 180.f - K * 14.f, 0.45f);
		// 8분 워킹 베이스: 근음 · 3도 · 5도 · 7도 · 옥타브 · 7도 · 5도 · 다음 근음 반음 위/아래
		float Nr = Ch[(B + 1) % 4].Root;
		while (Nr - C.Root > 6.f) Nr -= 12.f;
		while (C.Root - Nr > 6.f) Nr += 12.f;
		const float Wk[8] = {C.Root, C.Root + C.Third, C.Root + 7.f, C.Root + C.Seventh, C.Root + 12.f, C.Root + C.Seventh, C.Root + 7.f, Nr + (B % 2 ? 1.f : -1.f)};
		for (int K = 0; K < 8; ++K)
		{
			M.SawNote(1, T + K * Beat / 2.f, Beat / 2.f * 0.75f, Wk[K], 0.32f, 1700.f, 300.f, 0.04f, 0.002f, 0.015f, 0.005f);
			M.Upright(1, T + K * Beat / 2.f, Beat / 2.f * 0.8f, Wk[K], 0.22f);
		}
		// 금관 찌르기
		if (!bFill) { for (const float At : {0.f, 1.5f}) for (float N : C.Voice) M.Brass(1, T + At * Beat, Beat * (At == 0.f ? 0.45f : 0.9f), N, 0.06f, At > 0.f); }
		else for (const float At : {0.f, 2.f, 2.5f, 3.f}) for (float N : C.Voice) M.Brass(1, T + At * Beat, Beat * 0.4f, N + (At >= 2.f ? 2.f : 0.f), 0.065f, At == 3.f);
		// 반음 경보 동기
		const float Hi = C.Voice[0] + 12.f;
		for (int K = 0; K < 8; ++K) M.PulseNote(2, T + K * Beat / 2.f, Beat / 2.f * 0.8f, K % 2 ? Hi + 1.f : Hi, bLead ? 0.035f : 0.055f);
	}
	const FLead Lead[] = {
		{8, 0.f, 1.f, 74.f}, {8, 1.f, 1.f, 77.f}, {8, 2.f, 2.f, 76.f},
		{9, 0.f, 1.f, 74.f}, {9, 1.f, 1.f, 70.f}, {9, 2.f, 2.f, 72.f},
		{10, 0.f, 1.f, 70.f}, {10, 1.f, 1.f, 74.f}, {10, 2.f, 1.f, 79.f}, {10, 3.f, 1.f, 77.f},
		{11, 0.f, 2.f, 76.f}, {11, 2.f, 1.f, 73.f}, {11, 3.f, 1.f, 76.f},
		{12, 0.f, 1.5f, 81.f}, {12, 1.5f, 0.5f, 79.f}, {12, 2.f, 2.f, 77.f},
		{13, 0.f, 1.f, 74.f}, {13, 1.f, 1.f, 77.f}, {13, 2.f, 2.f, 82.f},
		{14, 0.f, 1.f, 79.f}, {14, 1.f, 1.f, 77.f}, {14, 2.f, 2.f, 74.f},
		{15, 0.f, 2.f, 73.f}, {15, 2.f, 2.f, 76.f}};
	for (const FLead& L : Lead) M.MuteLead(2, L.Bar * Bar + L.At * Beat, L.Len * Beat * 0.92f, L.Midi, 0.15f);
	FMixOpt O;
	O.Room = 0.12f;
	return MixDown(M, Kicks, Beat, LoopN, TailN, SampleRate, O);
}
} // namespace PD
