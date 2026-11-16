// 가짜 UHT 생성 헤더 (Tests/syntax.sh): UHT 가 만드는 Super·ThisClass 만 흉내 낸다
#pragma once
#undef GENERATED_BODY
#define GENERATED_BODY(...) \
public: \
	using Super = AGameModeBase; \
	using ThisClass = APDGameMode; \
\
private:
