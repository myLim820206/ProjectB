// 음악 세 곡(D-264)을 WAV 로: build\rm.bat render_music3.cpp 후 build\rm.exe 출력폴더
#include "../Source/ProjectB/PDSynth.h"
#include <cstdio>
#include <cmath>
#include <vector>
static void Save(const char* Out, const std::vector<int16_t>& One, int Rep)
{
	std::vector<int16_t> S; for (int K = 0; K < Rep; ++K) S.insert(S.end(), One.begin(), One.end());
	FILE* F = std::fopen(Out, "wb");
	auto U32 = [&](unsigned V) { for (int K = 0; K < 4; ++K) std::fputc((V >> (8 * K)) & 255, F); };
	auto U16 = [&](unsigned V) { std::fputc(V & 255, F); std::fputc((V >> 8) & 255, F); };
	const unsigned Bytes = (unsigned)S.size() * 2, Rate = 44100;
	std::fwrite("RIFF", 1, 4, F); U32(36 + Bytes); std::fwrite("WAVEfmt ", 1, 8, F); U32(16); U16(1); U16(1); U32(Rate); U32(Rate * 2); U16(2); U16(16);
	std::fwrite("data", 1, 4, F); U32(Bytes); std::fwrite(S.data(), 2, S.size(), F); std::fclose(F);
	double Sq = 0; int Pk = 0, Clip = 0; for (int16_t V : One) { Sq += (double)V * V; Pk = std::max(Pk, std::abs((int)V)); if (std::abs((int)V) > 27000) ++Clip; }
	std::printf("%s %.1fs rms %.0f peak %d near-clip %d\n", Out, One.size() / 44100.0, std::sqrt(Sq / One.size()), Pk, Clip);
}
int main(int argc, char** argv)
{
	const char* D = argv[1];
	char P[512];
	std::snprintf(P, 512, "%s/music_menu.wav", D); Save(P, PD::SynthesizeMenuMusic(44100), 1);
	std::snprintf(P, 512, "%s/music_ingame.wav", D); Save(P, PD::SynthesizeMusic(44100), 1);
	std::snprintf(P, 512, "%s/music_boss.wav", D); Save(P, PD::SynthesizeBossMusic(44100), 1);
	return 0;
}
