// 칵테일 셰이커 흔들기 · 따르기 소리를 wav 로 (D-306): build\rs.exe 출력폴더 → shake.wav, pour.wav, shake_pour.wav
#include "../Source/ProjectB/PDSynth.h"
#include <cstdio>
#include <string>
#include <vector>
using namespace PD;
static void SaveWav(const std::string& Path, const std::vector<int16_t>& S, int Rate)
{
	FILE* F = std::fopen(Path.c_str(), "wb");
	if (!F) return;
	auto U32 = [&](unsigned V) { std::fwrite(&V, 4, 1, F); };
	auto U16 = [&](unsigned short V) { std::fwrite(&V, 2, 1, F); };
	const unsigned Bytes = (unsigned)(S.size() * 2);
	std::fwrite("RIFF", 1, 4, F); U32(36 + Bytes); std::fwrite("WAVEfmt ", 1, 8, F);
	U32(16); U16(1); U16(1); U32(Rate); U32(Rate * 2); U16(2); U16(16);
	std::fwrite("data", 1, 4, F); U32(Bytes); std::fwrite(S.data(), 2, S.size(), F);
	std::fclose(F);
}
int main(int argc, char** argv)
{
	const std::string Dir = argc > 1 ? argv[1] : ".";
	const int Rate = 44100;
	const auto Sh = SynthesizeSound(EPDSound::Shake, 0, 0, Rate), Po = SynthesizeSound(EPDSound::Pour, 0, 0, Rate);
	SaveWav(Dir + "/shake.wav", Sh, Rate);
	SaveWav(Dir + "/pour.wav", Po, Rate);
	std::vector<int16_t> Both(Sh); // 게임처럼: 흔들기 → 0.3초 쉼(기울이기) → 따르기
	Both.resize((size_t)(1.5 * Rate), 0);
	Both.insert(Both.end(), Po.begin(), Po.end());
	SaveWav(Dir + "/shake_pour.wav", Both, Rate);
	std::printf("shake %.2fs, pour %.2fs\n", Sh.size() / (double)Rate, Po.size() / (double)Rate);
}
