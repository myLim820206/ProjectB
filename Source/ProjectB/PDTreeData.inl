// 생성됨: tools/tree/gen_tree.py 를 고치고 다시 실행할 것. 직접 고치지 말 것
// 노드: 종류, 색 묶음, 화면 위치(1280x720 트리 창), 효과 두 줄
static const FPDTreeNode GPDTreeNodes[] = {
	{EPDNode::Start, 7, 640.0f, 392.0f, {{EPDStat::None, 0, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 0
	{EPDNode::Small, 0, 640.0f, 353.5f, {{EPDStat::SuitDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 1
	{EPDNode::Small, 0, 640.0f, 318.0f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 2
	{EPDNode::Small, 0, 640.0f, 282.5f, {{EPDStat::SuitRange, 0, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 3
	{EPDNode::Notable, 0, 640.0f, 247.0f, {{EPDStat::SuitDmg, 0, 0.200f}, {EPDStat::SuitRange, 0, 0.100f}}}, // 4
	{EPDNode::Small, 0, 640.0f, 211.4f, {{EPDStat::CritMult, 0, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 5
	{EPDNode::Small, 0, 640.0f, 175.9f, {{EPDStat::SuitDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 6
	{EPDNode::Keystone, 0, 640.0f, 125.6f, {{EPDStat::Keystone, 6, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 7
	{EPDNode::Small, 6, 532.1f, 155.5f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 8
	{EPDNode::Joker, 6, 483.6f, 116.0f, {{EPDStat::Joker, 7, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 9
	{EPDNode::Small, 6, 747.9f, 155.5f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 10
	{EPDNode::Joker, 6, 796.4f, 116.0f, {{EPDStat::Joker, 6, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 11
	{EPDNode::Small, 0, 590.6f, 234.1f, {{EPDStat::CritChance, 0, 0.030f}, {EPDStat::None, 0, 0.000f}}}, // 12
	{EPDNode::Small, 0, 553.7f, 216.8f, {{EPDStat::CritChance, 0, 0.030f}, {EPDStat::None, 0, 0.000f}}}, // 13
	{EPDNode::Notable, 0, 523.7f, 199.5f, {{EPDStat::CritChance, 0, 0.050f}, {EPDStat::CritMult, 0, 0.300f}}}, // 14
	{EPDNode::Small, 0, 671.5f, 301.6f, {{EPDStat::SuitRange, 0, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 15
	{EPDNode::Small, 0, 700.6f, 277.6f, {{EPDStat::SuitRange, 0, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 16
	{EPDNode::Notable, 0, 730.4f, 251.2f, {{EPDStat::SuitRange, 0, 0.150f}, {EPDStat::SuitDmg, 0, 0.150f}}}, // 17
	{EPDNode::Small, 1, 716.0f, 392.0f, {{EPDStat::SuitDmg, 1, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 18
	{EPDNode::Small, 1, 786.2f, 392.0f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 19
	{EPDNode::Small, 1, 856.5f, 392.0f, {{EPDStat::SuitSplash, 1, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 20
	{EPDNode::Notable, 1, 926.6f, 392.0f, {{EPDStat::SuitDmg, 1, 0.200f}, {EPDStat::SuitSplash, 1, 0.200f}}}, // 21
	{EPDNode::Small, 1, 996.8f, 392.0f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 22
	{EPDNode::Small, 1, 1067.0f, 392.0f, {{EPDStat::SuitDmg, 1, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 23
	{EPDNode::Keystone, 1, 1166.5f, 392.0f, {{EPDStat::Keystone, 7, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 24
	{EPDNode::Small, 6, 1107.4f, 337.4f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 25
	{EPDNode::Joker, 6, 1185.5f, 312.9f, {{EPDStat::Joker, 9, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 26
	{EPDNode::Small, 6, 1107.4f, 446.6f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 27
	{EPDNode::Joker, 6, 1185.5f, 471.1f, {{EPDStat::Joker, 3, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 28
	{EPDNode::Small, 1, 952.0f, 367.0f, {{EPDStat::SuitDmg, 1, 0.120f}, {EPDStat::None, 0, 0.000f}}}, // 29
	{EPDNode::Small, 1, 986.3f, 348.3f, {{EPDStat::SuitDmg, 1, 0.120f}, {EPDStat::None, 0, 0.000f}}}, // 30
	{EPDNode::Notable, 1, 1020.4f, 333.2f, {{EPDStat::SuitDmg, 1, 0.250f}, {EPDStat::SuitRate, 1, 0.100f}}}, // 31
	{EPDNode::Small, 1, 818.6f, 407.9f, {{EPDStat::SuitSplash, 1, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 32
	{EPDNode::Small, 1, 866.0f, 422.6f, {{EPDStat::SuitSplash, 1, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 33
	{EPDNode::Notable, 1, 918.2f, 437.7f, {{EPDStat::SuitSplash, 1, 0.300f}, {EPDStat::SuitDmg, 1, 0.150f}}}, // 34
	{EPDNode::Small, 2, 640.0f, 430.5f, {{EPDStat::SuitRate, 2, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 35
	{EPDNode::Small, 2, 640.0f, 466.0f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 36
	{EPDNode::Small, 2, 640.0f, 501.5f, {{EPDStat::SuitDmg, 2, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 37
	{EPDNode::Notable, 2, 640.0f, 537.0f, {{EPDStat::SuitRate, 2, 0.200f}, {EPDStat::SuitDmg, 2, 0.100f}}}, // 38
	{EPDNode::Small, 2, 640.0f, 572.6f, {{EPDStat::AllRate, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 39
	{EPDNode::Small, 2, 640.0f, 608.1f, {{EPDStat::SuitRate, 2, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 40
	{EPDNode::Keystone, 2, 640.0f, 658.4f, {{EPDStat::Keystone, 1, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 41
	{EPDNode::Small, 6, 747.9f, 628.5f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 42
	{EPDNode::Joker, 6, 796.4f, 668.0f, {{EPDStat::Joker, 0, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 43
	{EPDNode::Small, 6, 532.1f, 628.5f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 44
	{EPDNode::Joker, 6, 483.6f, 668.0f, {{EPDStat::Joker, 5, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 45
	{EPDNode::Small, 2, 689.4f, 549.9f, {{EPDStat::SuitDmg, 2, 0.120f}, {EPDStat::None, 0, 0.000f}}}, // 46
	{EPDNode::Small, 2, 726.3f, 567.2f, {{EPDStat::SuitDmg, 2, 0.120f}, {EPDStat::None, 0, 0.000f}}}, // 47
	{EPDNode::Notable, 2, 756.3f, 584.5f, {{EPDStat::SuitDmg, 2, 0.250f}, {EPDStat::SuitRate, 2, 0.100f}}}, // 48
	{EPDNode::Small, 2, 608.5f, 482.4f, {{EPDStat::AllRate, 0, 0.050f}, {EPDStat::None, 0, 0.000f}}}, // 49
	{EPDNode::Small, 2, 579.4f, 506.4f, {{EPDStat::AllRate, 0, 0.050f}, {EPDStat::None, 0, 0.000f}}}, // 50
	{EPDNode::Notable, 2, 549.6f, 532.8f, {{EPDStat::AllRate, 0, 0.100f}, {EPDStat::SuitRate, 2, 0.150f}}}, // 51
	{EPDNode::Small, 3, 564.0f, 392.0f, {{EPDStat::SuitDmg, 3, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 52
	{EPDNode::Small, 3, 493.8f, 392.0f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 53
	{EPDNode::Small, 3, 423.6f, 392.0f, {{EPDStat::SuitSlow, 3, 0.050f}, {EPDStat::None, 0, 0.000f}}}, // 54
	{EPDNode::Notable, 3, 353.4f, 392.0f, {{EPDStat::SuitSlow, 3, 0.100f}, {EPDStat::SuitRange, 3, 0.150f}}}, // 55
	{EPDNode::Small, 3, 283.2f, 392.0f, {{EPDStat::CritChance, 0, 0.020f}, {EPDStat::None, 0, 0.000f}}}, // 56
	{EPDNode::Small, 3, 212.9f, 392.0f, {{EPDStat::SuitDmg, 3, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 57
	{EPDNode::Keystone, 3, 113.5f, 392.0f, {{EPDStat::Keystone, 5, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 58
	{EPDNode::Small, 6, 172.6f, 446.6f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 59
	{EPDNode::Joker, 6, 94.5f, 471.1f, {{EPDStat::Joker, 8, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 60
	{EPDNode::Small, 6, 172.6f, 337.4f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 61
	{EPDNode::Joker, 6, 94.5f, 312.9f, {{EPDStat::Joker, 1, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 62
	{EPDNode::Small, 3, 328.0f, 417.0f, {{EPDStat::SuitDmg, 3, 0.120f}, {EPDStat::None, 0, 0.000f}}}, // 63
	{EPDNode::Small, 3, 293.7f, 435.7f, {{EPDStat::SuitDmg, 3, 0.120f}, {EPDStat::None, 0, 0.000f}}}, // 64
	{EPDNode::Notable, 3, 259.6f, 450.8f, {{EPDStat::SuitDmg, 3, 0.250f}, {EPDStat::SuitRate, 3, 0.100f}}}, // 65
	{EPDNode::Small, 3, 461.4f, 376.1f, {{EPDStat::SuitSlow, 3, 0.050f}, {EPDStat::None, 0, 0.000f}}}, // 66
	{EPDNode::Small, 3, 414.0f, 361.4f, {{EPDStat::SuitSlow, 3, 0.050f}, {EPDStat::None, 0, 0.000f}}}, // 67
	{EPDNode::Notable, 3, 361.8f, 346.3f, {{EPDStat::SuitSlow, 3, 0.100f}, {EPDStat::SuitDmg, 3, 0.150f}}}, // 68
	{EPDNode::Small, 4, 706.8f, 319.6f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 69
	{EPDNode::Small, 4, 764.1f, 329.2f, {{EPDStat::HandDmg, 922, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 70
	{EPDNode::Small, 4, 783.2f, 358.2f, {{EPDStat::AllRate, 0, 0.030f}, {EPDStat::None, 0, 0.000f}}}, // 71
	{EPDNode::Small, 5, 710.3f, 356.4f, {{EPDStat::GoldPct, 0, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 72
	{EPDNode::Small, 4, 809.6f, 306.2f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 73
	{EPDNode::Notable, 4, 855.1f, 283.2f, {{EPDStat::HandDmg, 922, 0.250f}, {EPDStat::None, 0, 0.000f}}}, // 74
	{EPDNode::Small, 4, 900.6f, 260.1f, {{EPDStat::HandDmg, 922, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 75
	{EPDNode::Keystone, 4, 954.4f, 232.9f, {{EPDStat::Keystone, 3, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 76
	{EPDNode::Small, 4, 862.4f, 310.2f, {{EPDStat::HandDmg, 768, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 77
	{EPDNode::Small, 4, 917.8f, 304.2f, {{EPDStat::HandDmg, 768, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 78
	{EPDNode::Notable, 4, 974.4f, 294.3f, {{EPDStat::HandDmg, 768, 0.300f}, {EPDStat::None, 0, 0.000f}}}, // 79
	{EPDNode::Small, 4, 846.3f, 248.3f, {{EPDStat::HandDmg, 138, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 80
	{EPDNode::Small, 4, 859.8f, 220.7f, {{EPDStat::HandDmg, 138, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 81
	{EPDNode::Notable, 4, 884.9f, 193.7f, {{EPDStat::HandDmg, 138, 0.300f}, {EPDStat::None, 0, 0.000f}}}, // 82
	{EPDNode::Small, 4, 783.2f, 425.8f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 83
	{EPDNode::Small, 4, 764.1f, 454.8f, {{EPDStat::HandSplash, 37, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 84
	{EPDNode::Small, 4, 706.8f, 464.4f, {{EPDStat::CritChance, 0, 0.020f}, {EPDStat::None, 0, 0.000f}}}, // 85
	{EPDNode::Small, 5, 710.3f, 427.6f, {{EPDStat::HandMax, 0, 1.000f}, {EPDStat::None, 0, 0.000f}}}, // 86
	{EPDNode::Small, 4, 809.6f, 477.8f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 87
	{EPDNode::Notable, 4, 855.1f, 500.8f, {{EPDStat::HandDmg, 37, 0.250f}, {EPDStat::HandSplash, 37, 0.250f}}}, // 88
	{EPDNode::Small, 4, 900.6f, 523.9f, {{EPDStat::HandDmg, 37, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 89
	{EPDNode::Keystone, 4, 954.4f, 551.1f, {{EPDStat::Keystone, 0, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 90
	{EPDNode::Small, 4, 801.6f, 504.6f, {{EPDStat::HandDmg, 5, 0.200f}, {EPDStat::None, 0, 0.000f}}}, // 91
	{EPDNode::Small, 4, 813.6f, 532.6f, {{EPDStat::HandDmg, 5, 0.200f}, {EPDStat::None, 0, 0.000f}}}, // 92
	{EPDNode::Notable, 4, 833.1f, 561.2f, {{EPDStat::HandDmg, 5, 0.400f}, {EPDStat::HandRate, 5, 0.200f}}}, // 93
	{EPDNode::Small, 4, 924.0f, 496.4f, {{EPDStat::HandSplash, 37, 0.200f}, {EPDStat::None, 0, 0.000f}}}, // 94
	{EPDNode::Small, 4, 978.5f, 503.2f, {{EPDStat::HandSplash, 37, 0.200f}, {EPDStat::None, 0, 0.000f}}}, // 95
	{EPDNode::Notable, 4, 1031.9f, 515.9f, {{EPDStat::HandSplash, 37, 0.300f}, {EPDStat::HandDmg, 37, 0.200f}}}, // 96
	{EPDNode::Small, 6, 938.9f, 585.6f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 97
	{EPDNode::Joker, 6, 978.0f, 618.9f, {{EPDStat::Joker, 4, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 98
	{EPDNode::Small, 4, 573.2f, 464.4f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 99
	{EPDNode::Small, 4, 515.9f, 454.8f, {{EPDStat::HandRate, 69, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 100
	{EPDNode::Small, 4, 496.8f, 425.8f, {{EPDStat::AllRate, 0, 0.030f}, {EPDStat::None, 0, 0.000f}}}, // 101
	{EPDNode::Small, 5, 569.7f, 427.6f, {{EPDStat::Lives, 0, 4.000f}, {EPDStat::None, 0, 0.000f}}}, // 102
	{EPDNode::Small, 4, 470.4f, 477.8f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 103
	{EPDNode::Notable, 4, 424.9f, 500.8f, {{EPDStat::HandRate, 69, 0.200f}, {EPDStat::HandDmg, 69, 0.100f}}}, // 104
	{EPDNode::Small, 4, 379.4f, 523.9f, {{EPDStat::HandDmg, 69, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 105
	{EPDNode::Keystone, 4, 325.6f, 551.1f, {{EPDStat::Keystone, 2, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 106
	{EPDNode::Small, 4, 417.6f, 473.8f, {{EPDStat::HandDmg, 69, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 107
	{EPDNode::Small, 4, 362.2f, 479.8f, {{EPDStat::HandDmg, 69, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 108
	{EPDNode::Notable, 4, 305.6f, 489.7f, {{EPDStat::HandDmg, 69, 0.300f}, {EPDStat::None, 0, 0.000f}}}, // 109
	{EPDNode::Small, 4, 433.7f, 535.7f, {{EPDStat::HandRate, 69, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 110
	{EPDNode::Small, 4, 420.2f, 563.3f, {{EPDStat::HandRate, 69, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 111
	{EPDNode::Notable, 4, 395.1f, 590.3f, {{EPDStat::HandRate, 69, 0.200f}, {EPDStat::HandDmg, 69, 0.100f}}}, // 112
	{EPDNode::Small, 4, 496.8f, 358.2f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 113
	{EPDNode::Small, 4, 515.9f, 329.2f, {{EPDStat::ChainAdd, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 114
	{EPDNode::Small, 4, 573.2f, 319.6f, {{EPDStat::CritMult, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 115
	{EPDNode::Small, 5, 569.7f, 356.4f, {{EPDStat::Draw, 0, 1.000f}, {EPDStat::None, 0, 0.000f}}}, // 116
	{EPDNode::Small, 4, 470.4f, 306.2f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 117
	{EPDNode::Notable, 4, 424.9f, 283.2f, {{EPDStat::FuseAdd, 0, 0.150f}, {EPDStat::ChainAdd, 0, 0.050f}}}, // 118
	{EPDNode::Small, 4, 379.4f, 260.1f, {{EPDStat::FusedDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 119
	{EPDNode::Keystone, 4, 325.6f, 232.9f, {{EPDStat::Keystone, 4, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 120
	{EPDNode::Small, 4, 478.4f, 279.4f, {{EPDStat::FusedDmg, 0, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 121
	{EPDNode::Small, 4, 466.4f, 251.4f, {{EPDStat::FusedDmg, 0, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 122
	{EPDNode::Notable, 4, 446.9f, 222.8f, {{EPDStat::FuseAdd, 0, 0.200f}, {EPDStat::FusedDmg, 0, 0.150f}}}, // 123
	{EPDNode::Small, 4, 356.0f, 287.6f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 124
	{EPDNode::Small, 4, 301.5f, 280.8f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 125
	{EPDNode::Notable, 4, 248.1f, 268.1f, {{EPDStat::JokerAdd, 0, 0.100f}, {EPDStat::JokerDmg, 0, 0.150f}}}, // 126
	{EPDNode::Small, 6, 341.1f, 198.4f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 127
	{EPDNode::Joker, 6, 302.0f, 165.1f, {{EPDStat::Joker, 2, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 128
};
static const int GPDTreeLinks[][2] = {
	{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {6, 8},
	{8, 9}, {6, 10}, {10, 11}, {4, 12}, {12, 13}, {13, 14}, {2, 15}, {15, 16},
	{16, 17}, {0, 18}, {18, 19}, {19, 20}, {20, 21}, {21, 22}, {22, 23}, {23, 24},
	{23, 25}, {25, 26}, {23, 27}, {27, 28}, {21, 29}, {29, 30}, {30, 31}, {19, 32},
	{32, 33}, {33, 34}, {0, 35}, {35, 36}, {36, 37}, {37, 38}, {38, 39}, {39, 40},
	{40, 41}, {40, 42}, {42, 43}, {40, 44}, {44, 45}, {38, 46}, {46, 47}, {47, 48},
	{36, 49}, {49, 50}, {50, 51}, {0, 52}, {52, 53}, {53, 54}, {54, 55}, {55, 56},
	{56, 57}, {57, 58}, {57, 59}, {59, 60}, {57, 61}, {61, 62}, {55, 63}, {63, 64},
	{64, 65}, {53, 66}, {66, 67}, {67, 68}, {1, 69}, {69, 70}, {70, 71}, {71, 18},
	{70, 72}, {70, 73}, {73, 74}, {74, 75}, {75, 76}, {73, 77}, {77, 78}, {78, 79},
	{74, 80}, {80, 81}, {81, 82}, {18, 83}, {83, 84}, {84, 85}, {85, 35}, {84, 86},
	{84, 87}, {87, 88}, {88, 89}, {89, 90}, {87, 91}, {91, 92}, {92, 93}, {88, 94},
	{94, 95}, {95, 96}, {89, 97}, {97, 98}, {35, 99}, {99, 100}, {100, 101}, {101, 52},
	{100, 102}, {100, 103}, {103, 104}, {104, 105}, {105, 106}, {103, 107}, {107, 108}, {108, 109},
	{104, 110}, {110, 111}, {111, 112}, {52, 113}, {113, 114}, {114, 115}, {115, 1}, {114, 116},
	{114, 117}, {117, 118}, {118, 119}, {119, 120}, {117, 121}, {121, 122}, {122, 123}, {118, 124},
	{124, 125}, {125, 126}, {119, 127}, {127, 128},
};
