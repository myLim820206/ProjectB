// 생성됨: tools/tree/gen_tree.py 를 고치고 다시 실행할 것. 직접 고치지 말 것
// 노드: 종류, 색 묶음, 화면 위치(1280x720 트리 창), 효과 두 줄
static const FPDTreeNode GPDTreeNodes[] = {
	{EPDNode::Start, 7, 640.0f, 392.0f, {{EPDStat::None, 0, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 0
	{EPDNode::Small, 0, 640.0f, 334.3f, {{EPDStat::SuitDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 1
	{EPDNode::Small, 0, 640.0f, 281.0f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 2
	{EPDNode::Small, 0, 640.0f, 227.7f, {{EPDStat::SuitRange, 0, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 3
	{EPDNode::Notable, 0, 640.0f, 174.4f, {{EPDStat::SuitDmg, 0, 0.200f}, {EPDStat::SuitRange, 0, 0.100f}}}, // 4
	{EPDNode::Small, 0, 640.0f, 121.2f, {{EPDStat::CritMult, 0, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 5
	{EPDNode::Small, 0, 640.0f, 67.9f, {{EPDStat::SuitDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 6
	{EPDNode::Keystone, 0, 640.0f, -7.6f, {{EPDStat::Keystone, 6, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 7
	{EPDNode::Small, 6, 478.1f, 37.3f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 8
	{EPDNode::Joker, 6, 405.4f, -22.0f, {{EPDStat::Joker, 7, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 9
	{EPDNode::Small, 6, 801.9f, 37.3f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 10
	{EPDNode::Joker, 6, 874.6f, -22.0f, {{EPDStat::Joker, 6, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 11
	{EPDNode::Small, 0, 565.9f, 155.2f, {{EPDStat::CritChance, 0, 0.030f}, {EPDStat::None, 0, 0.000f}}}, // 12
	{EPDNode::Small, 0, 510.5f, 129.2f, {{EPDStat::CritChance, 0, 0.030f}, {EPDStat::None, 0, 0.000f}}}, // 13
	{EPDNode::Notable, 0, 465.5f, 103.3f, {{EPDStat::CritChance, 0, 0.050f}, {EPDStat::CritMult, 0, 0.300f}}}, // 14
	{EPDNode::Small, 0, 687.2f, 256.5f, {{EPDStat::SuitRange, 0, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 15
	{EPDNode::Small, 0, 730.8f, 220.5f, {{EPDStat::SuitRange, 0, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 16
	{EPDNode::Notable, 0, 775.6f, 180.9f, {{EPDStat::SuitRange, 0, 0.150f}, {EPDStat::SuitDmg, 0, 0.150f}}}, // 17
	{EPDNode::Small, 1, 754.1f, 392.0f, {{EPDStat::SuitDmg, 1, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 18
	{EPDNode::Small, 1, 859.4f, 392.0f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 19
	{EPDNode::Small, 1, 964.7f, 392.0f, {{EPDStat::SuitSplash, 1, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 20
	{EPDNode::Notable, 1, 1070.0f, 392.0f, {{EPDStat::SuitDmg, 1, 0.200f}, {EPDStat::SuitSplash, 1, 0.200f}}}, // 21
	{EPDNode::Small, 1, 1175.3f, 392.0f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 22
	{EPDNode::Small, 1, 1280.6f, 392.0f, {{EPDStat::SuitDmg, 1, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 23
	{EPDNode::Keystone, 1, 1429.8f, 392.0f, {{EPDStat::Keystone, 7, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 24
	{EPDNode::Small, 6, 1341.1f, 310.1f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 25
	{EPDNode::Joker, 6, 1458.2f, 273.3f, {{EPDStat::Joker, 9, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 26
	{EPDNode::Small, 6, 1341.1f, 473.9f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 27
	{EPDNode::Joker, 6, 1458.2f, 510.7f, {{EPDStat::Joker, 3, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 28
	{EPDNode::Small, 1, 1108.0f, 354.5f, {{EPDStat::SuitDmg, 1, 0.120f}, {EPDStat::None, 0, 0.000f}}}, // 29
	{EPDNode::Small, 1, 1159.4f, 326.5f, {{EPDStat::SuitDmg, 1, 0.120f}, {EPDStat::None, 0, 0.000f}}}, // 30
	{EPDNode::Notable, 1, 1210.6f, 303.7f, {{EPDStat::SuitDmg, 1, 0.250f}, {EPDStat::SuitRate, 1, 0.100f}}}, // 31
	{EPDNode::Small, 1, 907.9f, 415.9f, {{EPDStat::SuitSplash, 1, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 32
	{EPDNode::Small, 1, 979.0f, 438.0f, {{EPDStat::SuitSplash, 1, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 33
	{EPDNode::Notable, 1, 1057.3f, 460.6f, {{EPDStat::SuitSplash, 1, 0.300f}, {EPDStat::SuitDmg, 1, 0.150f}}}, // 34
	{EPDNode::Small, 2, 640.0f, 449.7f, {{EPDStat::SuitRate, 2, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 35
	{EPDNode::Small, 2, 640.0f, 503.0f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 36
	{EPDNode::Small, 2, 640.0f, 556.3f, {{EPDStat::SuitDmg, 2, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 37
	{EPDNode::Notable, 2, 640.0f, 609.6f, {{EPDStat::SuitRate, 2, 0.200f}, {EPDStat::SuitDmg, 2, 0.100f}}}, // 38
	{EPDNode::Small, 2, 640.0f, 662.8f, {{EPDStat::AllRate, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 39
	{EPDNode::Small, 2, 640.0f, 716.1f, {{EPDStat::SuitRate, 2, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 40
	{EPDNode::Keystone, 2, 640.0f, 791.6f, {{EPDStat::Keystone, 1, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 41
	{EPDNode::Small, 6, 801.9f, 746.7f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 42
	{EPDNode::Joker, 6, 874.6f, 806.0f, {{EPDStat::Joker, 0, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 43
	{EPDNode::Small, 6, 478.1f, 746.7f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 44
	{EPDNode::Joker, 6, 405.4f, 806.0f, {{EPDStat::Joker, 5, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 45
	{EPDNode::Small, 2, 714.1f, 628.8f, {{EPDStat::SuitDmg, 2, 0.120f}, {EPDStat::None, 0, 0.000f}}}, // 46
	{EPDNode::Small, 2, 769.5f, 654.8f, {{EPDStat::SuitDmg, 2, 0.120f}, {EPDStat::None, 0, 0.000f}}}, // 47
	{EPDNode::Notable, 2, 814.5f, 680.7f, {{EPDStat::SuitDmg, 2, 0.250f}, {EPDStat::SuitRate, 2, 0.100f}}}, // 48
	{EPDNode::Small, 2, 592.8f, 527.5f, {{EPDStat::AllRate, 0, 0.050f}, {EPDStat::None, 0, 0.000f}}}, // 49
	{EPDNode::Small, 2, 549.2f, 563.5f, {{EPDStat::AllRate, 0, 0.050f}, {EPDStat::None, 0, 0.000f}}}, // 50
	{EPDNode::Notable, 2, 504.4f, 603.1f, {{EPDStat::AllRate, 0, 0.100f}, {EPDStat::SuitRate, 2, 0.150f}}}, // 51
	{EPDNode::Small, 3, 525.9f, 392.0f, {{EPDStat::SuitDmg, 3, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 52
	{EPDNode::Small, 3, 420.6f, 392.0f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 53
	{EPDNode::Small, 3, 315.3f, 392.0f, {{EPDStat::SuitSlow, 3, 0.050f}, {EPDStat::None, 0, 0.000f}}}, // 54
	{EPDNode::Notable, 3, 210.0f, 392.0f, {{EPDStat::SuitSlow, 3, 0.100f}, {EPDStat::SuitRange, 3, 0.150f}}}, // 55
	{EPDNode::Small, 3, 104.7f, 392.0f, {{EPDStat::CritChance, 0, 0.020f}, {EPDStat::None, 0, 0.000f}}}, // 56
	{EPDNode::Small, 3, -0.6f, 392.0f, {{EPDStat::SuitDmg, 3, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 57
	{EPDNode::Keystone, 3, -149.8f, 392.0f, {{EPDStat::Keystone, 5, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 58
	{EPDNode::Small, 6, -61.1f, 473.9f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 59
	{EPDNode::Joker, 6, -178.2f, 510.7f, {{EPDStat::Joker, 8, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 60
	{EPDNode::Small, 6, -61.1f, 310.1f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 61
	{EPDNode::Joker, 6, -178.2f, 273.3f, {{EPDStat::Joker, 1, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 62
	{EPDNode::Small, 3, 172.0f, 429.5f, {{EPDStat::SuitDmg, 3, 0.120f}, {EPDStat::None, 0, 0.000f}}}, // 63
	{EPDNode::Small, 3, 120.6f, 457.5f, {{EPDStat::SuitDmg, 3, 0.120f}, {EPDStat::None, 0, 0.000f}}}, // 64
	{EPDNode::Notable, 3, 69.4f, 480.3f, {{EPDStat::SuitDmg, 3, 0.250f}, {EPDStat::SuitRate, 3, 0.100f}}}, // 65
	{EPDNode::Small, 3, 372.1f, 368.1f, {{EPDStat::SuitSlow, 3, 0.050f}, {EPDStat::None, 0, 0.000f}}}, // 66
	{EPDNode::Small, 3, 301.0f, 346.0f, {{EPDStat::SuitSlow, 3, 0.050f}, {EPDStat::None, 0, 0.000f}}}, // 67
	{EPDNode::Notable, 3, 222.7f, 323.4f, {{EPDStat::SuitSlow, 3, 0.100f}, {EPDStat::SuitDmg, 3, 0.150f}}}, // 68
	{EPDNode::Small, 4, 740.1f, 283.4f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 69
	{EPDNode::Small, 4, 826.1f, 297.8f, {{EPDStat::HandDmg, 922, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 70
	{EPDNode::Small, 4, 854.7f, 341.3f, {{EPDStat::AllRate, 0, 0.030f}, {EPDStat::None, 0, 0.000f}}}, // 71
	{EPDNode::Small, 5, 745.5f, 338.6f, {{EPDStat::XpPct, 0, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 72
	{EPDNode::Small, 4, 894.4f, 263.3f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 73
	{EPDNode::Notable, 4, 962.7f, 228.7f, {{EPDStat::HandDmg, 922, 0.250f}, {EPDStat::None, 0, 0.000f}}}, // 74
	{EPDNode::Small, 4, 1030.9f, 194.2f, {{EPDStat::HandDmg, 922, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 75
	{EPDNode::Keystone, 4, 1111.6f, 153.4f, {{EPDStat::Keystone, 3, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 76
	{EPDNode::Small, 4, 973.7f, 269.3f, {{EPDStat::HandDmg, 768, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 77
	{EPDNode::Small, 4, 1056.7f, 260.2f, {{EPDStat::HandDmg, 768, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 78
	{EPDNode::Notable, 4, 1141.6f, 245.5f, {{EPDStat::HandDmg, 768, 0.300f}, {EPDStat::None, 0, 0.000f}}}, // 79
	{EPDNode::Small, 4, 949.5f, 176.5f, {{EPDStat::HandDmg, 138, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 80
	{EPDNode::Small, 4, 969.8f, 135.1f, {{EPDStat::HandDmg, 138, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 81
	{EPDNode::Notable, 4, 1007.4f, 94.5f, {{EPDStat::HandDmg, 138, 0.300f}, {EPDStat::None, 0, 0.000f}}}, // 82
	{EPDNode::Small, 6, 1213.9f, 165.1f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 83
	{EPDNode::Keystone, 6, 1312.8f, 135.5f, {{EPDStat::Keystone, 8, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 84
	{EPDNode::Small, 4, 854.7f, 442.7f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 85
	{EPDNode::Small, 4, 826.1f, 486.2f, {{EPDStat::HandSplash, 37, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 86
	{EPDNode::Small, 4, 740.1f, 500.6f, {{EPDStat::CritChance, 0, 0.020f}, {EPDStat::None, 0, 0.000f}}}, // 87
	{EPDNode::Small, 5, 745.5f, 445.4f, {{EPDStat::HandMax, 0, 1.000f}, {EPDStat::None, 0, 0.000f}}}, // 88
	{EPDNode::Small, 4, 894.4f, 520.7f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 89
	{EPDNode::Notable, 4, 962.7f, 555.3f, {{EPDStat::HandDmg, 37, 0.250f}, {EPDStat::HandSplash, 37, 0.250f}}}, // 90
	{EPDNode::Small, 4, 1030.9f, 589.8f, {{EPDStat::HandDmg, 37, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 91
	{EPDNode::Keystone, 4, 1111.6f, 630.6f, {{EPDStat::Keystone, 0, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 92
	{EPDNode::Small, 4, 882.4f, 560.8f, {{EPDStat::HandDmg, 5, 0.200f}, {EPDStat::None, 0, 0.000f}}}, // 93
	{EPDNode::Small, 4, 900.4f, 602.9f, {{EPDStat::HandDmg, 5, 0.200f}, {EPDStat::None, 0, 0.000f}}}, // 94
	{EPDNode::Notable, 4, 929.6f, 645.8f, {{EPDStat::HandDmg, 5, 0.400f}, {EPDStat::HandRate, 5, 0.200f}}}, // 95
	{EPDNode::Small, 4, 1065.9f, 548.6f, {{EPDStat::HandSplash, 37, 0.200f}, {EPDStat::None, 0, 0.000f}}}, // 96
	{EPDNode::Small, 4, 1147.8f, 558.9f, {{EPDStat::HandSplash, 37, 0.200f}, {EPDStat::None, 0, 0.000f}}}, // 97
	{EPDNode::Notable, 4, 1227.9f, 577.9f, {{EPDStat::HandSplash, 37, 0.300f}, {EPDStat::HandDmg, 37, 0.200f}}}, // 98
	{EPDNode::Small, 6, 1088.4f, 682.4f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 99
	{EPDNode::Joker, 6, 1147.0f, 732.4f, {{EPDStat::Joker, 4, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 100
	{EPDNode::Small, 4, 539.9f, 500.6f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 101
	{EPDNode::Small, 4, 453.9f, 486.2f, {{EPDStat::HandRate, 69, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 102
	{EPDNode::Small, 4, 425.3f, 442.7f, {{EPDStat::AllRate, 0, 0.030f}, {EPDStat::None, 0, 0.000f}}}, // 103
	{EPDNode::Small, 5, 534.5f, 445.4f, {{EPDStat::Lives, 0, 4.000f}, {EPDStat::None, 0, 0.000f}}}, // 104
	{EPDNode::Small, 4, 385.6f, 520.7f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 105
	{EPDNode::Notable, 4, 317.3f, 555.3f, {{EPDStat::HandRate, 69, 0.200f}, {EPDStat::HandDmg, 69, 0.100f}}}, // 106
	{EPDNode::Small, 4, 249.1f, 589.8f, {{EPDStat::HandDmg, 69, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 107
	{EPDNode::Keystone, 4, 168.4f, 630.6f, {{EPDStat::Keystone, 2, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 108
	{EPDNode::Small, 4, 306.3f, 514.7f, {{EPDStat::HandDmg, 69, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 109
	{EPDNode::Small, 4, 223.3f, 523.8f, {{EPDStat::HandDmg, 69, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 110
	{EPDNode::Notable, 4, 138.4f, 538.5f, {{EPDStat::HandDmg, 69, 0.300f}, {EPDStat::None, 0, 0.000f}}}, // 111
	{EPDNode::Small, 4, 330.5f, 607.5f, {{EPDStat::HandRate, 69, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 112
	{EPDNode::Small, 4, 310.2f, 648.9f, {{EPDStat::HandRate, 69, 0.080f}, {EPDStat::None, 0, 0.000f}}}, // 113
	{EPDNode::Notable, 4, 272.6f, 689.5f, {{EPDStat::HandRate, 69, 0.200f}, {EPDStat::HandDmg, 69, 0.100f}}}, // 114
	{EPDNode::Small, 6, 66.1f, 618.9f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 115
	{EPDNode::Keystone, 6, -32.8f, 648.5f, {{EPDStat::Keystone, 9, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 116
	{EPDNode::Small, 4, 425.3f, 341.3f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 117
	{EPDNode::Small, 4, 453.9f, 297.8f, {{EPDStat::ChainAdd, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 118
	{EPDNode::Small, 4, 539.9f, 283.4f, {{EPDStat::CritMult, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 119
	{EPDNode::Small, 5, 534.5f, 338.6f, {{EPDStat::Draw, 0, 1.000f}, {EPDStat::None, 0, 0.000f}}}, // 120
	{EPDNode::Small, 4, 385.6f, 263.3f, {{EPDStat::AllDmg, 0, 0.040f}, {EPDStat::None, 0, 0.000f}}}, // 121
	{EPDNode::Notable, 4, 317.3f, 228.7f, {{EPDStat::FuseAdd, 0, 0.150f}, {EPDStat::ChainAdd, 0, 0.050f}}}, // 122
	{EPDNode::Small, 4, 249.1f, 194.2f, {{EPDStat::FusedDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 123
	{EPDNode::Keystone, 4, 168.4f, 153.4f, {{EPDStat::Keystone, 4, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 124
	{EPDNode::Small, 4, 397.6f, 223.2f, {{EPDStat::FusedDmg, 0, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 125
	{EPDNode::Small, 4, 379.6f, 181.1f, {{EPDStat::FusedDmg, 0, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 126
	{EPDNode::Notable, 4, 350.4f, 138.2f, {{EPDStat::FuseAdd, 0, 0.200f}, {EPDStat::FusedDmg, 0, 0.150f}}}, // 127
	{EPDNode::Small, 4, 214.1f, 235.4f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 128
	{EPDNode::Small, 4, 132.2f, 225.1f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 129
	{EPDNode::Notable, 4, 52.1f, 206.1f, {{EPDStat::JokerAdd, 0, 0.100f}, {EPDStat::JokerDmg, 0, 0.150f}}}, // 130
	{EPDNode::Small, 6, 191.6f, 101.6f, {{EPDStat::JokerDmg, 0, 0.100f}, {EPDStat::None, 0, 0.000f}}}, // 131
	{EPDNode::Joker, 6, 133.0f, 51.6f, {{EPDStat::Joker, 2, 0.000f}, {EPDStat::None, 0, 0.000f}}}, // 132
	{EPDNode::Notable, 6, 91.4f, 104.6f, {{EPDStat::UpperPrep, 0, 0.150f}, {EPDStat::None, 0, 0.000f}}}, // 133
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
	{74, 80}, {80, 81}, {81, 82}, {75, 83}, {83, 84}, {18, 85}, {85, 86}, {86, 87},
	{87, 35}, {86, 88}, {86, 89}, {89, 90}, {90, 91}, {91, 92}, {89, 93}, {93, 94},
	{94, 95}, {90, 96}, {96, 97}, {97, 98}, {91, 99}, {99, 100}, {35, 101}, {101, 102},
	{102, 103}, {103, 52}, {102, 104}, {102, 105}, {105, 106}, {106, 107}, {107, 108}, {105, 109},
	{109, 110}, {110, 111}, {106, 112}, {112, 113}, {113, 114}, {107, 115}, {115, 116}, {52, 117},
	{117, 118}, {118, 119}, {119, 1}, {118, 120}, {118, 121}, {121, 122}, {122, 123}, {123, 124},
	{121, 125}, {125, 126}, {126, 127}, {122, 128}, {128, 129}, {129, 130}, {123, 131}, {131, 132},
	{131, 133},
};
