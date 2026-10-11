# JOJO 格斗游戏

Windows 双人格斗游戏，包含承太郎、花京院和奶娃三名角色。窗口为 1600 × 900，城市街道地图为 3200 × 1800，包含可跳跃的平台、跟随镜头和技能命中震动。

## 安装与演示

最终交付文件位于 [`JoJo's Bizarre Adventure Fighting Game/outputs/release-package/交付/`](JoJo%27s%20Bizarre%20Adventure%20Fighting%20Game/outputs/release-package/交付/)：

- `JOJO格斗游戏安装程序.exe`：Windows 64 位安装程序，支持选择安装路径、快捷方式和卸载。
- `JoJoFightingGame-portable.zip`：完整解压后运行 `启动游戏.cmd`。
- `JOJO格斗游戏-90秒Demo.mp4`：90 秒、720p、24 帧/秒，包含选角与三名角色的技能演示、配乐及音效。
- `使用说明.txt`：安装、操作与数值调整说明。

演示使用实际游戏代码与素材渲染，由脚本触发技能并补充生命和能量，以完整展示角色技能。

## 操作

| 操作 | 1P | 2P |
| --- | --- | --- |
| 移动 | A / D | 左 / 右方向键 |
| 跳跃 | W | 上方向键 |
| 格挡（奶娃为瞬移） | S | 下方向键 |
| 普通攻击 | F | 逗号 |
| 特殊技能 | E | 句号 |
| 大招 | Q | 斜杠 |

选择角色后按 Enter 进入战斗。能量条显示技能消耗刻度，大招可释放时有动态提示。

## 源码与素材

使用 Visual Studio 打开本目录的 `JoJo's Bizarre Adventure Fighting Game.slnx`。项目使用 C++20、Windows SDK 和 EasyX，当前工具集为 `v145`；需安装匹配的 C++ 开发工具与 EasyX。选择 `Release / x64` 构建，随后运行本目录的 `启动游戏.cmd`，以正确设置资源工作目录。

```text
JoJo's Bizarre Adventure Fighting Game/
├── JoJo's Bizarre Adventure Fighting Game.slnx
├── 启动游戏.cmd
└── JoJo's Bizarre Adventure Fighting Game/
    ├── main.cpp、各角色与场景头文件
    ├── resources/                     # 游戏运行素材及角色数值
    ├── 角色设定集参考素材/             # 创作参考
    └── outputs/
        ├── naiwa-balance/角色数值.xlsx
        └── release-package/           # 安装器、打包与演示渲染脚本
            └── 交付/                  # 最终安装包与视频
```

## 修改角色数值

运行时读取 `resources/character_stats.csv`，其中包含角色基础数值、技能消耗、伤害、回能和技能释放时间。CSV 使用 GB18030 编码，修改后重启游戏生效。`outputs/naiwa-balance/角色数值.xlsx` 提供便于查看和编辑的表格；修改 Excel 后需要同步到 CSV，游戏不会直接读取 Excel。

打包脚本 `outputs/release-package/build-payload.py` 从 Release 构建结果收集运行素材。安装器源码为 `Installer.cs` 和 `Uninstaller.cs`；演示渲染与视频脚本为 `render_demo.cpp`、`encode-demo.mjs` 和 `finish-demo.mjs`。视频脚本中的 FFmpeg 路径需要根据本机安装位置调整。
