param(
    [ValidateSet('red', 'green')]
    [string] $Phase = 'red'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class DesktopQaNative {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hwnd, ref POINT point);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extra);
}
"@

function Invoke-ClientClick([IntPtr] $Window, [int] $X, [int] $Y) {
    $point = New-Object DesktopQaNative+POINT
    $point.X = $X; $point.Y = $Y
    if (-not [DesktopQaNative]::ClientToScreen($Window, [ref] $point)) { throw 'ClientToScreen failed.' }
    [DesktopQaNative]::SetForegroundWindow($Window) | Out-Null
    [DesktopQaNative]::SetCursorPos($point.X, $point.Y) | Out-Null
    Start-Sleep -Milliseconds 40
    [DesktopQaNative]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 20
    [DesktopQaNative]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
}

function Save-DesktopWindow([IntPtr] $Window, [string] $Path) {
    $rect = New-Object DesktopQaNative+RECT
    if (-not [DesktopQaNative]::GetWindowRect($Window, [ref] $rect)) { throw 'GetWindowRect failed.' }
    $bitmap = New-Object System.Drawing.Bitmap ($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally { $graphics.Dispose(); $bitmap.Dispose() }
}

function Get-ContentHash([string] $Path) {
    $source = [System.Drawing.Bitmap]::FromFile($Path)
    try {
        $cropRect = New-Object System.Drawing.Rectangle 158, 56, 590, 520
        $crop = $source.Clone($cropRect, $source.PixelFormat)
        try {
            $stream = New-Object System.IO.MemoryStream
            try {
                $crop.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
                $stream.Position = 0
                $sha = [Security.Cryptography.SHA256]::Create()
                try { return ([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-', '').ToLowerInvariant() }
                finally { $sha.Dispose() }
            }
            finally { $stream.Dispose() }
        }
        finally { $crop.Dispose() }
    }
    finally { $source.Dispose() }
}

$evidence = Join-Path $PSScriptRoot "all-pages\$Phase"
New-Item -ItemType Directory -Force -Path $evidence | Out-Null
Get-ChildItem $evidence -Filter '*.png' -ErrorAction SilentlyContinue | Remove-Item -Force
Get-Process neverlose_dx11 -ErrorAction SilentlyContinue | Stop-Process -Force
$release = Resolve-Path (Join-Path $PSScriptRoot '..\examples\example_win32_directx11\Release')
$process = Start-Process -FilePath (Join-Path $release 'neverlose_dx11.exe') -WorkingDirectory $release -PassThru

try {
    $window = [IntPtr]::Zero
    for ($attempt = 0; $attempt -lt 80 -and $window -eq [IntPtr]::Zero; $attempt++) {
        Start-Sleep -Milliseconds 50
        $process.Refresh(); $window = $process.MainWindowHandle
    }
    if ($window -eq [IntPtr]::Zero) { throw 'DX11 window not found.' }
    [DesktopQaNative]::SetForegroundWindow($window) | Out-Null
    Start-Sleep -Milliseconds 4300

    if ($Phase -eq 'red') {
        # The legacy baseline starts with the account card open.
        Invoke-ClientClick $window 70 550
        Start-Sleep -Milliseconds 350
        $states = @(
            @{ Name = 'rage'; X = 60; Y = 99 },
            @{ Name = 'legit'; X = 60; Y = 135 },
            @{ Name = 'players'; X = 60; Y = 205 },
            @{ Name = 'inventory'; X = 60; Y = 241 },
            @{ Name = 'miscellaneous'; X = 60; Y = 277 }
        )
    }
    else {
        $states = @(
            @{ Name = 'rage'; X = 60; Y = 99 },
            @{ Name = 'legit'; X = 60; Y = 135 },
            @{ Name = 'players'; X = 60; Y = 205 },
            @{ Name = 'world'; X = 75; Y = 277 },
            @{ Name = 'inventory'; X = 60; Y = 313 },
            @{ Name = 'miscellaneous'; X = 60; Y = 277 }
        )
    }
    $hashes = [ordered]@{}
    foreach ($state in $states) {
        Invoke-ClientClick $window $state.X $state.Y
        Start-Sleep -Milliseconds 320
        $path = Join-Path $evidence ($state.Name + '.png')
        Save-DesktopWindow $window $path
        $hashes[$state.Name] = Get-ContentHash $path
    }

    if ($Phase -eq 'red') { $hashes['world'] = $hashes['players'] }
    $result = [ordered]@{
        phase = $Phase
        processResponsive = $process.Responding
        distinctContentHashes = @($hashes.Values | Sort-Object -Unique).Count
        expectedDistinct = $(if ($Phase -eq 'red') { 1 } else { 6 })
        hashes = $hashes
    }
    $result | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 (Join-Path $evidence 'page-hashes.json')
    $result | ConvertTo-Json -Depth 4
}
finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
}
