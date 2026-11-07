// 웹 프로토타입용 효과음 파일을 만든다: 언리얼과 같은 합성기(TPSynth)로 WAV를 만들어 base64로 묶는다
//   g++ -std=c++17 -O2 -o build/render_sounds render_sounds.cpp ../Source/ProjectB/TPSynth.cpp
//   ./build/render_sounds > ../prototype/tetris-poker/sounds.js
#include "../Source/ProjectB/TPSynth.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace TP;

static std::string Base64(const std::vector<unsigned char>& D)
{
	static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string O;
	size_t I = 0;
	for (; I + 2 < D.size(); I += 3)
	{
		const unsigned V = (D[I] << 16) | (D[I + 1] << 8) | D[I + 2];
		O += T[V >> 18]; O += T[(V >> 12) & 63]; O += T[(V >> 6) & 63]; O += T[V & 63];
	}
	if (I < D.size())
	{
		const unsigned V = (D[I] << 16) | (I + 1 < D.size() ? D[I + 1] << 8 : 0);
		O += T[V >> 18]; O += T[(V >> 12) & 63];
		O += I + 1 < D.size() ? T[(V >> 6) & 63] : '=';
		O += '=';
	}
	return O;
}

static std::vector<unsigned char> Wav(const std::vector<int16_t>& S, int Rate)
{
	std::vector<unsigned char> W;
	auto U32 = [&](unsigned V) { for (int K = 0; K < 4; ++K) W.push_back((V >> (8 * K)) & 255); };
	auto U16 = [&](unsigned V) { W.push_back(V & 255); W.push_back((V >> 8) & 255); };
	const unsigned Bytes = (unsigned)S.size() * 2;
	W.insert(W.end(), {'R', 'I', 'F', 'F'}); U32(36 + Bytes);
	W.insert(W.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '}); U32(16); U16(1); U16(1); U32(Rate); U32(Rate * 2); U16(2); U16(16);
	W.insert(W.end(), {'d', 'a', 't', 'a'}); U32(Bytes);
	for (int16_t V : S) U16((unsigned)(uint16_t)V);
	return W;
}

int main()
{
	const int Rate = 22050;
	const char* Names[] = {"Move", "Rotate", "HardDrop", "Lock", "Hold", "Denied", "Burst", "Straight", "Flush", "Clear", "GameOver", "Win", "Start"};
	std::printf("// 자동 생성 파일: Tests/render_sounds.cpp (언리얼과 같은 합성기 TPSynth)\nwindow.TP_SOUNDS = {\n");
	size_t Total = 0;
	for (int S = 0; S < (int)ETPSound::Count; ++S)
	{
		std::vector<std::pair<int, int>> Vars = {{0, 0}};
		if ((ETPSound)S == ETPSound::Burst) { Vars.clear(); for (int A = 1; A <= 8; ++A) for (int B = 0; B <= 2; ++B) Vars.push_back({A, B}); }
		if ((ETPSound)S == ETPSound::Straight) { Vars.clear(); for (int A = 3; A <= 7; ++A) Vars.push_back({A, 0}); }
		for (auto [A, B] : Vars)
		{
			const std::string B64 = Base64(Wav(SynthesizeSound((ETPSound)S, A, B, Rate), Rate));
			Total += B64.size();
			std::printf("  \"%s_%d_%d\": \"data:audio/wav;base64,%s\",\n", Names[S], A, B, B64.c_str());
		}
	}
	std::printf("};\n");
	std::fprintf(stderr, "total base64 %zu bytes\n", Total);
}
