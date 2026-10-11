import hashlib
import json
import shutil
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "outputs" / "release-package"
STAGE = OUTPUT / "staging"
ARCHIVE = OUTPUT / "JoJoFightingGame-portable.zip"
EXE = ROOT.parent / "x64" / "Release" / "JoJo's Bizarre Adventure Fighting Game.exe"
EXTENSIONS = {".png", ".wav", ".mp3", ".ttf", ".csv"}

if STAGE.exists():
    if STAGE.resolve().parent != OUTPUT.resolve():
        raise RuntimeError("Refusing to clean outside the release output directory")
    shutil.rmtree(STAGE)
STAGE.mkdir(parents=True)

shutil.copy2(EXE, STAGE / "JoJoFightingGame.exe")
shutil.copy2(OUTPUT / "Uninstall.exe", STAGE / "Uninstall.exe")
files = [p for p in (ROOT / "resources").rglob("*") if p.is_file() and p.suffix.lower() in EXTENSIONS]
for source in files:
    destination = STAGE / source.relative_to(ROOT)
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)

(STAGE / "启动游戏.cmd").write_text(
    '@echo off\r\ncd /d "%~dp0"\r\nstart "" "%~dp0JoJoFightingGame.exe"\r\n', encoding="gbk"
)
(STAGE / "游戏说明.txt").write_text(
    "JOJO 格斗游戏\n\n"
    "双击 JoJoFightingGame.exe 或 启动游戏.cmd 开始游戏。\n"
    "1P：A/D 移动，W 跳跃，S 格挡（奶娃为瞬移），F 普攻，E 特殊技，Q 大招。\n"
    "2P：左右方向键移动，上方向键跳跃，下方向键格挡（奶娃为瞬移），\n"
    "    逗号键普攻，句号键特殊技，斜杠键大招。\n"
    "选人后按 Enter 开始。\n\n"
    "数值与技能释放时间：resources\\character_stats.csv。修改后重启游戏生效。\n"
    "通过安装程序安装时，可在 Windows 的“已安装的应用”中卸载。\n",
    encoding="utf-8-sig",
)

manifest = {
    "game": "JOJO 格斗游戏",
    "version": "1.0",
    "runtime_asset_count": len(files),
    "files": {
        path.relative_to(STAGE).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in STAGE.rglob("*") if path.is_file()
    },
}
(STAGE / "release-manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
with zipfile.ZipFile(ARCHIVE, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
    for path in sorted(STAGE.rglob("*")):
        if path.is_file():
            archive.write(path, path.relative_to(STAGE).as_posix())

print(f"Packaged {len(files)} runtime assets ({ARCHIVE.stat().st_size / 1048576:.1f} MiB ZIP)")
