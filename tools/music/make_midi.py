"""리프 · 코드 · 베이스 · 드럼을 MIDI 로 뽑는다 (D-364). 맥 GarageBand / Logic 에 끌어다 놓고 악기를 입힌다.

  python3 tools/music/make_midi.py   → docs/music/*.mid

곡은 아래 SONGS 에 적는다. 박 단위(4/4), 스윙 8분은 앞 2/3 · 뒤 1/3 자리로 직접 적는다.
"""
import os, struct

TPQ = 480  # 4분음표 하나의 틱

def vlq(n):
    out = [n & 0x7F]
    n >>= 7
    while n:
        out.append(0x80 | (n & 0x7F))
        n >>= 7
    return bytes(reversed(out))

def track(name, events, channel=0, program=None):
    """events: (박, 길이(박), 음, 세기) 목록"""
    ev = []  # (틱, 순서, 바이트)
    meta = b'\x00\xff\x03' + vlq(len(name.encode())) + name.encode()
    if program is not None:
        meta += b'\x00' + bytes([0xC0 | channel, program])
    for at, ln, note, vel in events:
        t0, t1 = round(at * TPQ), round((at + ln) * TPQ)
        ev.append((t0, 1, bytes([0x90 | channel, note, vel])))
        ev.append((t1, 0, bytes([0x80 | channel, note, 0])))
    ev.sort(key=lambda e: (e[0], e[1]))
    data, now = bytearray(meta), 0
    for t, _, b in ev:
        data += vlq(t - now) + b
        now = t
    data += b'\x00\xff\x2f\x00'
    return b'MTrk' + struct.pack('>I', len(data)) + bytes(data)

def write(path, bpm, tracks):
    tempo = round(60_000_000 / bpm)
    t0 = b'\x00\xff\x51\x03' + tempo.to_bytes(3, 'big') + b'\x00\xff\x58\x04\x04\x02\x18\x08' + b'\x00\xff\x2f\x00'
    body = b'MTrk' + struct.pack('>I', len(t0)) + t0 + b''.join(tracks)
    with open(path, 'wb') as f:
        f.write(b'MThd' + struct.pack('>IHHH', 6, 1, len(tracks) + 1, TPQ) + body)

# ---------------------------------------------------------------- 곡: Last Call (직접 만든 곡, 84 BPM 느린 스윙, C 단조 8마디 반복)
S = 2 / 3  # 스윙 뒷박 자리
# 마디마다 (앞 반 마디 코드, 뒤 반 마디 코드): (이름, 베이스 근음, 손 화음)
CM9 = ('Cm9', 36, [58, 62, 63, 67]); FM9 = ('Fm9', 41, [56, 60, 63, 67]); DM7B5 = ('Dm7b5', 38, [56, 60, 62, 65])
G7B9 = ('G7b9', 43, [53, 56, 59, 63]); ABM7 = ('Abmaj7', 44, [55, 58, 60, 63])
CHORDS = [(CM9, CM9), (FM9, FM9), (DM7B5, G7B9), (CM9, CM9), (ABM7, ABM7), (FM9, FM9), (DM7B5, G7B9), (CM9, G7B9)]
# 선율 리프: (마디, 박, 길이, 음)
MEL = [
    (0, 1, S, 67), (0, 1 + S, 1 - S, 70), (0, 2, 1.5, 72), (0, 3 + S, 1 - S, 75),
    (1, 0, 1, 74), (1, 1, S, 72), (1, 1 + S, 2.2, 68),
    (2, 0, S, 77), (2, S, 1 - S, 75), (2, 1, S, 74), (2, 1 + S, 1 - S, 72), (2, 2, S, 71), (2, 2 + S, 1 - S, 68), (2, 3, 1, 67),
    (3, S, 1 - S, 67), (3, 1, 2, 72), (3, 3 + S, 1 - S, 67),
    (4, 0, 1, 72), (4, 1, S, 75), (4, 1 + S, 1 - S, 79), (4, 2, 1.5, 79), (4, 3 + S, 1 - S, 77),
    (5, 0, 1, 75), (5, 1, S, 72), (5, 1 + S, 1.2, 68), (5, 3, 1, 67),
    (6, 0, S, 65), (6, S, 1 - S, 68), (6, 1, S, 72), (6, 1 + S, 1 - S, 75), (6, 2, 1, 74), (6, 3, S, 71), (6, 3 + S, 1 - S, 68),
    (7, 0, 3, 67),
]

def last_call():
    mel = [(b * 4 + at, ln * 0.95, n, 92 if at == int(at) else 78) for b, at, ln, n in MEL]
    keys, bass, drums = [], [], []
    for b, (A, B) in enumerate(CHORDS):
        T = b * 4
        # 로즈 찰스턴 반주 (홀짝 마디 리듬 바꿈), 반 마디 코드면 뒤 화음은 3박에
        hits = [(0, 0.6, A), (1 + S, 1.2, A if A == B else B)] if b % 2 == 0 else [(1, 0.5, A), (2 + S, 1.1, B)]
        for at, ln, C in hits:
            keys += [(T + at, ln, n, 62) for n in C[2]]
        # 워킹 베이스: 근음 · 3도 · 5도(또는 뒤 코드 근음) · 다음 근음 반음 옆
        nxt = CHORDS[(b + 1) % len(CHORDS)][0][1]
        third = 3 if 'm' in A[0] and 'maj' not in A[0] else 4
        walk = [A[1], A[1] + third, A[1] + 7 if A == B else B[1], nxt + (1 if b % 2 else -1)]
        walk = [n - 12 if n > 47 else n for n in walk]
        bass += [(T + k, 0.9, n, 96 if k == 0 else 84) for k, n in enumerate(walk)]
        # 드럼 (GM): 라이드 '딩 딩-다', 2 · 4박 발 하이햇, 1 · 3박 아주 작은 킥, 가끔 스네어 고스트
        for at in (0, 1, 1 + S, 2, 3, 3 + S):
            drums.append((T + at, 0.2, 51, 70 if at == int(at) else 52))
        for at in (1, 3):
            drums.append((T + at, 0.1, 44, 60))
        for at in (0, 2):
            drums.append((T + at, 0.2, 36, 38))
        if b % 4 == 3:
            drums += [(T + 2 + S, 0.1, 38, 34), (T + 3 + S, 0.1, 38, 48)]
    return mel, keys, bass, drums

if __name__ == '__main__':
    out = os.path.join(os.path.dirname(__file__), '..', '..', 'docs', 'music')
    os.makedirs(out, exist_ok=True)
    mel, keys, bass, drums = last_call()
    write(os.path.join(out, 'last_call.mid'), 84, [
        track('Melody (Vibes / Sax)', mel, 0, 11),
        track('Keys (Electric Piano)', keys, 1, 4),
        track('Bass (Upright)', bass, 2, 32),
        track('Drums (Brush / Jazz Kit)', drums, 9, 0),
    ])
    print('docs/music/last_call.mid')
