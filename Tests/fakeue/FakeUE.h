// 가짜 언리얼 헤더 (문법·형 검사 전용, Tests/syntax.sh)
// 엔진 없이 g++ -fsyntax-only 로 Slate 화면 코드를 검사하려고, 우리 코드가 실제로 쓰는 언리얼 API 만 흉내 낸다.
// 동작은 상관없고 선언·형만 맞으면 된다. 단, 우리 코드의 진짜 실수(인자 수, 없는 이름, 가림, 형 불일치)는
// 잡히도록 "아무거나 받는" 템플릿은 되도록 쓰지 않는다. 언리얼에 없는 API 를 여기 넣지 말 것.
// 새 언리얼 API 를 쓰기 시작해 검사가 깨지면, 실제 엔진 시그니처를 보고 여기에 추가한다.
#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ---------------------------------------------------------------- 빌드 설정
#ifndef WITH_EDITOR
#define WITH_EDITOR 1
#endif
#ifndef UE_BUILD_SHIPPING
#define UE_BUILD_SHIPPING 0
#endif
#define WITH_EDITORONLY_DATA WITH_EDITOR
#define PROJECTB_API
#define FORCEINLINE inline
#define FORCENOINLINE

// ---------------------------------------------------------------- 기본 형
using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;
using SIZE_T = std::size_t;
using SSIZE_T = std::ptrdiff_t;
using ANSICHAR = char;
using UTF8CHAR = char8_t;
using TCHAR = wchar_t;
#define TEXT_PASTE(x) L##x
#define TEXT(x) TEXT_PASTE(x)

#define INDEX_NONE (-1)
#define MAX_int32 (std::numeric_limits<int32>::max())
#define MIN_int32 (std::numeric_limits<int32>::min())
#define MAX_uint32 (std::numeric_limits<uint32>::max())
#define MAX_flt (3.402823466e+38F)
#define UE_PI (3.1415926535897932f)
#define UE_TWO_PI (6.28318530717958647692f)
#define UE_HALF_PI (1.57079632679489661923f)
#define UE_DOUBLE_PI (3.141592653589793238462643383279502884197169399)
#define UE_SMALL_NUMBER (1.e-8f)
#define UE_KINDA_SMALL_NUMBER (1.e-4f)
#define UE_BIG_NUMBER (3.4e+38f)
#define PI UE_PI
#define TWO_PI UE_TWO_PI
#define HALF_PI UE_HALF_PI
#define SMALL_NUMBER UE_SMALL_NUMBER
#define KINDA_SMALL_NUMBER UE_KINDA_SMALL_NUMBER
#define BIG_NUMBER UE_BIG_NUMBER

template <typename T, uint32 N> char (&UEArrayCountHelper(const T (&)[N]))[N + 1];
#define UE_ARRAY_COUNT(A) (sizeof(UEArrayCountHelper(A)) - 1)

template <typename T> constexpr std::remove_reference_t<T>&& MoveTemp(T&& V)
{
	static_assert(!std::is_const_v<std::remove_reference_t<T>>, "MoveTemp called on a const object");
	static_assert(std::is_lvalue_reference_v<T>, "MoveTemp called on an rvalue");
	return static_cast<std::remove_reference_t<T>&&>(V);
}
template <typename T> constexpr T&& Forward(std::remove_reference_t<T>& V) { return static_cast<T&&>(V); }
template <typename T> inline void Swap(T& A, T& B) { std::swap(A, B); }

template <typename T> struct TNumericLimits
{
	static constexpr T Min() { return std::numeric_limits<T>::min(); }
	static constexpr T Max() { return std::numeric_limits<T>::max(); }
	static constexpr T Lowest() { return std::numeric_limits<T>::lowest(); }
};

#define check(x) ((void)(x))
#define checkf(x, ...) ((void)(x))
#define checkNoEntry() ((void)0)
#define verify(x) ((void)(x))
#define ensure(x) (!!(x))
#define ensureMsgf(x, ...) (!!(x))

// ON_SCOPE_EXIT { ... };
template <typename F> struct TFakeScopeGuard { F Fn; ~TFakeScopeGuard() { Fn(); } };
struct FFakeScopeGuardSyntax { template <typename F> TFakeScopeGuard<F> operator+(F&& Fn) { return TFakeScopeGuard<F>{std::forward<F>(Fn)}; } };
#define UE_FAKE_CAT2(A, B) A##B
#define UE_FAKE_CAT(A, B) UE_FAKE_CAT2(A, B)
#define ON_SCOPE_EXIT const auto UE_FAKE_CAT(ScopeGuard_, __LINE__) = FFakeScopeGuardSyntax() + [&]()

// ---------------------------------------------------------------- 형식 문자열 검사 (UE 5.2+ 의 컴파일 시간 Printf 검사 흉내)
namespace UE::Fake
{
enum class EArgKind { Int, Int64, Float, Str, Ptr, Bad };
template <typename T> constexpr EArgKind ArgKind()
{
	using D = std::decay_t<T>;
	if constexpr (std::is_same_v<D, bool>) return EArgKind::Int;
	else if constexpr (std::is_integral_v<D> || std::is_enum_v<D>) return sizeof(D) == 8 ? EArgKind::Int64 : EArgKind::Int;
	else if constexpr (std::is_floating_point_v<D>) return EArgKind::Float;
	else if constexpr (std::is_same_v<D, const TCHAR*> || std::is_same_v<D, TCHAR*>) return EArgKind::Str;
	else if constexpr (std::is_pointer_v<D> || std::is_null_pointer_v<D>) return EArgKind::Ptr;
	else return EArgKind::Bad;
}
inline void FormatError(const char*) {} // consteval 안에서 부르면 컴파일 오류 (메시지가 오류에 찍힘)

template <typename... A> struct TFormat
{
	const TCHAR* Str;
	template <std::size_t N> consteval TFormat(const TCHAR (&S)[N]) : Str(S)
	{
		constexpr EArgKind Kinds[] = {ArgKind<A>()..., EArgKind::Bad};
		constexpr std::size_t NA = sizeof...(A);
		for (std::size_t K = 0; K < NA; ++K)
			if (Kinds[K] == EArgKind::Bad) FormatError("Printf/UE_LOG argument is not a number, enum or TCHAR pointer (FString needs *)");
		std::size_t Arg = 0;
		for (std::size_t I = 0; I + 1 < N; ++I)
		{
			if (S[I] != L'%') continue;
			++I;
			if (I + 1 >= N) { FormatError("format string ends with %"); break; }
			if (S[I] == L'%') continue;
			while (S[I] == L'-' || S[I] == L'+' || S[I] == L' ' || S[I] == L'#' || S[I] == L'0') ++I;
			if (S[I] == L'*') { if (Arg >= NA || (Kinds[Arg] != EArgKind::Int)) FormatError("* width needs an int argument"); ++Arg; ++I; }
			while (S[I] >= L'0' && S[I] <= L'9') ++I;
			if (S[I] == L'.')
			{
				++I;
				if (S[I] == L'*') { if (Arg >= NA || (Kinds[Arg] != EArgKind::Int)) FormatError("* precision needs an int argument"); ++Arg; ++I; }
				while (S[I] >= L'0' && S[I] <= L'9') ++I;
			}
			int Longs = 0;
			while (S[I] == L'l' || S[I] == L'h' || S[I] == L'z' || S[I] == L'I' || S[I] == L'j' || S[I] == L'6' || S[I] == L'4' || S[I] == L't')
			{
				if (S[I] == L'l') ++Longs;
				if (S[I] == L'z' || S[I] == L'j' || S[I] == L't') Longs = 2;
				if (S[I] == L'I' && S[I + 1] == L'6' && S[I + 2] == L'4') Longs = 2;
				++I;
			}
			if (Arg >= NA) { FormatError("too few arguments for format string"); break; }
			const EArgKind K = Kinds[Arg++];
			const TCHAR C = S[I];
			if (C == L'd' || C == L'i' || C == L'u' || C == L'x' || C == L'X' || C == L'o' || C == L'c')
			{
				if (K != EArgKind::Int && K != EArgKind::Int64) FormatError("%d/%u/%x/%c needs an integer argument");
				else if (Longs >= 2 && K != EArgKind::Int64) FormatError("%lld/%llu needs a 64-bit integer argument");
				else if (Longs < 2 && K == EArgKind::Int64 && C != L'c') FormatError("64-bit integer passed to %d/%u/%x (use %lld/%llu or cast)");
			}
			else if (C == L'f' || C == L'F' || C == L'g' || C == L'G' || C == L'e' || C == L'E' || C == L'a')
			{
				if (K != EArgKind::Float) FormatError("%f/%g/%e needs a floating point argument");
			}
			else if (C == L's' || C == L'S')
			{
				if (K != EArgKind::Str) FormatError("%s needs a const TCHAR* argument (FString needs *)");
			}
			else if (C == L'p')
			{
				if (K != EArgKind::Ptr && K != EArgKind::Str) FormatError("%p needs a pointer argument");
			}
			else FormatError("unknown format specifier");
		}
		if (Arg != NA) FormatError("too many arguments for format string");
	}
};
template <typename... A> using TFormatFor = std::type_identity_t<TFormat<std::decay_t<A>...>>;
} // namespace UE::Fake

// ---------------------------------------------------------------- 문자 도우미
struct FCString
{
	static int32 Strlen(const TCHAR* S) { return (int32)std::wcslen(S); }
	static int32 Strcmp(const TCHAR* A, const TCHAR* B) { return std::wcscmp(A, B); }
	static int32 Stricmp(const TCHAR* A, const TCHAR* B) { return std::wcscmp(A, B); }
	static int32 Strncmp(const TCHAR* A, const TCHAR* B, SIZE_T N) { return std::wcsncmp(A, B, N); }
	static const TCHAR* Strstr(const TCHAR* A, const TCHAR* B) { return std::wcsstr(A, B); }
	static const TCHAR* Strchr(const TCHAR* A, TCHAR C) { return std::wcschr(A, C); }
	static int32 Atoi(const TCHAR* S) { return (int32)std::wcstol(S, nullptr, 10); }
	static int64 Atoi64(const TCHAR* S) { return std::wcstoll(S, nullptr, 10); }
	static int64 Strtoi64(const TCHAR* S, TCHAR** End, int32 Base) { return std::wcstoll(S, End, Base); }
	static float Atof(const TCHAR* S) { return std::wcstof(S, nullptr); }
	static double Atod(const TCHAR* S) { return std::wcstod(S, nullptr); }
	static bool ToBool(const TCHAR* S) { return Atoi(S) != 0; }
	static uint64 Strtoui64(const TCHAR* S, TCHAR** End, int32 Base) { return std::wcstoull(S, End, Base); }
	static int32 Strtoi(const TCHAR* S, TCHAR** End, int32 Base) { return (int32)std::wcstol(S, End, Base); }
	static int32 GetVarArgs(TCHAR* Dest, SIZE_T DestSize, const TCHAR*& Fmt, va_list ArgPtr) { return std::vswprintf(Dest, DestSize, Fmt, ArgPtr); }
	static TCHAR* Strcpy(TCHAR* D, SIZE_T N, const TCHAR* S) { std::wcsncpy(D, S, N); return D; }
};
struct FChar
{
	static bool IsDigit(TCHAR C) { return C >= L'0' && C <= L'9'; }
	static bool IsAlpha(TCHAR C) { return (C >= L'a' && C <= L'z') || (C >= L'A' && C <= L'Z'); }
	static bool IsAlnum(TCHAR C) { return IsDigit(C) || IsAlpha(C); }
	static bool IsWhitespace(TCHAR C) { return C == L' ' || C == L'\t' || C == L'\n' || C == L'\r'; }
	static bool IsUpper(TCHAR C) { return C >= L'A' && C <= L'Z'; }
	static bool IsLower(TCHAR C) { return C >= L'a' && C <= L'z'; }
	static TCHAR ToUpper(TCHAR C) { return IsLower(C) ? (TCHAR)(C - 32) : C; }
	static TCHAR ToLower(TCHAR C) { return IsUpper(C) ? (TCHAR)(C + 32) : C; }
};

// ---------------------------------------------------------------- 컨테이너
enum class EAllowShrinking : uint8 { No, Yes, Default = Yes };
template <int32 N> struct TInlineAllocator {};
struct FDefaultAllocator {};

namespace UE::Fake
{
struct FBoolBox // TArray<bool> 저장용 (std::vector<bool> 은 bool& 를 못 준다)
{
	bool B = false;
	FBoolBox() = default;
	FBoolBox(bool In) : B(In) {}
};
template <typename T> using TStore = std::conditional_t<std::is_same_v<T, bool>, FBoolBox, T>;
} // namespace UE::Fake

template <typename T, typename Allocator = FDefaultAllocator> class TArray
{
	using S = UE::Fake::TStore<T>;
	static_assert(sizeof(S) == sizeof(T));
	std::vector<S> V;
	T* D() { return reinterpret_cast<T*>(V.data()); }
	const T* D() const { return reinterpret_cast<const T*>(V.data()); }

public:
	using ElementType = T;
	TArray() = default;
	TArray(std::initializer_list<T> L) : V(L.begin(), L.end()) {}
	TArray(const T* Ptr, int32 Count) : V(Ptr, Ptr + Count) {}
	template <typename OtherAlloc> explicit TArray(const TArray<T, OtherAlloc>& O) : V(O.begin(), O.end()) {}
	TArray(const TArray&) = default;
	TArray(TArray&&) = default;
	TArray& operator=(const TArray&) = default;
	TArray& operator=(TArray&&) = default;
	TArray& operator=(std::initializer_list<T> L) { V.assign(L.begin(), L.end()); return *this; }

	int32 Num() const { return (int32)V.size(); }
	bool IsEmpty() const { return V.empty(); }
	bool IsValidIndex(int32 I) const { return I >= 0 && I < Num(); }
	T* GetData() { return D(); }
	const T* GetData() const { return D(); }
	SIZE_T GetTypeSize() const { return sizeof(T); }
	T& operator[](int32 I) { return D()[I]; }
	const T& operator[](int32 I) const { return D()[I]; }
	T& Last(int32 FromEnd = 0) { return D()[Num() - 1 - FromEnd]; }
	const T& Last(int32 FromEnd = 0) const { return D()[Num() - 1 - FromEnd]; }
	T& Top() { return Last(); }
	const T& Top() const { return Last(); }
	T Pop(EAllowShrinking = EAllowShrinking::Default) { T R = std::move(Last()); V.pop_back(); return R; }
	void Push(const T& X) { V.push_back(X); }

	int32 Add(const T& X) { V.push_back(X); return Num() - 1; }
	int32 Add(T&& X) { V.push_back(std::move(X)); return Num() - 1; }
	int32 AddUnique(const T& X) { const int32 I = Find(X); return I != INDEX_NONE ? I : Add(X); }
	template <typename... A> int32 Emplace(A&&... Args) { V.emplace_back(std::forward<A>(Args)...); return Num() - 1; }
	template <typename... A> T& Emplace_GetRef(A&&... Args) { V.emplace_back(std::forward<A>(Args)...); return Last(); }
	T& Add_GetRef(const T& X) { V.push_back(X); return Last(); }
	int32 AddDefaulted(int32 N = 1) { const int32 I = Num(); V.resize(V.size() + (SIZE_T)N); return I; }
	T& AddDefaulted_GetRef() { V.emplace_back(); return Last(); }
	int32 AddZeroed(int32 N = 1) { return AddDefaulted(N); }
	int32 AddUninitialized(int32 N = 1) { return AddDefaulted(N); }
	template <typename OA> void Append(const TArray<T, OA>& O) { V.insert(V.end(), O.begin(), O.end()); }
	void Append(std::initializer_list<T> L) { V.insert(V.end(), L.begin(), L.end()); }
	void Append(const T* Ptr, int32 Count) { V.insert(V.end(), Ptr, Ptr + Count); }
	int32 Insert(const T& X, int32 I) { V.insert(V.begin() + I, S(X)); return I; }
	void InsertDefaulted(int32 I, int32 N = 1) { V.insert(V.begin() + I, (SIZE_T)N, S()); }
	void RemoveAt(int32 I, int32 Count = 1, EAllowShrinking = EAllowShrinking::Default) { V.erase(V.begin() + I, V.begin() + I + Count); }
	void RemoveAtSwap(int32 I, int32 Count = 1, EAllowShrinking = EAllowShrinking::Default) { V.erase(V.begin() + I, V.begin() + I + Count); }
	template <typename P> int32 RemoveAll(P Pred) { int32 N = 0; for (int32 I = Num() - 1; I >= 0; --I) if (Pred((*this)[I])) { RemoveAt(I); ++N; } return N; }
	template <typename P> int32 RemoveAllSwap(P Pred) { return RemoveAll(Pred); }
	int32 Remove(const T& X) { return RemoveAll([&X](const T& E) { return E == X; }); }
	int32 RemoveSingle(const T& X) { const int32 I = Find(X); if (I != INDEX_NONE) RemoveAt(I); return I != INDEX_NONE ? 1 : 0; }
	int32 RemoveSwap(const T& X) { return Remove(X); }
	void Reset(int32 NewSize = 0) { V.clear(); V.reserve((SIZE_T)NewSize); }
	void Empty(int32 Slack = 0) { V.clear(); V.reserve((SIZE_T)Slack); }
	void Reserve(int32 N) { V.reserve((SIZE_T)N); }
	void Shrink() { V.shrink_to_fit(); }
	void SetNum(int32 N, EAllowShrinking = EAllowShrinking::Default) { V.resize((SIZE_T)N); }
	void SetNumZeroed(int32 N, EAllowShrinking = EAllowShrinking::Default) { V.resize((SIZE_T)N); }
	void SetNumUninitialized(int32 N, EAllowShrinking = EAllowShrinking::Default) { V.resize((SIZE_T)N); }
	void Init(const T& X, int32 N) { V.assign((SIZE_T)N, S(X)); }
	int32 Find(const T& X) const { for (int32 I = 0; I < Num(); ++I) if ((*this)[I] == X) return I; return INDEX_NONE; }
	bool Find(const T& X, int32& Out) const { Out = Find(X); return Out != INDEX_NONE; }
	int32 FindLast(const T& X) const { for (int32 I = Num() - 1; I >= 0; --I) if ((*this)[I] == X) return I; return INDEX_NONE; }
	template <typename K> int32 IndexOfByKey(const K& Key) const { for (int32 I = 0; I < Num(); ++I) if ((*this)[I] == Key) return I; return INDEX_NONE; }
	template <typename P> int32 IndexOfByPredicate(P Pred) const { for (int32 I = 0; I < Num(); ++I) if (Pred((*this)[I])) return I; return INDEX_NONE; }
	template <typename P> T* FindByPredicate(P Pred) { for (T& X : *this) if (Pred(X)) return &X; return nullptr; }
	template <typename P> const T* FindByPredicate(P Pred) const { for (const T& X : *this) if (Pred(X)) return &X; return nullptr; }
	template <typename K> T* FindByKey(const K& Key) { for (T& X : *this) if (X == Key) return &X; return nullptr; }
	template <typename P> TArray FilterByPredicate(P Pred) const { TArray R; for (const T& X : *this) if (Pred(X)) R.Add(X); return R; }
	bool Contains(const T& X) const { return Find(X) != INDEX_NONE; }
	template <typename P> bool ContainsByPredicate(P Pred) const { return IndexOfByPredicate(Pred) != INDEX_NONE; }
	void Sort() { std::sort(begin(), end()); }
	template <typename P> void Sort(P Pred) { std::sort(begin(), end(), Pred); }
	void StableSort() { std::stable_sort(begin(), end()); }
	template <typename P> void StableSort(P Pred) { std::stable_sort(begin(), end(), Pred); }
	void Swap(int32 A, int32 B) { std::swap((*this)[A], (*this)[B]); }
	bool operator==(const TArray& O) const { return std::equal(begin(), end(), O.begin(), O.end()); }
	bool operator!=(const TArray& O) const { return !(*this == O); }

	T* begin() { return D(); }
	T* end() { return D() + Num(); }
	const T* begin() const { return D(); }
	const T* end() const { return D() + Num(); }
};

template <typename K, typename V> struct TPair
{
	K Key;
	V Value;
	TPair() = default;
	TPair(const K& InKey, const V& InValue) : Key(InKey), Value(InValue) {}
	bool operator==(const TPair& O) const { return Key == O.Key && Value == O.Value; }
	const K& Get(std::integral_constant<int, 0>) const { return Key; }
};
template <typename K, typename V> TPair<K, V> MakeTuple(const K& A, const V& B) { return TPair<K, V>(A, B); }

template <typename K, typename V> class TMap
{
	std::vector<TPair<K, V>> P;
	int32 IndexOf(const K& Key) const { for (SIZE_T I = 0; I < P.size(); ++I) if (P[I].Key == Key) return (int32)I; return INDEX_NONE; }

public:
	using ElementType = TPair<K, V>;
	int32 Num() const { return (int32)P.size(); }
	bool IsEmpty() const { return P.empty(); }
	V& Add(const K& Key, const V& Value) { const int32 I = IndexOf(Key); if (I != INDEX_NONE) { P[(SIZE_T)I].Value = Value; return P[(SIZE_T)I].Value; } P.push_back(TPair<K, V>(Key, Value)); return P.back().Value; }
	V& Add(const K& Key, V&& Value) { const int32 I = IndexOf(Key); if (I != INDEX_NONE) { P[(SIZE_T)I].Value = std::move(Value); return P[(SIZE_T)I].Value; } P.push_back(TPair<K, V>(Key, V())); P.back().Value = std::move(Value); return P.back().Value; }
	V& Add(const K& Key) { return FindOrAdd(Key); }
	template <typename... A> V& Emplace(const K& Key, A&&... Args) { return Add(Key, V(std::forward<A>(Args)...)); }
	V* Find(const K& Key) { const int32 I = IndexOf(Key); return I == INDEX_NONE ? nullptr : &P[(SIZE_T)I].Value; }
	const V* Find(const K& Key) const { const int32 I = IndexOf(Key); return I == INDEX_NONE ? nullptr : &P[(SIZE_T)I].Value; }
	V& FindOrAdd(const K& Key) { if (V* X = Find(Key)) return *X; P.push_back(TPair<K, V>(Key, V())); return P.back().Value; }
	V& FindOrAdd(const K& Key, const V& Default) { if (V* X = Find(Key)) return *X; P.push_back(TPair<K, V>(Key, Default)); return P.back().Value; }
	V& FindChecked(const K& Key) { return *Find(Key); }
	const V& FindChecked(const K& Key) const { return *Find(Key); }
	V FindRef(const K& Key) const { const V* X = Find(Key); return X ? *X : V(); }
	V FindRef(const K& Key, const V& Default) const { const V* X = Find(Key); return X ? *X : Default; }
	const K* FindKey(const V& Value) const { for (const auto& E : P) if (E.Value == Value) return &E.Key; return nullptr; }
	bool Contains(const K& Key) const { return IndexOf(Key) != INDEX_NONE; }
	int32 Remove(const K& Key) { const int32 I = IndexOf(Key); if (I == INDEX_NONE) return 0; P.erase(P.begin() + I); return 1; }
	bool RemoveAndCopyValue(const K& Key, V& Out) { const int32 I = IndexOf(Key); if (I == INDEX_NONE) return false; Out = P[(SIZE_T)I].Value; P.erase(P.begin() + I); return true; }
	void Empty(int32 Slack = 0) { P.clear(); P.reserve((SIZE_T)Slack); }
	void Reset() { P.clear(); }
	void Reserve(int32 N) { P.reserve((SIZE_T)N); }
	V& operator[](const K& Key) { return FindChecked(Key); }
	const V& operator[](const K& Key) const { return FindChecked(Key); }
	int32 GetKeys(TArray<K>& Out) const { Out.Reset(); for (const auto& E : P) Out.Add(E.Key); return Out.Num(); }
	void GenerateKeyArray(TArray<K>& Out) const { GetKeys(Out); }
	void GenerateValueArray(TArray<V>& Out) const { Out.Reset(); for (const auto& E : P) Out.Add(E.Value); }
	template <typename Pr> void KeySort(Pr Pred) { std::sort(P.begin(), P.end(), [&](const auto& A, const auto& B) { return Pred(A.Key, B.Key); }); }
	template <typename Pr> void ValueSort(Pr Pred) { std::sort(P.begin(), P.end(), [&](const auto& A, const auto& B) { return Pred(A.Value, B.Value); }); }

	template <bool bConst> class TIter
	{
		using MapT = std::conditional_t<bConst, const TMap, TMap>;
		MapT* M;
		SIZE_T I = 0;
		bool bRemoved = false;

	public:
		explicit TIter(MapT& InMap) : M(&InMap) {}
		explicit operator bool() const { return I < M->P.size(); }
		TIter& operator++() { if (!bRemoved) ++I; bRemoved = false; return *this; }
		const K& Key() const { return M->P[I].Key; }
		std::conditional_t<bConst, const V&, V&> Value() const { return M->P[I].Value; }
		auto& operator*() const { return M->P[I]; }
		auto* operator->() const { return &M->P[I]; }
		void RemoveCurrent() { static_assert(!bConst, "RemoveCurrent on const iterator"); M->P.erase(M->P.begin() + (std::ptrdiff_t)I); bRemoved = true; }
	};
	TIter<false> CreateIterator() { return TIter<false>(*this); }
	TIter<true> CreateConstIterator() const { return TIter<true>(*this); }

	auto begin() { return P.begin(); }
	auto end() { return P.end(); }
	auto begin() const { return P.begin(); }
	auto end() const { return P.end(); }
};

template <typename T> class TSet
{
	std::vector<T> E;

public:
	TSet() = default;
	TSet(std::initializer_list<T> L) { for (const T& X : L) Add(X); }
	int32 Num() const { return (int32)E.size(); }
	bool IsEmpty() const { return E.empty(); }
	void Add(const T& X, bool* bAlreadyInSet = nullptr) { const bool bHas = Contains(X); if (bAlreadyInSet) *bAlreadyInSet = bHas; if (!bHas) E.push_back(X); }
	bool Contains(const T& X) const { return std::find(E.begin(), E.end(), X) != E.end(); }
	const T* Find(const T& X) const { auto It = std::find(E.begin(), E.end(), X); return It == E.end() ? nullptr : &*It; }
	int32 Remove(const T& X) { const auto N0 = E.size(); E.erase(std::remove(E.begin(), E.end(), X), E.end()); return (int32)(N0 - E.size()); }
	void Empty(int32 Slack = 0) { E.clear(); E.reserve((SIZE_T)Slack); }
	void Reset() { E.clear(); }
	void Reserve(int32 N) { E.reserve((SIZE_T)N); }
	void Append(const TArray<T>& A) { for (const T& X : A) Add(X); }
	void Append(const TSet& S) { for (const T& X : S) Add(X); }
	TArray<T> Array() const { TArray<T> R; for (const T& X : E) R.Add(X); return R; }
	auto begin() const { return E.begin(); }
	auto end() const { return E.end(); }
};

template <typename T> class TOptional
{
	bool bSet = false;
	T Val{};

public:
	TOptional() = default;
	TOptional(const T& X) : bSet(true), Val(X) {}
	bool IsSet() const { return bSet; }
	explicit operator bool() const { return bSet; }
	const T& GetValue() const { return Val; }
	T& GetValue() { return Val; }
	const T& Get(const T& Default) const { return bSet ? Val : Default; }
	void Reset() { bSet = false; }
	T& Emplace(const T& X) { bSet = true; Val = X; return Val; }
	const T* operator->() const { return &Val; }
	const T& operator*() const { return Val; }
};

template <typename Sig> using TFunction = std::function<Sig>;
template <typename Sig> using TUniqueFunction = std::function<Sig>;
template <typename T> class TArrayView
{
	const T* D = nullptr;
	int32 N = 0;

public:
	TArrayView() = default;
	template <typename A> TArrayView(const TArray<std::remove_const_t<T>, A>& Arr) : D(Arr.GetData()), N(Arr.Num()) {}
	TArrayView(std::initializer_list<std::remove_const_t<T>> L) : N((int32)L.size()) {} // 가짜: 내용은 안 들고 있는다 (수명 경고 피함)
	int32 Num() const { return N; }
	const T& operator[](int32 I) const { return D[I]; }
	const T* begin() const { return D; }
	const T* end() const { return D + N; }
};

// ---------------------------------------------------------------- 공유 포인터
template <typename T> class TSharedRef;
template <typename T> class TSharedPtr;
template <typename T> class TWeakPtr;

template <typename T> class TSharedRef
{
	template <typename> friend class TSharedRef;
	template <typename> friend class TSharedPtr;
	template <typename> friend class TWeakPtr;
	template <typename> friend class TSharedFromThis;
	template <typename U, typename... A> friend TSharedRef<U> MakeShared(A&&... Args);
	template <typename To, typename From> friend TSharedRef<To> StaticCastSharedRef(const TSharedRef<From>& R);
	std::shared_ptr<T> P;
	explicit TSharedRef(std::shared_ptr<T> In) : P(std::move(In)) {}

public:
	TSharedRef() = delete;
	template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>> TSharedRef(const TSharedRef<U>& O) : P(O.P) {}
	T* operator->() const { return P.get(); }
	T& operator*() const { return *P; }
	T& Get() const { return *P; }
	bool IsValid() const { return true; }
	TSharedPtr<T> ToSharedPtr() const;
	bool operator==(const TSharedRef& O) const { return P == O.P; }
};

template <typename T> class TSharedPtr
{
	template <typename> friend class TSharedPtr;
	template <typename> friend class TWeakPtr;
	template <typename To, typename From> friend TSharedPtr<To> StaticCastSharedPtr(const TSharedPtr<From>& R);
	std::shared_ptr<T> P;

public:
	TSharedPtr() = default;
	TSharedPtr(std::nullptr_t) {}
	template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>> TSharedPtr(const TSharedRef<U>& R) : P(R.P) {}
	template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>> TSharedPtr(const TSharedPtr<U>& O) : P(O.P) {}
	T* operator->() const { return P.get(); }
	T& operator*() const { return *P; }
	T* Get() const { return P.get(); }
	bool IsValid() const { return (bool)P; }
	explicit operator bool() const { return (bool)P; }
	void Reset() { P.reset(); }
	TSharedRef<T> ToSharedRef() const { return TSharedRef<T>(P); }
	bool operator==(const TSharedPtr& O) const { return P == O.P; }
	bool operator==(std::nullptr_t) const { return !P; }
};
template <typename T> TSharedPtr<T> TSharedRef<T>::ToSharedPtr() const { return TSharedPtr<T>(*this); }

template <typename T> class TWeakPtr
{
	std::weak_ptr<T> P;

public:
	TWeakPtr() = default;
	template <typename U> TWeakPtr(const TSharedPtr<U>& S) : P(S.P) {}
	template <typename U> TWeakPtr(const TSharedRef<U>& S) : P(S.P) {}
	TSharedPtr<T> Pin() const;
	bool IsValid() const { return !P.expired(); }
	void Reset() { P.reset(); }
};

template <typename T, typename... A> TSharedRef<T> MakeShared(A&&... Args) { return TSharedRef<T>(std::make_shared<T>(std::forward<A>(Args)...)); }
template <typename T, typename... A> TSharedRef<T> MakeShareable(T* Ptr) { return MakeShared<T>(*Ptr); }

template <typename T> class TSharedFromThis
{
public:
	TSharedRef<T> AsShared() { return TSharedRef<T>(std::shared_ptr<T>()); }
	TSharedRef<const T> AsShared() const { return TSharedRef<const T>(std::shared_ptr<const T>()); }

protected:
	template <typename Derived> static TSharedRef<Derived> SharedThis(Derived* This) { return TSharedRef<Derived>(std::shared_ptr<Derived>(std::shared_ptr<Derived>(), This)); }
	template <typename Derived> static TSharedRef<const Derived> SharedThis(const Derived* This) { return TSharedRef<const Derived>(std::shared_ptr<const Derived>(std::shared_ptr<const Derived>(), This)); }
	virtual ~TSharedFromThis() = default;
};

// ---------------------------------------------------------------- FString
class FString
{
	std::wstring S;

public:
	using ElementType = TCHAR;
	FString() = default;
	FString(const TCHAR* In) : S(In ? In : L"") {}
	FString(const ANSICHAR* In) { if (In) while (*In) S.push_back((TCHAR)(unsigned char)*In++); }
	FString(int32 Len, const TCHAR* In) : S(In, (SIZE_T)Len) {}
	explicit FString(const std::wstring& In) : S(In) {}
	FString(const FString&) = default;
	FString(FString&&) = default;
	FString& operator=(const FString&) = default;
	FString& operator=(FString&&) = default;
	FString& operator=(const TCHAR* In) { S = In ? In : L""; return *this; }

	const TCHAR* operator*() const { return S.c_str(); }
	TCHAR& operator[](int32 I) { return S[(SIZE_T)I]; }
	const TCHAR& operator[](int32 I) const { return S[(SIZE_T)I]; }
	int32 Len() const { return (int32)S.size(); }
	bool IsEmpty() const { return S.empty(); }
	void Empty(int32 Slack = 0) { S.clear(); S.reserve((SIZE_T)Slack); }
	void Reset(int32 Slack = 0) { Empty(Slack); }
	void Reserve(int32 N) { S.reserve((SIZE_T)N); }
	bool IsValidIndex(int32 I) const { return I >= 0 && I < Len(); }

	FString& operator+=(const FString& O) { S += O.S; return *this; }
	FString& operator+=(const TCHAR* O) { S += O; return *this; }
	FString& operator+=(TCHAR C) { S += C; return *this; }
	FString& AppendChar(TCHAR C) { S += C; return *this; }
	FString& Append(const FString& O) { S += O.S; return *this; }
	FString& Append(const TCHAR* O) { S += O; return *this; }
	FString& Append(const TCHAR* O, int32 Count) { S.append(O, (SIZE_T)Count); return *this; }
	FString& AppendInt(int32 N) { S += std::to_wstring(N); return *this; }
	void InsertAt(int32 I, TCHAR C) { S.insert(S.begin() + I, C); }
	void InsertAt(int32 I, const FString& O) { S.insert((SIZE_T)I, O.S); }
	void RemoveAt(int32 I, int32 Count = 1, EAllowShrinking = EAllowShrinking::Default) { S.erase((SIZE_T)I, (SIZE_T)Count); }
	friend FString operator+(const FString& A, const FString& B) { FString R(A); R += B; return R; }
	friend FString operator+(const FString& A, const TCHAR* B) { FString R(A); R += B; return R; }
	friend FString operator+(const TCHAR* A, const FString& B) { FString R(A); R += B; return R; }
	friend FString operator+(const FString& A, TCHAR B) { FString R(A); R += B; return R; }
	friend FString operator/(const FString& A, const FString& B) { return A + TEXT("/") + B; }
	friend FString operator/(const FString& A, const TCHAR* B) { return A + TEXT("/") + B; }
	friend bool operator==(const FString& A, const FString& B) { return A.S == B.S; }
	friend bool operator==(const FString& A, const TCHAR* B) { return A.S == B; }
	friend bool operator==(const TCHAR* A, const FString& B) { return B.S == A; }
	friend bool operator!=(const FString& A, const FString& B) { return A.S != B.S; }
	friend bool operator!=(const FString& A, const TCHAR* B) { return A.S != B; }
	friend bool operator<(const FString& A, const FString& B) { return A.S < B.S; }

	FString Mid(int32 Start, int32 Count = MAX_int32) const { if (Start >= Len()) return FString(); return FString(S.substr((SIZE_T)std::max(0, Start), Count < 0 ? 0 : (SIZE_T)Count)); }
	FString Left(int32 Count) const { return FString(S.substr(0, (SIZE_T)std::clamp(Count, 0, Len()))); }
	FString Right(int32 Count) const { const int32 C = std::clamp(Count, 0, Len()); return FString(S.substr((SIZE_T)(Len() - C))); }
	FString LeftChop(int32 Count) const { return Left(Len() - Count); }
	FString RightChop(int32 Count) const { return Right(Len() - Count); }
	void LeftInline(int32 Count, EAllowShrinking = EAllowShrinking::Default) { *this = Left(Count); }
	void RightInline(int32 Count, EAllowShrinking = EAllowShrinking::Default) { *this = Right(Count); }
	void LeftChopInline(int32 Count, EAllowShrinking = EAllowShrinking::Default) { *this = LeftChop(Count); }
	void RightChopInline(int32 Count, EAllowShrinking = EAllowShrinking::Default) { *this = RightChop(Count); }
	void MidInline(int32 Start, int32 Count = MAX_int32, EAllowShrinking = EAllowShrinking::Default) { *this = Mid(Start, Count); }
	void TrimStartInline() { while (!S.empty() && FChar::IsWhitespace(S.front())) S.erase(0, 1); }
	void TrimEndInline() { while (!S.empty() && FChar::IsWhitespace(S.back())) S.pop_back(); }
	void TrimStartAndEndInline() { TrimStartInline(); TrimEndInline(); }
	FString TrimStart() const { FString R(*this); R.TrimStartInline(); return R; }
	FString TrimEnd() const { FString R(*this); R.TrimEndInline(); return R; }
	FString TrimStartAndEnd() const { FString R(*this); R.TrimStartAndEndInline(); return R; }
	FString ToUpper() const { FString R(*this); for (TCHAR& C : R.S) C = FChar::ToUpper(C); return R; }
	FString ToLower() const { FString R(*this); for (TCHAR& C : R.S) C = FChar::ToLower(C); return R; }
	void ToUpperInline() { *this = ToUpper(); }
	void ToLowerInline() { *this = ToLower(); }
	FString Reverse() const { FString R(*this); std::reverse(R.S.begin(), R.S.end()); return R; }
	void ReverseString() { std::reverse(S.begin(), S.end()); }
	FString LeftPad(int32 N) const { FString R(*this); while (R.Len() < N) R.S.insert(R.S.begin(), L' '); return R; }
	FString RightPad(int32 N) const { FString R(*this); while (R.Len() < N) R.S.push_back(L' '); return R; }

	bool Contains(const TCHAR* Sub) const { return S.find(Sub) != std::wstring::npos; }
	bool Contains(const FString& Sub) const { return S.find(Sub.S) != std::wstring::npos; }
	bool StartsWith(const TCHAR* P) const { return S.rfind(P, 0) == 0; }
	bool StartsWith(const FString& P) const { return S.rfind(P.S, 0) == 0; }
	bool EndsWith(const TCHAR* P) const { const std::wstring X(P); return S.size() >= X.size() && S.compare(S.size() - X.size(), X.size(), X) == 0; }
	bool EndsWith(const FString& P) const { return EndsWith(*P); }
	bool Equals(const FString& O) const { return S == O.S; }
	int32 Compare(const FString& O) const { return S.compare(O.S); }
	int32 Find(const TCHAR* Sub, int32 = 0, int32 = 0, int32 StartPos = INDEX_NONE) const { const auto P = S.find(Sub, StartPos < 0 ? 0 : (SIZE_T)StartPos); return P == std::wstring::npos ? INDEX_NONE : (int32)P; }
	int32 Find(const FString& Sub) const { return Find(*Sub); }
	bool FindChar(TCHAR C, int32& Index) const { const auto P = S.find(C); Index = P == std::wstring::npos ? INDEX_NONE : (int32)P; return Index != INDEX_NONE; }
	bool FindLastChar(TCHAR C, int32& Index) const { const auto P = S.rfind(C); Index = P == std::wstring::npos ? INDEX_NONE : (int32)P; return Index != INDEX_NONE; }
	bool Split(const FString& InS, FString* LeftS, FString* RightS) const
	{
		const auto P = S.find(InS.S);
		if (P == std::wstring::npos) return false;
		if (LeftS) *LeftS = FString(S.substr(0, P));
		if (RightS) *RightS = FString(S.substr(P + InS.S.size()));
		return true;
	}
	int32 ParseIntoArray(TArray<FString>& Out, const TCHAR* Delim, bool bCullEmpty = true) const
	{
		Out.Reset();
		const std::wstring D(Delim);
		SIZE_T Start = 0;
		while (true)
		{
			const auto P = S.find(D, Start);
			const std::wstring Part = S.substr(Start, P == std::wstring::npos ? std::wstring::npos : P - Start);
			if (!bCullEmpty || !Part.empty()) Out.Add(FString(Part));
			if (P == std::wstring::npos) break;
			Start = P + D.size();
		}
		return Out.Num();
	}
	int32 ParseIntoArrayWS(TArray<FString>& Out, const TCHAR* ExtraDelim = nullptr, bool bCullEmpty = true) const { (void)ExtraDelim; return ParseIntoArray(Out, TEXT(" "), bCullEmpty); }
	int32 ParseIntoArrayLines(TArray<FString>& Out, bool bCullEmpty = true) const { return ParseIntoArray(Out, TEXT("\n"), bCullEmpty); }
	FString Replace(const TCHAR* From, const TCHAR* To) const
	{
		FString R(*this);
		R.ReplaceInline(From, To);
		return R;
	}
	int32 ReplaceInline(const TCHAR* From, const TCHAR* To)
	{
		const std::wstring F(From), T(To);
		if (F.empty()) return 0;
		int32 N = 0;
		for (SIZE_T P = S.find(F); P != std::wstring::npos; P = S.find(F, P + T.size())) { S.replace(P, F.size(), T); ++N; }
		return N;
	}
	bool IsNumeric() const { if (S.empty()) return false; for (TCHAR C : S) if (!FChar::IsDigit(C) && C != L'.' && C != L'-') return false; return true; }
	TArray<TCHAR>& GetCharArray();

	static FString FromInt(int32 N) { return FString(std::to_wstring(N)); }
	static FString SanitizeFloat(double F, int32 MinFractionalDigits = 1) { (void)MinFractionalDigits; return FString(std::to_wstring(F)); }
	static FString Chr(TCHAR C) { FString R; R.S.push_back(C); return R; }
	static FString ChrN(int32 N, TCHAR C) { FString R; R.S.assign((SIZE_T)N, C); return R; }
	template <typename... A> static FString Printf(UE::Fake::TFormatFor<A...> Fmt, A... Args)
	{
		TCHAR Buf[4096];
		std::swprintf(Buf, 4096, Fmt.Str, Args...);
		return FString(Buf);
	}
	template <typename R> static FString Join(const R& Range, const TCHAR* Sep)
	{
		FString Out;
		bool bFirst = true;
		for (const auto& X : Range) { if (!bFirst) Out += Sep; Out += X; bFirst = false; }
		return Out;
	}
	auto begin() { return S.begin(); }
	auto end() { return S.end(); }
	auto begin() const { return S.begin(); }
	auto end() const { return S.end(); }
};
inline FString LexToString(int32 N) { return FString::FromInt(N); }
inline FString LexToString(float F) { return FString::SanitizeFloat(F); }

// 로그
struct FLogCategoryBase { const TCHAR* Name; };
inline FLogCategoryBase LogTemp{TEXT("LogTemp")};
namespace ELogVerbosity { enum Type : uint8 { NoLogging, Fatal, Error, Warning, Display, Log, Verbose, VeryVerbose }; }
template <typename... A> inline void FakeUELog(const FLogCategoryBase&, ELogVerbosity::Type, UE::Fake::TFormatFor<A...> Fmt, A... Args) { (void)Fmt; ((void)Args, ...); }
#define UE_LOG(Category, Verbosity, Format, ...) FakeUELog(Category, ELogVerbosity::Verbosity, Format, ##__VA_ARGS__)
#define DECLARE_LOG_CATEGORY_EXTERN(Name, Def, Comp) extern FLogCategoryBase Name;
#define DEFINE_LOG_CATEGORY(Name) FLogCategoryBase Name{TEXT(#Name)};
#define DEFINE_LOG_CATEGORY_STATIC(Name, Def, Comp) static FLogCategoryBase Name{TEXT(#Name)};

struct FTCHARToUTF8
{
	std::string Buf;
	explicit FTCHARToUTF8(const TCHAR* In) { if (In) while (*In) Buf.push_back((char)*In++); }
	const ANSICHAR* Get() const { return Buf.c_str(); }
	int32 Length() const { return (int32)Buf.size(); }
};
struct FUTF8ToTCHAR
{
	std::wstring Buf;
	explicit FUTF8ToTCHAR(const ANSICHAR* In) { if (In) while (*In) Buf.push_back((TCHAR)(unsigned char)*In++); }
	const TCHAR* Get() const { return Buf.c_str(); }
};
#define TCHAR_TO_UTF8(Str) ((ANSICHAR*)FTCHARToUTF8((const TCHAR*)(Str)).Get())
#define UTF8_TO_TCHAR(Str) ((TCHAR*)FUTF8ToTCHAR((const ANSICHAR*)(Str)).Get())
#define TCHAR_TO_ANSI(Str) TCHAR_TO_UTF8(Str)
#define ANSI_TO_TCHAR(Str) UTF8_TO_TCHAR(Str)

class FName
{
	FString S;

public:
	FName() = default;
	FName(const TCHAR* In) : S(In) {}
	FName(const ANSICHAR* In) : S(In) {}
	explicit FName(const FString& In) : S(In) {}
	FString ToString() const { return S; }
	bool IsNone() const { return S.IsEmpty(); }
	bool operator==(const FName& O) const { return S == O.S; }
	bool operator!=(const FName& O) const { return !(S == O.S); }
};
#define NAME_None FName()

class FText
{
	FString S;

public:
	FText() = default;
	static FText FromString(const FString& In) { FText T; T.S = In; return T; }
	static FText FromName(const FName& In) { return FromString(In.ToString()); }
	static FText AsNumber(int32 N) { return FromString(FString::FromInt(N)); }
	static FText GetEmpty() { return FText(); }
	const FString& ToString() const { return S; }
	bool IsEmpty() const { return S.IsEmpty(); }
};

// ---------------------------------------------------------------- 수학
namespace UE::Fake
{
template <typename A, typename B> constexpr bool MixFloat = !std::is_same_v<A, B> && std::is_arithmetic_v<A> && std::is_arithmetic_v<B> && (std::is_floating_point_v<A> || std::is_floating_point_v<B>);
}

struct FMath
{
	template <typename T> static constexpr T Min(const T A, const T B) { return A < B ? A : B; }
	template <typename T> static constexpr T Max(const T A, const T B) { return A > B ? A : B; }
	template <typename A, typename B, std::enable_if_t<UE::Fake::MixFloat<A, B>, int> = 0> static constexpr auto Min(A X, B Y) { using R = decltype(X * Y); return Min<R>((R)X, (R)Y); }
	template <typename A, typename B, std::enable_if_t<UE::Fake::MixFloat<A, B>, int> = 0> static constexpr auto Max(A X, B Y) { using R = decltype(X * Y); return Max<R>((R)X, (R)Y); }
	template <typename T> static constexpr T Min3(const T A, const T B, const T C) { return Min(Min(A, B), C); }
	template <typename T> static constexpr T Max3(const T A, const T B, const T C) { return Max(Max(A, B), C); }
	template <typename T> static constexpr T Clamp(const T X, const T Lo, const T Hi) { return X < Lo ? Lo : X < Hi ? X : Hi; }
	template <typename A, typename B, typename C, std::enable_if_t<!(std::is_same_v<A, B> && std::is_same_v<B, C>) && std::is_arithmetic_v<A> && std::is_arithmetic_v<B> && std::is_arithmetic_v<C> && (std::is_floating_point_v<A> || std::is_floating_point_v<B> || std::is_floating_point_v<C>), int> = 0>
	static constexpr auto Clamp(A X, B Lo, C Hi) { using R = decltype(X * Lo * Hi); return Clamp<R>((R)X, (R)Lo, (R)Hi); }
	template <typename T> static constexpr T Abs(const T A) { return A < (T)0 ? -A : A; }
	template <typename T> static constexpr T Sign(const T A) { return A > (T)0 ? (T)1 : A < (T)0 ? (T)-1 : (T)0; }
	template <typename T> static constexpr T Square(const T A) { return A * A; }
	template <typename T> static constexpr T Cube(const T A) { return A * A * A; }
	template <typename T, typename U> static constexpr T Lerp(const T& A, const T& B, const U& Alpha) { return (T)(A + Alpha * (B - A)); }
	template <typename T1, typename T2, typename U, std::enable_if_t<!std::is_same_v<T1, T2> && std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>, int> = 0>
	static constexpr auto Lerp(const T1& A, const T2& B, const U& Alpha) { using R = decltype(A * B); return Lerp<R, U>((R)A, (R)B, Alpha); }
	template <typename T> static T InterpEaseInOut(const T& A, const T& B, float Alpha, float Exp) { (void)Exp; return Lerp(A, B, Alpha); }
	template <typename T> static T CubicInterp(const T& P0, const T& T0, const T& P1, const T& T1, float A) { (void)T0; (void)T1; return Lerp(P0, P1, A); }
	template <typename T> static constexpr T SmoothStep(T A, T B, T X) { if (X < A) return (T)0; if (X >= B) return (T)1; const T F = (X - A) / (B - A); return F * F * ((T)3 - (T)2 * F); }

	static float Sqrt(float X) { return std::sqrt(X); }
	static double Sqrt(double X) { return std::sqrt(X); }
	static float InvSqrt(float X) { return 1.f / std::sqrt(X); }
	static float Sin(float X) { return std::sin(X); }
	static double Sin(double X) { return std::sin(X); }
	static float Cos(float X) { return std::cos(X); }
	static double Cos(double X) { return std::cos(X); }
	static float Tan(float X) { return std::tan(X); }
	static double Tan(double X) { return std::tan(X); }
	static float Asin(float X) { return std::asin(X); }
	static double Asin(double X) { return std::asin(X); }
	static float Acos(float X) { return std::acos(X); }
	static double Acos(double X) { return std::acos(X); }
	static float Atan(float X) { return std::atan(X); }
	static double Atan(double X) { return std::atan(X); }
	static float Atan2(float Y, float X) { return std::atan2(Y, X); }
	static double Atan2(double Y, double X) { return std::atan2(Y, X); }
	static void SinCos(float* S, float* C, float V) { *S = std::sin(V); *C = std::cos(V); }
	static float Pow(float A, float B) { return std::pow(A, B); }
	static double Pow(double A, double B) { return std::pow(A, B); }
	static float Exp(float X) { return std::exp(X); }
	static double Exp(double X) { return std::exp(X); }
	static float Exp2(float X) { return std::exp2(X); }
	static float Loge(float X) { return std::log(X); }
	static double Loge(double X) { return std::log(X); }
	static float LogX(float Base, float X) { return std::log(X) / std::log(Base); }
	static float Log2(float X) { return std::log2(X); }
	static double Log2(double X) { return std::log2(X); }
	static float Fmod(float X, float Y) { return std::fmod(X, Y); }
	static double Fmod(double X, double Y) { return std::fmod(X, Y); }
	static float Frac(float X) { return X - std::floor(X); }
	static double Frac(double X) { return X - std::floor(X); }
	static float Fractional(float X) { return X - std::trunc(X); }
	static int32 FloorToInt(float X) { return (int32)std::floor(X); }
	static int64 FloorToInt(double X) { return (int64)std::floor(X); }
	static int32 FloorToInt32(float X) { return (int32)std::floor(X); }
	static int32 FloorToInt32(double X) { return (int32)std::floor(X); }
	static float FloorToFloat(float X) { return std::floor(X); }
	static double FloorToFloat(double X) { return std::floor(X); }
	static double FloorToDouble(double X) { return std::floor(X); }
	static int32 CeilToInt(float X) { return (int32)std::ceil(X); }
	static int64 CeilToInt(double X) { return (int64)std::ceil(X); }
	static int32 CeilToInt32(float X) { return (int32)std::ceil(X); }
	static float CeilToFloat(float X) { return std::ceil(X); }
	static double CeilToFloat(double X) { return std::ceil(X); }
	static int32 RoundToInt(float X) { return (int32)std::floor(X + 0.5f); }
	static int64 RoundToInt(double X) { return (int64)std::floor(X + 0.5); }
	static int32 RoundToInt32(float X) { return (int32)std::floor(X + 0.5f); }
	static int32 RoundToInt32(double X) { return (int32)std::floor(X + 0.5); }
	static float RoundToFloat(float X) { return std::floor(X + 0.5f); }
	static double RoundToFloat(double X) { return std::floor(X + 0.5); }
	static double RoundToDouble(double X) { return std::floor(X + 0.5); }
	static float RoundHalfFromZero(float X) { return std::round(X); }
	static int32 TruncToInt(float X) { return (int32)X; }
	static int64 TruncToInt(double X) { return (int64)X; }
	static int32 TruncToInt32(float X) { return (int32)X; }
	static float TruncToFloat(float X) { return std::trunc(X); }
	static bool IsNaN(float X) { return std::isnan(X); }
	static bool IsFinite(float X) { return std::isfinite(X); }
	static bool IsNearlyEqual(float A, float B, float Tol = UE_SMALL_NUMBER) { return Abs(A - B) <= Tol; }
	static bool IsNearlyEqual(double A, double B, double Tol = UE_SMALL_NUMBER) { return Abs(A - B) <= Tol; }
	static bool IsNearlyZero(float A, float Tol = UE_SMALL_NUMBER) { return Abs(A) <= Tol; }
	static bool IsNearlyZero(double A, double Tol = UE_SMALL_NUMBER) { return Abs(A) <= Tol; }
	template <typename T> static constexpr bool IsPowerOfTwo(T V) { return V > 0 && (V & (V - 1)) == 0; }
	// std::bit 함수로 (g++ 와 MSVC cl 둘 다 됨)
	static int32 CountBits(uint64 Bits) { return (int32)std::popcount(Bits); }
	static uint32 FloorLog2(uint32 V) { return V ? 31u - (uint32)std::countl_zero(V) : 0u; }
	static uint32 CountLeadingZeros(uint32 V) { return (uint32)std::countl_zero(V); }
	static uint32 CountTrailingZeros(uint32 V) { return (uint32)std::countr_zero(V); }
	template <typename T> static constexpr auto DegreesToRadians(T const& D) -> decltype(D * (UE_PI / 180.f)) { return D * (UE_PI / 180.f); }
	template <typename T> static constexpr auto RadiansToDegrees(T const& R) -> decltype(R * (180.f / UE_PI)) { return R * (180.f / UE_PI); }
	template <typename T> static T Wrap(const T X, const T Lo, const T Hi) { return X < Lo ? Hi : X > Hi ? Lo : X; }
	static float UnwindRadians(float A) { while (A > UE_PI) A -= UE_TWO_PI; while (A < -UE_PI) A += UE_TWO_PI; return A; }
	static float UnwindDegrees(float A) { while (A > 180.f) A -= 360.f; while (A < -180.f) A += 360.f; return A; }
	static float FInterpTo(float Cur, float Target, float Dt, float Speed) { return Cur + (Target - Cur) * Clamp(Dt * Speed, 0.f, 1.f); }
	static float FInterpConstantTo(float Cur, float Target, float Dt, float Speed) { (void)Dt; (void)Speed; return Target + 0.f * Cur; }

	static int32 Rand() { return std::rand(); }
	static float FRand() { return (float)std::rand() / (float)RAND_MAX; }
	static bool RandBool() { return std::rand() & 1; }
	static int32 RandHelper(int32 A) { return A > 0 ? std::rand() % A : 0; }
	static int32 RandRange(int32 Lo, int32 Hi) { return Lo + RandHelper(Hi - Lo + 1); }
	static int64 RandRange(int64 Lo, int64 Hi) { return Lo + RandHelper((int32)(Hi - Lo + 1)); }
	static float RandRange(float Lo, float Hi) { return Lo + (Hi - Lo) * FRand(); }
	static double RandRange(double Lo, double Hi) { return Lo + (Hi - Lo) * FRand(); }
	static float FRandRange(float Lo, float Hi) { return Lo + (Hi - Lo) * FRand(); }
	static double FRandRange(double Lo, double Hi) { return Lo + (Hi - Lo) * FRand(); }
	template <typename A, typename B, std::enable_if_t<UE::Fake::MixFloat<A, B>, int> = 0> static auto FRandRange(A Lo, B Hi) { using R = decltype(Lo * Hi); return FRandRange((R)Lo, (R)Hi); }
	static float GetMappedRangeValueClamped(float InA, float InB, float OutA, float OutB, float V) { const float T = Clamp((V - InA) / (InB - InA), 0.f, 1.f); return OutA + (OutB - OutA) * T; }
	static float GetRangePct(float A, float B, float V) { return (V - A) / (B - A); }
	static float FastAsin(float X) { return std::asin(X); }
	template <typename T> static T GridSnap(T Loc, T Grid) { return Grid == T(0) ? Loc : std::floor((Loc + Grid / (T)2) / Grid) * Grid; }
};

template <typename T> struct TVector2;
template <typename T> struct TVector2
{
	T X = 0, Y = 0;
	using FReal = T;
	static const TVector2 ZeroVector;
	static const TVector2 UnitVector;
	TVector2() = default;
	constexpr TVector2(T InX, T InY) : X(InX), Y(InY) {}
	explicit constexpr TVector2(T V) : X(V), Y(V) {}
	template <typename U, std::enable_if_t<!std::is_same_v<T, U>, int> = 0> explicit constexpr TVector2(const TVector2<U>& O) : X((T)O.X), Y((T)O.Y) {}
	TVector2 operator+(const TVector2& O) const { return {X + O.X, Y + O.Y}; }
	TVector2 operator-(const TVector2& O) const { return {X - O.X, Y - O.Y}; }
	TVector2 operator*(const TVector2& O) const { return {X * O.X, Y * O.Y}; }
	TVector2 operator/(const TVector2& O) const { return {X / O.X, Y / O.Y}; }
	template <typename S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0> TVector2 operator*(S F) const { return {(T)(X * F), (T)(Y * F)}; }
	template <typename S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0> TVector2 operator/(S F) const { return {(T)(X / F), (T)(Y / F)}; }
	template <typename S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0> TVector2 operator+(S F) const { return {(T)(X + F), (T)(Y + F)}; }
	template <typename S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0> TVector2 operator-(S F) const { return {(T)(X - F), (T)(Y - F)}; }
	TVector2 operator-() const { return {-X, -Y}; }
	TVector2& operator+=(const TVector2& O) { X += O.X; Y += O.Y; return *this; }
	TVector2& operator-=(const TVector2& O) { X -= O.X; Y -= O.Y; return *this; }
	TVector2& operator*=(const TVector2& O) { X *= O.X; Y *= O.Y; return *this; }
	TVector2& operator/=(const TVector2& O) { X /= O.X; Y /= O.Y; return *this; }
	template <typename S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0> TVector2& operator*=(S F) { X = (T)(X * F); Y = (T)(Y * F); return *this; }
	template <typename S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0> TVector2& operator/=(S F) { X = (T)(X / F); Y = (T)(Y / F); return *this; }
	bool operator==(const TVector2& O) const { return X == O.X && Y == O.Y; }
	bool operator!=(const TVector2& O) const { return !(*this == O); }
	T& operator[](int32 I) { return I ? Y : X; }
	T operator[](int32 I) const { return I ? Y : X; }
	T operator|(const TVector2& O) const { return X * O.X + Y * O.Y; }
	T operator^(const TVector2& O) const { return X * O.Y - Y * O.X; }
	T Size() const { return std::sqrt(X * X + Y * Y); }
	T Length() const { return Size(); }
	T SizeSquared() const { return X * X + Y * Y; }
	T SquaredLength() const { return SizeSquared(); }
	TVector2 GetSafeNormal(T Tol = (T)UE_SMALL_NUMBER) const { const T S = SizeSquared(); return S > Tol ? *this / std::sqrt(S) : ZeroVector; }
	void Normalize(T Tol = (T)UE_SMALL_NUMBER) { *this = GetSafeNormal(Tol); }
	bool IsNearlyZero(T Tol = (T)UE_KINDA_SMALL_NUMBER) const { return std::abs(X) <= Tol && std::abs(Y) <= Tol; }
	bool IsZero() const { return X == 0 && Y == 0; }
	TVector2 GetRotated(T AngleDeg) const { const T A = AngleDeg * (T)(UE_PI / 180.f); return {X * std::cos(A) - Y * std::sin(A), X * std::sin(A) + Y * std::cos(A)}; }
	TVector2 GetAbs() const { return {std::abs(X), std::abs(Y)}; }
	T GetMax() const { return std::max(X, Y); }
	T GetMin() const { return std::min(X, Y); }
	TVector2 ClampAxes(T Lo, T Hi) const { return {FMath::Clamp(X, Lo, Hi), FMath::Clamp(Y, Lo, Hi)}; }
	TVector2 RoundToVector() const { return {std::round(X), std::round(Y)}; }
	static T DotProduct(const TVector2& A, const TVector2& B) { return A.X * B.X + A.Y * B.Y; }
	static T CrossProduct(const TVector2& A, const TVector2& B) { return A.X * B.Y - A.Y * B.X; }
	static T Distance(const TVector2& A, const TVector2& B) { return (A - B).Size(); }
	static T DistSquared(const TVector2& A, const TVector2& B) { return (A - B).SizeSquared(); }
	static TVector2 Max(const TVector2& A, const TVector2& B) { return {std::max(A.X, B.X), std::max(A.Y, B.Y)}; }
	static TVector2 Min(const TVector2& A, const TVector2& B) { return {std::min(A.X, B.X), std::min(A.Y, B.Y)}; }
	TVector2 ComponentMax(const TVector2& O) const { return Max(*this, O); }
	TVector2 ComponentMin(const TVector2& O) const { return Min(*this, O); }
};
template <typename T> const TVector2<T> TVector2<T>::ZeroVector(0, 0);
template <typename T> const TVector2<T> TVector2<T>::UnitVector(1, 1);
template <typename S, typename T, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0> inline TVector2<T> operator*(S F, const TVector2<T>& V) { return V * F; }
using FVector2f = TVector2<float>;
using FVector2D = TVector2<double>;
// Slate 의 FDeprecateSlateVector2D: FVector2f 이지만 FVector2D 로도 만들고 대입할 수 있다 (UE 5.1+)
struct FDeprecateSlateVector2D : FVector2f
{
	FDeprecateSlateVector2D() = default;
	FDeprecateSlateVector2D(const FVector2f& V) : FVector2f(V) {}
	FDeprecateSlateVector2D(const FVector2D& V) : FVector2f(V) {}
	FDeprecateSlateVector2D(float InX, float InY) : FVector2f(InX, InY) {}
	FDeprecateSlateVector2D& operator=(const FVector2f& V) { FVector2f::operator=(V); return *this; }
	FDeprecateSlateVector2D& operator=(const FVector2D& V) { FVector2f::operator=(FVector2f(V)); return *this; }
	operator FVector2D() const { return FVector2D(*this); }
};

struct FVector
{
	double X = 0, Y = 0, Z = 0;
	static const FVector ZeroVector;
	FVector() = default;
	constexpr FVector(double InX, double InY, double InZ) : X(InX), Y(InY), Z(InZ) {}
	explicit constexpr FVector(double V) : X(V), Y(V), Z(V) {}
	FVector operator*(double F) const { return {X * F, Y * F, Z * F}; }
	FVector operator+(const FVector& O) const { return {X + O.X, Y + O.Y, Z + O.Z}; }
};
inline const FVector FVector::ZeroVector(0, 0, 0);
struct FRotator
{
	double Pitch = 0, Yaw = 0, Roll = 0;
	static const FRotator ZeroRotator;
	FRotator() = default;
	constexpr FRotator(double P, double Y, double R) : Pitch(P), Yaw(Y), Roll(R) {}
};
inline const FRotator FRotator::ZeroRotator(0, 0, 0);
struct FIntPoint
{
	int32 X = 0, Y = 0;
	FIntPoint() = default;
	constexpr FIntPoint(int32 InX, int32 InY) : X(InX), Y(InY) {}
	bool operator==(const FIntPoint& O) const { return X == O.X && Y == O.Y; }
};
struct FIntVector
{
	int32 X = 0, Y = 0, Z = 0;
	FIntVector() = default;
	constexpr FIntVector(int32 InX, int32 InY, int32 InZ) : X(InX), Y(InY), Z(InZ) {}
};

struct FVector4f
{
	float X = 0, Y = 0, Z = 0, W = 0;
	FVector4f() = default;
	constexpr FVector4f(float InX, float InY, float InZ, float InW) : X(InX), Y(InY), Z(InZ), W(InW) {}
};
template <typename E> class TEnumAsByte
{
	uint8 V = 0;

public:
	TEnumAsByte() = default;
	TEnumAsByte(E In) : V((uint8)In) {}
	operator E() const { return (E)V; }
	E GetValue() const { return (E)V; }
	bool operator==(E O) const { return (E)V == O; }
};

template <typename T> struct TBox2
{
	TVector2<T> Min, Max;
	bool bIsValid = false;
	TBox2() = default;
	TBox2(const TVector2<T>& InMin, const TVector2<T>& InMax) : Min(InMin), Max(InMax), bIsValid(true) {}
	bool IsInside(const TVector2<T>& P) const { return P.X > Min.X && P.X < Max.X && P.Y > Min.Y && P.Y < Max.Y; }
	bool IsInsideOrOn(const TVector2<T>& P) const { return P.X >= Min.X && P.X <= Max.X && P.Y >= Min.Y && P.Y <= Max.Y; }
	bool Intersect(const TBox2& O) const { return !(Min.X > O.Max.X || O.Min.X > Max.X || Min.Y > O.Max.Y || O.Min.Y > Max.Y); }
	TVector2<T> GetCenter() const { return (Min + Max) * (T)0.5; }
	TVector2<T> GetSize() const { return Max - Min; }
	TVector2<T> GetExtent() const { return (Max - Min) * (T)0.5; }
	T GetArea() const { return (Max.X - Min.X) * (Max.Y - Min.Y); }
	TBox2 ExpandBy(T W) const { return TBox2(Min - TVector2<T>(W, W), Max + TVector2<T>(W, W)); }
	TBox2 ShiftBy(const TVector2<T>& O) const { return TBox2(Min + O, Max + O); }
	TBox2& operator+=(const TVector2<T>& P) { if (bIsValid) { Min = TVector2<T>::Min(Min, P); Max = TVector2<T>::Max(Max, P); } else { Min = Max = P; bIsValid = true; } return *this; }
	void Init() { Min = Max = TVector2<T>(); bIsValid = false; }
};
using FBox2f = TBox2<float>;
using FBox2D = TBox2<double>;

// ---------------------------------------------------------------- 색
struct FLinearColor;
struct FColor
{
	uint8 B = 0, G = 0, R = 0, A = 0;
	static const FColor White, Black, Transparent, Red, Green, Blue, Yellow, Cyan, Magenta, Orange, Purple, Silver, Emerald;
	FColor() = default;
	constexpr FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255) : B(InB), G(InG), R(InR), A(InA) {}
	explicit FColor(uint32 Packed) : B((uint8)Packed), G((uint8)(Packed >> 8)), R((uint8)(Packed >> 16)), A((uint8)(Packed >> 24)) {}
	static FColor FromHex(const FString& HexString) { (void)HexString; return FColor(); }
	FLinearColor ReinterpretAsLinear() const;
	uint32 ToPackedARGB() const { return ((uint32)A << 24) | ((uint32)R << 16) | ((uint32)G << 8) | B; }
	uint32 DWColor() const { return ToPackedARGB(); }
	FColor WithAlpha(uint8 Alpha) const { return FColor(R, G, B, Alpha); }
	FString ToHex() const { return FString(); }
	bool operator==(const FColor& O) const { return R == O.R && G == O.G && B == O.B && A == O.A; }
	bool operator!=(const FColor& O) const { return !(*this == O); }
};
inline const FColor FColor::White(255, 255, 255), FColor::Black(0, 0, 0), FColor::Transparent(0, 0, 0, 0), FColor::Red(255, 0, 0), FColor::Green(0, 255, 0),
	FColor::Blue(0, 0, 255), FColor::Yellow(255, 255, 0), FColor::Cyan(0, 255, 255), FColor::Magenta(255, 0, 255), FColor::Orange(243, 156, 18),
	FColor::Purple(169, 7, 228), FColor::Silver(189, 195, 199), FColor::Emerald(46, 204, 113);

struct FLinearColor
{
	float R = 0.f, G = 0.f, B = 0.f, A = 0.f;
	static const FLinearColor White, Gray, Black, Transparent, Red, Green, Blue, Yellow;
	FLinearColor() = default;
	constexpr FLinearColor(float InR, float InG, float InB, float InA = 1.f) : R(InR), G(InG), B(InB), A(InA) {}
	FLinearColor(const FColor& C) : R(C.R / 255.f), G(C.G / 255.f), B(C.B / 255.f), A(C.A / 255.f) {}
	explicit FLinearColor(const FVector& V) : R((float)V.X), G((float)V.Y), B((float)V.Z), A(1.f) {}
	FLinearColor operator+(const FLinearColor& O) const { return {R + O.R, G + O.G, B + O.B, A + O.A}; }
	FLinearColor operator-(const FLinearColor& O) const { return {R - O.R, G - O.G, B - O.B, A - O.A}; }
	FLinearColor operator*(const FLinearColor& O) const { return {R * O.R, G * O.G, B * O.B, A * O.A}; }
	FLinearColor operator*(float S) const { return {R * S, G * S, B * S, A * S}; }
	FLinearColor operator/(float S) const { return {R / S, G / S, B / S, A / S}; }
	FLinearColor& operator+=(const FLinearColor& O) { R += O.R; G += O.G; B += O.B; A += O.A; return *this; }
	FLinearColor& operator-=(const FLinearColor& O) { R -= O.R; G -= O.G; B -= O.B; A -= O.A; return *this; }
	FLinearColor& operator*=(const FLinearColor& O) { R *= O.R; G *= O.G; B *= O.B; A *= O.A; return *this; }
	FLinearColor& operator*=(float S) { R *= S; G *= S; B *= S; A *= S; return *this; }
	FLinearColor& operator/=(float S) { R /= S; G /= S; B /= S; A /= S; return *this; }
	bool operator==(const FLinearColor& O) const { return R == O.R && G == O.G && B == O.B && A == O.A; }
	bool operator!=(const FLinearColor& O) const { return !(*this == O); }
	float& Component(int32 I) { return (&R)[I]; }
	FColor ToFColor(const bool bSRGB) const { (void)bSRGB; return FColor((uint8)(R * 255.f), (uint8)(G * 255.f), (uint8)(B * 255.f), (uint8)(A * 255.f)); }
	FColor ToFColorSRGB() const { return ToFColor(true); }
	FColor QuantizeRound() const { return ToFColor(false); }
	FColor Quantize() const { return ToFColor(false); }
	FLinearColor CopyWithNewOpacity(float NewA) const { FLinearColor C = *this; C.A = NewA; return C; }
	FLinearColor LinearRGBToHSV() const { return *this; }
	FLinearColor HSVToLinearRGB() const { return *this; }
	FLinearColor Desaturate(float D) const { (void)D; return *this; }
	FLinearColor GetClamped(float Lo = 0.f, float Hi = 1.f) const { return {FMath::Clamp(R, Lo, Hi), FMath::Clamp(G, Lo, Hi), FMath::Clamp(B, Lo, Hi), FMath::Clamp(A, Lo, Hi)}; }
	float GetLuminance() const { return R * 0.3f + G * 0.59f + B * 0.11f; }
	float GetMax() const { return std::max({R, G, B, A}); }
	float GetMin() const { return std::min({R, G, B, A}); }
	bool Equals(const FLinearColor& O, float Tol = UE_KINDA_SMALL_NUMBER) const { return std::abs(R - O.R) < Tol && std::abs(G - O.G) < Tol && std::abs(B - O.B) < Tol && std::abs(A - O.A) < Tol; }
	static FLinearColor FromSRGBColor(const FColor& C) { return FLinearColor(C); }
	static FLinearColor FromPow22Color(const FColor& C) { return FLinearColor(C); }
	static FLinearColor MakeFromHSV8(uint8 H, uint8 S, uint8 V) { return FLinearColor(H / 255.f, S / 255.f, V / 255.f); }
	static FLinearColor MakeFromColorTemperature(float T) { (void)T; return White; }
	static FLinearColor MakeRandomColor() { return White; }
	static FLinearColor LerpUsingHSV(const FLinearColor& A, const FLinearColor& B, float T) { return FMath::Lerp(A, B, T); }
	static float Dist(const FLinearColor& A, const FLinearColor& B) { return std::sqrt((A.R - B.R) * (A.R - B.R) + (A.G - B.G) * (A.G - B.G) + (A.B - B.B) * (A.B - B.B) + (A.A - B.A) * (A.A - B.A)); }
};
inline FLinearColor operator*(float S, const FLinearColor& C) { return C * S; }
inline const FLinearColor FLinearColor::White(1.f, 1.f, 1.f), FLinearColor::Gray(0.5f, 0.5f, 0.5f), FLinearColor::Black(0.f, 0.f, 0.f), FLinearColor::Transparent(0.f, 0.f, 0.f, 0.f),
	FLinearColor::Red(1.f, 0.f, 0.f), FLinearColor::Green(0.f, 1.f, 0.f), FLinearColor::Blue(0.f, 0.f, 1.f), FLinearColor::Yellow(1.f, 1.f, 0.f);
inline FLinearColor FColor::ReinterpretAsLinear() const { return FLinearColor(R / 255.f, G / 255.f, B / 255.f, A / 255.f); }

struct FRandomStream
{
	int32 InitialSeed = 0;
	mutable int32 Seed = 0;
	FRandomStream() = default;
	explicit FRandomStream(int32 InSeed) : InitialSeed(InSeed), Seed(InSeed) {}
	void Initialize(int32 InSeed) { InitialSeed = Seed = InSeed; }
	void Reset() const { Seed = InitialSeed; }
	int32 GetInitialSeed() const { return InitialSeed; }
	int32 GetCurrentSeed() const { return Seed; }
	void GenerateNewSeed() { Seed = std::rand(); }
	float GetFraction() const { Seed = Seed * 196314165 + 907633515; return (float)((uint32)Seed >> 9) / (float)(1u << 23); }
	float FRand() const { return GetFraction(); }
	uint32 GetUnsignedInt() const { Seed = Seed * 196314165 + 907633515; return (uint32)Seed; }
	int32 RandHelper(int32 A) const { return A > 0 ? (int32)(GetFraction() * (float)A) : 0; }
	int32 RandRange(int32 Lo, int32 Hi) const { return Lo + RandHelper(Hi - Lo + 1); }
	float FRandRange(float Lo, float Hi) const { return Lo + (Hi - Lo) * GetFraction(); }
	float RandRange(float Lo, float Hi) const { return FRandRange(Lo, Hi); }
	bool RandBool() const { return GetFraction() < 0.5f; }
};

// ---------------------------------------------------------------- 플랫폼
struct FPlatformTime
{
	static double Seconds() { return 0.0; }
	static uint32 Cycles() { return 0; }
	static uint64 Cycles64() { return 0; }
	static float ToMilliseconds(const uint32 Cycles) { return (float)Cycles; }
	static double ToMilliseconds64(const uint64 Cycles) { return (double)Cycles; }
	static double ToSeconds(const uint32 Cycles) { return (double)Cycles; }
	static double GetSecondsPerCycle() { return 0.0; }
};
struct FPlatformMisc
{
	static void RequestExit(bool bForce, const TCHAR* CallSite = nullptr) { (void)bForce; (void)CallSite; }
};
struct FPlatformProcess
{
	static void Sleep(float Seconds) { (void)Seconds; }
};
struct FMemory
{
	static void* Memcpy(void* D, const void* S, SIZE_T N) { return std::memcpy(D, S, N); }
	static void* Memmove(void* D, const void* S, SIZE_T N) { return std::memmove(D, S, N); }
	static void* Memset(void* D, uint8 V, SIZE_T N) { return std::memset(D, V, N); }
	static void* Memzero(void* D, SIZE_T N) { return std::memset(D, 0, N); }
	static int32 Memcmp(const void* A, const void* B, SIZE_T N) { return std::memcmp(A, B, N); }
	template <typename T> static void Memzero(T& V) { std::memset((void*)&V, 0, sizeof(T)); }
};
inline uint64 GFrameCounter = 0;
inline bool GIsEditor = false;
inline uint32 GGameThreadTime = 0, GRenderThreadTime = 0, GRHIThreadTime = 0;
inline uint32 RHIGetGPUFrameCycles(uint32 GPUIndex = 0) { (void)GPUIndex; return 0; }
inline FString GGameUserSettingsIni;
inline FString GGameIni;
inline FString GEngineIni;

struct FCommandLine
{
	static const TCHAR* Get() { return TEXT(""); }
};
struct FParse
{
	static bool Param(const TCHAR* Stream, const TCHAR* Param) { (void)Stream; (void)Param; return false; }
	static bool Value(const TCHAR* Stream, const TCHAR* Match, int32& Out) { (void)Stream; (void)Match; (void)Out; return false; }
	static bool Value(const TCHAR* Stream, const TCHAR* Match, uint32& Out) { (void)Stream; (void)Match; (void)Out; return false; }
	static bool Value(const TCHAR* Stream, const TCHAR* Match, int64& Out) { (void)Stream; (void)Match; (void)Out; return false; }
	static bool Value(const TCHAR* Stream, const TCHAR* Match, float& Out) { (void)Stream; (void)Match; (void)Out; return false; }
	static bool Value(const TCHAR* Stream, const TCHAR* Match, double& Out) { (void)Stream; (void)Match; (void)Out; return false; }
	static bool Value(const TCHAR* Stream, const TCHAR* Match, FString& Out, bool bShouldStopOnSeparator = true) { (void)Stream; (void)Match; (void)Out; (void)bShouldStopOnSeparator; return false; }
	static bool Bool(const TCHAR* Stream, const TCHAR* Match, bool& Out) { (void)Stream; (void)Match; (void)Out; return false; }
};

struct FPaths
{
	static FString ProjectSavedDir() { return FString(); }
	static FString ProjectDir() { return FString(); }
	static FString ProjectContentDir() { return FString(); }
	static FString ProjectLogDir() { return FString(); }
	static FString ProjectConfigDir() { return FString(); }
	static bool FileExists(const FString& Path) { (void)Path; return false; }
	static bool DirectoryExists(const FString& Path) { (void)Path; return false; }
	static FString GetPath(const FString& Path) { return Path; }
	static FString GetCleanFilename(const FString& Path) { return Path; }
	static FString GetBaseFilename(const FString& Path, bool bRemovePath = true) { (void)bRemovePath; return Path; }
	static FString GetExtension(const FString& Path, bool bIncludeDot = false) { (void)bIncludeDot; return Path; }
	template <typename... A> static FString Combine(const A&... Parts)
	{
		static_assert(sizeof...(A) >= 1, "FPaths::Combine needs parts");
		static_assert((std::is_convertible_v<const A&, FString> && ...), "FPaths::Combine parts must be strings");
		FString R;
		((R = R.IsEmpty() ? FString(Parts) : R / FString(Parts)), ...);
		return R;
	}
};

class IFileManager
{
public:
	static IFileManager& Get() { static IFileManager M; return M; }
	bool Delete(const TCHAR* Filename, bool RequireExists = false, bool EvenReadOnly = false, bool Quiet = false) { (void)Filename; (void)RequireExists; (void)EvenReadOnly; (void)Quiet; return true; }
	bool MakeDirectory(const TCHAR* Path, bool Tree = false) { (void)Path; (void)Tree; return true; }
	bool Move(const TCHAR* Dest, const TCHAR* Src, bool bReplace = true, bool bEvenIfReadOnly = false, bool bAttributes = false, bool bDoNotRetryOrError = false) { (void)Dest; (void)Src; (void)bReplace; (void)bEvenIfReadOnly; (void)bAttributes; (void)bDoNotRetryOrError; return true; }
	int64 FileSize(const TCHAR* Filename) { (void)Filename; return 0; }
};

struct FFileHelper
{
	enum class EEncodingOptions { AutoDetect, ForceAnsi, ForceUnicode, ForceUTF8, ForceUTF8WithoutBOM };
	enum class EHashOptions { None = 0 };
	static bool LoadFileToString(FString& Result, const TCHAR* Filename, EHashOptions = EHashOptions::None, uint32 ReadFlags = 0) { (void)Result; (void)Filename; (void)ReadFlags; return false; }
	static bool LoadFileToStringArray(TArray<FString>& Result, const TCHAR* Filename) { (void)Result; (void)Filename; return false; }
	static bool LoadFileToArray(TArray<uint8>& Result, const TCHAR* Filename, uint32 Flags = 0) { (void)Result; (void)Filename; (void)Flags; return false; }
	static bool SaveStringToFile(const FString& String, const TCHAR* Filename, EEncodingOptions EncodingOptions = EEncodingOptions::AutoDetect, IFileManager* FileManager = &IFileManager::Get(), uint32 WriteFlags = 0)
	{ (void)String; (void)Filename; (void)EncodingOptions; (void)FileManager; (void)WriteFlags; return true; }
	static bool SaveStringArrayToFile(const TArray<FString>& Lines, const TCHAR* Filename, EEncodingOptions EncodingOptions = EEncodingOptions::AutoDetect, IFileManager* FileManager = &IFileManager::Get(), uint32 WriteFlags = 0)
	{ (void)Lines; (void)Filename; (void)EncodingOptions; (void)FileManager; (void)WriteFlags; return true; }
	static bool SaveArrayToFile(const TArray<uint8>& Array, const TCHAR* Filename, IFileManager* FileManager = &IFileManager::Get(), uint32 WriteFlags = 0)
	{ (void)Array; (void)Filename; (void)FileManager; (void)WriteFlags; return true; }
};

class FConfigCacheIni
{
public:
	bool GetBool(const TCHAR* Section, const TCHAR* Key, bool& Value, const FString& Filename) { (void)Section; (void)Key; (void)Value; (void)Filename; return false; }
	bool GetInt(const TCHAR* Section, const TCHAR* Key, int32& Value, const FString& Filename) { (void)Section; (void)Key; (void)Value; (void)Filename; return false; }
	bool GetFloat(const TCHAR* Section, const TCHAR* Key, float& Value, const FString& Filename) { (void)Section; (void)Key; (void)Value; (void)Filename; return false; }
	bool GetDouble(const TCHAR* Section, const TCHAR* Key, double& Value, const FString& Filename) { (void)Section; (void)Key; (void)Value; (void)Filename; return false; }
	bool GetString(const TCHAR* Section, const TCHAR* Key, FString& Value, const FString& Filename) { (void)Section; (void)Key; (void)Value; (void)Filename; return false; }
	void SetBool(const TCHAR* Section, const TCHAR* Key, bool Value, const FString& Filename) { (void)Section; (void)Key; (void)Value; (void)Filename; }
	void SetInt(const TCHAR* Section, const TCHAR* Key, int32 Value, const FString& Filename) { (void)Section; (void)Key; (void)Value; (void)Filename; }
	void SetFloat(const TCHAR* Section, const TCHAR* Key, float Value, const FString& Filename) { (void)Section; (void)Key; (void)Value; (void)Filename; }
	void SetDouble(const TCHAR* Section, const TCHAR* Key, double Value, const FString& Filename) { (void)Section; (void)Key; (void)Value; (void)Filename; }
	void SetString(const TCHAR* Section, const TCHAR* Key, const TCHAR* Value, const FString& Filename) { (void)Section; (void)Key; (void)Value; (void)Filename; }
	void Flush(bool bRemoveFromCache, const FString& Filename = FString()) { (void)bRemoveFromCache; (void)Filename; }
};
inline FConfigCacheIni* GConfig = nullptr;

struct FTimespan
{
	double TotalSeconds = 0;
	double GetTotalSeconds() const { return TotalSeconds; }
};
struct FDateTime
{
	int64 Ticks = 0;
	FDateTime() = default;
	FDateTime(int32 Y, int32 Mo, int32 D, int32 H = 0, int32 Mi = 0, int32 S = 0, int32 Ms = 0) { (void)Y; (void)Mo; (void)D; (void)H; (void)Mi; (void)S; (void)Ms; }
	static FDateTime Now() { return FDateTime(); }
	static FDateTime UtcNow() { return FDateTime(); }
	static FDateTime FromUnixTimestamp(int64 T) { FDateTime D; D.Ticks = T; return D; }
	int64 ToUnixTimestamp() const { return Ticks; }
	int32 GetYear() const { return 0; }
	int32 GetMonth() const { return 0; }
	int32 GetDay() const { return 0; }
	int32 GetHour() const { return 0; }
	int32 GetHour12() const { return 0; }
	int32 GetMinute() const { return 0; }
	int32 GetSecond() const { return 0; }
	int32 GetDayOfYear() const { return 0; }
	FString ToString() const { return FString(); }
	FString ToString(const TCHAR* Format) const { (void)Format; return FString(); }
	FTimespan operator-(const FDateTime& O) const { return FTimespan{(double)(Ticks - O.Ticks)}; }
};

// 콘솔 변수
enum EConsoleVariableFlags { ECVF_Default = 0, ECVF_Cheat = 1, ECVF_SetByConstructor = 0, ECVF_SetByScalability = 1 << 24, ECVF_SetByGameSetting = 2 << 24, ECVF_SetByProjectSetting = 3 << 24, ECVF_SetByCode = 10 << 24, ECVF_SetByConsole = 13 << 24 };
class IConsoleVariable
{
public:
	virtual ~IConsoleVariable() = default;
	void Set(const TCHAR* Value, EConsoleVariableFlags Flags = ECVF_SetByCode) { (void)Value; (void)Flags; }
	void Set(bool Value, EConsoleVariableFlags Flags = ECVF_SetByCode) { (void)Value; (void)Flags; }
	void Set(int32 Value, EConsoleVariableFlags Flags = ECVF_SetByCode) { (void)Value; (void)Flags; }
	void Set(float Value, EConsoleVariableFlags Flags = ECVF_SetByCode) { (void)Value; (void)Flags; }
	int32 GetInt() const { return 0; }
	float GetFloat() const { return 0.f; }
	bool GetBool() const { return false; }
	FString GetString() const { return FString(); }
};
class IConsoleManager
{
public:
	static IConsoleManager& Get() { static IConsoleManager M; return M; }
	IConsoleVariable* FindConsoleVariable(const TCHAR* Name, bool bTrackFrequentCalls = true) const { (void)Name; (void)bTrackFrequentCalls; return nullptr; }
};

// ---------------------------------------------------------------- 국제화
class FCulture
{
public:
	FString GetTwoLetterISOLanguageName() const { return FString(); }
	FString GetName() const { return FString(); }
	FString GetDisplayName() const { return FString(); }
};
using FCultureRef = TSharedRef<const FCulture>;
using FCulturePtr = TSharedPtr<const FCulture>;
class FInternationalization
{
public:
	static FInternationalization& Get() { static FInternationalization I; return I; }
	FCultureRef GetCurrentLanguage() const { return MakeShared<const FCulture>(); }
	FCultureRef GetCurrentCulture() const { return MakeShared<const FCulture>(); }
};

// ---------------------------------------------------------------- UObject
enum EObjectFlags : uint32 { RF_NoFlags = 0, RF_Public = 1, RF_Standalone = 2, RF_Transient = 4, RF_Transactional = 8 };
inline EObjectFlags operator|(EObjectFlags A, EObjectFlags B) { return (EObjectFlags)((uint32)A | (uint32)B); }
enum ELoadFlags : uint32 { LOAD_None = 0, LOAD_NoWarn = 1, LOAD_Quiet = 2 };
inline ELoadFlags operator|(ELoadFlags A, ELoadFlags B) { return (ELoadFlags)((uint32)A | (uint32)B); }
class UClass;
class FProperty;
class UPackage;
class UWorld;
class UObject
{
public:
	virtual ~UObject() = default;
	void AddToRoot() {}
	void RemoveFromRoot() {}
	bool IsRooted() const { return false; }
	FString GetName() const { return FString(); }
	FString GetPathName() const { return FString(); }
	UPackage* GetOutermost() const { return nullptr; }
	UPackage* GetPackage() const { return nullptr; }
	bool MarkPackageDirty() const { return true; }
	virtual UWorld* GetWorld() const { return nullptr; }
	bool IsValidLowLevel() const { return true; }
	void ConditionalBeginDestroy() {}
	static UClass* StaticClass() { return nullptr; }
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) { (void)PropertyAboutToChange; }
	virtual void PostEditChange() {}
#endif
};
class UClass : public UObject {};
class UPackage : public UObject
{
public:
	void FullyLoad() {}
	void SetDirtyFlag(bool b) { (void)b; }
	static bool SavePackage(UPackage* InOuter, UObject* InAsset, const TCHAR* Filename, const struct FSavePackageArgs& SaveArgs);
};
struct FSavePackageArgs
{
	EObjectFlags TopLevelFlags = RF_NoFlags;
	bool bForceByteSwapping = false;
	bool bWarnOfLongFilename = true;
	uint32 SaveFlags = 0;
	void* Error = nullptr;
};
inline bool UPackage::SavePackage(UPackage* InOuter, UObject* InAsset, const TCHAR* Filename, const FSavePackageArgs& SaveArgs) { (void)InOuter; (void)InAsset; (void)Filename; (void)SaveArgs; return true; }
inline UPackage* CreatePackage(const TCHAR* PackageName) { (void)PackageName; return nullptr; }
template <typename T> inline T* NewObject(UObject* Outer = nullptr, FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags) { static_assert(std::is_base_of_v<UObject, T>, "NewObject needs a UObject type"); (void)Outer; (void)Name; (void)Flags; return nullptr; }
template <typename T> inline T* FindObject(UObject* Outer, const TCHAR* Name, bool bExactClass = false) { static_assert(std::is_base_of_v<UObject, T>); (void)Outer; (void)Name; (void)bExactClass; return nullptr; }
template <typename T> inline T* LoadObject(UObject* Outer, const TCHAR* Name, const TCHAR* Filename = nullptr, uint32 LoadFlags = LOAD_None, void* Sandbox = nullptr) { static_assert(std::is_base_of_v<UObject, T>); (void)Outer; (void)Name; (void)Filename; (void)LoadFlags; (void)Sandbox; return nullptr; }
template <typename T, typename F> inline T* Cast(F* Src) { static_assert(std::is_base_of_v<UObject, T> && std::is_base_of_v<UObject, F>, "Cast needs UObject types"); return dynamic_cast<T*>(Src); }
template <typename T, typename F> inline const T* Cast(const F* Src) { static_assert(std::is_base_of_v<UObject, T> && std::is_base_of_v<UObject, F>, "Cast needs UObject types"); return dynamic_cast<const T*>(Src); }
template <typename T, typename F> inline T* CastChecked(F* Src) { return Cast<T>(Src); }
inline bool IsValid(const UObject* O) { return O != nullptr; }

template <typename T> class TObjectPtr
{
	T* P = nullptr;

public:
	TObjectPtr() = default;
	TObjectPtr(std::nullptr_t) {}
	TObjectPtr(T* In) : P(In) {}
	TObjectPtr& operator=(T* In) { P = In; return *this; }
	TObjectPtr& operator=(std::nullptr_t) { P = nullptr; return *this; }
	operator T*() const { return P; }
	T* operator->() const { return P; }
	T& operator*() const { return *P; }
	T* Get() const { return P; }
	explicit operator bool() const { return P != nullptr; }
};
template <typename T> class TWeakObjectPtr
{
	T* P = nullptr;

public:
	TWeakObjectPtr() = default;
	TWeakObjectPtr(T* In) : P(In) {}
	T* Get() const { return P; }
	bool IsValid() const { return P != nullptr; }
	T* operator->() const { return P; }
	void Reset() { P = nullptr; }
};
template <typename T> class TStrongObjectPtr
{
	T* P = nullptr;

public:
	TStrongObjectPtr() = default;
	explicit TStrongObjectPtr(T* In) : P(In) {}
	T* Get() const { return P; }
	bool IsValid() const { return P != nullptr; }
	T* operator->() const { return P; }
	void Reset(T* In = nullptr) { P = In; }
	explicit operator bool() const { return P != nullptr; }
};
struct FSoftObjectPath
{
	FSoftObjectPath() = default;
	FSoftObjectPath(const TCHAR* Path) { (void)Path; }
	FSoftObjectPath(const FString& Path) { (void)Path; }
	UObject* TryLoad() const { return nullptr; }
	UObject* ResolveObject() const { return nullptr; }
};

// 리플렉션 매크로 (가짜 헤더에선 비움)
#define UCLASS(...)
#define USTRUCT(...)
#define UENUM(...)
#define UPROPERTY(...)
#define UFUNCTION(...)
#define UMETA(...)
#define GENERATED_BODY(...)
#define GENERATED_UCLASS_BODY(...)
#define GENERATED_USTRUCT_BODY(...)

// ---------------------------------------------------------------- 텍스처·렌더
enum TextureFilter { TF_Nearest, TF_Bilinear, TF_Trilinear, TF_Default };
enum TextureCompressionSettings { TC_Default, TC_Normalmap, TC_Masks, TC_Grayscale, TC_Displacementmap, TC_VectorDisplacementmap, TC_HDR, TC_EditorIcon, TC_Alpha, TC_BC7 };
enum TextureGroup { TEXTUREGROUP_World, TEXTUREGROUP_UI, TEXTUREGROUP_Pixels2D };
enum TextureMipGenSettings { TMGS_FromTextureGroup, TMGS_SimpleAverage, TMGS_Sharpen0, TMGS_NoMipmaps, TMGS_LeaveExistingMips };
enum ETextureSourceFormat { TSF_Invalid, TSF_G8, TSF_BGRA8, TSF_BGRE8, TSF_RGBA16, TSF_RGBA16F };
enum EPixelFormat : uint8 { PF_Unknown, PF_A32B32G32R32F, PF_B8G8R8A8, PF_R8G8B8A8, PF_FloatRGBA };
enum EBulkDataLockFlags { LOCK_READ_ONLY = 1, LOCK_READ_WRITE = 2 };
enum class ERHIAccess : uint32 { Unknown = 0, CPURead = 1, Present = 2, IndirectArgs = 4, VertexOrIndexBuffer = 8, SRVCompute = 16, SRVGraphics = 32, SRVMask = SRVCompute | SRVGraphics, CopySrc = 64, CopyDest = 128, RTV = 256, UAVMask = 512 };

struct FByteBulkData
{
	void* Lock(uint32 LockFlags) { (void)LockFlags; return nullptr; }
	const void* LockReadOnly() const { return nullptr; }
	void Unlock() const {}
	int64 GetBulkDataSize() const { return 0; }
};
struct FTexture2DMipMap
{
	int32 SizeX = 0, SizeY = 0;
	FByteBulkData BulkData;
};
template <typename T> struct TIndirectArray
{
	std::vector<T> V = std::vector<T>(1);
	T& operator[](int32 I) { return V[(SIZE_T)I]; }
	int32 Num() const { return (int32)V.size(); }
};
struct FTexturePlatformData
{
	int32 SizeX = 0, SizeY = 0;
	EPixelFormat PixelFormat = PF_Unknown;
	TIndirectArray<FTexture2DMipMap> Mips;
};
struct FTextureSource
{
	void Init(int32 NewSizeX, int32 NewSizeY, int32 NewNumSlices, int32 NewNumMips, ETextureSourceFormat NewFormat, const uint8* NewData = nullptr)
	{ (void)NewSizeX; (void)NewSizeY; (void)NewNumSlices; (void)NewNumMips; (void)NewFormat; (void)NewData; }
};
class FRHITexture
{
public:
	FIntPoint GetSizeXY() const { return FIntPoint(); }
};
template <typename T> class TRefCountPtr
{
	T* P = nullptr;

public:
	T* GetReference() const { return P; }
	bool IsValid() const { return P != nullptr; }
	T* operator->() const { return P; }
	operator T*() const { return P; }
};
using FTextureRHIRef = TRefCountPtr<FRHITexture>;
using FTexture2DRHIRef = TRefCountPtr<FRHITexture>;
struct FReadSurfaceDataFlags {};
class FTextureResource
{
public:
	virtual ~FTextureResource() = default;
	FTextureRHIRef TextureRHI;
	FRHITexture* GetTexture2DRHI() const { return TextureRHI.GetReference(); }
	uint32 GetSizeX() const { return 0; }
	uint32 GetSizeY() const { return 0; }
};
class FRenderTarget
{
public:
	virtual ~FRenderTarget() = default;
	bool ReadPixels(TArray<FColor>& OutImageData, FReadSurfaceDataFlags InFlags = FReadSurfaceDataFlags()) { (void)OutImageData; (void)InFlags; return false; }
	FIntPoint GetSizeXY() const { return FIntPoint(); }
};
class FTextureRenderTargetResource : public FTextureResource, public FRenderTarget {};

class UTexture : public UObject
{
public:
	uint8 SRGB : 1 = 1;
	uint8 NeverStream : 1 = 0;
	TextureFilter Filter = TF_Default;
	TextureCompressionSettings CompressionSettings = TC_Default;
	TextureGroup LODGroup = TEXTUREGROUP_World;
#if WITH_EDITORONLY_DATA
	TextureMipGenSettings MipGenSettings = TMGS_FromTextureGroup;
	FTextureSource Source;
#endif
	void UpdateResource() {}
	FTextureResource* GetResource() const { return nullptr; }
};
class UTexture2D : public UTexture
{
public:
	static UTexture2D* CreateTransient(int32 InSizeX, int32 InSizeY, EPixelFormat InFormat = PF_B8G8R8A8, const FName InName = NAME_None) { (void)InSizeX; (void)InSizeY; (void)InFormat; (void)InName; return nullptr; }
	int32 GetSizeX() const { return 0; }
	int32 GetSizeY() const { return 0; }
	FTexturePlatformData* GetPlatformData() { return nullptr; }
	const FTexturePlatformData* GetPlatformData() const { return nullptr; }
};
class UTextureRenderTarget2D : public UTexture
{
public:
	int32 SizeX = 0, SizeY = 0;
	FLinearColor ClearColor = FLinearColor::Black;
	EPixelFormat OverrideFormat = PF_Unknown;
	FTextureRenderTargetResource* GameThread_GetRenderTargetResource() { return nullptr; }
	void InitAutoFormat(uint32 InSizeX, uint32 InSizeY) { (void)InSizeX; (void)InSizeY; }
	void UpdateResourceImmediate(bool bClearRenderTarget = true) { (void)bClearRenderTarget; }
};

struct FRHITransitionInfo
{
	FRHITransitionInfo(FRHITexture* InTexture, ERHIAccess InPrevious, ERHIAccess InNext) { (void)InTexture; (void)InPrevious; (void)InNext; }
};
struct FRHICopyTextureInfo
{
	FIntVector Size = FIntVector(0, 0, 0);
	FIntVector SourcePosition = FIntVector(0, 0, 0);
	FIntVector DestPosition = FIntVector(0, 0, 0);
	uint32 SourceSliceIndex = 0, DestSliceIndex = 0, NumSlices = 1, SourceMipIndex = 0, DestMipIndex = 0, NumMips = 1;
};
class FRHICommandListImmediate
{
public:
	void Transition(TArrayView<const FRHITransitionInfo> Infos) { (void)Infos; }
	void CopyTexture(FRHITexture* SourceTexture, FRHITexture* DestTexture, const FRHICopyTextureInfo& CopyInfo) { (void)SourceTexture; (void)DestTexture; (void)CopyInfo; }
};
struct FFakeRenderCommandEnqueuer
{
	template <typename F> void operator()(F&& Lambda) const
	{
		static_assert(std::is_invocable_v<F&, FRHICommandListImmediate&>, "render command lambda must take FRHICommandListImmediate&");
		(void)Lambda;
	}
};
#define ENQUEUE_RENDER_COMMAND(Type) FFakeRenderCommandEnqueuer()
inline void FlushRenderingCommands() {}

struct FScreenshotRequest
{
	static void RequestScreenshot(const FString& InFilename, bool bInShowUI, bool bAddFilenameSuffix, bool bHdrScreenshot = false) { (void)InFilename; (void)bInShowUI; (void)bAddFilenameSuffix; (void)bHdrScreenshot; }
	static void RequestScreenshot(bool bInShowUI) { (void)bInShowUI; }
};

namespace Algo
{
template <typename R> void Reverse(R& Range) { std::reverse(Range.begin(), Range.end()); }
template <typename R, typename P> void Sort(R& Range, P Pred) { std::sort(Range.begin(), Range.end(), Pred); }
template <typename R> void Sort(R& Range) { std::sort(Range.begin(), Range.end()); }
} // namespace Algo

struct FPackageName
{
	static FString LongPackageNameToFilename(const FString& InLongPackageName, const FString& InExtension = FString()) { return InLongPackageName + InExtension; }
	static const FString& GetAssetPackageExtension() { static FString E(TEXT(".uasset")); return E; }
	static const FString& GetMapPackageExtension() { static FString E(TEXT(".umap")); return E; }
};

#include "FakeSlate.h"
#include "FakeEngine.h"
