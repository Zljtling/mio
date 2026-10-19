$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing.Common,System.Drawing.Primitives,System.Private.Windows.GdiPlus,System.Private.Windows.Core -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
public static class SelectorAssetExport
{
    public static void Export(string sourcePath, string destinationPath, int width, int height)
    {
        using (var source = new Bitmap(sourcePath))
        {
            // Generated sprites may contain invisible RGB residue at alpha 1.
            // Quantize alpha for crisp pixel assets before nearest-neighbor export.
            for (int y = 0; y < source.Height; ++y)
                for (int x = 0; x < source.Width; ++x)
                {
                    Color pixel = source.GetPixel(x, y);
                    source.SetPixel(x, y, pixel.A < 128 ? Color.Transparent : Color.FromArgb(255, pixel.R, pixel.G, pixel.B));
                }
            using (var target = new Bitmap(width, height, PixelFormat.Format32bppArgb))
            {
                using (var graphics = Graphics.FromImage(target))
                {
                    graphics.CompositingMode = CompositingMode.SourceCopy;
                    graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
                    graphics.PixelOffsetMode = PixelOffsetMode.Half;
                    graphics.Clear(Color.Transparent);
                    graphics.DrawImage(source, new Rectangle(0, 0, width, height), 0, 0, source.Width, source.Height, GraphicsUnit.Pixel);
                }
                target.Save(destinationPath, ImageFormat.Png);
            }
        }
    }
}
'@
$selectorAssetRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../resources/selector'))
$selectorGeneratedRoot = 'C:\Users\34003\.codex\generated_images\01a124bc-9ecf-7cb0-afcf-70aa6f42838a'
$selectorExports = @(
    @('exec-680027f6-0f4e-44a2-88e1-1de26e639ddb.png', 'avatar_naiwa.png', 84, 84),
    @('exec-e6956a1a-4478-484b-a133-01f98577e002.png', '1P_naiwa_desc.png', 2032, 774),
    @('exec-3485f85f-0b18-436f-9019-966686e8abea.png', '2P_naiwa_desc.png', 1983, 793),
    @('exec-0f54bb40-6f49-41bd-8341-bc2273f844d0.png', 'naiwa_selector_background.png', 560, 720)
)
foreach ($selectorExport in $selectorExports) {
    $selectorSource = Join-Path $selectorGeneratedRoot $selectorExport[0]
    $selectorDestination = Join-Path $selectorAssetRoot $selectorExport[1]
    if (Test-Path -LiteralPath $selectorDestination) { throw "Asset already exists: $selectorDestination" }
    Copy-Item -LiteralPath $selectorSource -Destination (Join-Path $PSScriptRoot $selectorExport[1])
    [SelectorAssetExport]::Export($selectorSource, $selectorDestination, [int]$selectorExport[2], [int]$selectorExport[3])
    Write-Output "$($selectorExport[1]): $($selectorExport[2]) x $($selectorExport[3])"
}
