import csv
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PATH = ROOT / "resources" / "character_stats.csv"

TIMINGS = {
    "jotaro": [
        ("normal_cast_ms", 640, "普通攻击动作时长；命中点按比例调整"),
        ("kick_cast_ms", 860, "踢击动作时长；命中点按比例调整"),
        ("ora_cast_ms", 480, "欧拉连打一次动作时长"),
        ("special_cast_ms", 1733, "特殊攻击动作时长；蓄力命中点按比例调整"),
        ("ultimate_startup_ms", 0, "大招起手到时停生效的时间；0为立即生效"),
        ("ultimate_effect_duration_ms", 5000, "时停生效持续时间"),
    ],
    "kakyoin": [
        ("normal_cast_ms", 160, "普通攻击动作时长；飞针发射点按比例调整"),
        ("special_cast_ms", 900, "特殊攻击动作时长；绿宝石发射节奏按比例调整"),
        ("ultimate_startup_ms", 560, "大招起手到结界生效的时间"),
        ("ultimate_effect_duration_ms", 3000, "结界生效持续时间"),
        ("ultimate_recovery_ms", 440, "大招结束后的收招时间；0为立即结束"),
    ],
    "naiwa": [
        ("normal_cast_ms", 480, "普通攻击召唤幻影的动作时长"),
        ("special_cast_ms", 720, "特殊技召唤豆包小姐的动作时长"),
        ("teleport_cast_ms", 360, "瞬移完整动作时长；中途移动点按比例调整"),
        ("ultimate_startup_ms", 270, "大招起手到声波出现的时间；0为立即出现"),
        ("ultimate_recovery_ms", 180, "声波结束后的收招时间；0为立即结束"),
        ("phantom_summon_ms", 500, "幻影出现到开始左右攻击的时间"),
        ("phantom_attack_cycle_ms", 1000, "幻影左右攻击一轮的时间"),
        ("ultimate_wave_cycle_ms", 560, "大招声波由中心向外扩散一轮的时间"),
    ],
}


with PATH.open("r", encoding="gb18030", newline="") as handle:
    rows = list(csv.reader(handle))

header, body = rows[0], rows[1:]
assert header == ["character", "key", "value", "unit", "description"]
known = {(row[0], row[1]) for row in body}
result = [header]
for index, row in enumerate(body):
    result.append(row)
    character = row[0]
    if index + 1 < len(body) and body[index + 1][0] == character:
        continue
    for key, value, description in TIMINGS.get(character, []):
        if (character, key) not in known:
            result.append([character, key, str(value), "ms", description])

with PATH.open("w", encoding="gb18030", newline="") as handle:
    csv.writer(handle, lineterminator="\n").writerows(result)

print(f"Wrote {len(result) - 1} character stats, including 19 timing fields")
