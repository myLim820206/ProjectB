"""주막 경제 밸런스 시뮬레이터.

data/*.csv 를 읽어 본편 80일을 하루 단위로 돌린다. 플레이어 숙련도별로
빚 상환, 평판 단계 도달일, 밤 영업 해금일을 확인한다.
영업 1회를 손님 단위로 시뮬하지 않고, 하루 단위 기대값 모델로 근사한다.
빚을 다 갚는 날이 결말이다. 80일 이후 밀린 빚은 10일마다 상환하며 최대 160일까지 돌린다.

사용법: python3 tools/balance/sim.py [--seeds 200] [--verbose 보통]
"""
import argparse
import csv
import pathlib
import random
import statistics

DATA = pathlib.Path(__file__).resolve().parents[2] / "data"

# ---------------------------------------------------------------- 조정 가능한 상수
BASE_ARRIVALS = 12          # 평판 1단계, 맑은 평일 기준 하루 방문 기대 인원
SERVICE_SEC = 300           # 낮 영업 실시간 길이
NIGHT_SEC = 125             # 밤 영업 실시간 길이
STAY_SEC = 45               # 손님 1명이 자리를 차지하는 평균 시간
SEAT_FILL = 0.75            # 일행 단위 착석으로 생기는 빈자리 손실
STAFF_THROUGHPUT = 2.0      # 일꾼 1명이 더하는 처리량 (명/분). 플레이어의 절반 수준
STAFF_WAGE = 35             # 일꾼 일당 (푼)
WASTE_RATE = 0.08           # 미리 끓인 솥 요리 남김 손실 (재료비 대비)
TIP = 1.05                  # 최고 만족 손님 팁을 평균으로 반영
REP_PER_SERVED = 1.6        # 응대 손님 1명당 평판 기본값 (만족 등급 평균)
REP_STAGE_BONUS = 0.15      # 단계가 오를수록 품질이 좋아져 늘어나는 평판
RUMOR = 1.35                # 손님 소문력 평균 배수
ANGRY_PENALTY = 3           # 기다리다 화내고 떠난 손님 1명당 평판 감소
LEFT_PENALTY = 0.3          # 자리가 없어 그냥 지나친 손님 1명당 평판 감소
MISS_INTEREST = 0.10        # 상환 실패 시 해당 회차 금액에 붙는 가산금
MISS_REP = 50               # 상환 실패 시 평판 감소
YOKAI_COIN = 12             # 요괴 손님 평균 엽전 (푼)
PRANK_LOSS = 30             # 요괴 장난 1회 손실 (푼)

SKILL = {"초보": 3.0, "보통": 4.2, "숙련": 5.5}  # 플레이어 혼자 처리하는 손님 수 (명/분)
WEATHER = {  # 계절별 (비/눈 확률)
    0: 0.20, 1: 0.35, 2: 0.15, 3: 0.30,
}


def load(name):
    with open(DATA / name, encoding="utf-8") as f:
        return list(csv.DictReader(f))


def stage_of(unlock):
    return {"start": 1, "rep2": 2, "rep3": 3, "rep4": 4, "market_merchant": 2,
            "regular_story": 3, "festival": 2, "special_guest": 3, "night_open": 3}.get(unlock, 9)


class Model:
    def __init__(self):
        self.ing = {r["Name"]: float(r["PriceFun"]) for r in load("ingredients.csv")}
        self.recipes = load("recipes.csv")
        for r in self.recipes:
            cost = 0.0
            for part in r["Ingredients"].split(";"):
                k, q = part.split(":")
                cost += self.ing[k] * float(q)
            r["cost"] = cost
            r["stage"] = stage_of(r["Unlock"])
        self.rep = load("reputation.csv")
        self.debt = load("debt.csv")
        self.fac = {r["Name"]: r for r in load("facilities.csv")}
        for r in self.fac.values():
            r["eff"] = dict(kv.split("=") for kv in r["Effect"].split(";"))

    def menu(self, stage, season):
        sname = ["spring", "summer", "autumn", "winter"][season]
        ok = [r for r in self.recipes if r["stage"] <= stage and (r["Season"] == "all" or sname in r["Season"])]
        food = [r for r in ok if r["Alcohol"] == "0"]
        drink = [r for r in ok if r["Alcohol"] == "1"]
        # 손님은 비싼 메뉴를 더 고르는 경향 (가격 상위 절반 가중)
        food.sort(key=lambda r: -float(r["PriceFun"]))
        top = food[: max(3, len(food) // 2)]
        fp = statistics.mean(float(r["PriceFun"]) for r in top)
        fc = statistics.mean(r["cost"] for r in top)
        dp = statistics.mean(float(r["PriceFun"]) for r in drink)
        dc = statistics.mean(r["cost"] for r in drink)
        return fp, fc, dp, dc


def run(model, skill, seed, verbose=False):
    rnd = random.Random(seed)
    money = 300  # 시작 자금 3냥
    rep = 0.0
    owned = {"kitchen": 1, "madang": 1, "sulgwang": 1, "judeung": 1, "ilkkunbang": 0}
    pyeongsang = 2
    staff = 0
    night_day = None
    stage_day = {1: 1}
    missed = 0
    carry = 0.0
    debt_left = sum(float(d["InstallmentNyang"]) for d in model.debt) * 100
    due = {int(d["DueDay"]): float(d["InstallmentNyang"]) * 100 for d in model.debt}
    log = []

    def stage():
        s = 1
        for r in model.rep:
            if rep >= float(r["Threshold"]):
                s = int(r["Stage"])
        return s

    def eff(key, default=0.0):
        total = default
        for name, lvl in owned.items():
            r = model.fac.get(f"{name}_{lvl}")
            if r and int(r["ReqStage"]) > stage():
                continue
            if r and key in r["eff"]:
                total = float(r["eff"][key])
        return total

    def seats(rain):
        s = pyeongsang * 4 * (0.5 if rain else 1.0)
        if owned.get("toetmaru"):
            s += 2
        if owned.get("marubang"):
            s += 6 if owned["marubang"] == 1 else 10
        return s

    end_day = None
    for day in range(1, 161):
        if day > 80 and carry > 0.01 and day % 10 == 0:
            due[day] = 0.0
        season = (day - 1) // 10 % 4
        dis = (day - 1) % 10 + 1
        st = stage()
        rain = rnd.random() < WEATHER[season]
        market = dis in (1, 6)
        mult = float(model.rep[st - 1]["ArrivalMult"])
        arrivals = BASE_ARRIVALS * mult * (0.7 if rain else 1.0) * (1.5 if market else 1.0)
        arrivals *= 1 + eff("arrival_bonus")
        arrivals *= 1 + 0.3 * 0.25 * (eff("bobusang_mult", 1.0) - 1) / 0.3 if owned.get("magugan") else 1
        arrivals *= rnd.uniform(0.85, 1.15)

        seat_cap = seats(rain) * SERVICE_SEC / STAY_SEC * SEAT_FILL
        service_cap = (SKILL[skill] + staff * STAFF_THROUGHPUT) * SERVICE_SEC / 60
        seated = min(arrivals, seat_cap)
        served = min(seated, service_cap)
        angry = seated - served
        passed = arrivals - seated

        fp, fc, dp, dc = model.menu(st, season)
        spend = fp + 0.6 * dp
        cost = fc + 0.6 * dc
        revenue = served * spend * TIP
        expense = served * cost * (1 + WASTE_RATE) + staff * STAFF_WAGE
        if owned.get("gaekbang"):
            rooms = {1: 2, 2: 4, 3: 6}[owned["gaekbang"]]
            revenue += rooms * 0.6 * 30
        strain = served / service_cap if service_cap else 1
        gain = (REP_PER_SERVED + REP_STAGE_BONUS * (st - 1)) * RUMOR * (0.8 if strain > 0.85 else 1.0)
        rep += served * gain - angry * ANGRY_PENALTY - passed * LEFT_PENALTY

        if night_day:
            lit = eff("night_lit_seats", 4)
            night_cap = (SKILL[skill] + staff * STAFF_THROUGHPUT) * NIGHT_SEC / 60
            yokai = min(3 + st, lit / 2 * 2, night_cap)
            revenue += yokai * YOKAI_COIN
            expense += yokai * 0.2 * 0.3 * (1 - eff("prank_reduce")) * PRANK_LOSS
            rep += yokai * 2
        money += revenue - expense
        rep = max(rep, 0)
        profit_avg = (revenue - expense) if day == 1 else profit_avg * 0.7 + (revenue - expense) * 0.3

        new_st = stage()
        if new_st not in stage_day:
            stage_day[new_st] = day
        if new_st >= 3 and not night_day:
            night_day = day + 1

        # 상환
        if day in due:
            need = due[day] + carry
            pay = max(0.0, min(money, need))  # 부분 상환 허용
            money -= pay
            debt_left -= pay
            carry = need - pay
            if carry > 0.01:
                missed += 1
                carry *= 1 + MISS_INTEREST  # 못 갚은 금액에만 가산금
                debt_left += carry - (need - pay)
                rep = max(0, rep - MISS_REP)

        # 구매 (보통 플레이어의 탐욕적 우선순위)
        next_due = min([d for d in due if d > day] or [81])
        # 남은 날 벌 돈의 80%를 믿고, 모자라는 만큼만 남겨 둔다
        reserve = max(0.0, due.get(next_due, 0) + carry - max(profit_avg, 0) * (next_due - day) * 0.8)
        budget = money - reserve
        st = stage()

        def buy(name, key, lvl, cost_ny, req):
            nonlocal budget, money
            if st >= req and owned.get(key, 0) < lvl and budget >= cost_ny * 100:
                owned[key] = lvl
                money -= cost_ny * 100
                budget -= cost_ny * 100
                return True
            return False

        bottleneck_seat = seat_cap < arrivals * 0.95
        bottleneck_service = service_cap < seated * 0.98
        for _ in range(3):
            maxp = int(eff("pyeongsang_max", 3))
            if bottleneck_seat and pyeongsang < maxp and budget >= 800:
                pyeongsang += 1; money -= 800; budget -= 800; continue
            if bottleneck_service and staff < int(eff("staff_max", 0)) and budget >= 300:
                staff += 1; budget -= 300; continue  # 첫 열흘치 일당을 여유로 확보
            if bottleneck_service and buy("ilkkunbang_1", "ilkkunbang", 1, 15, 2): continue
            if bottleneck_service and buy("ilkkunbang_2", "ilkkunbang", 2, 45, 3): continue
            if buy("judeung_2", "judeung", 2, 15, 1): continue
            if buy("toetmaru_1", "toetmaru", 1, 15, 1): continue
            if buy("ilkkunbang_1", "ilkkunbang", 1, 15, 2): continue
            if buy("madang_2", "madang", 2, 40, 2): continue
            if buy("marubang_1", "marubang", 1, 40, 2): continue
            if buy("magugan_1", "magugan", 1, 30, 2): continue
            if buy("gaekbang_1", "gaekbang", 1, 50, 3): continue
            if buy("judeung_3", "judeung", 3, 45, 3): continue
            if buy("geumjul_1", "geumjul", 1, 10, 3): continue
            if buy("madang_3", "madang", 3, 100, 3): continue
            if buy("ilkkunbang_2", "ilkkunbang", 2, 45, 3): continue
            if buy("marubang_2", "marubang", 2, 80, 4): continue
            if buy("gaekbang_2", "gaekbang", 2, 100, 4): continue
            if buy("magugan_2", "magugan", 2, 70, 4): continue
            if buy("ilkkunbang_3", "ilkkunbang", 3, 90, 4): continue
            break

        if debt_left <= 0.01 and end_day is None:
            end_day = day  # 빚 완납 = 결말
        if end_day:
            if verbose and dis == 10:
                log.append((day, money / 100, max(0, debt_left) / 100, stage(), int(rep), pyeongsang, staff,
                            round(arrivals, 1), round(served, 1), missed))
            break
        if verbose and dis == 10:
            log.append((day, money / 100, debt_left / 100, stage(), int(rep), pyeongsang, staff,
                        round(arrivals, 1), round(served, 1), missed))

    return dict(stage_day=stage_day, night_day=night_day, missed=missed, debt_left=debt_left / 100,
                money=money / 100, log=log, final_stage=stage(), end_day=end_day or 999)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seeds", type=int, default=200)
    ap.add_argument("--verbose", choices=list(SKILL), default=None)
    a = ap.parse_args()
    m = Model()

    print("| 숙련도 | 2단계 | 3단계(밤 해금) | 4단계 | 결말일 중앙값 | 결말일 90% | 80일 결말률 | 결말 시 5단계 | 상환 실패 평균 |")
    print("|---|---|---|---|---|---|---|---|---|")
    for sk in SKILL:
        rs = [run(m, sk, s) for s in range(a.seeds)]

        def med(k):
            v = [r["stage_day"].get(k) for r in rs if r["stage_day"].get(k)]
            return f"{int(statistics.median(v))}일" if v else "-"
        ends = sorted(r["end_day"] for r in rs)
        on_time = sum(1 for e in ends if e <= 80) / len(rs)
        st5 = sum(1 for r in rs if r["final_stage"] >= 5) / len(rs)
        p90 = ends[int(len(ends) * 0.9) - 1]
        print(f"| {sk} | {med(2)} | {med(3)} | {med(4)} | {int(statistics.median(ends))}일 | {p90}일 | "
              f"{on_time:.0%} | {st5:.0%} | {statistics.mean(r['missed'] for r in rs):.2f}회 |")

    if a.verbose:
        r = run(m, a.verbose, 1, verbose=True)
        print(f"\n[{a.verbose}] 계절 말 상태 (시드 1)")
        print("| 일 | 엽전(냥) | 남은 빚(냥) | 단계 | 평판 | 평상 | 일꾼 | 방문 | 응대 | 상환 실패 |")
        print("|---|---|---|---|---|---|---|---|---|---|")
        for row in r["log"]:
            print("| " + " | ".join(f"{v:.1f}" if isinstance(v, float) else str(v) for v in row) + " |")


if __name__ == "__main__":
    main()
