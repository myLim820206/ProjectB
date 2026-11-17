// 레이아웃 검사기 (D-365) 시험: 사용자가 찾은 두 겹침(스킬 트리 잡식가 이름 알약 ↔ 바닥 안내문, 주크박스 그릴 ↔ 선곡 버튼)을
// 좌표 그대로 재현해 고치기 전엔 잡히고 고친 뒤엔 깨끗한지, 일부러 겹치게 그린 것(네온 글자 · 흐린 벽돌 · 버튼 속 아이콘)은 거르는지 본다.
//   g++ -std=c++20 -w -I Tests/fakeue -I Source/ProjectB -o Tests/build/layout_check_test Tests/layout_check_test.cpp && Tests/build/layout_check_test
//   기대: tree before 1 · tree fixed 0 · jukebox before 2 · jukebox fixed 0 · should be clean 0
#include <cstdio>
static void Dump(const char* Title)
{
	GPDLayout.Analyze(Title);
	std::printf("== %s: %d\n", Title, (int)GPDLayout.Hits.Num());
	for (const auto& H : GPDLayout.Hits) std::printf("   A (%.1f,%.1f)-(%.1f,%.1f)  B (%.1f,%.1f)-(%.1f,%.1f)\n", H.A.X0, H.A.Y0, H.A.X1, H.A.Y1, H.B.X0, H.B.Y0, H.B.X1, H.B.Y1);
	GPDLayout.NewLines.Reset(); GPDLayout.Logged.Reset();
}
int main()
{
	// 1) 스킬 트리: 맨 아래 키스톤 (640, 658.4), 이름 알약 y 685.4 ~ 703.4, 바닥 안내문 가운데 y 708 (10px 글자, 줄 높이 약 13)
	const float KW = 50.f, LY = 658.4f + 36.f, H = 13.f, TW = 230.f;
	for (int Fixed = 0; Fixed < 2; ++Fixed)
	{
		GPDLayout.Begin();
		GPDLayout.AddPanel(640.f - KW / 2.f, LY - 9.f, KW, 18.f, 0.95f, false);
		GPDLayout.AddText(TEXT("잡식가"), 640.f - 18.f, LY - 7.f, 36.f, 14.f, false, 1.f);
		if (!Fixed) GPDLayout.AddText(TEXT("이번 판에서만 · 레벨이 오를 때마다 포인트 1"), 640.f - TW / 2.f, 708.f - H / 2.f, TW, H, false, 1.f);
		else GPDLayout.AddText(TEXT("이번 판에서만 · 레벨이 오를 때마다 포인트 1"), 42.f, 66.f - H * 0.8f, TW, H, false, 1.f);
		Dump(Fixed ? "tree fixed" : "tree before");
	}
	// 2) 주크박스: 설계 단위 → 화면 (P), 버튼 BtnFill, 번호 칸, 그릴 세로줄 (예전 y 72~83 / 고친 y 81.5~87)
	const float K = JukeK, CX = JukeDW / 2.f, Bot = JukeDH;
	auto P = [&](float X, float Y) { return FVector2f(JukeX0 + X * K, JukeY0 + Y * K); };
	for (int Fixed = 0; Fixed < 2; ++Fixed)
	{
		GPDLayout.Begin();
		for (int Dir = -1; Dir <= 1; Dir += 2) GPDLayout.AddPanel(JukeBtnX(Dir), JukeBtnY + 2.f * K, JukeBtnS, JukeBtnS - 4.f * K, 1.f, true);
		for (int I = 0; I < 9; ++I)
		{
			const float SX = CX - 32.f + I * 8.f;
			const TArray<FVector2f> L = Fixed ? TArray<FVector2f>{P(SX, Bot - 7.f - 5.5f), P(SX, Bot - 7.f)} : TArray<FVector2f>{P(SX, Bot - 11.f - 11.f), P(SX, Bot - 11.f)};
			GPDLayout.AddLines(L, FLinearColor(0.66f, 0.59f, 0.88f, 0.8f), 1.f);
		}
		Dump(Fixed ? "jukebox fixed" : "jukebox before");
	}
	// 3) 걸러야 하는 것: 네온 글자 겹침(같은 글자 3.5px 옆), 흐린 벽돌 선, 버튼 속 아이콘
	GPDLayout.Begin();
	for (int K2 = 0; K2 < 8; ++K2) GPDLayout.AddText(TEXT("SKILL TREE"), 560.f + (K2 % 3) * 1.5f, 30.f + (K2 / 3) * 1.5f, 160.f, 34.f, false, 1.f);
	GPDLayout.AddLines({FVector2f(0.f, 40.f), FVector2f(1280.f, 40.f)}, FLinearColor(0.29f, 0.24f, 0.28f, 0.75f), 1.f);
	GPDLayout.AddPanel(100.f, 100.f, 40.f, 20.f, 1.f, true);
	GPDLayout.AddLines({FVector2f(115.f, 105.f), FVector2f(125.f, 110.f), FVector2f(115.f, 115.f)}, FLinearColor(1.f, 1.f, 1.f, 1.f), 1.1f);
	Dump("should be clean");
}
