using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Windows.Forms;
using Microsoft.Win32;

internal sealed class InstallerForm : Form
{
    private readonly TextBox directoryBox = new TextBox();
    private readonly CheckBox desktopBox = new CheckBox();
    private readonly ProgressBar progress = new ProgressBar();
    private readonly Label status = new Label();
    private readonly Button installButton = new Button();
    private readonly Button launchButton = new Button();

    internal InstallerForm()
    {
        Text = "JOJO 格斗游戏 · 安装程序";
        ClientSize = new Size(580, 296);
        MinimumSize = MaximumSize = Size;
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Color.FromArgb(22, 28, 43);
        ForeColor = Color.White;
        Font = new Font("Microsoft YaHei", 10);

        Label heading = new Label { Text = "JOJO 格斗游戏", Font = new Font("Microsoft YaHei", 23, FontStyle.Bold),
            ForeColor = Color.FromArgb(255, 218, 91), Location = new Point(24, 18), Size = new Size(520, 49) };
        Label subheading = new Label { Text = "三名角色 · 城市街道地图 · 可编辑角色数值", Location = new Point(26, 72), Size = new Size(520, 26) };
        Label pathLabel = new Label { Text = "安装位置", Location = new Point(26, 110), Size = new Size(170, 22) };
        directoryBox.Location = new Point(26, 138);
        directoryBox.Size = new Size(444, 28);
        directoryBox.Text = InstallerCore.DefaultDirectory;
        Button browse = new Button { Text = "浏览…", Location = new Point(482, 136), Size = new Size(75, 30) };
        browse.Click += (sender, args) => { using (FolderBrowserDialog dialog = new FolderBrowserDialog())
            { dialog.SelectedPath = directoryBox.Text; if (dialog.ShowDialog() == DialogResult.OK) directoryBox.Text = dialog.SelectedPath; } };
        desktopBox.Text = "创建桌面快捷方式";
        desktopBox.Checked = true;
        desktopBox.Location = new Point(26, 180);
        desktopBox.Size = new Size(220, 27);
        progress.Location = new Point(26, 218);
        progress.Size = new Size(531, 13);
        status.Text = "准备安装";
        status.Location = new Point(26, 243);
        status.Size = new Size(330, 25);
        installButton.Text = "安装游戏";
        installButton.BackColor = Color.FromArgb(45, 125, 203);
        installButton.ForeColor = Color.White;
        installButton.Location = new Point(379, 244);
        installButton.Size = new Size(84, 32);
        installButton.Click += InstallClick;
        launchButton.Text = "启动游戏";
        launchButton.Location = new Point(471, 244);
        launchButton.Size = new Size(86, 32);
        launchButton.Enabled = false;
        launchButton.Click += (sender, args) => InstallerCore.Launch(directoryBox.Text);
        Controls.AddRange(new Control[] { heading, subheading, pathLabel, directoryBox, browse,
            desktopBox, progress, status, installButton, launchButton });
    }

    private void InstallClick(object sender, EventArgs args)
    {
        installButton.Enabled = false;
        try
        {
            InstallerCore.Install(directoryBox.Text, desktopBox.Checked, true, (done, total) =>
            {
                progress.Maximum = total;
                progress.Value = Math.Min(done, total);
                status.Text = "安装资源 " + done + " / " + total;
                Application.DoEvents();
            });
            status.Text = "安装完成。";
            launchButton.Enabled = true;
            MessageBox.Show("安装完成，可以启动游戏。", "JOJO 格斗游戏", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        catch (Exception error)
        {
            status.Text = "安装失败。";
            MessageBox.Show(error.Message, "安装失败", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally { installButton.Enabled = true; }
    }
}

internal static class InstallerCore
{
    internal static readonly string DefaultDirectory = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "JoJoFightingGame");
    private const string Marker = "JOJO_FIGHTING_GAME_INSTALL_V1";

    [STAThread]
    private static int Main(string[] args)
    {
        if (args.Length > 0 && args[0] == "/quiet")
        {
            string directory = args.Length > 1 ? args[1] : DefaultDirectory;
            try { Install(directory, false, false, (done, total) => { }); return 0; }
            catch (Exception error) { Console.Error.WriteLine(error); return 1; }
        }
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new InstallerForm());
        return 0;
    }

    internal static void Install(string target, bool desktopShortcut, bool registerApp, Action<int, int> report)
    {
        if (String.IsNullOrWhiteSpace(target)) throw new InvalidOperationException("请选择安装位置。");
        string directory = Path.GetFullPath(target).TrimEnd('\\');
        if (directory == Path.GetPathRoot(directory).TrimEnd('\\'))
            throw new InvalidOperationException("安装位置不能是磁盘根目录。");
        string marker = Path.Combine(directory, ".jojo-install-marker");
        bool existed = Directory.Exists(directory);
        if (existed && Directory.GetFileSystemEntries(directory).Length != 0
            && (!File.Exists(marker) || File.ReadAllText(marker).Trim() != Marker))
            throw new InvalidOperationException("安装目录已有其他文件，请选择空目录。");

        Stream payload = Assembly.GetExecutingAssembly().GetManifestResourceStream("GamePayload");
        if (payload == null) throw new InvalidOperationException("安装包缺少游戏数据。");
        try
        {
            Directory.CreateDirectory(directory);
            using (payload)
            using (ZipArchive zip = new ZipArchive(payload, ZipArchiveMode.Read))
            {
                int done = 0;
                foreach (ZipArchiveEntry entry in zip.Entries)
                {
                    string relative = entry.FullName.Replace('/', '\\');
                    string file = Path.GetFullPath(Path.Combine(directory, relative));
                    if (!file.StartsWith(directory + "\\", StringComparison.OrdinalIgnoreCase))
                        throw new InvalidOperationException("安装包中的文件路径无效。");
                    Directory.CreateDirectory(Path.GetDirectoryName(file));
                    if (!String.IsNullOrEmpty(entry.Name)) entry.ExtractToFile(file, true);
                    report(++done, zip.Entries.Count);
                }
            }
            string game = Path.Combine(directory, "JoJoFightingGame.exe");
            if (!File.Exists(game) || !File.Exists(Path.Combine(directory, "resources", "character_stats.csv")))
                throw new InvalidOperationException("安装包不完整。");
            File.WriteAllText(marker, Marker);
            if (registerApp)
            {
                string menu = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Programs), "JOJO 格斗游戏.lnk");
                CreateShortcut(menu, game, directory);
                if (desktopShortcut)
                {
                    string desktop = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), "JOJO 格斗游戏.lnk");
                    CreateShortcut(desktop, game, directory);
                }
                using (RegistryKey key = Registry.CurrentUser.CreateSubKey(
                    @"Software\Microsoft\Windows\CurrentVersion\Uninstall\JoJoFightingGame"))
                {
                    key.SetValue("DisplayName", "JOJO 格斗游戏");
                    key.SetValue("DisplayVersion", "1.0");
                    key.SetValue("InstallLocation", directory);
                    key.SetValue("DisplayIcon", game);
                    key.SetValue("UninstallString", "\"" + Path.Combine(directory, "Uninstall.exe") + "\"");
                    key.SetValue("Publisher", "JoJo Fighting Game Project");
                }
            }
        }
        catch
        {
            if (!existed && Directory.Exists(directory)) Directory.Delete(directory, true);
            throw;
        }
    }

    internal static void Launch(string directory)
    {
        Process.Start(new ProcessStartInfo(Path.Combine(directory, "JoJoFightingGame.exe"))
        { WorkingDirectory = directory, UseShellExecute = true });
    }

    private static void CreateShortcut(string path, string target, string workingDirectory)
    {
        Type type = Type.GetTypeFromProgID("WScript.Shell");
        if (type == null) return;
        dynamic shell = Activator.CreateInstance(type);
        dynamic link = shell.CreateShortcut(path);
        link.TargetPath = target;
        link.WorkingDirectory = workingDirectory;
        link.IconLocation = target;
        link.Description = "JOJO 格斗游戏";
        link.Save();
    }
}
