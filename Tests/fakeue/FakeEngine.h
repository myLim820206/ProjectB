// 가짜 엔진 (게임 모드·설정·오디오·나이아가라). FakeUE.h 에서 포함
#pragma once

#include <future>

namespace EWindowMode { enum Type { Fullscreen, WindowedFullscreen, Windowed, NumWindowModes }; }
class UGameUserSettings : public UObject
{
public:
	EWindowMode::Type GetFullscreenMode() const { return EWindowMode::Windowed; }
	void SetFullscreenMode(EWindowMode::Type InFullscreenMode) { (void)InFullscreenMode; }
	FIntPoint GetScreenResolution() const { return FIntPoint(); }
	void SetScreenResolution(FIntPoint Resolution) { (void)Resolution; }
	FIntPoint GetDesktopResolution() const { return FIntPoint(); }
	void SetFrameRateLimit(float NewLimit) { (void)NewLimit; }
	void SetVSyncEnabled(bool bEnable) { (void)bEnable; }
	void SetOverallScalabilityLevel(int32 Value) { (void)Value; }
	void SetResolutionScaleNormalized(float NewScaleNormalized) { (void)NewScaleNormalized; }
	void ApplySettings(bool bCheckForCommandLineOverrides) { (void)bCheckForCommandLineOverrides; }
	void ApplyNonResolutionSettings() {}
	void ApplyResolutionSettings(bool bCheckForCommandLineOverrides) { (void)bCheckForCommandLineOverrides; }
	void SaveSettings() {}
	void ConfirmVideoMode() {}
};
class UGameViewportClient : public UObject
{
public:
	void AddViewportWidgetContent(TSharedRef<SWidget> ViewportContent, const int32 ZOrder = 0) { (void)ViewportContent; (void)ZOrder; }
	void RemoveViewportWidgetContent(TSharedRef<SWidget> ViewportContent) { (void)ViewportContent; }
	void GetViewportSize(FVector2D& OutViewportSize) const { (void)OutViewportSize; }
};
class UEngine : public UObject
{
public:
	UGameViewportClient* GameViewport = nullptr;
	UGameUserSettings* GetGameUserSettings() { return nullptr; }
};
inline UEngine* GEngine = nullptr;

// ---------------------------------------------------------------- 액터·월드
namespace EEndPlayReason { enum Type { Destroyed, LevelTransition, EndPlayInEditor, RemovedFromWorld, Quit }; }
enum class ESpawnActorCollisionHandlingMethod : uint8 { Undefined, AlwaysSpawn, AdjustIfPossibleButAlwaysSpawn, AdjustIfPossibleButDontSpawnIfColliding, DontSpawnIfColliding };
struct FActorSpawnParameters
{
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;
	FName Name;
};
struct FActorTickFunction
{
	uint8 bCanEverTick : 1 = 0;
	uint8 bTickEvenWhenPaused : 1 = 0;
	uint8 bStartWithTickEnabled : 1 = 1;
};
class AActor : public UObject
{
public:
	FActorTickFunction PrimaryActorTick;
	virtual void BeginPlay() {}
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) { (void)EndPlayReason; }
	virtual void Tick(float DeltaSeconds) { (void)DeltaSeconds; }
	bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true) { (void)bNetForce; (void)bShouldModifyLevel; return true; }
};
class APawn : public AActor {};
class UActorComponent : public UObject
{
public:
	void DestroyComponent(bool bPromoteChildren = false) { (void)bPromoteChildren; }
};
class USceneComponent : public UActorComponent {};
class UClass;
template <typename T> class TSubclassOf
{
	UClass* C = nullptr;

public:
	TSubclassOf() = default;
	TSubclassOf(std::nullptr_t) {}
	TSubclassOf(UClass* In) : C(In) {}
	operator UClass*() const { return C; }
};
enum class EMouseLockMode : uint8 { DoNotLock, LockOnCapture, LockAlways, LockInFullscreen };
struct FInputModeUIOnly
{
	FInputModeUIOnly& SetWidgetToFocus(TSharedPtr<SWidget> InWidgetToFocus) { (void)InWidgetToFocus; return *this; }
	FInputModeUIOnly& SetLockMouseToViewportBehavior(EMouseLockMode InMouseLockMode) { (void)InMouseLockMode; return *this; }
};
class APlayerController : public AActor
{
public:
	uint32 bShowMouseCursor : 1 = 0;
	void SetInputMode(const FInputModeUIOnly& InData) { (void)InData; }
	void SetViewTarget(AActor* NewViewTarget) { (void)NewViewTarget; }
};
class UWorld : public UObject
{
public:
	APlayerController* GetFirstPlayerController() const { return nullptr; }
	template <typename T> T* SpawnActor(const FVector& Location, const FRotator& Rotation, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters())
	{ static_assert(std::is_base_of_v<AActor, T>); (void)Location; (void)Rotation; (void)SpawnParameters; return nullptr; }
	double GetTimeSeconds() const { return 0.0; }
};
class AGameModeBase : public AActor
{
public:
	TSubclassOf<APawn> DefaultPawnClass;
};

// ---------------------------------------------------------------- 카메라
namespace ECameraProjectionMode { enum Type { Perspective, Orthographic }; }
enum EAspectRatioAxisConstraint { AspectRatio_MaintainYFOV, AspectRatio_MaintainXFOV, AspectRatio_MajorAxisFOV };
enum EAutoExposureMethod { AEM_Histogram, AEM_Basic, AEM_Manual };
enum class EDynamicGlobalIlluminationMethod : uint8 { None, Lumen, ScreenSpace, Plugin };
enum class EReflectionMethod : uint8 { None, Lumen, ScreenSpace };
struct FPostProcessSettings
{
	uint8 bOverride_BloomIntensity : 1 = 0, bOverride_BloomThreshold : 1 = 0, bOverride_AutoExposureMethod : 1 = 0, bOverride_AutoExposureBias : 1 = 0,
		bOverride_AutoExposureApplyPhysicalCameraExposure : 1 = 0, bOverride_VignetteIntensity : 1 = 0, bOverride_DynamicGlobalIlluminationMethod : 1 = 0,
		bOverride_ReflectionMethod : 1 = 0, bOverride_AmbientOcclusionIntensity : 1 = 0, bOverride_MotionBlurAmount : 1 = 0;
	float BloomIntensity = 0.f, BloomThreshold = 0.f, AutoExposureBias = 0.f, VignetteIntensity = 0.f, AmbientOcclusionIntensity = 0.f, MotionBlurAmount = 0.f;
	TEnumAsByte<EAutoExposureMethod> AutoExposureMethod = AEM_Histogram;
	uint32 AutoExposureApplyPhysicalCameraExposure : 1 = 1;
	EDynamicGlobalIlluminationMethod DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None;
	EReflectionMethod ReflectionMethod = EReflectionMethod::None;
};
class UCameraComponent : public USceneComponent
{
public:
	uint8 bOverrideAspectRatioAxisConstraint : 1 = 0;
	FPostProcessSettings PostProcessSettings;
	float PostProcessBlendWeight = 1.f;
	void SetProjectionMode(ECameraProjectionMode::Type InProjectionMode) { (void)InProjectionMode; }
	void SetConstraintAspectRatio(bool bInConstrainAspectRatio) { (void)bInConstrainAspectRatio; }
	void SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint InAspectRatioAxisConstraint) { (void)InAspectRatioAxisConstraint; }
	void SetOrthoNearClipPlane(float InOrthoNearClipPlane) { (void)InOrthoNearClipPlane; }
	void SetOrthoFarClipPlane(float InOrthoFarClipPlane) { (void)InOrthoFarClipPlane; }
	void SetOrthoWidth(float InOrthoWidth) { (void)InOrthoWidth; }
};
class ACameraActor : public AActor
{
public:
	UCameraComponent* GetCameraComponent() const { return nullptr; }
};

// ---------------------------------------------------------------- 오디오
#define INDEFINITELY_LOOPING_DURATION 10000.0f
class USoundBase : public UObject
{
public:
	float Duration = 0.f;
};
class USoundWave : public USoundBase
{
public:
	int32 NumChannels = 0;
	uint8 bLooping : 1 = 0;
	void SetSampleRate(uint32 InSampleRate) { (void)InSampleRate; }
};
class USoundWaveProcedural : public USoundWave
{
public:
	void QueueAudio(const uint8* AudioData, const int32 BufferSize) { (void)AudioData; (void)BufferSize; }
	int32 GetAvailableAudioByteCount() const { return 0; }
	void ResetAudio() {}
};
class USoundConcurrency;
class UAudioComponent : public USceneComponent
{
public:
	uint8 bIsUISound : 1 = 0;
	void Play(float StartTime = 0.f) { (void)StartTime; }
	void Stop() {}
	bool IsPlaying() const { return false; }
	void SetVolumeMultiplier(float NewVolumeMultiplier) { (void)NewVolumeMultiplier; }
};
class UGameplayStatics
{
public:
	static UAudioComponent* CreateSound2D(const UObject* WorldContextObject, USoundBase* Sound, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, USoundConcurrency* ConcurrencySettings = nullptr, bool bPersistAcrossLevelTransition = false, bool bAutoDestroy = true)
	{ (void)WorldContextObject; (void)Sound; (void)VolumeMultiplier; (void)PitchMultiplier; (void)StartTime; (void)ConcurrencySettings; (void)bPersistAcrossLevelTransition; (void)bAutoDestroy; return nullptr; }
};
namespace EQuitPreference { enum Type { Quit, Background }; }
class UKismetSystemLibrary
{
public:
	static void QuitGame(const UObject* WorldContextObject, APlayerController* SpecificPlayer, TEnumAsByte<EQuitPreference::Type> QuitPreference, bool bIgnorePlatformRestrictions)
	{ (void)WorldContextObject; (void)SpecificPlayer; (void)QuitPreference; (void)bIgnorePlatformRestrictions; }
};

// ---------------------------------------------------------------- 나이아가라
class UNiagaraSystem : public UObject {};
enum class ENCPoolMethod : uint8 { None, AutoRelease, ManualRelease, ManualRelease_OnComplete, FreeInPool };
class UNiagaraComponent : public USceneComponent
{
public:
	void SetVariableLinearColor(FName InVariableName, const FLinearColor& InValue) { (void)InVariableName; (void)InValue; }
	void SetVariableFloat(FName InVariableName, float InValue) { (void)InVariableName; (void)InValue; }
	void SetVariableVec3(FName InVariableName, FVector InValue) { (void)InVariableName; (void)InValue; }
};
class UNiagaraFunctionLibrary
{
public:
	static UNiagaraComponent* SpawnSystemAtLocation(const UObject* WorldContextObject, UNiagaraSystem* SystemTemplate, FVector Location, FRotator Rotation = FRotator::ZeroRotator, FVector Scale = FVector(1.f),
		bool bAutoDestroy = true, bool bAutoActivate = true, ENCPoolMethod PoolingMethod = ENCPoolMethod::None, bool bPreCullCheck = true)
	{ (void)WorldContextObject; (void)SystemTemplate; (void)Location; (void)Rotation; (void)Scale; (void)bAutoDestroy; (void)bAutoActivate; (void)PoolingMethod; (void)bPreCullCheck; return nullptr; }
};

// ---------------------------------------------------------------- 비동기
enum class EAsyncExecution { TaskGraph, TaskGraphMainThread, Thread, ThreadIfForkSafe, ThreadPool, LargeThreadPool };
template <typename T> class TFuture
{
	std::shared_future<T> F;

public:
	TFuture() = default;
	explicit TFuture(std::shared_future<T> In) : F(std::move(In)) {}
	bool IsValid() const { return F.valid(); }
	bool IsReady() const { return F.valid() && F.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }
	const T& Get() const { return F.get(); }
	T Consume() { T V = F.get(); F = std::shared_future<T>(); return V; } // 결과를 꺼내고 비운다 (엔진의 TFuture::Consume)
	void Wait() const { F.wait(); }
};
template <typename F> auto Async(EAsyncExecution Execution, F&& Function, TUniqueFunction<void()> CompletionCallback = nullptr) -> TFuture<std::invoke_result_t<F>>
{
	(void)Execution; (void)CompletionCallback;
	return TFuture<std::invoke_result_t<F>>(std::async(std::launch::deferred, std::forward<F>(Function)).share());
}
