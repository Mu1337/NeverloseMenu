$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class MultiQaNative {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hwnd, ref POINT point);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extra);
}
"@
function Click([IntPtr]$Window,[int]$X,[int]$Y) {
    $p=New-Object MultiQaNative+POINT; $p.X=$X; $p.Y=$Y
    [MultiQaNative]::ClientToScreen($Window,[ref]$p)|Out-Null
    [MultiQaNative]::SetForegroundWindow($Window)|Out-Null
    [MultiQaNative]::SetCursorPos($p.X,$p.Y)|Out-Null
    Start-Sleep -Milliseconds 35
    [MultiQaNative]::mouse_event(2,0,0,0,[UIntPtr]::Zero); Start-Sleep -Milliseconds 20
    [MultiQaNative]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
}
function Capture([IntPtr]$Window,[string]$Path) {
    $r=New-Object MultiQaNative+RECT; [MultiQaNative]::GetWindowRect($Window,[ref]$r)|Out-Null
    $bmp=New-Object Drawing.Bitmap ($r.Right-$r.Left),($r.Bottom-$r.Top);$g=[Drawing.Graphics]::FromImage($bmp)
    try{$g.CopyFromScreen($r.Left,$r.Top,0,0,$bmp.Size);$bmp.Save($Path,[Drawing.Imaging.ImageFormat]::Png)}finally{$g.Dispose();$bmp.Dispose()}
}
function RegionHash([string]$Path,[Drawing.Rectangle]$Region) {
    $src=[Drawing.Bitmap]::FromFile($Path)
    try{$crop=$src.Clone($Region,$src.PixelFormat);try{$ms=New-Object IO.MemoryStream;try{$crop.Save($ms,[Drawing.Imaging.ImageFormat]::Png);$ms.Position=0;$sha=[Security.Cryptography.SHA256]::Create();try{return([BitConverter]::ToString($sha.ComputeHash($ms))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}}finally{$ms.Dispose()}}finally{$crop.Dispose()}}finally{$src.Dispose()}
}
$out=Join-Path $PSScriptRoot 'all-pages\green';New-Item -ItemType Directory -Force $out|Out-Null
Get-Process neverlose_dx11 -ErrorAction SilentlyContinue|Stop-Process -Force
$release=Resolve-Path(Join-Path $PSScriptRoot '..\examples\example_win32_directx11\Release')
$process=Start-Process -FilePath(Join-Path $release 'neverlose_dx11.exe') -WorkingDirectory $release -PassThru
try{
    $window=[IntPtr]::Zero;for($i=0;$i-lt 80-and$window-eq[IntPtr]::Zero;$i++){Start-Sleep -Milliseconds 50;$process.Refresh();$window=$process.MainWindowHandle}
    if($window-eq[IntPtr]::Zero){throw 'Window not found'}
    [MultiQaNative]::SetForegroundWindow($window)|Out-Null;Start-Sleep -Milliseconds 4300
    $baseline=Join-Path $out 'dropdown-baseline.png';Capture $window $baseline
    Click $window 375 396;Start-Sleep -Milliseconds 150
    $open150=Join-Path $out 'dropdown-open-0150ms.png';Capture $window $open150
    Start-Sleep -Milliseconds 1350
    $open1500=Join-Path $out 'dropdown-open-1500ms.png';Capture $window $open1500
    Click $window 320 315;Start-Sleep -Milliseconds 250
    $toggled=Join-Path $out 'dropdown-toggled.png';Capture $window $toggled
    $popup=New-Object Drawing.Rectangle 295,290,150,215
    $page=New-Object Drawing.Rectangle 158,56,120,100
    $audit=[ordered]@{
        popupVisibleAt1500=((RegionHash $baseline $popup)-ne(RegionHash $open1500 $popup))
        selectionChanged=((RegionHash $open1500 $popup)-ne(RegionHash $toggled $popup))
        pageStayedRage=((RegionHash $open1500 $page)-eq(RegionHash $toggled $page))
        processResponsive=$process.Responding
        popupHashes=[ordered]@{baseline=RegionHash $baseline $popup;open150=RegionHash $open150 $popup;open1500=RegionHash $open1500 $popup;toggled=RegionHash $toggled $popup}
    }
    $audit['pass']=$audit.popupVisibleAt1500-and$audit.selectionChanged-and$audit.pageStayedRage-and$audit.processResponsive
    $audit|ConvertTo-Json -Depth 4|Set-Content -Encoding UTF8(Join-Path $out 'interaction-audit.json');$audit|ConvertTo-Json -Depth 4
}finally{if(-not$process.HasExited){Stop-Process -Id $process.Id -Force}}
