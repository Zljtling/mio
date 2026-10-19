# 将奶蛋求生打包成 Windows 应用

根据 2026-10-04 你的 MESA 项目编写。**本次已制作图标、资源脚本与安装包模板，没有直接修改 main.cpp 或项目配置；以下操作由你完成。**

完成第 1～7 节后，得到一个可以双击 EXE 运行的免安装游戏目录。把整个目录压缩成 ZIP，别人解压后即可打开游戏，不需要安装 Visual Studio 或 EasyX 开发环境。第 8 节可选，进一步制作一个安装 EXE，安装后生成快捷方式。

你的游戏当前从磁盘读取图片、音频和视频。因此免安装版需要保留 EXE 旁边的资源目录；单独复制一个 EXE 不会带上素材。安装包 EXE 可以包含整套资源，安装时展开到游戏目录。

## 1. 先准备图标与备份

关闭正在运行的游戏，把 main.cpp 复制一份保存为 main.cpp.bak，备份项目文件 MESA.vcxproj。备份不需要添加到项目或发布目录。

我已经准备：

| 文件 | 用途 |
|---|---|
| [game.ico](<packaging/game.ico>) | Windows 应用图标，包含 16、24、32、48、64、128、256 七个尺寸 |
| [game_icon.png](<packaging/game_icon.png>) | 1024×1024 图标预览，可以用于介绍游戏 |
| [app_icon.rc](<packaging/app_icon.rc>) | 将图标和 1.0.0 版本信息嵌入 EXE 的资源脚本 |
| [MESA_installer.iss](<packaging/MESA_installer.iss>) | 可选的 Inno Setup 安装包模板 |
| [图标生成说明](<packaging/图标生成说明.md>) | 原图位置、工具与完整生成提示词 |

图标是奶黄色蛋形角色、绿色眼睛、白色蛋壳和蓝白牛奶盒，采用原创角色特写构图。

## 2. 设置发布配置与输出位置

在 Visual Studio 顶部把配置切换为 **Release**、平台切换为 **x64**。右键「MESA 项目」→「属性」，在属性窗口顶部也确认配置是 Release、平台是 x64。

下面以 Windows 10/11 的 64 位电脑作为测试目标。

| 属性位置 | 设置 |
|---|---|
| 配置属性 → 常规 → 配置类型 | 应用程序 (.exe)，当前已经是该类型 |
| 配置属性 → 常规 → 输出目录 | `$(ProjectDir)dist\奶蛋求生\` |
| 配置属性 → 常规 → 目标名称 | `奶蛋求生`，这里不写 .exe |
| 配置属性 → 高级 → 字符集 | 使用 Unicode 字符集，保留现有设置 |
| C/C++ → 代码生成 → 运行库 | 多线程 (/MT)，你的 Release/x64 当前已经是这个设置 |
| C/C++ → 命令行 → 其他选项 | 保留已有的 /utf-8 |
| 链接器 → 系统 → 子系统 | 窗口 (/SUBSYSTEM:WINDOWS) |
| 链接器 → 高级 → 入口点 | 留空，使用编译器提供的默认启动代码 |

点击「应用」。暂时不要运行，因为第 4 节还需要配套调整入口函数，第 5 节需要放好资源。

输出目录改为：

`Projects\Milk Egg Survival Adventure\MESA\dist\奶蛋求生\`

这是后面直接交付给玩家的目录。中间目录保持原值，不要把 obj、中间文件都放进发布目录。

你此前遇到过 EasyX 的运行库不匹配。这里沿用项目当前的 /MT；不要改成 /MDd，也不要复制 Debug 版本来发布。/MT 是静态运行库配置，必须与参与链接的库兼容。[微软运行库说明](https://learn.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library?view=msvc-170)

## 3. 把图标资源添加到项目

1. 在解决方案资源管理器中右键 **MESA 项目** → 添加 → 现有项。
2. 选择 `Projects\Milk Egg Survival Adventure\MESA\packaging\app_icon.rc`，点击添加。
3. 确认该文件出现在项目里。如果需要检查它的文件属性，「项类型」应该是「资源编译器」，「从生成中排除」为「否」。
4. game.ico 放在原来的 packaging 目录即可，不需要复制到 img，也不需要单独添加到项目。

资源脚本的关键行已经写好：

~~~rc
1 ICON "packaging\\game.ico"
~~~

ID 1 是后面加载窗口图标时使用的编号。app_icon.rc 会把图标嵌进 EXE；只把 ICO 放在 EXE 旁边，并不会自动改变 EXE 图标。[微软 ICON 资源说明](https://learn.microsoft.com/en-us/windows/win32/menurc/icon-resource)

文件还包含「奶蛋求生」产品名称和 1.0.0.0 版本信息，以后发布新版本时可以更新这些数字。

## 4. 调整启动入口、资源查找与窗口图标

当前 main.cpp 的入口是原第 1443 行附近的 `int main()`。按下面三个小步骤改，原来的全部游戏机制继续保留。行号会随着插入代码顺延，请优先搜索函数名定位。

### 4.1 在入口函数上面添加两个帮助函数

找到 `int main()`，在它**上面**插入以下整段代码，必须放在其他函数或类的花括号外面：

```cpp
// 在启动时固定资源查找目录，之后不再改变它
bool UseApplicationDirectory()
{
	wchar_t path[32768] = {};
	DWORD length = GetModuleFileNameW(nullptr, path, _countof(path));
	if (length == 0 || length >= _countof(path))
		return false;

	wchar_t* last_slash = std::wcsrchr(path, L'\\');
	if (!last_slash)
		return false;
	*(last_slash + 1) = L'\0'; // 保留末尾反斜杠，也支持 exe 位于磁盘根目录
	return SetCurrentDirectoryW(path) != FALSE;
}

// ID 1 来自 packaging/app_icon.rc；同时设置标题栏和任务栏图标
void ApplyApplicationIcon(HWND hwnd)
{
	HINSTANCE instance = GetModuleHandleW(nullptr);
	HICON large_icon = static_cast<HICON>(LoadImageW(instance,
		MAKEINTRESOURCEW(1), IMAGE_ICON,
		GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
	HICON small_icon = static_cast<HICON>(LoadImageW(instance,
		MAKEINTRESOURCEW(1), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));

	if (large_icon)
		SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(large_icon));
	if (small_icon)
		SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(small_icon));
	SetWindowTextW(hwnd, L"奶蛋求生");
}
```

你已经包含了 `<cwchar>`，不用重复增加。UseApplicationDirectory 仅在创建游戏窗口、启动其他游戏处理前调用一次，之后不要在每帧循环改变目录。

它将资源查找目录固定到 EXE 所在目录。这样从桌面快捷方式、其他目录或资源管理器启动，都按同一位置寻找 img、mus、art_resources、video，而不是依赖某个开发机器的 D 盘路径。[获取 EXE 路径](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulefilenamew)、[设置进程目录](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory)

### 4.2 替换入口函数的签名

把原来的**这一行**：

```cpp
int main()
```

替换为：

```cpp
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
```

原来的左花括号、函数内部代码、return 0 和结束花括号都继续保留。不要同时保留两个入口函数。

它与第 2 节的 Windows 子系统配套，打开时直接显示游戏窗口，不会附带原来的控制台黑窗口。入口点属性仍然留空。[微软 wWinMain 说明](https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point)

### 4.3 修改入口最前面的几行

把入口函数开头改成下面的样子。保留你已接入的视频窗口初始化处理，也保留后面的素材加载和游戏循环：

```cpp
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	if (!UseApplicationDirectory())
	{
		MessageBoxW(nullptr, L"无法确定游戏资源目录。", L"启动失败",
			MB_OK | MB_ICONERROR);
		return 1;
	}

	initgraph(1280, 720);
	ApplyApplicationIcon(GetHWnd()); // 这一行新增：应用图标与窗口标题

	// 下面从你现有的 if (!InstallVideoWindowSupport(GetHWnd())) 开始，
	// 窗口处理、素材加载和游戏循环全部保留。
```

上面最后一条注释是插入位置提示，不是让你删除剩余函数内容，也不要在这个位置新增 main 的结束花括号。

ApplyApplicationIcon 使用 app_icon.rc 内的图标，设置标题栏与任务栏图标，并把窗口标题设为「奶蛋求生」。本函数使用共享图标句柄，不需要在结束时对它们调用 DestroyIcon。

**注意：** 加入 UseApplicationDirectory 后，即使按 F5，资源也会从新 EXE 目录读取。因此必须完成下一节的资源复制；仅把 VS「调试工作目录」设为原项目目录，不会覆盖这里的设置。

## 5. 整理游戏资源

在资源管理器里创建：

`Projects\Milk Egg Survival Adventure\MESA\dist\奶蛋求生\`

按下面的表复制资源，保持文件名和目录层级：

| 从项目复制 | 放到发布目录内 |
|---|---|
| 整个 `MESA\img` 目录 | `img` |
| 整个 `MESA\mus` 目录 | `mus` |
| 整个 `MESA\art_resources\enemy_variant_2` 目录 | `art_resources\enemy_variant_2` |
| 整个 `MESA\art_resources\enemy_variant_3` 目录 | `art_resources\enemy_variant_3` |
| `MESA\video\game_over.avi` | `video\game_over.avi` |

先在发布目录创建 art_resources，再把 enemy_variant_2、enemy_variant_3 放进去，不能直接把两个敌人目录放在 EXE 同层。

整理完成后目录如下：

~~~text
奶蛋求生/
├─ 奶蛋求生.exe                 ← 第 6 节重新生成得到
├─ img/                         ← 地图、菜单、按钮、玩家、普通敌人、特效等
├─ mus/
│  ├─ bgm.mp3
│  ├─ hit_original_2s.wav
│  └─ dead.wav
├─ art_resources/
│  ├─ enemy_variant_2/           ← 精英敌人素材
│  └─ enemy_variant_3/           ← BOSS 素材
└─ video/
   └─ game_over.avi
~~~

mus 中列出的三个文件名是你当前源码实际读取的名称。视频只需 AVI，原 MP4 可继续保存在开发项目里。img 直接复制完整目录，可以避免漏掉某一帧动画。

myimg、原始生成图、预览图、素材备份、main.cpp、Visual Studio 项目文件和各份开发指南不需要随游戏发布。当前上表资源共 82 个文件，约 52 MiB，不含新 EXE。较大文件是结束视频。

以后更新素材，需要把改动同步到发布目录；编译 C++ 不会自动复制这些文件。

## 6. 重新生成并检查

1. 在 VS 保存所有文件。
2. 再确认顶部是 Release / x64。
3. 菜单「生成」→「重新生成解决方案」。等待成功，先处理错误列表中的第一条错误。
4. 打开输出目录，确认出现 `奶蛋求生.exe`，时间是刚才重新生成的时间。
5. 确认图标已经变成奶蛋图案。EXE 属性 → 详细信息应能看到「奶蛋求生」与版本信息。
6. 在资源管理器里直接双击 EXE，进行下面的检查。

必测项目：

- 窗口直接打开，不附带控制台黑窗口；标题与图标正确。
- 菜单、按钮、医院地图、玩家和三种敌人素材都正常。
- 玩家移动、F 冰冻、血条、血包、灼烧和 BOSS 行为正常。
- 背景音乐、攻击音效和死亡音效正常。
- 死亡后按 1 可以播放视频，视频稳定，点播放中的窗口 × 或等到结束都回菜单。
- 回菜单再开始一局，分数、血量与技能状态重新初始化。
- 菜单选择退出后游戏进程关闭。

还应把**整个游戏目录**复制到一个与项目无关的位置，例如桌面的测试目录，再双击 EXE 测试。也可以新建桌面快捷方式，把其「起始位置」暂时留空，确认资源仍能加载。发布前最好在没有 Visual Studio/EasyX 开发环境的另一台 64 位 Windows 电脑实际检查。

如果输出目录里还有 `.pdb`、`.ilk`，它们属于调试/链接文件，不是运行必需素材，可以在制作发布副本时省略；开发机器上的原件可以保留。

## 7. 做成可分享的免安装版

1. 将第 6 节通过检查的完整「奶蛋求生」目录复制为发布副本。
2. 右键该目录 →「压缩为 ZIP 文件」，或「发送到 → 压缩(zipped)文件夹」，以系统菜单实际名称为准。
3. ZIP 可以命名为 `奶蛋求生_1.0.0_Windows_x64.zip`。
4. 对方解压整个 ZIP，再双击目录内的 `奶蛋求生.exe`。不要在压缩包预览窗口里直接运行 EXE。
5. 想放到桌面时，为 EXE 创建快捷方式，资源仍保留在完整游戏目录里。

按上述流程，玩家不需要 Visual Studio，也不需要 EasyX 头文件或 lib 文件。这些属于开发时使用的文件。你当前 Release EXE 与按本教程编译的临时 EXE 都只直接依赖 Windows 系统 DLL；以后如果加入其他动态库，需重新检查并随包带上相应依赖。

## 8. 可选：制作安装 EXE 与桌面快捷方式

如果希望给别人一个安装文件，运行后自动建立游戏目录、快捷方式和卸载入口，可使用 Inno Setup。免安装版完成后再做这一节。

**本次没有安装或下载 Inno Setup。** 若你选择安装，建议把打包工具统一安装到 `D:\Tools\Programs\Inno Setup\`，游戏项目和素材不放进工具目录。下载使用 [Inno Setup 官方页面](https://jrsoftware.org/isinfo.php)。

我已经准备模板：

`Projects\Milk Egg Survival Adventure\MESA\packaging\MESA_installer.iss`

使用步骤：

1. 先完成第 1～7 节，确认 `MESA\dist\奶蛋求生\奶蛋求生.exe` 及资源已全部就绪。
2. 安装支持 `x64compatible` 的新版 Inno Setup，用它打开该 .iss 文件，保持文件的 UTF-8 BOM 编码。
3. 检查文件开头 AppName、AppVersion、AppExeName。第一版已设置为「奶蛋求生」、1.0.0、奶蛋求生.exe。
4. 按 F9 编译。得到 `MESA\dist\installer\MESA_Setup_1.0.0.exe`。
5. 在测试环境运行安装包，选择创建桌面快捷方式，完成后启动游戏，重复第 6 节的运行检查。
6. 最后测试卸载入口能正常移除安装内容。

模板读取 `..\dist\奶蛋求生` 的文件并保留子目录，使用 game.ico 作为安装包图标。它会省略 pdb、ilk、exp、lib，安装到当前用户的 `%LOCALAPPDATA%\Programs\MESA`，创建开始菜单和可选的桌面快捷方式，不要求管理员安装。[文件与子目录](https://jrsoftware.org/ishelp/topic_filessection.htm)、[快捷方式](https://jrsoftware.org/ishelp/topic_iconssection.htm)、[用户安装权限](https://jrsoftware.org/ishelp/topic_setup_privilegesrequired.htm)

这里的安装 EXE 是安装整套游戏的分发文件；安装后运行的游戏仍有自己的图片、音频和视频目录。升级同一个游戏时保留 AppId，增加版本号；创建另一个独立游戏时应换成不同的 AppId。

模板未在本机编译为安装包，因为没有安装 Inno Setup，正式安装包的安装与卸载仍需你实际测试。

## 常见问题

| 现象 | 检查 |
|---|---|
| 报找不到 main / WinMain / wWinMain | 第 2 节 Windows 子系统与第 4 节 wWinMain 是否一起修改；入口点留空 |
| LNK2038 运行库不匹配 | 是否选择 Release/x64、/MT，并与当前 EasyX 库匹配 |
| EXE 有新图标，窗口/任务栏还是旧图标 | ApplyApplicationIcon 是否在 initgraph 后调用 |
| EXE 图标没有变化 | app_icon.rc 是否添加并参与资源编译；是否重新生成正确的 EXE |
| 图标资源 RC2135 找不到文件 | game.ico 是否仍在 MESA/packaging，rc 中路径是否保持 packaging\\game.ico |
| 运行后没图片、没声音或找不到视频 | 发布目录层级是否正确，82 个资源是否按第 5 节复制 |
| F5 突然无法加载资源 | UseApplicationDirectory 已固定到新 EXE 目录；该目录同样需要放素材 |
| 在自己电脑能运行，另一台不行 | 是否复制整套目录、使用 64 位 Windows；检查具体缺失 DLL 或资源提示 |
| 安装模板找不到 Source 文件 | dist/奶蛋求生 是否已生成 EXE，并放在与 packaging 相邻的 dist 目录下 |
| 安装模板不认识 x64compatible | 使用支持该架构标识的新版 Inno Setup |
| 调试时已有图标，但桌面快捷方式显示旧图标 | 先确认新 EXE 已嵌入图标，再为新发布目录重新创建快捷方式 |

## 本次验证与文件位置

- 图标 PNG 1024×1024，透明圆角；ICO 七个尺寸已用 Windows LoadImageW 实际加载并核对尺寸。
- app_icon.rc 已通过 Windows SDK 资源编译器，图标、中文产品名称和版本信息可以从临时 EXE 中读取。
- 按第 4 节将改动合并到临时完整源文件，用当前 MSVC、EasyX、C++20、Unicode、/utf-8、/MT 编译链接成功。PE 文件为 x64 Windows GUI 子系统。
- 启动目录、窗口图标句柄和中文标题通过独立隐藏窗口检查。临时完整游戏没有启动，因此不替代第 6 节的实际运行检查。
- 当前已有 Release EXE 和临时窗口版 EXE 的直接依赖均为 KERNEL32、USER32、WINMM、MSIMG32、SHELL32 等 Windows 系统 DLL，未出现额外的 VC 运行库 DLL。
- 没有创建正式发布 ZIP 或安装 EXE；发布目录需要按教程由你整理并测试，安装模板没有实际编译。
- main.cpp 与 MESA.vcxproj 均保持原样。临时检查文件统一在系统 TEMP 的 mesa-packaging-check，不参与项目编译。
- 所有新增图标与打包模板统一在 MESA/packaging，原始生成图在 MESA/art_resources/source/app_icon_source.png。工具记录见 MESA/说明.md 和 packaging/图标生成说明.md，未安装新工具或插件。
