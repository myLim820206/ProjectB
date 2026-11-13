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

	void Kick(float T0)
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
			Add(0, I0 + I, (std::sin(SynthTwoPi * Ph) * Env + Click) * 0.9f);
		}
	}
	void Snare(float T0)
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
			Add(0, I0 + I, (Hp * 1.6f * Body + Tone) * 0.55f);
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
};
} // namespace

std::vector<int16_t> SynthesizeMusic(int SampleRate)
{
	const float Bpm = 104.f, Beat = 60.f / Bpm, Bar = 4.f * Beat, S16 = Beat / 4.f;
	const int Bars = 16;
	const float Loop = Bars * Bar;
	const size_t LoopN = (size_t)(Loop * SampleRate), TailN = (size_t)(2.f * SampleRate);
	FMusic M(SampleRate, LoopN + TailN);
	// Am - F - C - G (마디마다). 베이스 근음, 패드 세 음
	struct FChord { float Bass; float Pad[3]; };
	const FChord Ch[4] = {{45.f, {57.f, 60.f, 64.f}}, {41.f, {57.f, 60.f, 65.f}}, {48.f, {55.f, 60.f, 64.f}}, {43.f, {55.f, 59.f, 62.f}}};
	std::vector<float> Kicks;
	for (int B = 0; B < Bars; ++B)
	{
		const float T = B * Bar;
		const FChord& C = Ch[B % 4];
		const bool bIntro = B < 4, bLead = B >= 12;
		// 드럼
		for (int K = 0; K < 4; ++K) { M.Kick(T + K * Beat); Kicks.push_back(T + K * Beat); }
		if (!bIntro || B >= 2) { M.Snare(T + Beat); M.Snare(T + 3.f * Beat); }
		for (int K = 0; K < 8; ++K) M.Hat(T + K * Beat / 2.f, K % 2 ? 0.06f : 0.035f, K % 2 ? 0.16f : 0.09f);
		if (B % 4 == 3) M.Hat(T + 3.5f * Beat, 0.25f, 0.13f); // 열린 하이햇
		// 베이스: 8분 근음, 옥타브 왕복
		for (int K = 0; K < 8; ++K) M.SawNote(1, T + K * Beat / 2.f, Beat / 2.f * 0.8f, C.Bass + (K % 2 ? 12.f : 0.f), 0.42f, 1400.f, 280.f, 0.06f, 0.003f, 0.02f, 0.004f);
		// 패드: 마디 내내, 천천히 열림
		for (float N : C.Pad) M.SawNote(1, T, Bar * 0.98f, N, 0.075f, 500.f, 1600.f, 0.8f, 0.25f, 0.35f, 0.006f);
		// 아르페지오: 16분, 화음 두 옥타브 오르내림 (도입부엔 없음)
		if (!bIntro)
		{
			const float Arp[8] = {C.Pad[0] + 12.f, C.Pad[1] + 12.f, C.Pad[2] + 12.f, C.Pad[0] + 24.f, C.Pad[2] + 12.f, C.Pad[1] + 12.f, C.Pad[0] + 12.f, C.Pad[1] + 12.f};
			for (int K = 0; K < 16; ++K) M.PulseNote(2, T + K * S16, S16 * 0.9f, Arp[K % 8], bLead ? 0.07f : 0.1f);
		}
	}
	// 리드 멜로디 (13~16마디): (마디 안 박, 길이 박, 음)
	struct FLeadNote { int Bar; float At, Len, Midi; };
	const FLeadNote Lead[] = {
		{12, 0.f, 1.5f, 76.f}, {12, 1.5f, 0.5f, 74.f}, {12, 2.f, 1.f, 72.f}, {12, 3.f, 1.f, 69.f},
		{13, 0.f, 1.f, 72.f}, {13, 1.f, 0.5f, 69.f}, {13, 1.5f, 0.5f, 72.f}, {13, 2.f, 2.f, 74.f},
		{14, 0.f, 1.f, 76.f}, {14, 1.f, 1.f, 79.f}, {14, 2.f, 1.f, 76.f}, {14, 3.f, 1.f, 74.f},
		{15, 0.f, 1.5f, 74.f}, {15, 1.5f, 0.5f, 71.f}, {15, 2.f, 2.f, 67.f}};
	for (const FLeadNote& L : Lead) M.SawNote(2, L.Bar * Bar + L.At * Beat, L.Len * Beat * 0.95f, L.Midi, 0.16f, 2600.f, 1800.f, 0.3f, 0.01f, 0.12f, 0.005f, 0.006f);
	// 킥 눌림(사이드체인): 눌리는 소리를 킥마다 잠깐 줄인다
	std::vector<float>& Duck = M.Bus[1];
	size_t KI = 0;
	for (size_t I = 0; I < Duck.size(); ++I)
	{
		const float T = (float)I / SampleRate;
		while (KI + 1 < Kicks.size() && Kicks[KI + 1] <= T) ++KI;
		const float Since = Kicks.empty() || Kicks[KI] > T ? 1.f : T - Kicks[KI];
		Duck[I] *= 1.f - 0.65f * std::exp(-Since / 0.11f);
	}
	// 메아리 (점 8분, 되먹임 0.35)
	std::vector<float>& Send = M.Bus[2];
	const size_t D = (size_t)(Beat * 0.75f * SampleRate);
	std::vector<float> Wet(Send.size(), 0.f);
	for (size_t I = D; I < Send.size(); ++I) Wet[I] = (Send[I - D] + Wet[I - D]) * 0.35f;
	// 섞기 + 끝에서 넘친 꼬리를 처음에 더해 이음매 없이 반복
	std::vector<float> Mix(LoopN, 0.f);
	for (size_t I = 0; I < LoopN + TailN; ++I)
	{
		const float V = M.Bus[0][I] * 0.8f + M.Bus[1][I] + Send[I] + Wet[I] * 0.6f;
		Mix[I % LoopN] += V;
	}
	std::vector<int16_t> Out(LoopN);
	for (size_t I = 0; I < LoopN; ++I) Out[I] = (int16_t)std::lround(std::tanh(Mix[I] * 0.9f) * 28000.f);
	return Out;
}
} // namespace PD
