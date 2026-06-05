# 验证地图草稿
with open("level_draft.txt", "r", encoding="utf-8") as f:
    lines = f.readlines()

# 找到地图数据行（以行号标记的行之后）
map_start = 0
for i, line in enumerate(lines):
    if line.startswith("行0:"):
        map_start = i
        break

rows = []
for i in range(map_start, min(map_start + 13, len(lines))):
    # 提取行号后的内容
    line = lines[i]
    # 格式: "行N:  <content>"
    content = line.split(":", 1)[1].strip() if ":" in line else line.strip()
    rows.append(content)
    print(f"行{i - map_start}: 长度={len(content)}")

print(f"\n总行数: {len(rows)}")

# 检查长度一致性
lengths = [len(r) for r in rows]
if len(set(lengths)) == 1:
    print(f"所有行长度一致: {lengths[0]}")
else:
    print(f"长度不一致! {set(lengths)}")

# 统计各种字符出现次数
from collections import Counter
all_chars = Counter()
for r in rows:
    all_chars.update(r)
print(f"\n字符统计:")
for ch in sorted(all_chars.keys()):
    print(f"  '{ch}': {all_chars[ch]}")

# 验证关键特征位置
print(f"\n=== 特征位置验证 ===")
features_to_check = {
    'P': '存档点',
    'T': '终点旗',
    'E': '普通敌',
    'F': '火敌',
    'I': '冰敌',
    'G': '叶敌',
    'L': '电敌',
    'C': '蛋糕',
    'K': '木箱',
    '3': '水面',
    '4': '水体',
    '5': '冰砖',
    '6': '碎石砖',
}

for ch, name in features_to_check.items():
    positions = []
    for r_idx, row in enumerate(rows):
        for c_idx, char in enumerate(row):
            if char == ch:
                positions.append((r_idx, c_idx))
    if positions:
        print(f"{name}({ch}): {positions}")
    else:
        print(f"{name}({ch}): 无")
