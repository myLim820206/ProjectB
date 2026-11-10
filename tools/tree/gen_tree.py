"""스킬 트리 표 생성기 (docs/16-skill-tree.md)

트리 배치와 노드 효과의 원본은 이 파일 하나다. 고친 뒤 실행하면 세 파일을 다시 만든다:

    python3 tools/tree/gen_tree.py

    Source/ProjectB/PDTreeData.inl   규칙 코어용 노드·연결 표 (글자 없음)
    Source/ProjectB/PDTreeText.inl   화면용 특급 노드·키스톤 이름 (UTF-8 BOM, TEXT())
    tools/tree/tree.json             tools/neon/neon.py tree 미리보기용

구조 (PoE 식 '큰길 + 막다른 주머니'):
  - 가운데 시작점에서 무늬 큰길 4개 (♠ 위, ♥ 오른쪽, ♦ 아래, ♣ 왼쪽)
  - 큰길 옆에 주머니 2개 (입구 하나, 작은 2 + 특급). 큰길 끝에 키스톤, 양옆에 조커 갈림길 → 조커 강화
  - 이웃한 두 큰길의 첫 노드를 잇는 연결 길 (고리는 이 4개뿐) 가운데에서 족보 큰길이 뻗는다
  - 족보 큰길 옆에도 주머니 2개, 끝에 키스톤. 연결 길 안쪽에 살림 노드 하나
  - 키스톤과 조커 강화는 모두 막다른 끝 (끝자락끼리 안 이어짐)
"""
import json
import math
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CX, CY = 640.0, 392.0       # 트리 창 가운데 (1280 x 720 화면)
RA, RB = 585.0, 296.0       # 타원 반지름 (가로, 세로)

# 효과 이름은 PDCore.h EPDStat 과 같은 순서
STATS = ['None', 'SuitDmg', 'SuitRate', 'SuitRange', 'SuitSplash', 'SuitSlow', 'HandDmg', 'HandRate', 'HandSplash', 'AllDmg', 'ChainAdd',
         'FuseAdd', 'FusedDmg', 'GoldPct', 'HandMax', 'Draw', 'Lives', 'Keystone', 'Joker', 'JokerDmg', 'JokerAdd', 'AllRate', 'CritChance', 'CritMult']
HANDS = ['RoyalFlush', 'Five', 'StraightFlush', 'Four', 'FullHouse', 'Flush', 'Straight', 'Triple', 'TwoPair', 'Pair']
KEYS = ['Purist', 'Omni', 'BigTable', 'SmallTable', 'Smith', 'ClownPath', 'Lone', 'Crowd']
JOKERS = ['Clown', 'Hatter', 'Mask', 'Comedy', 'Harlequin', 'Baton', 'Jack', 'Cheshire', 'Bell', 'Star']
KINDS = ['Start', 'Small', 'Notable', 'Keystone', 'Joker']
# 색 묶음: 0~3 무늬, 4 족보, 5 살림, 6 조커, 7 시작
THEME_HAND, THEME_UTIL, THEME_JOKER, THEME_START = 4, 5, 6, 7


def hm(*names):
    m = 0
    for n in names:
        m |= 1 << HANDS.index(n)
    return m


PAIR_M = hm('Pair', 'TwoPair')
TRIP_M = hm('Triple', 'Four', 'Five')
SAME_M = hm('Pair', 'TwoPair', 'Triple', 'FullHouse', 'Four', 'Five')
FLUSH_M = hm('Flush', 'StraightFlush', 'RoyalFlush')
CROWN_M = hm('StraightFlush', 'RoyalFlush')
STR_M = hm('Straight', 'StraightFlush', 'RoyalFlush')

nodes, links = [], []


def pos(deg, f):
    a = math.radians(deg)
    return round(CX + math.cos(a) * RA * f, 1), round(CY + math.sin(a) * RB * f, 1)


def add(kind, theme, deg, f, fx=(), name=None):
    x, y = pos(deg, f)
    nodes.append({'id': len(nodes), 'k': KINDS.index(kind), 'theme': theme, 'x': x, 'y': y,
                  'fx': [[STATS.index(s), p, v] for (s, p, v) in fx], 'name': name})
    return len(nodes) - 1


def link(a, b):
    links.append((a, b))


def chain(prev, ids):
    for i in ids:
        link(prev, i)
        prev = i
    return prev


# ---------------------------------------------------------------- 시작
START = add('Start', THEME_START, 0, 0)

# ---------------------------------------------------------------- 무늬 큰길
SUIT_DEG = [-90, 0, 90, 180]
SUIT_KEY = ['Lone', 'Crowd', 'Omni', 'ClownPath']
SUIT_JOKERS = [('Cheshire', 'Jack'), ('Star', 'Comedy'), ('Clown', 'Baton'), ('Bell', 'Hatter')]  # (각도 -쪽, +쪽)
# 무늬마다: 큰길 효과 6개, 주머니 A (무늬 위력), 주머니 B (무늬 특기)
SUIT_ROAD = [
    [('SuitDmg', 0, .10), ('AllDmg', 0, .04), ('SuitRange', 0, .08), None, ('CritMult', 0, .15), ('SuitDmg', 0, .10)],
    [('SuitDmg', 1, .10), ('AllDmg', 0, .04), ('SuitSplash', 1, .15), None, ('AllDmg', 0, .04), ('SuitDmg', 1, .10)],
    [('SuitRate', 2, .08), ('AllDmg', 0, .04), ('SuitDmg', 2, .10), None, ('AllRate', 0, .04), ('SuitRate', 2, .08)],
    [('SuitDmg', 3, .10), ('AllDmg', 0, .04), ('SuitSlow', 3, .05), None, ('CritChance', 0, .02), ('SuitDmg', 3, .10)],
]
SUIT_ROAD_NOTABLE = [
    ('명사수', [('SuitDmg', 0, .20), ('SuitRange', 0, .10)]),
    ('불꽃놀이', [('SuitDmg', 1, .20), ('SuitSplash', 1, .20)]),
    ('기관총', [('SuitRate', 2, .20), ('SuitDmg', 2, .10)]),
    ('얼음 손', [('SuitSlow', 3, .10), ('SuitRange', 3, .15)]),
]
SUIT_POCKET_A = [  # 작은 2 + 특급
    ([('CritChance', 0, .03)] * 2, '저격수의 눈', [('CritChance', 0, .05), ('CritMult', 0, .30)]),
    ([('SuitDmg', 1, .12)] * 2, '화약고', [('SuitDmg', 1, .25), ('SuitRate', 1, .10)]),
    ([('SuitDmg', 2, .12)] * 2, '탄창', [('SuitDmg', 2, .25), ('SuitRate', 2, .10)]),
    ([('SuitDmg', 3, .12)] * 2, '서릿발', [('SuitDmg', 3, .25), ('SuitRate', 3, .10)]),
]
SUIT_POCKET_B = [
    ([('SuitRange', 0, .08)] * 2, '망원 조준', [('SuitRange', 0, .15), ('SuitDmg', 0, .15)]),
    ([('SuitSplash', 1, .15)] * 2, '연쇄 폭발', [('SuitSplash', 1, .30), ('SuitDmg', 1, .15)]),
    ([('AllRate', 0, .05)] * 2, '과열', [('AllRate', 0, .10), ('SuitRate', 2, .15)]),
    ([('SuitSlow', 3, .05)] * 2, '빙하', [('SuitSlow', 3, .10), ('SuitDmg', 3, .15)]),
]
ROAD_F = [0.13, 0.25, 0.37, 0.49, 0.61, 0.73]
suit_first = []
for s, a in enumerate(SUIT_DEG):
    road = []
    for i, f in enumerate(ROAD_F):
        if i == 3:
            nm, fx = SUIT_ROAD_NOTABLE[s]
            road.append(add('Notable', s, a, f, fx, nm))
        else:
            road.append(add('Small', s, a, f, [SUIT_ROAD[s][i]]))
    chain(START, road)
    suit_first.append(road[0])
    link(road[5], add('Keystone', s, a, 0.90, [('Keystone', KEYS.index(SUIT_KEY[s]), 0)]))
    for side, jk in zip((-1, 1), SUIT_JOKERS[s]):
        fork = add('Small', THEME_JOKER, a + side * 13, 0.82, [('JokerDmg', 0, .10)])
        link(road[5], fork)
        link(fork, add('Joker', THEME_JOKER, a + side * 16, 0.97, [('Joker', JOKERS.index(jk), 0)]))
    # 주머니 A: - 쪽, 큰길 특급에서
    smalls, nm, nfx = SUIT_POCKET_A[s]
    chain(road[3], [add('Small', s, a - 9, 0.54, [smalls[0]]), add('Small', s, a - 14, 0.61, [smalls[1]]), add('Notable', s, a - 17, 0.68, nfx, nm)])
    # 주머니 B: + 쪽, 큰길 둘째 노드에서
    smalls, nm, nfx = SUIT_POCKET_B[s]
    chain(road[1], [add('Small', s, a + 10, 0.31, [smalls[0]]), add('Small', s, a + 15, 0.40, [smalls[1]]), add('Notable', s, a + 18, 0.50, nfx, nm)])

# ---------------------------------------------------------------- 족보 큰길 (이웃 무늬 사이)
HAND_KEY = ['SmallTable', 'Purist', 'BigTable', 'Smith']
HAND_JOKER = [None, 'Harlequin', None, 'Mask']
UTIL = [('GoldPct', 0, .15), ('HandMax', 0, 1), ('Lives', 0, 4), ('Draw', 0, 1)]
HAND_ROAD = [  # 큰길 작은 3 (특급 앞 2, 뒤 1)
    [('HandDmg', SAME_M, .08), ('AllDmg', 0, .04), ('HandDmg', SAME_M, .08)],
    [('HandSplash', FLUSH_M, .15), ('AllDmg', 0, .04), ('HandDmg', FLUSH_M, .10)],
    [('HandRate', STR_M, .08), ('AllDmg', 0, .04), ('HandDmg', STR_M, .10)],
    [('ChainAdd', 0, .04), ('AllDmg', 0, .04), ('FusedDmg', 0, .10)],
]
HAND_ROAD_NOTABLE = [
    ('거울', [('HandDmg', SAME_M, .25)]),
    ('물든 패', [('HandDmg', FLUSH_M, .25), ('HandSplash', FLUSH_M, .25)]),
    ('흐름', [('HandRate', STR_M, .20), ('HandDmg', STR_M, .10)]),
    ('단조', [('FuseAdd', 0, .15), ('ChainAdd', 0, .05)]),
]
HAND_POCKET_1 = [
    ([('HandDmg', PAIR_M, .15)] * 2, '쌍둥이', [('HandDmg', PAIR_M, .30)]),
    ([('HandDmg', CROWN_M, .20)] * 2, '왕관', [('HandDmg', CROWN_M, .40), ('HandRate', CROWN_M, .20)]),
    ([('HandDmg', STR_M, .15)] * 2, '긴 줄', [('HandDmg', STR_M, .30)]),
    ([('FusedDmg', 0, .15)] * 2, '대장간', [('FuseAdd', 0, .20), ('FusedDmg', 0, .15)]),
]
HAND_POCKET_2 = [
    ([('HandDmg', TRIP_M, .15)] * 2, '다섯 손가락', [('HandDmg', TRIP_M, .30)]),
    ([('HandSplash', FLUSH_M, .20)] * 2, '번지는 물감', [('HandSplash', FLUSH_M, .30), ('HandDmg', FLUSH_M, .20)]),
    ([('HandRate', STR_M, .08)] * 2, '장전', [('HandRate', STR_M, .20), ('HandDmg', STR_M, .10)]),
    ([('JokerDmg', 0, .10)] * 2, '조커 공방', [('JokerAdd', 0, .10), ('JokerDmg', 0, .15)]),
]
for h in range(4):
    d = SUIT_DEG[h] + 45
    a_first, b_first = suit_first[h], suit_first[(h + 1) % 4]
    c1 = add('Small', THEME_HAND, d - 20, 0.27, [('AllDmg', 0, .04)])
    c2 = add('Small', THEME_HAND, d, 0.30, [HAND_ROAD[h][0]])
    c3 = add('Small', THEME_HAND, d + 20, 0.27, [[('AllRate', 0, .03), ('CritChance', 0, .02), ('AllRate', 0, .03), ('CritMult', 0, .10)][h]])
    link(a_first, c1); link(c1, c2); link(c2, c3); link(c3, b_first)
    link(c2, add('Small', THEME_UTIL, d, 0.17, [UTIL[h]]))
    nm, nfx = HAND_ROAD_NOTABLE[h]
    d1 = add('Small', THEME_HAND, d, 0.41, [HAND_ROAD[h][1]])
    d2 = add('Notable', THEME_HAND, d, 0.52, nfx, nm)
    d3 = add('Small', THEME_HAND, d, 0.63, [HAND_ROAD[h][2]])
    key = add('Keystone', THEME_HAND, d, 0.76, [('Keystone', KEYS.index(HAND_KEY[h]), 0)])
    chain(c2, [d1, d2, d3, key])
    smalls, nm, nfx = HAND_POCKET_1[h]
    chain(d1, [add('Small', THEME_HAND, d + 9, 0.47, [smalls[0]]), add('Small', THEME_HAND, d + 13, 0.56, [smalls[1]]), add('Notable', THEME_HAND, d + 15, 0.66, nfx, nm)])
    smalls, nm, nfx = HAND_POCKET_2[h]
    chain(d2, [add('Small', THEME_HAND, d - 9, 0.60, [smalls[0]]), add('Small', THEME_HAND, d - 12, 0.69, [smalls[1]]), add('Notable', THEME_HAND, d - 13, 0.79, nfx, nm)])
    if HAND_JOKER[h]:
        fork = add('Small', THEME_JOKER, d + 7, 0.83, [('JokerDmg', 0, .10)])
        link(d3, fork)
        link(fork, add('Joker', THEME_JOKER, d + 8, 0.96, [('Joker', JOKERS.index(HAND_JOKER[h]), 0)]))

# ---------------------------------------------------------------- 쓰기
assert len(nodes) <= 160, len(nodes)
adj = [[] for _ in nodes]
for a, b in links:
    adj[a].append(b)
    adj[b].append(a)
for n in nodes:
    n['l'] = adj[n['id']]

with open(os.path.join(ROOT, 'Source/ProjectB/PDTreeData.inl'), 'w', encoding='utf-8') as f:
    f.write('// 생성됨: tools/tree/gen_tree.py 를 고치고 다시 실행할 것. 직접 고치지 말 것\n')
    f.write('// 노드: 종류, 색 묶음, 화면 위치(1280x720 트리 창), 효과 두 줄\n')
    f.write('static const FPDTreeNode GPDTreeNodes[] = {\n')
    for n in nodes:
        fx = n['fx'] + [[0, 0, 0]] * (2 - len(n['fx']))
        fxs = ', '.join('{EPDStat::%s, %d, %.3ff}' % (STATS[s], p, v) for s, p, v in fx)
        f.write('\t{EPDNode::%s, %d, %.1ff, %.1ff, {%s}}, // %d\n' % (KINDS[n['k']], n['theme'], n['x'], n['y'], fxs, n['id']))
    f.write('};\nstatic const int GPDTreeLinks[][2] = {\n')
    for i in range(0, len(links), 8):
        f.write('\t' + ' '.join('{%d, %d},' % l for l in links[i:i + 8]) + '\n')
    f.write('};\n')

with open(os.path.join(ROOT, 'Source/ProjectB/PDTreeText.inl'), 'w', encoding='utf-8-sig') as f:
    f.write('// 생성됨: tools/tree/gen_tree.py. 특급 노드 이름 (없으면 nullptr)\n')
    f.write('static const TCHAR* GPDTreeNames[] = {\n')
    for n in nodes:
        f.write('\t%s, // %d\n' % ('TEXT("%s")' % n['name'] if n['name'] and n['k'] == 2 else 'nullptr', n['id']))
    f.write('};\n')

with open(os.path.join(ROOT, 'tools/tree/tree.json'), 'w', encoding='utf-8') as f:
    json.dump(nodes, f, ensure_ascii=False)
print('nodes', len(nodes), 'links', len(links), 'loops', len(links) - len(nodes) + 1)
