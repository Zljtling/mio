; 奶蛋求生安装包模板
; 先完成 Windows应用打包教程.md，确保 ..\dist\奶蛋求生 内已有 EXE 与资源
; 用新版 Inno Setup 打开本文件后按 F9 编译；不会自动修改 Visual Studio 项目
#define AppName "奶蛋求生"
#define AppVersion "1.0.0"
#define AppExeName "奶蛋求生.exe"
#define StageDir "..\dist\奶蛋求生"

[Setup]
AppId={{67C938D8-62C3-4A12-AB54-729713E2F06C}
AppName={#AppName}
AppVersion={#AppVersion}
DefaultDirName={localappdata}\Programs\MESA
DefaultGroupName={#AppName}
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist\installer
OutputBaseFilename=MESA_Setup_{#AppVersion}
SetupIconFile=game.ico
UninstallDisplayIcon={app}\{#AppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "快捷方式："

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Excludes: "*.pdb,*.ilk,*.exp,*.lib"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{userprograms}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{userdesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "启动奶蛋求生"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

