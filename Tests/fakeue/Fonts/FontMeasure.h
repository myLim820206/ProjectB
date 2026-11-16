// 가짜 언리얼 헤더 (Tests/syntax.sh). 내용은 FakeUE.h 에 모여 있다
#pragma once
#include "FakeUE.h"

// TrF 형식 검사 (가짜 헤더 전용): TrF(TEXT("..."), 인자...) 의 형식 글자와 인자 형이 맞는지 컴파일 때 본다 (FString::Printf 와 같은 검사).
// TrF 는 C 가변 인자라 언리얼에선 형이 틀려도 조용히 지나가고 실행 중에 깨진다 (MSVC) / clang 은 FString 을 넘기면 오류.
// PDLang.h 의 선언을 먼저 들인 뒤 매크로를 정의한다 (PDLang.cpp 의 정의는 이 헤더를 안 들이므로 그대로).
#ifndef FAKEUE_NO_TRF_CHECK
#include "PDLang.h"
namespace UE::Fake
{
template <typename... A> inline void TrFCheck(TFormatFor<A...> Fmt, const A&... Args) { (void)Fmt; ((void)Args, ...); }
}
#define TrF(Fmt, ...) (UE::Fake::TrFCheck(Fmt, ##__VA_ARGS__), ::TrF(Fmt, ##__VA_ARGS__))
#endif
