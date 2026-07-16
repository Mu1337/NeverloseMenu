$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class AuditInput {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr key, IntPtr data);
}
"@

function Get-RegionDifference([string] $First, [string] $Second) {
    $a = [System.Drawing.Bitmap]::FromFile($First)
    $b = [System.Drawing.Bitmap]::FromFile($Second)
    try {
        $different = 0
        $total = 0
        for ($y = 650; $y -lt 750; $y++) {
            for ($x = 650; $x -lt 750; $x++) {
                $total++
                if ($a.GetPixel($x, $y).ToArgb() -ne $b.GetPixel($x, $y).ToArgb()) { $different++ }
            }
        }
        return [pscustomobject]@{ DifferentPixels = $different; TotalPixels = $total }
    }
    finally {
        $a.Dispose()
        $b.Dispose()
    }
}

$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$release = Join-Path $root 'examples\example_win32_directx11\Release'
$process = Get-Process neverlose_dx11 -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $process) {
    $process = Start-Process -FilePath (Join-Path $release 'neverlose_dx11.exe') -WorkingDirectory $release -PassThru
    Start-Sleep -Milliseconds 700
}
$process.Refresh()
$window = $process.MainWindowHandle
if ($window -eq [IntPtr]::Zero) { throw 'Main window unavailable.' }
[AuditInput]::SetForegroundWindow($window) | Out-Null

$sourceFiles = @(
    (Join-Path $root 'examples\example_win32_directx11\hexsync_intro.cpp'),
    (Join-Path $root 'examples\example_win32_directx11\neverlose_menu.cpp'),
    (Join-Path $root 'examples\example_win32_directx11\main.cpp')
)
$executable = Get-Item (Join-Path $release 'neverlose_dx11.exe')
$newestSource = $sourceFiles | Get-Item | Sort-Object LastWriteTime -Descending | Select-Object -First 1
$logoHash = (Get-FileHash (Join-Path $root 'examples\example_win32_directx11\hexsync-logo.png') -Algorithm SHA256).Hash
$releaseLogoHash = (Get-FileHash (Join-Path $release 'hexsync-logo.png') -Algorithm SHA256).Hash
$pixelDifference = Get-RegionDifference (Join-Path $PSScriptRoot 'hexsync\02-ring-1050ms.png') (Join-Path $PSScriptRoot 'hexsync\07-menu-settled.png')

$process.Refresh()
$before = [pscustomobject]@{ Handles = $process.HandleCount; PrivateBytes = $process.PrivateMemorySize64; Responding = $process.Responding }
for ($cycle = 0; $cycle -lt 30; $cycle++) {
    [AuditInput]::PostMessage($window, 0x0100, [IntPtr] 0x74, [IntPtr] 0) | Out-Null
    [AuditInput]::PostMessage($window, 0x0101, [IntPtr] 0x74, [IntPtr] 0) | Out-Null
    Start-Sleep -Milliseconds 35
    [AuditInput]::PostMessage($window, 0x0100, [IntPtr] 0x1B, [IntPtr] 0) | Out-Null
    [AuditInput]::PostMessage($window, 0x0101, [IntPtr] 0x1B, [IntPtr] 0) | Out-Null
    Start-Sleep -Milliseconds 35
}
Start-Sleep -Milliseconds 650
$process.Refresh()
$after = [pscustomobject]@{ Handles = $process.HandleCount; PrivateBytes = $process.PrivateMemorySize64; Responding = $process.Responding; Exited = $process.HasExited }

$result = [ordered]@{
    Executable = $executable.FullName
    ExecutableNewerThanSource = $executable.LastWriteTimeUtc -ge $newestSource.LastWriteTimeUtc
    LogoAssetHashesMatch = $logoHash -eq $releaseLogoHash
    IntroVsSettledBackgroundRegion = $pixelDifference
    ReplaySkipCycles = 30
    Before = $before
    After = $after
    HandleDelta = $after.Handles - $before.Handles
    PrivateBytesDelta = $after.PrivateBytes - $before.PrivateBytes
}
$result | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $PSScriptRoot 'hexsync\runtime-audit.json') -Encoding UTF8
$result | ConvertTo-Json -Depth 5
