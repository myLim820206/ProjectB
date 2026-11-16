// 인게임 재즈 4곡 (D-319)을 WAV 로 뽑아 들어 본다: 언리얼과 같은 합성기(PDSynth)
//   g++ -std=c++17 -O2 -o build/render_jazz render_jazz.cpp ../Source/ProjectB/PDSynth.cpp
//   ./build/render_jazz 출력폴더   → jazz_0.wav ~ jazz_3.wav
#include "../Source/ProjectB/PDSynth.h"

#include <cstdio>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
	const std::string Dir = argc > 1 ? argv[1] : ".";
	const int Rate = 44100;
	for (int T = 0; T < PD::JazzTrackCount; ++T)
	{
		const std::vector<int16_t> S = PD::SynthesizeJazz(T, Rate);
		const std::string Out = Dir + "/jazz_" + std::to_string(T) + ".wav";
		FILE* F = std::fopen(Out.c_str(), "wb");
		if (!F) return 1;
		auto U32 = [&](unsigned V) { for (int K = 0; K < 4; ++K) std::fputc((V >> (8 * K)) & 255, F); };
		auto U16 = [&](unsigned V) { std::fputc(V & 255, F); std::fputc((V >> 8) & 255, F); };
		const unsigned Bytes = (unsigned)S.size() * 2;
		std::fwrite("RIFF", 1, 4, F); U32(36 + Bytes);
		std::fwrite("WAVEfmt ", 1, 8, F); U32(16); U16(1); U16(1); U32(Rate); U32(Rate * 2); U16(2); U16(16);
		std::fwrite("data", 1, 4, F); U32(Bytes);
		std::fwrite(S.data(), 2, S.size(), F);
		std::fclose(F);
		int Peak = 0; for (int16_t V : S) Peak = std::max(Peak, (int)(V < 0 ? -V : V));
		std::printf("%s  %-20s %.0f BPM  %.1f s  peak %d\n", Out.c_str(), PD::JazzTrackName(T), PD::JazzTrackBpm(T), S.size() / (float)Rate, Peak);
	}
	return 0;
}
