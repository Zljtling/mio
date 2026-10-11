using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32;

internal static class Uninstaller
{
    private const string Marker = "JOJO_FIGHTING_GAME_INSTALL_V1";
    private const string RegistryPath = @"Software\Microsoft\Windows\CurrentVersion\Uninstall\JoJoFightingGame";

    [STAThread]
    private static int Main(string[] args)
    {
        if (args.Length >= 2 && args[0] == "--cleanup")
        {
            Thread.Sleep(900);
            try
            {
                string directory = Path.GetFullPath(args[1]);
                string marker = Path.Combine(directory, ".jojo-install-marker");
                if (!File.Exists(marker) || File.ReadAllText(marker).Trim() != Marker)
                    throw new InvalidOperationException("安装标识不匹配，已停止卸载。");
                if (Path.GetPathRoot(directory).TrimEnd('\\') == directory.TrimEnd('\\'))
                    throw new InvalidOperationException("不能卸载磁盘根目录。");
                string desktop = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), "JOJO 格斗游戏.lnk");
                string menu = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Programs), "JOJO 格斗游戏.lnk");
                DeleteShortcutIfOwned(desktop, directory);
                DeleteShortcutIfOwned(menu, directory);
                using (RegistryKey key = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Uninstall", true))
                    if (key != null) key.DeleteSubKeyTree("JoJoFightingGame", false);
                Directory.Delete(directory, true);
                MessageBox.Show("游戏已卸载。", "JOJO 格斗游戏", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return 0;
            }
            catch (Exception error)
            {
                MessageBox.Show(error.Message, "卸载失败", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return 1;
            }
        }

        string installed = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
        if (MessageBox.Show("确定卸载 JOJO 格斗游戏？安装目录内修改过的数值表也会被删除。", "卸载游戏",
                MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return 0;
        string copy = Path.Combine(Path.GetTempPath(), "JoJoGameUninstall-" + Guid.NewGuid().ToString("N") + ".exe");
        File.Copy(Assembly.GetExecutingAssembly().Location, copy);
        Process.Start(new ProcessStartInfo(copy, "--cleanup \"" + installed + "\"")
        {
            WorkingDirectory = Path.GetTempPath(),
            UseShellExecute = false
        });
        return 0;
    }

    private static void DeleteShortcutIfOwned(string shortcut, string directory)
    {
        if (!File.Exists(shortcut)) return;
        try
        {
            Type type = Type.GetTypeFromProgID("WScript.Shell");
            dynamic shell = Activator.CreateInstance(type);
            dynamic link = shell.CreateShortcut(shortcut);
            string target = Path.GetFullPath((string)link.TargetPath);
            if (target.StartsWith(directory.TrimEnd('\\') + "\\", StringComparison.OrdinalIgnoreCase))
                File.Delete(shortcut);
        }
        catch { }
    }
}
