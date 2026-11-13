#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/SCompoundWidget.h"
#include "PDCore.h"

// 포커 디펜스 화면 전체. 1280 x 720 기준 좌표로 그리고 창 크기에 맞춰 확대한다.
// 그래픽은 도형과 선뿐이고, 네온 빛번짐은 굵고 옅은 선 위에 가늘고 밝은 선을 겹쳐 흉내 낸다 (docs/15-art-style.md).
// Slate 이펙트(빛줄기, 파티클, 파동, 별빛, 몸 조각, 흔들림, 배너)는 이 위젯이 그리고, 나이아가라 월드 이펙트는 요청만 쌓는다.
// 파일: PDBoardWidget.cpp 틱·입력 / PDBoardFx.cpp 이펙트 / PDBoardPaint.cpp 그리기 / PDBoardDraw.h 그리기 도우미
class SPDBoardWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPDBoardWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// 게임이 낸 소리를 꺼내 간다 (게임 모드가 매 프레임 재생)
	std::vector<PD::FPDSoundEvent> TakeSounds()
	{
		std::vector<PD::FPDSoundEvent> Out;
		Out.swap(Game.Sounds);
		return Out;
	}

	// 월드 이펙트 층 (D-092): 판 뒤 3D 월드에서 나이아가라로 재생할 이펙트 요청.
	// 위치는 "화면 가운데를 0으로 한 판 좌표(px)", 아래가 +Y. 게임 모드가 정사영 카메라로 1 월드 단위 = 판 1px 로 맞춘다
	// Hit·Death 는 공통, Imp* 는 발사체 무리별 타격 (D-094), 그 뒤는 그 밖의 사건 (D-100). 순서는 게임 모드의 에셋 이름 표와 같다
	enum class EWorldFx : uint8 { Hit, Death, ImpSpark, ImpBlast, ImpFrost, ImpZap, ImpGlitter, ImpStarburst,
		Muzzle, Crit, Splash, Place, Synergy, Fuse, JokerFuse, Leak, Count };
	struct FWorldFx
	{
		EWorldFx Kind;
		FVector2f Pos;
		FLinearColor Col;
		float Scale; // 맞음·타격: 위력 (1~6), 죽음: 몸 크기 (작은 적 0.7, 보스 4), 총구: 레벨, 치명타: 횟수, 범위: 반지름 칸, 족보: 1 + 점수/40, 합성: 새 레벨
		FVector2f Dir; // 판 좌표 방향 (단위 벡터, 아래가 +Y). 원뿔로 튀는 이펙트가 쓴다
	};
	TArray<FWorldFx> TakeWorldFx()
	{
		TArray<FWorldFx> Out;
		Swap(Out, WorldFx);
		return Out;
	}
	// 정사영 카메라 폭 (월드 단위 = 판 px). 창 전체 폭을 판 배율로 나눈 값
	float GetWorldViewWidth() const { return ViewSize.X / FMath::Max(0.01f, ViewScale); }

	// 소리 크기 0~1 (설정에서 끄면 0). 게임 모드가 소리마다 곱한다 (D-110)
	float GetVolume() const { return Settings.bSound ? Settings.Volume : 0.f; }
	float GetMusicVolume() const { return Settings.bSound ? Settings.MusicVolume : 0.f; } // 배경 음악 (D-117)

	// 메뉴의 [게임 종료]. 게임 모드가 채운다 (월드가 있어야 PIE·패키지 모두 바르게 끝낼 수 있어서)
	TFunction<void()> OnQuit;

private:
	struct FCanvas; // 그리기 도우미 (PDBoardDraw.h)
	void PaintAll(FCanvas& C, FCanvas& F) const; // C = 고정 화면, F = 흔들림을 받는 판
	void PaintField(FCanvas& F) const;     // 판, 족보 장판, 타워, 적
	void PaintShots(FCanvas& F) const;     // 날아가는 발사체
	void PaintEffects(FCanvas& F) const;   // 빛줄기, 별빛, 몸 조각, 파동, 파티클
	void PaintBoardUi(FCanvas& F) const;   // 족보 이름표, 합성·먹이기·조커 버튼, 타워 정보, 떠오르는 글자, 배너
	void PaintPanels(FCanvas& C) const;    // 손패, 오른쪽 상태 패널, 부적
	void PaintScreens(FCanvas& C) const;   // 부적 고르기, 조커 도감, 골드 강화 창
	void PaintTree(FCanvas& C) const;      // 스킬 트리 창
	void PaintHandGuide(FCanvas& C) const; // 왼쪽 족보 패널
	void PaintHud(FCanvas& C) const;       // 위쪽 상태 띠 (D-119)
	void PaintCloseButton(FCanvas& C, const FLinearColor& Col) const; // 도감·트리 창 닫기 버튼
	void PaintMenu(FCanvas& C) const;      // Esc 메뉴 창
	void MenuSelect(int32 Item);           // 메뉴 항목 실행
	int32 MenuHoverAt(FVector2f P) const;  // 메뉴 창에서 이 위치의 항목 (-1 = 없음)
	void RestartRun();                     // 새 판 시작 (게임 오버·메뉴 공통)
	// 합성 위치 고르기: [합성]·[조커] 버튼을 누르면 재료 칸 중 하나를 클릭해 결과가 놓일 칸을 고른다
	std::vector<int> PickCells;            // 고르는 중인 재료 칸 (비어 있으면 고르는 중 아님)
	bool bPickJoker = false;               // 조커 합성을 고르는 중
	void StartPick(const std::vector<int>& Cells, bool bJoker);
	void FinishPick(int32 Cell);           // 고른 칸으로 합성 (-1 = 기본 칸)
	int32 FindPickIndex() const;           // 고르는 재료와 같은 족보·조커 묶음 번호 (-1 = 사라짐)
	bool bMenu = false;                    // 메뉴 창 (Esc). 여는 동안 게임은 멈춘다
	// ---- 설정 (D-110): GameUserSettings.ini [ProjectB.Settings]. 화면 모드는 엔진 GameUserSettings 가 저장
	struct FSettings
	{
		int32 WindowMode = 0; // 0 창, 1 전체화면, 2 테두리 없는 전체화면
		bool bWorldFx = true; // 나이아가라 월드 이펙트
		bool bShake = true;   // 화면 흔들림·번쩍임
		bool bSound = true;
		float Volume = 0.8f;
		float MusicVolume = 0.6f; // 배경 음악 (D-117)
		int32 FrameCap = 0;       // 프레임 제한 (D-120): 0 = 60, 1 = 120, 2 = 제한 없음
		bool bVSync = false;      // 수직 동기화
	};
	FSettings Settings;
	bool bSettings = false;                // 설정 창 (메뉴에서 연다, 여는 동안 메뉴도 열려 있어 게임은 멈춤)
	int32 SetSel = 0;                      // 키보드로 고른 설정 줄
	void LoadUserData();                   // 설정 + 진행(도감·최고 기록) 불러오기
	void SaveSettings() const;
	void SaveProgress();                   // 도감·최고 기록 저장 (Saved/SaveGames/Progress.txt)
	void ApplyWindowMode(int32 Mode);
	void ApplyFrameSettings() const;       // 프레임 제한·수직 동기화를 엔진에 (t.MaxFPS, r.VSync)
	void SetSound(bool bOn);
	void SettingsClick(FVector2f P);
	void SettingsKey(const FKey& K);
	void SettingsChange(int32 Row, int32 Delta); // 줄 값 바꾸기 (Delta: -1/+1, 0 = 켜기·끄기 뒤집기)
	void PaintSettings(FCanvas& C) const;
	uint32 SavedDiscovered = 0;
	int32 SavedBest = 0;
	int32 MenuSel = 0;                     // 키보드로 고른 메뉴 항목
	void HandleEvents();
	void UiSound(PD::EPDSound S);           // 화면 조작 소리 (음소거면 안 냄)
	void UpdateFx(float Dt);
	void UpdatePreview();
	FVector2f ToDesign(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;
	void Click(FVector2f P, bool bRight);
	void DoFuse(int32 SynergyIdx, int32 Cell = -1); // 합성 + 빨려 들어가는 연출 준비 (Cell = 결과 칸, -1 = 기본)
	int32 HoveredSynergy() const;      // 마우스가 올라간 타워의 족보 번호
	// 판 위 합성·먹이기·조커 버튼은 그 족보 타워(또는 버튼)에 마우스가 있을 때만 보이고 눌린다 (D-113). 평소엔 작은 점
	bool SynRevealed(int32 SynergyIdx) const;
	bool JokerRevealed(const PD::FPDJokerGroup& Gr) const;
	int32 TreeHoverAt(FVector2f P) const; // 트리 창에서 이 위치의 노드 (-1 = 없음)
	TArray<FVector2f> PendingFuseCells; // 합성 직전 족보 칸 위치 (연출용)

	// ---- 이펙트
	struct FParticle
	{
		FVector2f P, V;
		float Life, MaxLife, Size, Drag, Gravity;
		FLinearColor Col;
		bool bStreak;
	};
	struct FBeam
	{
		FVector2f A, B;
		float T, MaxT, Width;
		FLinearColor Col;
		int32 Style = 0; // 0 보통(레벨 2면 쌍줄), 1 레일건, 2 번개, 3 로열 왕관, 4 체셔 웃음
		int32 Lv = 1;
	};
	// 퍼지는 파동. 모양이 여러 가지라 맞을 때마다 동그라미만 퍼지지 않는다 (R1 < R0 이면 안으로 빨려 든다, T 음수면 늦게)
	enum class EWave : uint8_t { Circle, Poly, Suit, Broken, Star, Rays };
	struct FRing
	{
		FVector2f C;
		float R0, R1, T, MaxT, Width;
		FLinearColor Col;
		EWave Shape = EWave::Circle;
		int32 N = 0;      // Poly: 변 수, Suit: 무늬 번호, Broken: 조각 수, Star: 뿔 수, Rays: 빛줄기 수
		float Rot = 0.f;  // 시작 각
		float Spin = 0.f; // 초당 회전
	};
	struct FFloatText
	{
		FString S;
		FVector2f P;
		float T, MaxT, Size;
		FLinearColor Col;
	};
	struct FCellFlash
	{
		int Key;
		float T;
		FLinearColor Col;
	};
	// 번쩍이는 별빛 (네 갈래 빛살 + 부드러운 빛 덩어리)
	struct FFlare
	{
		FVector2f C;
		float T, MaxT, Size, Rot; // T 가 음수면 그만큼 늦게 나타난다
		FLinearColor Col;
		int32 Spokes = 4; // 빛살 갈래 수 (0 = 빛 덩어리만, 2 = 한 줄)
	};
	// 몸 조각: 돌며 날아가는 짧은 선
	struct FShard
	{
		FVector2f P, V;
		float A, AV, Len, Life, MaxLife;
		FLinearColor Col;
	};
	TArray<FFlare> Flares;
	TArray<FShard> Shards;
	TMap<int32, float> HitFlash; // 적 Id → 맞은 순간 번쩍임 남은 시간
	TMap<int32, float> HpShown;  // 적 Id → 천천히 줄어드는 체력 막대 (깎인 만큼 흰색으로 보임)
	void HitFx(FVector2f P, float Power, int32 Crits, float Dmg);                  // 맞는 순간 공통: 적 번쩍임, 피해 숫자, 작은 흰 섬광
	void ImpactFx(PD::EPDShot K, FVector2f P, FVector2f Dir, FLinearColor Col, int32 Lv, float Power); // 발사체마다 다른 타격 효과
	void BurstCone(FVector2f P, FVector2f Dir, float Spread, int N, FLinearColor Col, float SpeedMin, float SpeedMax, float Life, float Size, bool bStreak, float Gravity = 0.f);
	void AddShards(FVector2f P, int N, FLinearColor Col, float SpeedMin, float SpeedMax, float LenMin, float LenMax, float Life);
	void DeathFx(FVector2f P, PD::EPDEnemy Type);                                   // 죽는 순간
	void CritFx(FVector2f P, int32 N); // 치명타 흰 불꽃
	void Burst(FVector2f P, int N, FLinearColor Col, float SpeedMin, float SpeedMax, float Life, float Size, bool bStreak = false, float Gravity = 0.f);
	// PendingFuseCells 에서 P 로 빨려 드는 입자 (합성·조커 합성·먹이기). 색은 I % Every 가 0 이면 ColB, 아니면 ColA
	void SuckIn(FVector2f P, int PerCell, float Jitter, float Life, float SizeMin, float SizeMax, const FLinearColor& ColA, const FLinearColor& ColB, int Every);

	PD::FPDGame Game;
	FSlateColorBrush WhiteBrush = FSlateColorBrush(FLinearColor::White);

	// 입력과 미리보기
	int32 Sel = -1;
	int32 HoverX = -1, HoverY = -1;
	FVector2f Mouse = FVector2f(0.f, 0.f);
	std::vector<PD::FPDSynergy> Preview; // 고른 카드를 마우스 칸에 놓으면 생길 족보
	std::vector<PD::FPDJokerGroup> JGroups; // 조커 합성 가능한 묶음 (매 프레임 갱신)
	bool bCodex = false;                   // 조커 도감 화면
	int32 CodexSel = 0;                    // 도감 오른쪽에 크게 보여 줄 조커 (D-119)
	float PowerRef = 0.f;                  // 이번 웨이브 시작 때 판 위력 (위쪽 띠의 ±%)
	// 위력 정보 (D-119): 그릴 때 한 번 계산해 패널·타워 정보가 같이 쓴다
	mutable std::vector<std::pair<int, float>> PowerList; // 타워 칸 키, 위력 (큰 것부터)
	mutable float PowerSum = 0.f, PowerGain = 1.f;
	bool bTree = false;                    // 스킬 트리 화면 (K)
	bool bUpg = false;                     // 골드 강화 화면 (U)
	int32 TreeBuyId = -1;                  // 방금 찍은 노드 (연출)
	float TreeBuyT = 0.f;
	FString PreviewKey;

	// 이펙트 상태
	TArray<FParticle> Particles;
	TArray<FBeam> Beams;
	TArray<FRing> Rings;
	TArray<FFloatText> Floats;
	TArray<FCellFlash> CellFlashes;
	TMap<int32, TArray<FVector2f>> Trails; // 빠른 적 잔상
	float Shake = 0.f;
	float FlashT = 0.f;
	FLinearColor FlashCol = FLinearColor::White;
	float BannerT = 0.f, BannerMax = 1.f;
	FString BannerText, BannerSub;
	FLinearColor BannerCol = FLinearColor::White;
	float Clock = 0.f;

	// 그릴 때 정한 화면 배율 (마우스 좌표 변환, 월드 이펙트 좌표에 씀)
	mutable float ViewScale = 1.f;
	mutable FVector2f ViewOffset = FVector2f(0.f, 0.f);
	mutable FVector2f ViewSize = FVector2f(1280.f, 720.f);

	// 월드 이펙트 요청 큐 (게임 모드가 매 프레임 꺼내 감)
	TArray<FWorldFx> WorldFx;
	void PushWorldFx(EWorldFx Kind, FVector2f DesignPos, const FLinearColor& Col, float Scale, FVector2f Dir = FVector2f(0.f, -1.f));
};
