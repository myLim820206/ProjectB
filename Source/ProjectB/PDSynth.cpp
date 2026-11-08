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
	default:
		break;
	}
	return Buf.ToPcm(Gain);
}
} // namespace PD
