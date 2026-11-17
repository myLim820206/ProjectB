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
	case EPDSound::Shake: // 쉐킷쉐킷 (D-306): 초당 6번 오가며, 매번 쇠통 벽에 얼음이 쏟아지는 '샥' 잡음 + 얼음 부딪는 높은 쇳소리 몇 개
	{
		Gain = 1.4f;
		const int Strokes = 8;
		const float Per = 0.155f;
		Buf.Ensure((size_t)((Strokes * Per + 0.25f) * Rate));
		uint32_t X = 0x51ed270bu;
		auto Rnd = [&X]() { X ^= X << 13; X ^= X >> 17; X ^= X << 5; return (float)(X & 0xFFFF) / 65535.f; };
		for (int K = 0; K < Strokes; ++K)
		{
			const float T0 = K * Per, Amp = (K % 2 ? 0.75f : 1.f) * (K == Strokes - 1 ? 0.8f : 1.f);
			// '샥': 밝은 잡음 (낮은 소리를 뺀 잡음), 빠른 어택 + 70ms 감쇠
			const size_t I0 = (size_t)(T0 * Rate), N = (size_t)(0.085f * Rate);
			float Lp = 0.f, Prev = 0.f;
			for (size_t I = 0; I < N && I0 + I < Buf.S.size(); ++I)
			{
				const float R = Rnd() * 2.f - 1.f;
				Lp += (R - Lp) * 0.55f;
				const float Hp = Lp - Prev; Prev = Lp;
				const float U = (float)I / N, Env = std::min(1.f, U * 14.f) * std::pow(1.f - U, 2.2f);
				Buf.S[I0 + I] += Hp * 0.55f * Amp * Env;
			}
			// 얼음 쇳소리: 짧게 울리는 높은 음 둘 (쇠 울림 비 2.76)
			const int Clinks = 3 + (int)(Rnd() * 3.f);
			for (int Q = 0; Q < Clinks; ++Q)
			{
				const float Tc = T0 + 0.005f + Rnd() * 0.06f, F = 2600.f + Rnd() * 2600.f, Ca = (0.05f + Rnd() * 0.05f) * Amp;
				Buf.Tone(Tc, 0.05f, F, F * 0.97f, EWave::Sine, Ca, 0.0005f, 9.f);
				Buf.Tone(Tc, 0.03f, F * 2.76f, F * 2.7f, EWave::Sine, Ca * 0.4f, 0.0005f, 12.f);
			}
			Buf.Tone(T0, 0.04f, 240.f, 160.f, EWave::Sine, 0.08f * Amp, 0.001f, 8.f); // 손에 쥔 통의 둔한 '툭'
		}
		break;
	}
	case EPDSound::Pour: // 조로록 (D-306): 가늘게 떨어지는 물줄기 잡음 + 보글보글 올라가는 작은 방울 소리 (잔이 차며 방울 음이 조금씩 높아진다)
	{
		Gain = 2.2f;
		const float Len = 1.15f;
		const size_t N = (size_t)((Len + 0.1f) * Rate);
		Buf.Ensure(N);
		uint32_t X = 0x2545f491u;
		auto Rnd = [&X]() { X ^= X << 13; X ^= X >> 17; X ^= X << 5; return (float)(X & 0xFFFF) / 65535.f; };
		{ // 물줄기: 부드러운 잡음, 세기가 살짝 일렁임
			float Lp = 0.f, Lp2 = 0.f;
			for (size_t I = 0; I < (size_t)(Len * Rate); ++I)
			{
				const float T = (float)I / Rate;
				const float R = Rnd() * 2.f - 1.f;
				Lp += (R - Lp) * 0.18f; Lp2 += (Lp - Lp2) * 0.35f;
				const float Env = std::min(1.f, T / 0.04f) * std::min(1.f, (Len - T) / 0.25f) * (0.8f + 0.2f * std::sin(T * 37.f) * std::sin(T * 11.f));
				Buf.S[I] += Lp2 * 0.22f * Env;
			}
		}
		// 방울: 짧게 위로 미끄러지는 사인 (방울 공명), 초당 약 55개
		float T = 0.02f;
		while (T < Len - 0.08f)
		{
			const float Fill = T / Len;
			const float F = (520.f + Rnd() * 900.f) * (1.f + 0.45f * Fill), D = 0.018f + Rnd() * 0.03f;
			const float Ba = (0.05f + Rnd() * 0.07f) * std::min(1.f, (Len - T) / 0.3f);
			Buf.Tone(T, D, F, F * (1.35f + Rnd() * 0.3f), EWave::Sine, Ba, 0.002f, 5.f);
			T += 0.006f + Rnd() * 0.03f;
		}
		Buf.Tone(Len - 0.06f, 0.08f, 900.f, 1400.f, EWave::Sine, 0.05f, 0.002f, 6.f); // 끝 방울 하나
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
	// ---------------------------------------------------------------- 위압감 악기 (D-269)
	// 쿵 (영화 같은 큰 타격): 80 → 28 Hz 로 떨어지는 사인 + 짧은 잡음, 길게 울림
	void Boom(float T0, float Amp)
	{
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)(2.2f * Rate);
		float Ph = 0.f, Lp = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			Ph += (28.f + 52.f * std::exp(-T / 0.18f)) / Rate;
			const float R = Rand();
			Lp += (R - Lp) * 0.06f;
			const float V = std::tanh(1.8f * std::sin(SynthTwoPi * Ph)) * std::exp(-T / 0.7f) + Lp * 1.5f * std::exp(-T / 0.12f);
			Add(0, I0 + I, V * Amp);
		}
	}
	// 팀파니: 낮은 사인 + 2.0 · 2.7배 부분음, 가죽 잡음, 긴 울림
	void Timpani(float T0, float Midi, float Amp)
	{
		const float F = MidiHz(Midi);
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)(1.6f * Rate);
		float P = 0.f, Lp = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			P += F * (1.f + 0.04f * std::exp(-T / 0.05f)) / Rate;
			Lp += (Rand() - Lp) * 0.2f;
			const float V = std::sin(SynthTwoPi * P) + 0.45f * std::sin(2.f * SynthTwoPi * P) * std::exp(-T / 0.4f) + 0.25f * std::sin(2.7f * SynthTwoPi * P) * std::exp(-T / 0.2f);
			Add(0, I0 + I, (V * std::exp(-T / 0.55f) + Lp * 0.6f * std::exp(-T / 0.03f)) * Amp);
		}
	}
	// 무거운 금관: 디튠 톱니 셋 → 두 번 거르고 → 찌그러뜨림(tanh), 천천히 부풀거나(Swell) 세게 때림
	void HeavyBrass(int B, float T0, float Dur, float Midi, float Amp, float Swell = 0.02f, float Drive = 2.5f)
	{
		const float F = MidiHz(Midi), Rel = 0.25f;
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)((Dur + Rel) * Rate);
		float P[3] = {Rand() * 0.5f + 0.5f, Rand() * 0.5f + 0.5f, Rand() * 0.5f + 0.5f}, L1 = 0.f, L2 = 0.f;
		static const float Dt[3] = {-0.009f, 0.f, 0.008f};
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			float X = 0.f;
			for (int K = 0; K < 3; ++K) { P[K] += F * (1.f + Dt[K]) / Rate; X += Saw(P[K]); }
			X /= 3.f;
			const float Sw = std::min(1.f, T / Swell);
			const float Cut = 180.f + 1600.f * Sw * (0.55f + 0.45f * std::exp(-T / 0.25f));
			const float A = 1.f - std::exp(-SynthTwoPi * Cut / Rate);
			L1 += (X - L1) * A;
			L2 += (L1 - L2) * A;
			float Env = Sw;
			if (T > Dur) Env *= std::max(0.f, 1.f - (T - Dur) / Rel);
			Add(B, I0 + I, std::tanh(L2 * Drive) / std::tanh(Drive) * Env * Amp);
		}
	}
	// 어두운 합창 패드: 톱니를 '아' 모음 근처(700 · 1100 Hz)로 두 번 공명시켜 거른 소리, 느린 비브라토
	void DarkChoir(int B, float T0, float Dur, float Midi, float Amp)
	{
		const float F = MidiHz(Midi), Rel = 0.8f;
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)((Dur + Rel) * Rate);
		float P1 = Rand() * 0.5f + 0.5f, P2 = Rand() * 0.5f + 0.5f;
		float B1[2] = {0.f, 0.f}, B2[2] = {0.f, 0.f};
		auto Res = [&](float X, float Fc, float Q, float* S) { // 상태 변수 공명 필터 (대역 통과)
			const float Fq = 2.f * std::sin(3.1415926f * Fc / Rate);
			const float Lo = S[1] + Fq * S[0];
			const float Hi = X - Lo - S[0] / Q;
			const float Bd = Fq * Hi + S[0];
			S[0] = Bd; S[1] = Lo;
			return Bd;
		};
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			const float Vib = 1.f + 0.004f * std::sin(SynthTwoPi * 4.8f * T);
			P1 += F * Vib * 1.004f / Rate;
			P2 += F * Vib * 0.996f / Rate;
			const float X = 0.5f * (Saw(P1) + Saw(P2));
			const float V = Res(X, 700.f, 6.f, B1) * 0.6f + Res(X, 1100.f, 7.f, B2) * 0.4f;
			float Env = std::min(1.f, T / 0.6f);
			if (T > Dur) Env *= std::max(0.f, 1.f - (T - Dur) / Rel);
			Add(B, I0 + I, V * Env * Amp);
		}
	}
	// ---------------------------------------------------------------- 클래식 재즈 악기 (D-362)
	// 재즈 기타: 카플러스-스트롱 (걸러진 잡음을 음 길이 지연선에 돌리며 평균 → 줄 뜯는 소리). Bright 클수록 밝은 픽, Dur 뒤에는 손으로 막음
	void Guitar(int B, float T0, float Dur, float Midi, float Amp, float Bright = 0.5f)
	{
		const float F = MidiHz(Midi), Rel = 0.07f;
		const int L = std::max(2, (int)std::lround(Rate / F));
		std::vector<float> D(L);
		float Lp = 0.f, Mean = 0.f;
		for (int K = 0; K < L; ++K) { Lp += (Rand() - Lp) * Bright; D[K] = Lp; Mean += Lp; }
		Mean /= L;
		for (float& V : D) V -= Mean;
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)((Dur + Rel) * Rate);
		int P = 0;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			const int Q = P + 1 == L ? 0 : P + 1;
			const float V = D[P];
			D[P] = 0.5f * (D[P] + D[Q]) * 0.997f;
			P = Q;
			const float Env = T > Dur ? std::max(0.f, 1.f - (T - Dur) / Rel) : 1.f;
			Add(B, I0 + I, V * Env * Amp * 2.2f);
		}
	}
	// 플루트: 사인 + 약한 배음, 숨소리 잡음, 처음 '츄' 하는 바람 소리, 늦게 들어오는 비브라토
	void Flute(int B, float T0, float Dur, float Midi, float Amp)
	{
		const float F = MidiHz(Midi), Rel = 0.14f;
		const size_t I0 = (size_t)(T0 * Rate), N = (size_t)((Dur + Rel) * Rate);
		float P = 0.f, Lp = 0.f;
		for (size_t I = 0; I < N; ++I)
		{
			const float T = (float)I / Rate;
			const float Vib = T > 0.18f ? 1.f + 0.006f * std::sin(SynthTwoPi * 5.f * T) * std::min(1.f, (T - 0.18f) / 0.3f) : 1.f;
			P += F * Vib / Rate;
			const float Tone = std::sin(SynthTwoPi * P) + 0.16f * std::sin(2.f * SynthTwoPi * P) + 0.05f * std::sin(3.f * SynthTwoPi * P);
			const float R = Rand();
			Lp += (R - Lp) * 0.25f;
			const float Air = R - Lp; // 높은 쪽 숨소리
			float Env = std::min(1.f, T / 0.07f) * (0.8f + 0.2f * std::exp(-T / 0.25f)) * std::exp(-T / 9.f);
			if (T > Dur) Env *= std::max(0.f, 1.f - (T - Dur) / Rel);
			const float Chiff = T < 0.05f ? 0.35f * (1.f - T / 0.05f) : 0.f;
			Add(B, I0 + I, (Tone * Env + Air * (0.07f * Env + Chiff)) * Amp);
		}
	}
	// 현악 패드: 디튠 톱니를 거른 소리, 천천히 부풀고 천천히 사라짐, 약한 비브라토
	void Strings(int B, float T0, float Dur, float Midi, float Amp) { SawNote(B, T0, Dur, Midi, Amp, 1500.f, 1500.f, 1.f, 0.4f, 0.8f, 0.005f, 0.004f); }
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

// ---------------------------------------------------------------- 인게임 재즈 4곡 (D-319): 늦은 밤 바
// 선율은 코드 음에서 씨앗으로 고른다 (곡마다 같은 씨앗 → 늘 같은 곡). 0 느린 스윙 · 1 보사노바 · 2 블루스 · 3 피아노 발라드
namespace
{
struct FJazzSong { const char* Name; float Bpm; };
const FJazzSong GJazz[JazzTrackCount] = {{"Last Call Swing", 80.f}, {"Neon Rain Bossa", 128.f}, {"After Hours Blues", 96.f}, {"Empty Glass Ballad", 66.f}};
// 한 마디 선율: 리듬 틀 하나를 고르고, 음은 코드 음(+12)에서 앞 음과 가까운 것으로 (가끔 이웃 음). Inst 0 비브라폰, 1 약음 리드, 2 로즈 한 음
// C2 가 있으면 3박부터는 그 코드 음에서 (반 마디마다 코드가 바뀌는 곡, D-359)
void MelodyBar(FMusic& M, int Inst, const FJazzChord& C0, float T, float Beat, bool bSwing, uint32_t& Seed, float& Prev, float Amp, const FJazzChord* C2 = nullptr)
{
	auto R = [&Seed]() { Seed ^= Seed << 13; Seed ^= Seed >> 17; Seed ^= Seed << 5; return (Seed & 0xFFFF) / 65535.f; };
	const float Sw = bSwing ? 2.f / 3.f : 0.5f;
	static const float Pat[5][6][2] = { // {박 자리, 길이}, 길이 0 = 끝
		{{0.f, 1.f}, {1.f, 1.f}, {2.f, 2.f}, {0, 0}, {0, 0}, {0, 0}},
		{{0.f, 1.5f}, {1.5f, 0.5f}, {2.f, 1.f}, {3.f, 1.f}, {0, 0}, {0, 0}},
		{{0.f, 0.5f}, {0.5f, 0.5f}, {1.f, 1.f}, {2.5f, 0.5f}, {3.f, 1.f}, {0, 0}},
		{{1.f, 1.f}, {2.f, 0.5f}, {2.5f, 1.5f}, {0, 0}, {0, 0}, {0, 0}},
		{{0.f, 3.f}, {3.f, 1.f}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}};
	const int P = (int)(R() * 4.999f);
	for (int K = 0; K < 6 && Pat[P][K][1] > 0.f; ++K)
	{
		float At = Pat[P][K][0];
		if (bSwing && At - std::floor(At) > 0.4f) At = std::floor(At) + Sw; // 뒷박은 스윙 자리
		const FJazzChord& C = C2 && At >= 2.f ? *C2 : C0;
		float Best = C.Voice[0] + 12.f, BestD = 99.f;
		for (int V = 0; V < 4; ++V) for (const float Oc : {12.f, 24.f})
		{
			const float N = C.Voice[V] + Oc;
			if (N < 67.f || N > 86.f) continue;
			const float D = std::fabs(N - Prev) + R() * 4.f;
			if (D < BestD) { BestD = D; Best = N; }
		}
		if (R() < 0.2f && K > 0) Best += R() < 0.5f ? 2.f : -1.f; // 이웃 음
		Prev = Best;
		const float Len = Pat[P][K][1] * Beat * 0.92f;
		if (Inst == 0) M.Vibes(2, T + At * Beat, Len, Best, Amp);
		else if (Inst == 1) M.MuteLead(2, T + At * Beat, Len, Best, Amp);
		else M.Rhodes(2, T + At * Beat, Len, Best, Amp);
	}
}
}

const char* JazzTrackName(int Track) { return GJazz[Track < 0 || Track >= JazzTrackCount ? 0 : Track].Name; }
float JazzTrackBpm(int Track) { return GJazz[Track < 0 || Track >= JazzTrackCount ? 0 : Track].Bpm; }

std::vector<int16_t> SynthesizeJazz(int Track, int SampleRate)
{
	Track = Track < 0 || Track >= JazzTrackCount ? 0 : Track;
	const float Bpm = GJazz[Track].Bpm, Beat = 60.f / Bpm, Bar = 4.f * Beat, Sw = 2.f / 3.f;
	// 코드: {근음, 3도, 7도, 손 화음 넷}
	const FJazzChord Bb6{46.f, 4.f, 11.f, {58.f, 62.f, 65.f, 69.f}}, G7{43.f, 4.f, 10.f, {59.f, 62.f, 65.f, 67.f}}, Cm7{36.f, 3.f, 10.f, {58.f, 60.f, 63.f, 67.f}}, F7{41.f, 4.f, 10.f, {57.f, 60.f, 63.f, 65.f}};
	const FJazzChord Bb7{46.f, 4.f, 10.f, {58.f, 62.f, 65.f, 68.f}}, Ebm7{39.f, 4.f, 11.f, {58.f, 62.f, 63.f, 67.f}}, Ebmi{39.f, 3.f, 10.f, {58.f, 61.f, 63.f, 66.f}}, Dm7{38.f, 3.f, 10.f, {57.f, 60.f, 62.f, 65.f}};
	const FJazzChord Dm9{38.f, 3.f, 10.f, {57.f, 60.f, 64.f, 65.f}}, G13{43.f, 4.f, 10.f, {59.f, 64.f, 65.f, 69.f}}, Cmaj9{36.f, 4.f, 11.f, {59.f, 62.f, 64.f, 67.f}}, A7b9{45.f, 4.f, 10.f, {58.f, 61.f, 64.f, 67.f}};
	const FJazzChord F7b{41.f, 4.f, 10.f, {57.f, 60.f, 63.f, 67.f}}, Bb9{46.f, 4.f, 10.f, {56.f, 60.f, 62.f, 65.f}}, C9{36.f, 4.f, 10.f, {58.f, 62.f, 64.f, 67.f}};
	const FJazzChord EbM{39.f, 4.f, 11.f, {58.f, 62.f, 63.f, 67.f}}, Cm9{36.f, 3.f, 10.f, {58.f, 62.f, 63.f, 67.f}}, Fm9{41.f, 3.f, 10.f, {56.f, 60.f, 63.f, 67.f}}, Bb13{46.f, 4.f, 10.f, {56.f, 62.f, 67.f, 68.f}};
	std::vector<FJazzChord> Ch;
	if (Track == 0) Ch = {Bb6, G7, Cm7, F7, Bb6, Bb7, Ebm7, Ebmi, Dm7, G7, Cm7, F7, Dm7, G7, Cm7, F7};
	else if (Track == 1) { for (int K = 0; K < 4; ++K) { Ch.push_back(Dm9); Ch.push_back(G13); Ch.push_back(Cmaj9); Ch.push_back(A7b9); } }
	else if (Track == 2) { const FJazzChord B12[12] = {F7b, Bb9, F7b, F7b, Bb9, Bb9, F7b, F7b, C9, Bb9, F7b, C9}; for (int K = 0; K < 24; ++K) Ch.push_back(B12[K % 12]); }
	else Ch = {EbM, Cm9, Fm9, Bb13, EbM, Cm9, Fm9, Bb13, Cm9, Fm9, Bb13, EbM, Fm9, Bb13, EbM, EbM};
	const int Bars = (int)Ch.size();
	const size_t LoopN = (size_t)(Bars * Bar * SampleRate), TailN = (size_t)(3.f * SampleRate);
	FMusic M(SampleRate, LoopN + TailN);
	M.Rng = 0x7a2c11u + (uint32_t)Track * 977u;
	uint32_t Seed = 0x9e3779b9u ^ (uint32_t)(Track * 2654435761u);
	float Prev = 74.f;
	std::vector<float> Kicks;
	for (int B = 0; B < Bars; ++B)
	{
		const float T = B * Bar;
		const FJazzChord& C = Ch[B];
		const FJazzChord& Nx = Ch[(B + 1) % Bars];
		if (Track == 0 || Track == 2) // 스윙 · 블루스 셔플: 라이드 '딩 딩-다', 2·4박 발 하이햇 + 브러시, 워킹 베이스, 로즈 찰스턴
		{
			for (const float At : {0.f, 1.f, 1.f + Sw, 2.f, 3.f, 3.f + Sw}) M.Ride(T + At * Beat, At - std::floor(At) > 0.1f ? 0.055f : 0.09f);
			for (const float At : {1.f, 3.f}) { M.Hat(T + At * Beat, 0.03f, 0.07f); M.Brush(T + At * Beat, 0.2f, Track == 2 ? 0.2f : 0.15f, true); }
			M.Brush(T, Bar, 0.05f, false);
			for (const float At : {0.f, 2.f}) M.Kick(T + At * Beat, 0.22f);
			WalkBar(M, T, Beat, C, Nx.Root, B % 2 == 1, 0.42f, false);
			const float Hits[2][2] = {{0.f, 0.6f}, {1.f + Sw, 1.2f}}, HitsO[2][2] = {{1.f, 0.5f}, {2.f + Sw, 1.1f}};
			for (int H = 0; H < 2; ++H)
			{
				const float At = B % 2 ? HitsO[H][0] : Hits[H][0], Len = B % 2 ? HitsO[H][1] : Hits[H][1];
				for (float N : C.Voice) M.Rhodes(1, T + At * Beat, Len * Beat, N, 0.065f);
			}
			if (Track == 0 ? (B >= 4 && B < 12) : (B < 12)) MelodyBar(M, 0, C, T, Beat, true, Seed, Prev, 0.14f); // 비브라폰
			else if (Track == 0 || B >= 12) MelodyBar(M, 1, C, T, Beat, true, Seed, Prev, 0.12f); // 약음 리드 (블루스는 두 번째 바퀴)
		}
		else if (Track == 1) // 보사노바: 곧은 8분, 림 클릭 3-2 클라베, 근음 · 5도 베이스, 로즈 당김
		{
			static const int Clave[2][3] = {{0, 3, 6}, {2, 4, -1}};
			for (int K = 0; K < 3; ++K) if (Clave[B % 2][K] >= 0) M.Brush(T + Clave[B % 2][K] * Beat * 0.5f, 0.06f, 0.2f, true);
			for (int K = 0; K < 8; ++K) M.Hat(T + K * Beat * 0.5f, 0.025f, K % 2 ? 0.04f : 0.07f);
			for (const float At : {0.f, 1.5f, 2.f, 3.5f}) { M.Kick(T + At * Beat, 0.18f); Kicks.push_back(T + At * Beat); }
			const float BAt[4] = {0.f, 1.5f, 2.f, 3.5f}, BNt[4] = {C.Root, C.Root + 7.f, C.Root + 7.f, Nx.Root + (Nx.Root > C.Root ? -12.f : 0.f) + 7.f};
			for (int K = 0; K < 4; ++K) M.Upright(1, T + BAt[K] * Beat, (K == 1 || K == 3 ? 0.5f : 1.4f) * Beat, BNt[K], 0.4f);
			for (const float At : {0.5f, 1.5f, 2.f, 3.f}) for (float N : C.Voice) M.Rhodes(1, T + At * Beat, Beat * 0.45f, N, 0.05f);
			if (B >= 4 && B < 12) MelodyBar(M, 0, C, T, Beat, false, Seed, Prev, 0.13f);
			else if (B >= 12) MelodyBar(M, 2, C, T, Beat, false, Seed, Prev, 0.1f);
		}
		else // 피아노 발라드: 반 마디마다 로즈 화음, 2분 업라이트, 브러시 쓸기, 비브라폰 긴 음
		{
			M.Brush(T, Bar, 0.07f, false);
			M.Brush(T + 3.f * Beat, 0.25f, 0.08f, true);
			for (const float At : {0.f, 2.f}) { for (float N : C.Voice) M.Rhodes(1, T + At * Beat, 2.f * Beat * 0.95f, N, 0.06f); M.Upright(1, T + At * Beat, 1.9f * Beat, At == 0.f ? C.Root : C.Root + 7.f, 0.38f); }
			if (B >= 2) MelodyBar(M, B < 10 ? 0 : 2, C, T, Beat, false, Seed, Prev, 0.12f);
		}
	}
	M.Crackle(LoopN + TailN, 0.03f); // 늦은 밤 레코드판
	FMixOpt O;
	O.Duck = Track == 1 ? 0.15f : 0.f; O.EchoFb = 0.22f; O.EchoMix = 0.3f; O.Room = Track == 3 ? 0.45f : 0.3f; O.Gain = 1.05f;
	return MixDown(M, Kicks, Beat, LoopN, TailN, SampleRate, O);
}

// ---------------------------------------------------------------- 클래식 재즈 시안 (D-359 → D-362): 저작권이 끝난 클래식 선율을 재즈로 편곡
// 테마 선율은 악보에서 옮겨 적고(작곡가가 죽은 지 70년이 훨씬 넘음, 녹음은 쓰지 않고 우리가 합성), 화음 · 반주 · 즉흥은 재즈로.
// 구간이 지날수록 악기가 하나씩 더해진다 (D-362). 0 그리그 '산왕의 궁전에서' 셔플 · 1 사티 '짐노페디 1번' 재즈 왈츠 · 2 파헬벨 '캐논' 스윙
namespace
{
enum EChordQ : uint8_t { QMaj7, Q6, QMin7, QMin6, QDom7, QDom7b9, QDom9 };
// 코드 만들기: 근음은 베이스 자리(36~47), 손 화음 네 음은 G3~F#4(55~66) 안에 모은다
FJazzChord MkChord(float Root, EChordQ Q)
{
	static const int Iv[7][4] = {{4, 7, 11, 14}, {4, 7, 9, 14}, {3, 7, 10, 14}, {3, 7, 9, 14}, {4, 7, 10, 14}, {4, 7, 10, 13}, {4, 10, 14, 19}};
	static const float Th[7] = {4.f, 4.f, 3.f, 3.f, 4.f, 4.f, 4.f}, Sv[7] = {11.f, 9.f, 10.f, 9.f, 10.f, 10.f, 10.f};
	while (Root >= 48.f) Root -= 12.f;
	while (Root < 36.f) Root += 12.f;
	FJazzChord C{Root, Th[Q], Sv[Q], {0.f, 0.f, 0.f, 0.f}};
	for (int K = 0; K < 4; ++K) C.Voice[K] = 55.f + (float)((((int)Root + Iv[Q][K]) % 12 - 7 + 12) % 12);
	std::sort(C.Voice, C.Voice + 4);
	return C;
}
bool SameChord(const FJazzChord& A, const FJazzChord& B) { return A.Root == B.Root && A.Voice[0] == B.Voice[0] && A.Voice[3] == B.Voice[3]; }

// 악기 번호: 0 비브라폰 · 1 약음 리드 · 2 로즈 · 3 플루트 · 4 재즈 기타 · 5 현악
void PlayInst(FMusic& M, int Inst, float T0, float Dur, float Midi, float Amp)
{
	switch (Inst)
	{
	case 0: M.Vibes(2, T0, Dur, Midi, Amp); break;
	case 1: M.MuteLead(2, T0, Dur, Midi, Amp); break;
	case 2: M.Rhodes(2, T0, Dur, Midi, Amp); break;
	case 3: M.Flute(2, T0, Dur, Midi, Amp); break;
	case 4: M.Guitar(2, T0, Dur, Midi, Amp, 0.35f); break;
	default: M.Strings(2, T0, Dur, Midi, Amp); break;
	}
}

// 반주 층 (구간마다 켠다)
enum : uint32_t
{
	LRide = 1,     // 라이드 · 하이햇 · 킥 (없으면 브러시만)
	LWalk = 2,     // 워킹 베이스 (없으면 반 마디마다 근음 = 투 필)
	LRhodes = 4,   // 로즈 반주
	LGuitar = 8,   // 기타 반주 (스윙 = 박마다 짧게 긁기, 왈츠 = 8분 아르페지오)
	LStrings = 16, // 현악 패드
	LBrass = 32,   // 금관 찌르기
	LVibes = 64,   // 비브라폰 화음 (왈츠 문 열기)
};
struct FCNote { int Bar; float At, Len, Midi; int Voice = 0; }; // 테마 선율 한 음 (테마 마디, 박 자리, 길이(박), 음높이, 성부)
struct FCVoice { int Inst = -1; float Shift = 0.f, Amp = 0.f; };
struct FCSection
{
	int Bars = 8, From = 0;   // 길이, 테마 몇 마디째부터
	bool bSolo = false;       // 테마 대신 즉흥 (Lead 악기)
	uint32_t Layers = 0;
	FCVoice Lead, Double;     // 첫 성부와 그것을 겹쳐 치는 악기
	FCVoice V1, V2;           // 따라 들어오는 둘째 · 셋째 성부 (캐논)
};
struct FClassicTune { const char* Name; float Bpm; int Meter; bool bWaltz; bool bSwing; };
const FClassicTune GClassic[ClassicJazzCount] = { // 템포는 늦은 밤 바 배경음답게 (D-361)
	{"Mountain King Shuffle", 90.f, 4, false, true}, // 그리그 (1907 졸)
	{"Gymnopedie After Dark", 60.f, 3, true, false}, // 사티 (1925 졸)
	{"Canon After Midnight", 76.f, 4, false, true},  // 파헬벨 (1706 졸)
};

FCSection Sec(int Bars, int From, uint32_t Layers, FCVoice Lead, FCVoice Double = {}, bool bSolo = false)
{
	FCSection S;
	S.Bars = Bars; S.From = From; S.Layers = Layers; S.Lead = Lead; S.Double = Double; S.bSolo = bSolo;
	return S;
}

// 곡마다 테마(마디마다 앞 · 뒤 반 마디 코드 + 선율)와 짜임
void ClassicTheme(int Track, std::vector<FJazzChord>& ChA, std::vector<FJazzChord>& ChB, std::vector<FCNote>& Mel, std::vector<FCSection>& Form)
{
	auto Bar2 = [&](const FJazzChord& A, const FJazzChord& B) { ChA.push_back(A); ChB.push_back(B); };
	auto N = [&](int Bar, float At, float Len, float Midi) { Mel.push_back({Bar, At, Len, Midi, 0}); };
	if (Track == 0) // 산왕의 궁전: B 단조 8마디, 원곡 4마디 주제 두 번(두 번째 끝은 으뜸음). 반음 위 코드 → 트라이톤 대리
	{
		const FJazzChord Bm = MkChord(47, QMin6), Cs7 = MkChord(37, QDom7), C7 = MkChord(36, QDom9), D6 = MkChord(38, Q6), Fs7 = MkChord(42, QDom7b9);
		auto Up = [&](int Bar, bool bHigh) { const float R[8] = {59, 61, 62, 64, 66, 62, 66, 71}; for (int K = 0; K < (bHigh ? 8 : 6); ++K) N(Bar, K * .5f, .5f, R[K]); if (!bHigh) N(Bar, 3, 1, 66); }; // 시 도# 레 미 파# 레 파#
		auto Half = [&](int Bar) { N(Bar, 0, .5f, 65); N(Bar, .5f, .5f, 61); N(Bar, 1, 1, 65); N(Bar, 2, .5f, 64); N(Bar, 2.5f, .5f, 60); N(Bar, 3, 1, 64); }; // 미# 도# 미# / 미 도 미
		for (int H = 0; H < 2; ++H)
		{
			const int B0 = H * 4;
			Bar2(Bm, Bm); Up(B0, false);
			Bar2(Cs7, C7); Half(B0 + 1);
			Bar2(Bm, Bm); Up(B0 + 2, true);
			Bar2(D6, Fs7);
			N(B0 + 3, 0, .5f, 69); N(B0 + 3, .5f, .5f, 66); N(B0 + 3, 1, .5f, 62); N(B0 + 3, 1.5f, .5f, 66);
			if (H == 0) N(3, 2, 2, 69); // 라—
			else { N(7, 2, .5f, 65); N(7, 2.5f, .5f, 61); N(7, 3, 1, 59); } // 미# 도# 시
		}
		// 몰래 다가오다(베이스 · 브러시 · 낮은 약음 리드) → 기타 · 라이드 · 비브라폰 겹침 → 비브라폰 즉흥 + 현악 → 금관 · 플루트까지 다 함께
		Form = {Sec(8, 0, LWalk, {1, 0.f, 0.13f}),
			Sec(8, 0, LWalk | LRide | LRhodes | LGuitar, {1, 0.f, 0.12f}, {0, 12.f, 0.07f}),
			Sec(8, 0, LWalk | LRide | LRhodes | LGuitar | LStrings, {0, 0.f, 0.14f}, {}, true),
			Sec(8, 0, LWalk | LRide | LRhodes | LGuitar | LStrings | LBrass, {1, 0.f, 0.12f}, {3, 12.f, 0.07f})};
	}
	else if (Track == 1) // 짐노페디: 3/4 20마디, 4마디 반주 뒤 원곡 선율. G maj7 · D maj7 번갈아
	{
		const FJazzChord Gm7 = MkChord(43, QMaj7), Dm7 = MkChord(38, QMaj7);
		for (int K = 0; K < 20; ++K) Bar2(K % 2 ? Dm7 : Gm7, K % 2 ? Dm7 : Gm7);
		for (const int B0 : {4, 12})
		{
			N(B0, 1, 1, 78); N(B0, 2, 1, 81);                              // 파# 라
			N(B0 + 1, 0, 1, 79); N(B0 + 1, 1, 1, 78); N(B0 + 1, 2, 1, 73); // 솔 파# 도#
			N(B0 + 2, 0, 1, 71); N(B0 + 2, 1, 1, 73); N(B0 + 2, 2, 1, 74); // 시 도# 레
			N(B0 + 3, 0, 3, 69);                                           // 라—
		}
		N(8, 0, 10.f, 66);                                   // 파#———
		N(16, 0, 3, 73); N(17, 0, 3, 78); N(18, 0, 5.f, 76); // 도# · 파# · 미—
		// 첫 바퀴 로즈 선율 + 나일론 기타 아르페지오, 둘째 바퀴 플루트 선율 + 비브라폰 옥타브 + 현악 패드
		Form = {Sec(20, 0, LRhodes | LGuitar | LVibes, {2, 0.f, 0.14f}),
			Sec(20, 0, LRhodes | LGuitar | LStrings, {3, 0.f, 0.12f}, {0, 12.f, 0.035f})};
	}
	else // 캐논: D 장조 8코드(D A Bm F#m G D G A)를 반 마디씩 = 4마디 한 바퀴, 여섯 바퀴. 원곡 4분음 화음을 2분음으로 펴서
	{	// 첫 성부(비브라폰): 2분 A → 2분 B → 4분 C1 · C2 → 스윙 8분 D → 옥타브 위 A
		// 둘째 성부(약음 리드)는 한 바퀴, 셋째 성부(플루트)는 두 바퀴 늦게 같은 차례로 (원곡처럼 세 성부 카논)
		const FJazzChord D = MkChord(38, QMaj7), A7 = MkChord(45, QDom7), Bm = MkChord(47, QMin7), Fsm = MkChord(42, QMin7), Gm7 = MkChord(43, QMaj7), A13 = MkChord(45, QDom9);
		for (int C = 0; C < 6; ++C) { Bar2(D, A7); Bar2(Bm, Fsm); Bar2(Gm7, D); Bar2(Gm7, A13); }
		static const float LA[8] = {78, 76, 74, 73, 71, 69, 71, 73}, LB[8] = {74, 73, 71, 69, 67, 66, 67, 64};
		static const float LC1[16] = {62, 66, 69, 67, 66, 62, 66, 64, 62, 59, 62, 69, 67, 71, 69, 67}, LC2[16] = {66, 62, 64, 73, 74, 78, 81, 69, 71, 67, 69, 66, 62, 74, 74, 73};
		static const float LD[32] = {74, 73, 74, 62, 61, 69, 64, 66, 62, 74, 73, 71, 73, 78, 81, 83, 79, 78, 76, 79, 78, 76, 74, 73, 71, 69, 67, 66, 64, 67, 66, 64};
		auto Line = [&](int Cyc, int Which, int Voice, float Shift) { // Which 0 A · 1 B · 2 C1 · 3 C2 · 4 D
			const float* L = Which == 0 ? LA : Which == 1 ? LB : Which == 2 ? LC1 : Which == 3 ? LC2 : LD;
			const int Per = Which < 2 ? 1 : Which < 4 ? 2 : 4, Cnt = 8 * Per; // 코드 하나(2박)에 몇 음
			for (int K = 0; K < Cnt; ++K)
			{
				const float Beat0 = K * 2.f / Per;
				Mel.push_back({Cyc * 4 + (int)(Beat0 / 4.f), std::fmod(Beat0, 4.f), 2.f / Per, L[K] + Shift, Voice});
			}
		};
		for (int C = 0; C < 5; ++C) Line(C, C, 0, 0.f);
		Line(5, 0, 0, 12.f);
		for (int C = 1; C < 6; ++C) Line(C, C - 1, 1, 0.f);
		for (int C = 2; C < 6; ++C) Line(C, C - 2, 2, 0.f);
		// 바퀴마다 하나씩: 기타 투 필 → 로즈 → 워킹 · 라이드 → 현악 → 금관 살짝 → 금관 빼고 다시 처음으로
		const uint32_t Ly[6] = {LGuitar, LGuitar | LRhodes, LGuitar | LRhodes | LWalk | LRide, LGuitar | LRhodes | LWalk | LRide | LStrings,
			LGuitar | LRhodes | LWalk | LRide | LStrings | LBrass, LGuitar | LRhodes | LWalk | LRide | LStrings};
		for (int C = 0; C < 6; ++C)
		{
			FCSection S = Sec(4, C * 4, Ly[C], {0, 0.f, 0.14f});
			S.V1 = {1, 0.f, 0.085f};
			S.V2 = {3, 0.f, 0.08f};
			Form.push_back(S);
		}
	}
}
} // namespace

const char* ClassicJazzName(int Track) { return GClassic[Track < 0 || Track >= ClassicJazzCount ? 0 : Track].Name; }
float ClassicJazzBpm(int Track) { return GClassic[Track < 0 || Track >= ClassicJazzCount ? 0 : Track].Bpm; }

std::vector<int16_t> SynthesizeClassicJazz(int Track, int SampleRate)
{
	Track = Track < 0 || Track >= ClassicJazzCount ? 0 : Track;
	const FClassicTune& Tu = GClassic[Track];
	const float Beat = 60.f / Tu.Bpm, Bar = Tu.Meter * Beat, Sw = 2.f / 3.f;
	std::vector<FJazzChord> ChA, ChB;
	std::vector<FCNote> Mel;
	std::vector<FCSection> Form;
	ClassicTheme(Track, ChA, ChB, Mel, Form);
	const int ThemeBars = (int)ChA.size();
	int Bars = 0;
	for (const FCSection& S : Form) Bars += S.Bars;
	const size_t LoopN = (size_t)(Bars * Bar * SampleRate), TailN = (size_t)(4.f * SampleRate);
	FMusic M(SampleRate, LoopN + TailN);
	M.Rng = 0x51c0ffeu + (uint32_t)Track * 7919u;
	uint32_t Seed = 0x2545f491u ^ (uint32_t)(Track * 2654435761u);
	float Prev = 74.f;
	// 스윙: 8분 짝에서 앞은 길게(2/3), 뒤는 짧게
	auto Swung = [&](float At, float& Len) {
		if (!Tu.bSwing) return At;
		const float Fr = At - std::floor(At);
		if (Fr > 0.4f && Fr < 0.6f) { if (Len <= 0.5f) Len = 1.f - Sw; return std::floor(At) + Sw; }
		if (Fr < 0.01f && Len == 0.5f) Len = Sw;
		return At;
	};
	int G = 0; // 전체 마디 번호
	for (size_t Si = 0; Si < Form.size(); ++Si)
	{
		const FCSection& S = Form[Si];
		const uint32_t Ly = S.Layers;
		for (int K = 0; K < S.Bars; ++K, ++G)
		{
			const float T = G * Bar;
			const int Ti = (S.From + K) % ThemeBars;
			const FJazzChord& A = ChA[Ti];
			const FJazzChord& B = ChB[Ti];
			const FCSection& NS = K + 1 < S.Bars ? S : Form[(Si + 1) % Form.size()];
			const FJazzChord& Nx = ChA[(K + 1 < S.Bars ? S.From + K + 1 : NS.From) % ThemeBars];
			const bool bSplit = !SameChord(A, B);
			if (!Tu.bWaltz) // 4/4 스윙
			{
				M.Brush(T, Bar, 0.05f, false);
				for (const float At : {1.f, 3.f}) M.Brush(T + At * Beat, 0.2f, Ly & LRide ? 0.15f : 0.09f, true);
				if (Ly & LRide) // 라이드 '딩 딩-다', 2 · 4박 발 하이햇, 1 · 3박 작은 킥
				{
					for (const float At : {0.f, 1.f, 1.f + Sw, 2.f, 3.f, 3.f + Sw}) M.Ride(T + At * Beat, At - std::floor(At) > 0.1f ? 0.045f : 0.075f);
					for (const float At : {1.f, 3.f}) M.Hat(T + At * Beat, 0.03f, 0.06f);
					for (const float At : {0.f, 2.f}) M.Kick(T + At * Beat, 0.2f);
				}
				if (Ly & LWalk)
				{
					if (!bSplit) WalkBar(M, T, Beat, A, Nx.Root, G % 2 == 1, 0.42f, false);
					else
					{
						float Br = B.Root, Nr = Nx.Root;
						while (Br - A.Root > 6.f) Br -= 12.f;
						while (A.Root - Br > 6.f) Br += 12.f;
						while (Nr - Br > 6.f) Nr -= 12.f;
						while (Br - Nr > 6.f) Nr += 12.f;
						const float Wn[4] = {A.Root, A.Root + A.Third, Br, Nr + (G % 2 ? 1.f : -1.f)};
						for (int W = 0; W < 4; ++W) M.Upright(1, T + W * Beat, Beat * 0.88f, Wn[W], 0.42f);
					}
				}
				else for (int H = 0; H < 2; ++H) M.Upright(1, T + H * 2.f * Beat, 1.9f * Beat, (H ? B : A).Root, 0.4f); // 투 필
				// 로즈: 워킹이면 찰스턴 당김, 투 필이면 반 마디마다 길게
				const float Hits[2][2] = {{0.f, 0.6f}, {1.f + Sw, 1.2f}}, HitsO[2][2] = {{1.f, 0.5f}, {2.f + Sw, 1.1f}};
				if (Ly & LRhodes)
				{
					if (Ly & LWalk)
						for (int H = 0; H < 2; ++H)
						{
							const float At = G % 2 ? HitsO[H][0] : Hits[H][0], Len = G % 2 ? HitsO[H][1] : Hits[H][1];
							for (float V : (At >= 2.f ? B : A).Voice) M.Rhodes(1, T + At * Beat, Len * Beat, V, Ly & LGuitar ? 0.045f : 0.06f);
						}
					else
						for (int H = 0; H < 2; ++H) for (float V : (H ? B : A).Voice) M.Rhodes(1, T + (H * 2.f + (G % 2 ? Sw : 0.f)) * Beat, 1.8f * Beat, V, 0.05f);
				}
				// 기타: 워킹이면 박마다 짧게 긁기(2 · 4박 조금 세게), 투 필이면 반 마디마다 천천히 긁어 울림
				if (Ly & LGuitar)
				{
					if (Ly & LWalk)
						for (int Bt = 0; Bt < 4; ++Bt)
						{
							const FJazzChord& C = Bt >= 2 ? B : A;
							const float Low[3] = {C.Root + 12.f, C.Root + 12.f + C.Third + (C.Root + 12.f + C.Third < 52.f ? 12.f : 0.f), C.Root + 12.f + C.Seventh};
							for (int V = 0; V < 3; ++V) M.Guitar(1, T + Bt * Beat + V * 0.008f, Beat * 0.32f, Low[V], Bt % 2 ? 0.075f : 0.06f, 0.3f);
						}
					else
						for (int H = 0; H < 2; ++H)
						{
							const FJazzChord& C = H ? B : A;
							const float Str[5] = {C.Root + 12.f, C.Voice[0], C.Voice[1], C.Voice[2], C.Voice[3]};
							for (int V = 0; V < 5; ++V) M.Guitar(1, T + H * 2.f * Beat + V * 0.025f, 1.9f * Beat, Str[V], 0.06f, 0.3f);
						}
				}
				if (Ly & LStrings) for (int H = 0; H < 2; ++H) { const FJazzChord& C = H ? B : A; for (float V : C.Voice) M.Strings(1, T + H * 2.f * Beat, 1.95f * Beat, V + 12.f, 0.022f); M.Strings(1, T + H * 2.f * Beat, 1.95f * Beat, C.Root + 12.f, 0.025f); }
				if (Ly & LBrass)
					for (int H = 0; H < 2; ++H)
					{
						const float At = G % 2 ? HitsO[H][0] : Hits[H][0];
						for (float V : (At >= 2.f ? B : A).Voice) M.Brass(1, T + At * Beat, 0.45f * Beat, V + 12.f, 0.04f, H == 1 && K % 4 == 3);
					}
			}
			else // 3/4 재즈 왈츠 발라드: 1박 베이스, 2박 로즈 화음, 브러시 쓸기, 3박 가볍게 탁
			{
				M.Brush(T, Bar, 0.06f, false);
				M.Brush(T + 2.f * Beat, 0.2f, 0.06f, true);
				M.Upright(1, T, 2.8f * Beat, A.Root, 0.4f);
				if (Ly & LRhodes) for (float V : A.Voice) M.Rhodes(1, T + Beat, 1.9f * Beat, V + 12.f, Ly & LGuitar ? 0.035f : 0.045f);
				if ((Ly & LVibes) && K < 2) for (float V : A.Voice) M.Vibes(1, T + Beat, 1.8f * Beat, V + 12.f, 0.025f); // 첫 두 마디 비브라폰 화음으로 문 열기
				if (Ly & LGuitar) // 나일론 기타 8분 아르페지오: 근음 → 화음 위로 → 한 음 내려옴
				{
					const float Arp[6] = {A.Root + 12.f, A.Voice[0], A.Voice[1], A.Voice[2], A.Voice[3], A.Voice[1]};
					for (int E = 0; E < 6; ++E) M.Guitar(1, T + E * 0.5f * Beat, (E == 0 ? 2.8f : 1.2f) * Beat, Arp[E], E == 0 ? 0.075f : 0.055f, 0.25f);
				}
				if (Ly & LStrings) { for (float V : A.Voice) M.Strings(1, T, Bar * 0.98f, V + 12.f, 0.02f); M.Strings(1, T, Bar * 0.98f, A.Root + 24.f, 0.02f); }
			}
			if (S.bSolo) MelodyBar(M, S.Lead.Inst, A, T, Beat, Tu.bSwing, Seed, Prev, S.Lead.Amp, bSplit ? &B : nullptr);
		}
	}
	// 테마 선율: 구간마다 악보대로 (구간이 테마 몇 마디째부터인지 From 으로)
	G = 0;
	for (const FCSection& S : Form)
	{
		if (!S.bSolo)
			for (int K = 0; K < S.Bars; ++K)
			{
				const int Ti = (S.From + K) % ThemeBars;
				for (const FCNote& Nt : Mel)
				{
					if (Nt.Bar != Ti) continue;
					const FCVoice& V = Nt.Voice == 0 ? S.Lead : Nt.Voice == 1 ? S.V1 : S.V2;
					if (V.Inst < 0) continue;
					float Len = Nt.Len;
					const float At = Swung(Nt.At, Len);
					const float T0 = (G + K) * Bar + At * Beat, Dur = Len * Beat * 0.94f;
					PlayInst(M, V.Inst, T0, Dur, Nt.Midi + V.Shift, V.Amp);
					if (Nt.Voice == 0 && S.Double.Inst >= 0) PlayInst(M, S.Double.Inst, T0, Dur, Nt.Midi + S.Double.Shift, S.Double.Amp);
				}
			}
		G += S.Bars;
	}
	M.Crackle(LoopN + TailN, 0.03f); // 늦은 밤 레코드판
	FMixOpt O;
	O.Duck = 0.f; O.EchoFb = 0.22f; O.EchoMix = 0.3f; O.Room = Tu.bWaltz ? 0.45f : 0.3f; O.Gain = 1.0f;
	return MixDown(M, {}, Beat, LoopN, TailN, SampleRate, O);
}

// ---------------------------------------------------------------- 3. 보스 (D-188 → D-264 → D-267 → D-270): 느와르 위압 시안(3)으로 확정
std::vector<int16_t> SynthesizeBossMusic(int SampleRate) { return SynthesizeBossMusicAlt(3, SampleRate); } // 느와르 위압 (D-270)

// ---------------------------------------------------------------- 위압감 보스 시안 (D-269)
// 3 느와르 위압: 66 BPM D 단조 Dm · Dm · B♭ · A · Dm · Dm · E♭ · A7, 마디 첫 박 쿵 + 팀파니 + 찌그러진 낮은 금관 덩어리(반음 부딪힘),
//   3박 낮은 금관, 심장 박동 킥(쿵-쿵), 낮은 반음 베이스 반복(근음 · 반음 위), 어두운 합창, 비브라폰 증4도 동기, 마지막 마디 팀파니 연타
// 4 보스 행진: 80 BPM, 박마다 낮은 금관 + 팀파니가 한 발씩 '쿵 쿵 쿵 쿵', 작은 스네어 연타, 합창, 5~8마디 낮은 금관 선율
static std::vector<int16_t> SynthesizeBossMenace(int Variant, int SampleRate)
{
	const float Bpm = Variant == 0 ? 66.f : 80.f;
	const float Beat = 60.f / Bpm, Bar = 4.f * Beat, S16 = Beat / 4.f;
	const int Bars = 8;
	const size_t LoopN = (size_t)(Bars * Bar * SampleRate), TailN = (size_t)(3.f * SampleRate);
	FMusic M(SampleRate, LoopN + TailN);
	M.Rng = 0x2545f491u + (uint32_t)Variant;
	// 근음 (낮은 D2 = 38 기준), 단조 여부
	const float Roots0[8] = {38.f, 38.f, 46.f - 12.f, 45.f - 12.f, 38.f, 38.f, 39.f, 45.f - 12.f};
	const bool Minor0[8] = {true, true, false, false, true, true, false, false};
	const float Roots1[8] = {38.f, 43.f - 12.f, 38.f, 45.f - 12.f, 38.f, 46.f - 12.f, 43.f - 12.f, 45.f - 12.f};
	const bool Minor1[8] = {true, true, true, false, true, false, true, false};
	std::vector<float> Kicks;
	for (int B = 0; B < Bars; ++B)
	{
		const float T = B * Bar;
		const float R = Variant == 0 ? Roots0[B] : Roots1[B];
		const float Third = (Variant == 0 ? Minor0[B] : Minor1[B]) ? 3.f : 4.f;
		const bool bLast = B == Bars - 1;
		// 어두운 합창 (마디 내내)
		for (const float Iv : {24.f, 24.f + Third, 31.f}) M.DarkChoir(1, T, Bar * 0.97f, R + Iv, 0.075f);
		if (Variant == 0)
		{
			M.Boom(T, B % 2 == 0 ? 0.9f : 0.6f);
			M.Timpani(T, R + 12.f, 0.6f);
			// 찌그러진 낮은 금관 덩어리: 근음 옥타브 + 5도 + 반음 위(부딪힘), 2박 길이
			for (const float Iv : {12.f, 19.f, 24.f}) M.HeavyBrass(1, T, Beat * 2.f, R + Iv, 0.075f, 0.03f);
			M.HeavyBrass(1, T, Beat * 2.f, R + 25.f, 0.035f, 0.03f);
			for (const float Iv : {12.f, 19.f}) M.HeavyBrass(1, T + 2.f * Beat, Beat * 0.9f, R + Iv, 0.06f, 0.015f);
			M.Timpani(T + 2.f * Beat, R + 12.f - 5.f, 0.4f);
			// 심장 박동 킥: 1박 쿵-쿵, 3박 쿵-쿵
			for (const float At : {0.f, 2.f}) { M.Kick(T + At * Beat, 1.f); M.Kick(T + (At + 0.33f) * Beat, 0.6f); Kicks.push_back(T + At * Beat); }
			M.Snare(T + 3.f * Beat, 0.45f);
			// 낮은 반음 베이스 반복 (8분): 근음 근음 근음 반음위 근음 근음 반음아래 반음위
			const float Ost[8] = {0.f, 0.f, 0.f, 1.f, 0.f, 0.f, -1.f, 1.f};
			for (int K = 0; K < 8; ++K)
			{
				M.Upright(1, T + K * Beat / 2.f, Beat / 2.f * 0.85f, R + Ost[K], 0.4f);
				M.SawNote(1, T + K * Beat / 2.f, Beat / 2.f * 0.8f, R + Ost[K] - 12.f + 12.f * (R - 12.f < 26.f), 0.2f, 220.f, 110.f, 0.1f, 0.003f, 0.05f, 0.003f);
			}
			if (bLast) for (int K = 0; K < 12; ++K) M.Timpani(T + 2.f * Beat + K * S16 * 0.66f, R + 12.f, 0.18f + 0.04f * K);
		}
		else
		{
			// 박마다 '쿵': 낮은 금관 유니즌 + 팀파니 (1박이 가장 셈)
			for (int K = 0; K < 4; ++K)
			{
				const float A = K == 0 ? 1.f : 0.7f;
				for (const float Iv : {12.f, 24.f}) M.HeavyBrass(1, T + K * Beat, Beat * 0.55f, R + Iv, 0.075f * A, 0.012f, 3.f);
				M.Timpani(T + K * Beat, R + 12.f, 0.45f * A);
				M.Kick(T + K * Beat, 0.8f * A); Kicks.push_back(T + K * Beat);
			}
			if (B % 2 == 0) M.Boom(T, 0.7f);
			for (int K = 0; K < 16; ++K) if (K % 4 != 0) M.Snare(T + K * S16, 0.12f + (K % 4 == 3 ? 0.08f : 0.f)); // 작은 스네어 연타
			M.Upright(1, T, Bar * 0.95f, R, 0.4f);
			M.SawNote(1, T, Bar * 0.95f, R - 12.f + 12.f * (R - 12.f < 26.f), 0.18f, 200.f, 120.f, 0.5f, 0.05f, 0.3f, 0.003f);
			if (bLast) for (int K = 0; K < 8; ++K) M.Timpani(T + 2.f * Beat + K * S16, R + 12.f, 0.2f + 0.05f * K);
		}
	}
	// 선율
	struct FN { int Bar; float At, Len, Midi; };
	if (Variant == 0) // 비브라폰 증4도 동기 (D - G# - A)
	{
		const FN V[] = {{0, 1.f, 1.f, 74.f}, {0, 2.f, 1.f, 68.f}, {0, 3.f, 1.f, 69.f}, {2, 1.f, 1.f, 70.f}, {2, 2.f, 2.f, 64.f}, {4, 1.f, 1.f, 74.f}, {4, 2.f, 1.f, 68.f}, {4, 3.f, 1.f, 69.f}, {6, 1.f, 1.f, 75.f}, {6, 2.f, 2.f, 73.f}};
		for (const FN& L : V) M.Vibes(2, L.Bar * Bar + L.At * Beat, L.Len * Beat, L.Midi, 0.14f);
	}
	else // 5~8마디 낮은 금관 선율
	{
		const FN V[] = {{4, 0.f, 2.f, 62.f}, {4, 2.f, 2.f, 65.f}, {5, 0.f, 2.f, 64.f}, {5, 2.f, 2.f, 58.f}, {6, 0.f, 2.f, 62.f}, {6, 2.f, 2.f, 56.f}, {7, 0.f, 4.f, 57.f}};
		for (const FN& L : V) M.HeavyBrass(2, L.Bar * Bar + L.At * Beat, L.Len * Beat * 0.95f, L.Midi - 12.f, 0.1f, 0.06f, 2.f);
	}
	FMixOpt O;
	O.Duck = 0.25f; O.EchoFb = 0.25f; O.EchoMix = 0.3f; O.Room = 0.45f; O.Gain = Variant == 0 ? 0.75f : 0.95f;
	return MixDown(M, Kicks, Beat, LoopN, TailN, SampleRate, O);
}

// ---------------------------------------------------------------- 보스 시안 (D-266): 빠르지 않고 무겁게, 1 · 2번 곡과 같은 재즈 결
// 0 느와르 기어가기: 72 BPM 반박자 느낌, D 단조 블루스(Dm · Dm · Gm · A7♭9), 낮은 업라이트 + 서브 베이스 2분음, 팀파니 같은 낮은 탐,
//   낮은 금관이 천천히 부풀었다 꺼짐, 로즈 불협 덩어리, 비브라폰 불길한 동기, 브러시 대신 묵직한 킥 · 림
// 1 느린 빅밴드 위협: 92 BPM 스윙, 낮은 트롬본 느낌 금관 화음이 박마다 '쿵-', 2분 워킹 베이스, 라이드 스윙, 마디 끝 금관 폴, 약음 리드가 낮게
// 2 둠 펑크: 84 BPM 반박자, 킥 1 · 3박 뒷박, 스네어 3박만 (무겁게), 옥타브 떨어지는 신스 베이스, 로즈 트레몰로 화음, 낮은 금관 찌르기, 경보 동기 느리게
std::vector<int16_t> SynthesizeBossMusicAlt(int Variant, int SampleRate)
{
	if (Variant >= 3) return SynthesizeBossMenace(Variant - 3, SampleRate); // 위압감 시안 (D-269)
	const float Bpm = Variant == 0 ? 72.f : Variant == 1 ? 92.f : 84.f;
	const float Beat = 60.f / Bpm, Bar = 4.f * Beat, Sw = 2.f / 3.f, S16 = Beat / 4.f;
	const int Bars = Variant == 0 ? 8 : 12;
	const size_t LoopN = (size_t)(Bars * Bar * SampleRate), TailN = (size_t)(3.f * SampleRate);
	FMusic M(SampleRate, LoopN + TailN);
	M.Rng = 0x7f4a7c15u + (uint32_t)Variant;
	// D 단조: Dm(m9 느낌) · Gm · A7♭9 · B♭maj7
	const FJazzChord Dm = {38.f, 3.f, 10.f, {53.f, 57.f, 60.f, 64.f}}, Gm = {43.f, 3.f, 10.f, {53.f, 57.f, 58.f, 62.f}}, A7 = {45.f, 4.f, 10.f, {55.f, 58.f, 61.f, 67.f}}, Bb = {46.f, 4.f, 11.f, {53.f, 57.f, 62.f, 65.f}};
	const FJazzChord Prog0[8] = {Dm, Dm, Gm, Gm, Dm, Bb, A7, A7};
	const FJazzChord Prog[12] = {Dm, Dm, Dm, Dm, Gm, Gm, Dm, Dm, Bb, A7, Dm, A7};
	std::vector<float> Kicks;
	for (int B = 0; B < Bars; ++B)
	{
		const float T = B * Bar;
		const FJazzChord& C = Variant == 0 ? Prog0[B] : Prog[B];
		const bool bTurn = B == Bars - 1;
		if (Variant == 0)
		{
			// 반박자: 킥 1박 · 3박 뒷박 살짝, 3박 림(낮은 스네어 대신), 팀파니 탐 1박
			M.Kick(T, 1.1f); Kicks.push_back(T);
			M.Kick(T + 2.5f * Beat, 0.6f); Kicks.push_back(T + 2.5f * Beat);
			M.Snare(T + 2.f * Beat, 0.55f);
			M.Tom(T, 62.f, 0.55f);
			for (const float At : {0.f, 1.f, 1.f + Sw, 2.f, 3.f, 3.f + Sw}) M.Ride(T + At * Beat, At == 1.f + Sw || At == 3.f + Sw ? 0.035f : 0.06f, 1.2f);
			if (bTurn) for (int K = 0; K < 4; ++K) M.Tom(T + (2.f + K * 0.5f) * Beat, 95.f - K * 10.f, 0.5f);
			// 베이스: 2분음 근음 · 5도, 서브 겹침
			for (int K = 0; K < 2; ++K)
			{
				const float N = K == 0 ? C.Root : C.Root + 7.f - 12.f;
				M.Upright(1, T + K * 2.f * Beat, 2.f * Beat * 0.9f, N, 0.5f);
				M.SawNote(1, T + K * 2.f * Beat, 2.f * Beat * 0.9f, N - 12.f, 0.22f, 160.f, 120.f, 0.3f, 0.02f, 0.2f, 0.002f);
			}
			// 낮은 금관: 마디 내내 천천히 부풀었다 꺼짐 (한 옥타브 아래)
			for (float N : C.Voice) M.SawNote(1, T, Bar * 0.95f, N - 12.f, 0.05f, 200.f, 1400.f, Bar * 0.5f, Bar * 0.45f, 0.4f, 0.007f);
			// 로즈 불협 덩어리: 1박 + 3박 뒷박
			for (const float At : {0.f, 2.f + Sw}) { for (float N : C.Voice) M.Rhodes(1, T + At * Beat, Beat * 1.2f, N, 0.06f); M.Rhodes(1, T + At * Beat, Beat * 1.2f, C.Voice[0] + 1.f, 0.035f); }
		}
		else if (Variant == 1)
		{
			// 스윙 드럼: 라이드, 2 · 4박 발 하이햇, 1 · 3박 무거운 킥, 4박 스네어
			for (const float At : {0.f, 1.f, 1.f + Sw, 2.f, 3.f, 3.f + Sw}) M.Ride(T + At * Beat, At == 1.f + Sw || At == 3.f + Sw ? 0.05f : 0.09f);
			for (const float At : {0.f, 2.f}) { M.Kick(T + At * Beat, 0.9f); Kicks.push_back(T + At * Beat); }
			M.Snare(T + 3.f * Beat, 0.6f);
			M.Hat(T + Beat, 0.03f, 0.07f); M.Hat(T + 3.f * Beat, 0.03f, 0.07f);
			if (bTurn) for (int K = 0; K < 6; ++K) M.Tom(T + (2.f + K / 3.f) * Beat, 120.f - K * 9.f, 0.45f);
			// 2분 워킹 베이스 (낮게) + 서브
			const float Nr = Prog[(B + 1) % Bars].Root;
			for (int K = 0; K < 4; ++K)
			{
				const float N = K == 0 ? C.Root : K == 1 ? C.Root + C.Third : K == 2 ? C.Root + 7.f : Nr - 1.f;
				M.Upright(1, T + K * Beat, Beat * 0.9f, N - 12.f + 12.f * (N - 12.f < 28.f), 0.48f);
			}
			// 낮은 트롬본 느낌 금관: 1박 '쿵-' 길게, 2박 뒷박 짧게, 마지막 마디는 폴
			for (float N : C.Voice) { M.Brass(1, T, Beat * 1.6f, N - 12.f, 0.075f, false); M.Brass(1, T + (2.f + Sw) * Beat, Beat * 0.6f, N - 12.f, 0.06f, bTurn); }
		}
		else
		{
			// 둠 펑크: 킥 1 · 2박 뒷박 · 3박 뒷박, 스네어 3박 (반박자), 8분 하이햇 무겁게
			for (const int K : {0, 6, 10}) { M.Kick(T + K * S16, 1.f); Kicks.push_back(T + K * S16); }
			M.Snare(T + 2.f * Beat, 0.9f);
			for (int K = 0; K < 8; ++K) M.Hat(T + K * Beat / 2.f, 0.04f, K % 2 ? 0.05f : 0.09f);
			if (bTurn) for (int K = 0; K < 8; ++K) M.Tom(T + 2.f * Beat + K * S16, 150.f - K * 12.f, 0.5f);
			// 옥타브 떨어지는 신스 베이스 + 업라이트
			const int BPos[5] = {0, 3, 6, 10, 14};
			const float BN[5] = {C.Root, C.Root, C.Root - 12.f, C.Root + 3.f, C.Root - 1.f};
			for (int K = 0; K < 5; ++K)
			{
				M.SawNote(1, T + BPos[K] * S16, S16 * (K == 2 ? 3.f : 1.8f), BN[K], 0.36f, 900.f, 160.f, 0.08f, 0.002f, 0.04f, 0.005f);
				M.Upright(1, T + BPos[K] * S16, S16 * 1.8f, BN[K], 0.22f);
			}
			// 로즈 트레몰로 화음 (2박 길게), 낮은 금관 찌르기 (1박 · 3박 뒷박)
			for (float N : C.Voice) M.Rhodes(1, T + Beat, Beat * 2.f, N, 0.055f);
			for (const float At : {0.f, 2.5f}) for (float N : C.Voice) M.Brass(1, T + At * Beat, Beat * 0.5f, N - 12.f, 0.06f, At > 0.f);
			// 느린 경보 동기 (4분)
			for (int K = 0; K < 4; ++K) M.PulseNote(2, T + K * Beat, Beat * 0.7f, K % 2 ? C.Voice[0] + 13.f : C.Voice[0] + 12.f, 0.035f);
		}
	}
	// 선율: 0 = 비브라폰 불길한 동기, 1 · 2 = 약음 리드 낮게
	const FLead L0[] = {
		{0, 0.f, 1.5f, 74.f}, {0, 1.5f, 0.5f, 73.f}, {0, 2.f, 2.f, 69.f},
		{2, 0.f, 1.f, 70.f}, {2, 1.f, 1.f, 74.f}, {2, 2.f, 2.f, 73.f},
		{4, 0.f, 1.5f, 77.f}, {4, 1.5f, 0.5f, 76.f}, {4, 2.f, 2.f, 74.f},
		{6, 0.f, 1.f, 73.f}, {6, 1.f, 1.f, 70.f}, {6, 2.f, 2.f, 69.f}};
	const FLead L1[] = {
		{4, 0.f, 2.f, 65.f}, {4, 2.f, 1.f, 67.f}, {4, 3.f, 1.f, 70.f},
		{5, 0.f, 3.f, 69.f}, {5, 3.f, 1.f, 67.f},
		{6, 0.f, 1.f, 65.f}, {6, 1.f, 1.f, 62.f}, {6, 2.f, 2.f, 64.f},
		{7, 0.f, 4.f, 62.f},
		{8, 0.f, 2.f, 65.f}, {8, 2.f, 2.f, 69.f}, {9, 0.f, 2.f, 70.f}, {9, 2.f, 2.f, 67.f},
		{10, 0.f, 2.f, 65.f}, {10, 2.f, 2.f, 62.f}, {11, 0.f, 2.f, 61.f}, {11, 2.f, 2.f, 64.f}};
	if (Variant == 0) for (const FLead& L : L0) M.Vibes(2, L.Bar * Bar + L.At * Beat, L.Len * Beat, L.Midi, 0.15f);
	else for (const FLead& L : L1) M.MuteLead(2, L.Bar * Bar + L.At * Beat, L.Len * Beat * 0.92f, L.Midi, 0.14f);
	FMixOpt O;
	O.Duck = Variant == 1 ? 0.2f : 0.35f; O.EchoFb = 0.3f; O.EchoMix = 0.35f; O.Room = 0.3f; O.Gain = 1.05f;
	return MixDown(M, Kicks, Beat, LoopN, TailN, SampleRate, O);
}
} // namespace PD
