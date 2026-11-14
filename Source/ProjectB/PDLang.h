#pragma once

// 언어팩 (D-159): 한국어 원문을 키로 번역을 찾는다 (gettext 방식).
// 번역 표는 tools/lang/strings.tsv → tools/lang/gen_lang.py → PDLangTable.inl (자동 생성).
// 화면 글자는 FCanvas::Text 등이 그릴 때 Tr 를 거치고, Printf 형식 글자는 TrF 로 형식을 먼저 번역한다.
// 번역이 없거나 비어 있으면 원문(한국어) 그대로.
#include "CoreMinimal.h"

enum class EPDLang : uint8 { Ko, En, Ja, ZhHans, ZhHant, Count };

inline int32 GPDLang = 0; // EPDLang

const TCHAR* Tr(const TCHAR* Ko);       // 원문 → 지금 언어 (표의 글자를 가리킨다, 없으면 원문)
FString TrS(const FString& Ko);         // FString 판
FString TrF(const TCHAR* KoFmt, ...);   // 형식 글자를 번역한 뒤 printf (인자 순서는 원문과 같아야 한다)
const TCHAR* LangNativeName(int32 L);   // 한국어, English, 日本語, 简体中文, 繁體中文
int32 LangFromSystem();                 // 처음 켤 때: 시스템 언어로 고른다
