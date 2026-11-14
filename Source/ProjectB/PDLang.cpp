#include "PDLang.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include <cstdarg>

namespace
{
#include "PDLangTable.inl" // 생성됨: tools/lang/gen_lang.py (PDLangRows[][5], PDLangRowCount)

const TMap<FString, int32>& LangIndex()
{
	static TMap<FString, int32> Index;
	if (Index.Num() == 0)
	{
		Index.Reserve(PDLangRowCount);
		for (int32 I = 0; I < PDLangRowCount; ++I) Index.Add(PDLangRows[I][0], I);
	}
	return Index;
}

const TCHAR* Lookup(const FString& Ko)
{
	if (GPDLang <= 0 || GPDLang >= (int32)EPDLang::Count) return nullptr;
	const int32* Row = LangIndex().Find(Ko);
	if (!Row) return nullptr;
	const TCHAR* S = PDLangRows[*Row][GPDLang];
	return S && S[0] ? S : nullptr;
}
} // namespace

const TCHAR* Tr(const TCHAR* Ko)
{
	if (!Ko || GPDLang <= 0) return Ko;
	const TCHAR* S = Lookup(FString(Ko));
	return S ? S : Ko;
}

FString TrS(const FString& Ko)
{
	if (GPDLang <= 0 || Ko.IsEmpty()) return Ko;
	const TCHAR* S = Lookup(Ko);
	return S ? FString(S) : Ko;
}

FString TrF(const TCHAR* KoFmt, ...)
{
	TCHAR Buf[4096];
	const TCHAR* Fmt = Tr(KoFmt);
	va_list Ap;
	va_start(Ap, KoFmt);
	FCString::GetVarArgs(Buf, UE_ARRAY_COUNT(Buf), Fmt, Ap);
	va_end(Ap);
	return FString(Buf);
}

const TCHAR* LangNativeName(int32 L)
{
	static const TCHAR* N[] = {TEXT("한국어"), TEXT("English"), TEXT("日本語"), TEXT("简体中文"), TEXT("繁體中文")};
	return N[FMath::Clamp(L, 0, (int32)EPDLang::Count - 1)];
}

int32 LangFromSystem()
{
	const FCultureRef C = FInternationalization::Get().GetCurrentLanguage();
	const FString Two = C->GetTwoLetterISOLanguageName(), Name = C->GetName();
	if (Two == TEXT("ko")) return (int32)EPDLang::Ko;
	if (Two == TEXT("ja")) return (int32)EPDLang::Ja;
	if (Two == TEXT("zh"))
	{
		const bool bHant = Name.Contains(TEXT("Hant")) || Name.EndsWith(TEXT("TW")) || Name.EndsWith(TEXT("HK")) || Name.EndsWith(TEXT("MO"));
		return (int32)(bHant ? EPDLang::ZhHant : EPDLang::ZhHans);
	}
	return (int32)EPDLang::En;
}
