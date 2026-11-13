// 배경 음악(D-117)을 WAV 로 뽑아 들어 본다: 언리얼과 같은 합성기(PDSynth)
//   g++ -std=c++17 -O2 -o build/render_music render_music.cpp ../Source/ProjectB/PDSynth.cpp
//   ./build/render_music 출력.wav [반복 횟수]
#include "../Source/ProjectB/PDSynth.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv)
{
	const char* Out = argc > 1 ? argv[1] : "music.wav";
	const int Repeat = argc > 2 ? std::atoi(argv[2]) : 2; // 이음매를 들어 보려고 두 번 이어 붙인다
	const int Rate = 44100;
	const std::vector<int16_t> One = PD::SynthesizeMusic(Rate);
	std::vector<int16_t> S;
	for (int K = 0; K < Repeat; ++K) S.insert(S.end(), One.begin(), One.end());
	FILE* F = std::fopen(Out, "wb");
	if (!F) return 1;
	auto U32 = [&](unsigned V) { for (int K = 0; K < 4; ++K) std::fputc((V >> (8 * K)) & 255, F); };
	auto U16 = [&](unsigned V) { std::fputc(V & 255, F); std::fputc((V >> 8) & 255, F); };
	const unsigned Bytes = (unsigned)S.size() * 2;
	std::fwrite("RIFF", 1, 4, F); U32(36 + Bytes);
	std::fwrite("WAVEfmt ", 1, 8, F); U32(16); U16(1); U16(1); U32(Rate); U32(Rate * 2); U16(2); U16(16);
	std::fwrite("data", 1, 4, F); U32(Bytes);
	std::fwrite(S.data(), 2, S.size(), F);
	std::fclose(F);
	std::printf("saved %s (%.1f s, loop %.1f s)\n", Out, S.size() / (float)Rate, One.size() / (float)Rate);
	return 0;
}
