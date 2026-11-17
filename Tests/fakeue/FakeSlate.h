// 가짜 Slate (FakeUE.h 에서 포함). 실제 UE 5.x 시그니처를 따라 적는다
#pragma once

// UE::Slate::FDeprecateVector2DParameter / Result: FVector2f·FVector2D 어느 쪽에서든 받고, 어느 쪽으로든 넘긴다
namespace UE::Slate
{
struct FDeprecateVector2DParameter : FVector2f
{
	FDeprecateVector2DParameter(const FVector2f& V) : FVector2f(V) {}
	FDeprecateVector2DParameter(const FVector2D& V) : FVector2f(V) {}
	FDeprecateVector2DParameter(float InX, float InY) : FVector2f(InX, InY) {}
};
struct FDeprecateVector2DResult : FVector2f
{
	FDeprecateVector2DResult(const FVector2f& V) : FVector2f(V) {}
	operator FVector2D() const { return FVector2D(*this); }
};
} // namespace UE::Slate

class FSlateLayoutTransform
{
	float Scale = 1.f;
	FVector2f Translation;

public:
	explicit FSlateLayoutTransform(float InScale = 1.f) : Scale(InScale) {}
	template <typename T> explicit FSlateLayoutTransform(const TVector2<T>& InTranslation) : Translation(InTranslation) {}
	template <typename T> FSlateLayoutTransform(float InScale, const TVector2<T>& InTranslation) : Scale(InScale), Translation(InTranslation) {}
	float GetScale() const { return Scale; }
	UE::Slate::FDeprecateVector2DResult GetTranslation() const { return Translation; }
};
// Math/Matrix2x2.h: TMatrix2x2<float> (m00, m01, m10, m11)
class FMatrix2x2f
{
public:
	FMatrix2x2f() = default;
	FMatrix2x2f(float m00, float m01, float m10, float m11) { (void)m00; (void)m01; (void)m10; (void)m11; }
};
class FSlateRenderTransform // = TTransform2<float>
{
public:
	FSlateRenderTransform() = default;
	explicit FSlateRenderTransform(float InScale) { (void)InScale; }
	explicit FSlateRenderTransform(const FMatrix2x2f& InM, const FVector2f& InTrans = FVector2f(0.f, 0.f)) { (void)InM; (void)InTrans; }
	FVector2f TransformPoint(const FVector2f& P) const { return P; }
};
struct FSlateRect
{
	float Left = 0, Top = 0, Right = 0, Bottom = 0;
	FSlateRect() = default;
	FSlateRect(float L, float T, float R, float B) : Left(L), Top(T), Right(R), Bottom(B) {}
};
struct FPaintGeometry
{
	FPaintGeometry() = default;
};
class FGeometry
{
public:
	FGeometry() = default;
	FGeometry MakeChild(const UE::Slate::FDeprecateVector2DParameter& InLocalSize, const FSlateLayoutTransform& LayoutTransform) const { (void)InLocalSize; (void)LayoutTransform; return *this; }
	FGeometry MakeChild(const UE::Slate::FDeprecateVector2DParameter& InLocalSize, const FSlateLayoutTransform& LayoutTransform, const FSlateRenderTransform& InRenderTransform, const UE::Slate::FDeprecateVector2DParameter& RenderTransformPivot) const { (void)InLocalSize; (void)LayoutTransform; (void)InRenderTransform; (void)RenderTransformPivot; return *this; }
	FGeometry MakeChild(const UE::Slate::FDeprecateVector2DParameter& ChildOffset, const UE::Slate::FDeprecateVector2DParameter& InLocalSize, float ChildScale = 1.f) const { (void)ChildOffset; (void)InLocalSize; (void)ChildScale; return *this; }
	FPaintGeometry ToPaintGeometry() const { return FPaintGeometry(); }
	FPaintGeometry ToPaintGeometry(const UE::Slate::FDeprecateVector2DParameter& InLocalSize, const FSlateLayoutTransform& InLayoutTransform) const { (void)InLocalSize; (void)InLayoutTransform; return FPaintGeometry(); }
	FPaintGeometry ToPaintGeometry(const FSlateLayoutTransform& InLayoutTransform) const { (void)InLayoutTransform; return FPaintGeometry(); }
	UE::Slate::FDeprecateVector2DResult GetLocalSize() const { return FVector2f(); }
	UE::Slate::FDeprecateVector2DResult GetAbsoluteSize() const { return FVector2f(); }
	UE::Slate::FDeprecateVector2DResult GetAbsolutePosition() const { return FVector2f(); }
	UE::Slate::FDeprecateVector2DResult AbsoluteToLocal(UE::Slate::FDeprecateVector2DParameter AbsoluteCoordinate) const { return FVector2f(AbsoluteCoordinate); }
	UE::Slate::FDeprecateVector2DResult LocalToAbsolute(UE::Slate::FDeprecateVector2DParameter LocalCoordinate) const { return FVector2f(LocalCoordinate); }
	bool IsUnderLocation(const UE::Slate::FDeprecateVector2DParameter& AbsoluteCoordinate) const { (void)AbsoluteCoordinate; return false; }
	const FSlateRenderTransform& GetAccumulatedRenderTransform() const { return RenderTransform; }
	FSlateLayoutTransform GetAccumulatedLayoutTransform() const { return FSlateLayoutTransform(Scale); }
	float Scale = 1.f;

private:
	FSlateRenderTransform RenderTransform;
};
class FPaintArgs {};
class FWidgetStyle
{
public:
	const FLinearColor& GetColorAndOpacityTint() const { return FLinearColor::White; }
};
// Layout/Clipping.h: 화면 좌표 사각 클리핑 (DrawElements.h 가 포함)
class FSlateClippingZone
{
public:
	explicit FSlateClippingZone(const FPaintGeometry& PaintingGeometry) { (void)PaintingGeometry; }
};
class FSlateWindowElementList
{
public:
	int32 PushClip(const FSlateClippingZone& InClipZone) { (void)InClipZone; return 0; }
	void PopClip() {}
};

enum class ESlateDrawEffect : uint8 { None = 0, NoBlending = 1 << 0, PreMultipliedAlpha = 1 << 1, NoGamma = 1 << 2, InvertAlpha = 1 << 3, NoPixelSnapping = 1 << 4, DisabledEffect = 1 << 5, IgnoreTextureAlpha = 1 << 6, ReverseGamma = 1 << 7 };
inline ESlateDrawEffect operator|(ESlateDrawEffect A, ESlateDrawEffect B) { return (ESlateDrawEffect)((uint8)A | (uint8)B); }
namespace ESlateBrushDrawType { enum Type { NoDrawType, Box, Border, Image, RoundedBox }; }
namespace ESlateBrushTileType { enum Type { NoTile, Horizontal, Vertical, Both }; }
enum class ESlateVertexRounding : uint8 { Disabled, Enabled };
typedef uint32 SlateIndex;

struct FMargin
{
	float Left = 0, Top = 0, Right = 0, Bottom = 0;
	FMargin() = default;
	FMargin(float U) : Left(U), Top(U), Right(U), Bottom(U) {}
	FMargin(float H, float V) : Left(H), Top(V), Right(H), Bottom(V) {}
	FMargin(float L, float T, float R, float B) : Left(L), Top(T), Right(R), Bottom(B) {}
};
struct FSlateColor
{
	FLinearColor Color;
	FSlateColor() = default;
	FSlateColor(const FLinearColor& C) : Color(C) {}
	const FLinearColor& GetSpecifiedColor() const { return Color; }
};
struct FSlateBrush
{
	FDeprecateSlateVector2D ImageSize = FVector2f(32.f, 32.f);
	FMargin Margin;
	FSlateColor TintColor = FLinearColor::White;
	TEnumAsByte<ESlateBrushDrawType::Type> DrawAs = ESlateBrushDrawType::Image;
	TEnumAsByte<ESlateBrushTileType::Type> Tiling = ESlateBrushTileType::NoTile;
	FSlateBrush() = default;
	virtual ~FSlateBrush() = default;
	void SetResourceObject(UObject* InResourceObject) { ResourceObject = InResourceObject; }
	UObject* GetResourceObject() const { return ResourceObject; }
	void SetUVRegion(const FBox2f& InUVRegion) { UVRegion = InUVRegion; }
	FBox2f GetUVRegion() const { return UVRegion; }
	void SetImageSize(FVector2f S) { ImageSize = S; }
	FName GetResourceName() const { return FName(); }

private:
	UObject* ResourceObject = nullptr;
	FBox2f UVRegion;
};
struct FSlateColorBrush : FSlateBrush
{
	FSlateColorBrush(const FLinearColor& InColor) { TintColor = InColor; }
	FSlateColorBrush(const FColor& InColor) { TintColor = FLinearColor(InColor); }
};
struct FSlateNoResource : FSlateBrush {};

struct FFontOutlineSettings
{
	int32 OutlineSize = 0;
	FLinearColor OutlineColor = FLinearColor::Black;
	bool bSeparateFillAlpha = false;
	bool bApplyOutlineToDropShadows = false;
};
struct FSlateFontInfo
{
	float Size = 10.f;
	FName TypefaceFontName;
	FFontOutlineSettings OutlineSettings;
	int32 LetterSpacing = 0;
	float SkewAmount = 0.f;
	FSlateFontInfo() = default;
};
class FSlateFontMeasure
{
public:
	FVector2D Measure(const FString& Text, const FSlateFontInfo& InFontInfo, float FontScale = 1.f) const { (void)Text; (void)InFontInfo; (void)FontScale; return FVector2D(); }
	FVector2D Measure(const FString& Text, int32 StartIndex, int32 EndIndex, const FSlateFontInfo& InFontInfo, bool IncludeKerningWithPrecedingChar = true, float FontScale = 1.f) const
	{ (void)Text; (void)StartIndex; (void)EndIndex; (void)InFontInfo; (void)IncludeKerningWithPrecedingChar; (void)FontScale; return FVector2D(); }
	uint16 GetMaxCharacterHeight(const FSlateFontInfo& InFontInfo, float FontScale = 1.f) const { (void)InFontInfo; (void)FontScale; return 0; }
	int16 GetBaseline(const FSlateFontInfo& InFontInfo, float FontScale = 1.f) const { (void)InFontInfo; (void)FontScale; return 0; }
};
class FCoreStyle
{
public:
	static FSlateFontInfo GetDefaultFontStyle(const FName InTypefaceFontName, const float InSize, const FFontOutlineSettings& InOutlineSettings = FFontOutlineSettings())
	{ (void)InTypefaceFontName; (void)InSize; (void)InOutlineSettings; return FSlateFontInfo(); }
};

class FSlateResourceHandle
{
public:
	bool IsValid() const { return false; }
};
class FSlateRenderer
{
public:
	FSlateResourceHandle GetResourceHandle(const FSlateBrush& Brush, FVector2f LocalSize, float DrawScale) { (void)Brush; (void)LocalSize; (void)DrawScale; return FSlateResourceHandle(); }
	FSlateResourceHandle GetResourceHandle(const FSlateBrush& Brush) { (void)Brush; return FSlateResourceHandle(); }
	TSharedRef<FSlateFontMeasure> GetFontMeasureService() const { return MakeShared<FSlateFontMeasure>(); }
};

struct FSlateVertex
{
	float TexCoords[4] = {};
	FVector2f MaterialTexCoords;
	FVector2f Position;
	FColor Color;
	template <ESlateVertexRounding Rounding>
	static FSlateVertex Make(const FSlateRenderTransform& RenderTransform, const FVector2f InLocalPosition, const FVector2f InTexCoord, const FColor& InColor, const FColor& InSecondaryColor = FColor())
	{ (void)RenderTransform; (void)InLocalPosition; (void)InTexCoord; (void)InColor; (void)InSecondaryColor; return FSlateVertex(); }
	template <ESlateVertexRounding Rounding>
	static FSlateVertex Make(const FSlateRenderTransform& RenderTransform, const FVector2f InLocalPosition, const FVector4f& InTexCoords, const FVector2f InMaterialTexCoords, const FColor& InColor, const FColor& InSecondaryColor = FColor())
	{ (void)RenderTransform; (void)InLocalPosition; (void)InTexCoords; (void)InMaterialTexCoords; (void)InColor; (void)InSecondaryColor; return FSlateVertex(); }
};
class ISlateUpdatableInstanceBuffer;

class FSlateDrawElement
{
public:
	enum ERotationSpace { RelativeToElement, RelativeToWorld };
	static void MakeBox(FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FSlateBrush* InBrush, ESlateDrawEffect InDrawEffects = ESlateDrawEffect::None, const FLinearColor& InTint = FLinearColor::White)
	{ (void)ElementList; (void)InLayer; (void)PaintGeometry; (void)InBrush; (void)InDrawEffects; (void)InTint; }
	static void MakeRotatedBox(FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FSlateBrush* InBrush, ESlateDrawEffect InDrawEffects = ESlateDrawEffect::None, float Angle = 0.f,
		TOptional<FVector2f> InRotationPoint = TOptional<FVector2f>(), ERotationSpace RotationSpace = RelativeToElement, const FLinearColor& InTint = FLinearColor::White)
	{ (void)ElementList; (void)InLayer; (void)PaintGeometry; (void)InBrush; (void)InDrawEffects; (void)Angle; (void)InRotationPoint; (void)RotationSpace; (void)InTint; }
	static void MakeText(FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FString& InText, const FSlateFontInfo& InFontInfo, ESlateDrawEffect InDrawEffects = ESlateDrawEffect::None, const FLinearColor& InTint = FLinearColor::White)
	{ (void)ElementList; (void)InLayer; (void)PaintGeometry; (void)InText; (void)InFontInfo; (void)InDrawEffects; (void)InTint; }
	static void MakeText(FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FText& InText, const FSlateFontInfo& InFontInfo, ESlateDrawEffect InDrawEffects = ESlateDrawEffect::None, const FLinearColor& InTint = FLinearColor::White)
	{ (void)ElementList; (void)InLayer; (void)PaintGeometry; (void)InText; (void)InFontInfo; (void)InDrawEffects; (void)InTint; }
	static void MakeLines(FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, TArray<FVector2f> Points, ESlateDrawEffect InDrawEffects = ESlateDrawEffect::None, const FLinearColor& InTint = FLinearColor::White, bool bAntialias = true, float Thickness = 1.f)
	{ (void)ElementList; (void)InLayer; (void)PaintGeometry; (void)Points; (void)InDrawEffects; (void)InTint; (void)bAntialias; (void)Thickness; }
	static void MakeCustomVerts(FSlateWindowElementList& ElementList, uint32 InLayer, const FSlateResourceHandle& InRenderResourceHandle, const TArray<FSlateVertex>& InVerts, const TArray<SlateIndex>& InIndexes,
		ISlateUpdatableInstanceBuffer* InInstanceData, uint32 InInstanceOffset, uint32 InNumInstances, ESlateDrawEffect InDrawEffects = ESlateDrawEffect::None)
	{ (void)ElementList; (void)InLayer; (void)InRenderResourceHandle; (void)InVerts; (void)InIndexes; (void)InInstanceData; (void)InInstanceOffset; (void)InNumInstances; (void)InDrawEffects; }
};

// ---------------------------------------------------------------- 입력
class FKey
{
	FName KeyName;

public:
	FKey() = default;
	FKey(const TCHAR* InName) : KeyName(InName) {}
	FKey(const FName InName) : KeyName(InName) {}
	bool IsValid() const { return true; }
	bool IsMouseButton() const { return false; }
	bool IsGamepadKey() const { return false; }
	bool IsModifierKey() const { return false; }
	FName GetFName() const { return KeyName; }
	FText GetDisplayName(bool bLongDisplayName = true) const { (void)bLongDisplayName; return FText(); }
	friend bool operator==(const FKey& A, const FKey& B) { return A.KeyName == B.KeyName; }
	friend bool operator!=(const FKey& A, const FKey& B) { return !(A == B); }
};
struct EKeys
{
#define FAKE_KEY(N) static inline const FKey N{TEXT(#N)};
	FAKE_KEY(AnyKey) FAKE_KEY(MouseX) FAKE_KEY(MouseY) FAKE_KEY(MouseWheelAxis) FAKE_KEY(MouseScrollUp) FAKE_KEY(MouseScrollDown)
	FAKE_KEY(LeftMouseButton) FAKE_KEY(RightMouseButton) FAKE_KEY(MiddleMouseButton) FAKE_KEY(ThumbMouseButton) FAKE_KEY(ThumbMouseButton2)
	FAKE_KEY(BackSpace) FAKE_KEY(Tab) FAKE_KEY(Enter) FAKE_KEY(Pause) FAKE_KEY(CapsLock) FAKE_KEY(Escape) FAKE_KEY(SpaceBar) FAKE_KEY(PageUp) FAKE_KEY(PageDown)
	FAKE_KEY(End) FAKE_KEY(Home) FAKE_KEY(Left) FAKE_KEY(Up) FAKE_KEY(Right) FAKE_KEY(Down) FAKE_KEY(Insert) FAKE_KEY(Delete)
	FAKE_KEY(Zero) FAKE_KEY(One) FAKE_KEY(Two) FAKE_KEY(Three) FAKE_KEY(Four) FAKE_KEY(Five) FAKE_KEY(Six) FAKE_KEY(Seven) FAKE_KEY(Eight) FAKE_KEY(Nine)
	FAKE_KEY(A) FAKE_KEY(B) FAKE_KEY(C) FAKE_KEY(D) FAKE_KEY(E) FAKE_KEY(F) FAKE_KEY(G) FAKE_KEY(H) FAKE_KEY(I) FAKE_KEY(J) FAKE_KEY(K) FAKE_KEY(L) FAKE_KEY(M)
	FAKE_KEY(N) FAKE_KEY(O) FAKE_KEY(P) FAKE_KEY(Q) FAKE_KEY(R) FAKE_KEY(S) FAKE_KEY(T) FAKE_KEY(U) FAKE_KEY(V) FAKE_KEY(W) FAKE_KEY(X) FAKE_KEY(Y) FAKE_KEY(Z)
	FAKE_KEY(NumPadZero) FAKE_KEY(NumPadOne) FAKE_KEY(NumPadTwo) FAKE_KEY(NumPadThree) FAKE_KEY(NumPadFour) FAKE_KEY(NumPadFive) FAKE_KEY(NumPadSix) FAKE_KEY(NumPadSeven) FAKE_KEY(NumPadEight) FAKE_KEY(NumPadNine)
	FAKE_KEY(Multiply) FAKE_KEY(Add) FAKE_KEY(Subtract) FAKE_KEY(Decimal) FAKE_KEY(Divide)
	FAKE_KEY(F1) FAKE_KEY(F2) FAKE_KEY(F3) FAKE_KEY(F4) FAKE_KEY(F5) FAKE_KEY(F6) FAKE_KEY(F7) FAKE_KEY(F8) FAKE_KEY(F9) FAKE_KEY(F10) FAKE_KEY(F11) FAKE_KEY(F12)
	FAKE_KEY(NumLock) FAKE_KEY(ScrollLock) FAKE_KEY(LeftShift) FAKE_KEY(RightShift) FAKE_KEY(LeftControl) FAKE_KEY(RightControl) FAKE_KEY(LeftAlt) FAKE_KEY(RightAlt) FAKE_KEY(LeftCommand) FAKE_KEY(RightCommand)
	FAKE_KEY(Semicolon) FAKE_KEY(Equals) FAKE_KEY(Comma) FAKE_KEY(Underscore) FAKE_KEY(Hyphen) FAKE_KEY(Period) FAKE_KEY(Slash) FAKE_KEY(Tilde) FAKE_KEY(LeftBracket) FAKE_KEY(Backslash) FAKE_KEY(RightBracket) FAKE_KEY(Apostrophe)
#undef FAKE_KEY
};
struct FModifierKeysState {};
class FInputEvent
{
public:
	bool IsRepeat() const { return false; }
	bool IsShiftDown() const { return false; }
	bool IsLeftShiftDown() const { return false; }
	bool IsControlDown() const { return false; }
	bool IsAltDown() const { return false; }
	bool IsCommandDown() const { return false; }
	uint32 GetUserIndex() const { return 0; }
};
class FKeyEvent : public FInputEvent
{
public:
	FKey GetKey() const { return FKey(); }
	uint32 GetCharacter() const { return 0; }
	uint32 GetKeyCode() const { return 0; }
};
class FCharacterEvent : public FInputEvent
{
public:
	TCHAR GetCharacter() const { return 0; }
};
class FPointerEvent : public FInputEvent
{
public:
	UE::Slate::FDeprecateVector2DResult GetScreenSpacePosition() const { return FVector2f(); }
	UE::Slate::FDeprecateVector2DResult GetLastScreenSpacePosition() const { return FVector2f(); }
	UE::Slate::FDeprecateVector2DResult GetCursorDelta() const { return FVector2f(); }
	FKey GetEffectingButton() const { return FKey(); }
	bool IsMouseButtonDown(FKey MouseButton) const { (void)MouseButton; return false; }
	float GetWheelDelta() const { return 0.f; }
	bool IsTouchEvent() const { return false; }
};
enum class EFocusCause : uint8 { Mouse, Navigation, SetDirectly, Cleared, OtherWidgetLostFocus, WindowActivate };
enum class EInvalidateWidgetReason : uint8 { None = 0, Layout = 1, Paint = 2, Volatility = 4, ChildOrder = 8, RenderTransform = 16, Visibility = 32, AttributeRegistration = 64, Prepass = 128 };
inline EInvalidateWidgetReason operator|(EInvalidateWidgetReason A, EInvalidateWidgetReason B) { return (EInvalidateWidgetReason)((uint8)A | (uint8)B); }
namespace EMouseCursor { enum Type { None, Default, TextEditBeam, ResizeLeftRight, ResizeUpDown, CardinalCross, Crosshairs, Hand, GrabHand, GrabHandClosed, SlashedCircle, EyeDropper }; }

class SWidget;
class FReply
{
public:
	static FReply Handled() { return FReply(true); }
	static FReply Unhandled() { return FReply(false); }
	bool IsEventHandled() const { return bHandled; }
	FReply& SetUserFocus(TSharedRef<SWidget> GiveMeFocus, EFocusCause ReasonFocusIsChanging = EFocusCause::SetDirectly, bool bInAllUsers = false) { (void)GiveMeFocus; (void)ReasonFocusIsChanging; (void)bInAllUsers; return *this; }
	FReply& ClearUserFocus(bool bInAllUsers = false) { (void)bInAllUsers; return *this; }
	FReply& CaptureMouse(TSharedRef<SWidget> InMouseCaptor) { (void)InMouseCaptor; return *this; }
	FReply& ReleaseMouseCapture() { return *this; }
	FReply& PreventThrottling() { return *this; }

private:
	explicit FReply(bool b) : bHandled(b) {}
	bool bHandled = false;
};
class FCursorReply
{
public:
	static FCursorReply Unhandled() { return FCursorReply(); }
	static FCursorReply Cursor(EMouseCursor::Type InCursor) { (void)InCursor; return FCursorReply(); }
};

// ---------------------------------------------------------------- 위젯
template <typename WidgetType> struct TSlateBaseNamedArgs
{
	typedef WidgetType WidgetType_Fake;
};
template <typename WidgetType> class TFakeSlateDecl;

class SWidget : public TSharedFromThis<SWidget>
{
public:
	SWidget() = default;
	virtual ~SWidget() = default;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) { (void)AllottedGeometry; (void)InCurrentTime; (void)InDeltaTime; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const = 0;
	virtual bool SupportsKeyboardFocus() const { return false; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) { (void)MyGeometry; (void)InKeyEvent; return FReply::Unhandled(); }
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) { (void)MyGeometry; (void)InKeyEvent; return FReply::Unhandled(); }
	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) { (void)MyGeometry; (void)InCharacterEvent; return FReply::Unhandled(); }
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) { (void)MyGeometry; (void)MouseEvent; return FReply::Unhandled(); }
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) { (void)MyGeometry; (void)MouseEvent; return FReply::Unhandled(); }
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) { (void)MyGeometry; (void)MouseEvent; return FReply::Unhandled(); }
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) { (void)MyGeometry; (void)MouseEvent; return FReply::Unhandled(); }
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) { (void)MyGeometry; (void)MouseEvent; return FReply::Unhandled(); }
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) { (void)MyGeometry; (void)MouseEvent; }
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) { (void)MouseEvent; }
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const { (void)MyGeometry; (void)CursorEvent; return FCursorReply::Unhandled(); }
	virtual bool IsInteractable() const { return false; }
	void Invalidate(EInvalidateWidgetReason InvalidateReason) { (void)InvalidateReason; }
	void SetCanTick(bool bInCanTick) { (void)bInCanTick; }
	bool HasKeyboardFocus() const { return false; }
	const FGeometry& GetCachedGeometry() const { static FGeometry G; return G; }
	const FGeometry& GetTickSpaceGeometry() const { static FGeometry G; return G; }
	UE::Slate::FDeprecateVector2DResult GetDesiredSize() const { return FVector2f(); }
	void SlatePrepass(float InLayoutScaleMultiplier = 1.f) { (void)InLayoutScaleMultiplier; }

protected:
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const = 0;
};
class SLeafWidget : public SWidget
{
};
class SCompoundWidget : public SWidget
{
public:
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{ (void)Args; (void)AllottedGeometry; (void)MyCullingRect; (void)OutDrawElements; (void)InWidgetStyle; (void)bParentEnabled; return LayerId; }

protected:
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { (void)LayoutScaleMultiplier; return FVector2D(); }
};

// SLATE_BEGIN_ARGS(T) ... SLATE_END_ARGS()
#define SLATE_BEGIN_ARGS(WidgetType) \
public: \
	struct FArguments : public TSlateBaseNamedArgs<WidgetType> \
	{ \
		typedef FArguments WidgetArgsType; \
		FArguments()
#define SLATE_ARGUMENT(ArgType, ArgName) \
	ArgType _##ArgName; \
	WidgetArgsType& ArgName(ArgType InArg) \
	{ \
		_##ArgName = InArg; \
		return *this; \
	}
#define SLATE_ATTRIBUTE(ArgType, ArgName) SLATE_ARGUMENT(ArgType, ArgName)
#define SLATE_END_ARGS() \
	};

template <typename WidgetType> class TFakeSlateDecl
{
public:
	TSharedRef<WidgetType> Widget = MakeShared<WidgetType>();
	TFakeSlateDecl& Expose(TSharedPtr<WidgetType>& Out) { Out = Widget; return *this; }
	TFakeSlateDecl& Expose(TSharedRef<WidgetType>& Out) { Out = Widget; return *this; }
	TSharedRef<WidgetType> operator<<=(const typename WidgetType::FArguments& InArgs) const
	{
		Widget->Construct(InArgs);
		return Widget;
	}
};
#define SNew(WidgetType, ...) TFakeSlateDecl<WidgetType>() <<= WidgetType::FArguments()
#define SAssignNew(ExposeAs, WidgetType, ...) TFakeSlateDecl<WidgetType>().Expose(ExposeAs) <<= WidgetType::FArguments()

class FSlateApplication
{
public:
	static FSlateApplication& Get() { static FSlateApplication A; return A; }
	static bool IsInitialized() { return true; }
	FSlateRenderer* GetRenderer() const { return nullptr; }
	bool SetKeyboardFocus(const TSharedPtr<SWidget>& OptionalWidgetToFocus, EFocusCause ReasonFocusIsChanging = EFocusCause::SetDirectly) { (void)OptionalWidgetToFocus; (void)ReasonFocusIsChanging; return true; }
	bool SetUserFocus(uint32 InUserIndex, const TSharedPtr<SWidget>& WidgetToFocus, EFocusCause ReasonFocusIsChanging = EFocusCause::SetDirectly) { (void)InUserIndex; (void)WidgetToFocus; (void)ReasonFocusIsChanging; return true; }
	UE::Slate::FDeprecateVector2DResult GetCursorPos() const { return FVector2f(); }
	float GetApplicationScale() const { return 1.f; }
	double GetCurrentTime() const { return 0.0; }
	float GetDeltaTime() const { return 0.f; }
};

class FWidgetRenderer
{
public:
	FWidgetRenderer(bool bUseGammaCorrection = false, bool bInClearTarget = true) { (void)bUseGammaCorrection; (void)bInClearTarget; }
	static UTextureRenderTarget2D* CreateTargetFor(FVector2D DrawSize, TextureFilter InFilter, bool bUseGammaCorrection) { (void)DrawSize; (void)InFilter; (void)bUseGammaCorrection; return nullptr; }
	void DrawWidget(UTextureRenderTarget2D* RenderTarget, const TSharedRef<SWidget>& Widget, FVector2D DrawSize, float DeltaTime, bool bDeferRenderTargetUpdate = false)
	{ (void)RenderTarget; (void)Widget; (void)DrawSize; (void)DeltaTime; (void)bDeferRenderTargetUpdate; }
	void DrawWidget(UTextureRenderTarget2D* RenderTarget, const TSharedRef<SWidget>& Widget, float Scale, FVector2D DrawSize, float DeltaTime, bool bDeferRenderTargetUpdate = false)
	{ (void)RenderTarget; (void)Widget; (void)Scale; (void)DrawSize; (void)DeltaTime; (void)bDeferRenderTargetUpdate; }
	void SetIsPrepassNeeded(bool bInIsPrepassNeeded) { (void)bInIsPrepassNeeded; }
	void SetShouldClearTarget(bool bShouldClear) { (void)bShouldClear; }
};
