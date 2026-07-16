$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Win32Capture {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr FindWindow(string className, string windowName);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr window, out RECT rect);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr window, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr key, IntPtr data);
}
"@

function Save-WindowCapture([IntPtr] $Window, [string] $Path) {
    $rect = New-Object Win32Capture+RECT
    if (-not [Win32Capture]::GetWindowRect($Window, [ref] $rect)) {
        throw 'GetWindowRect failed.'
    }
    $bitmap = New-Object System.Drawing.Bitmap ($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $hdc = $graphics.GetHdc()
        try {
            if (-not [Win32Capture]::PrintWindow($Window, $hdc, 2)) {
                throw 'PrintWindow failed.'
            }
        }
        finally {
            $graphics.ReleaseHdc($hdc)
        }
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Send-Mouse([IntPtr] $Window, [int] $Message, [int] $X, [int] $Y, [int] $Buttons = 0) {
    $position = [IntPtr](($Y -shl 16) -bor ($X -band 0xFFFF))
    [Win32Capture]::PostMessage($Window, [uint32]$Message, [IntPtr]$Buttons, $position) | Out-Null
}

function Click-Client([IntPtr] $Window, [int] $X, [int] $Y) {
    Send-Mouse $Window 0x0200 $X $Y
    Send-Mouse $Window 0x0201 $X $Y 1
    Start-Sleep -Milliseconds 20
    Send-Mouse $Window 0x0202 $X $Y
}

Get-Process neverlose_dx11 -ErrorAction SilentlyContinue | Stop-Process -Force
$evidence = Join-Path $PSScriptRoot 'hexsync'
New-Item -ItemType Directory -Force -Path $evidence | Out-Null
Get-ChildItem $evidence -Filter '*.png' -ErrorAction SilentlyContinue | Remove-Item -Force
$release = Resolve-Path (Join-Path $PSScriptRoot '..\examples\example_win32_directx11\Release')
$process = Start-Process -FilePath (Join-Path $release 'neverlose_dx11.exe') -WorkingDirectory $release -PassThru

$window = [IntPtr]::Zero
for ($attempt = 0; $attempt -lt 60 -and $window -eq [IntPtr]::Zero; $attempt++) {
    Start-Sleep -Milliseconds 50
    $process.Refresh()
    $window = $process.MainWindowHandle
}
if ($window -eq [IntPtr]::Zero) {
    throw 'DX11 window not found.'
}
[Win32Capture]::SetForegroundWindow($window) | Out-Null

$clock = [Diagnostics.Stopwatch]::StartNew()
$captures = @(
    @{ Milliseconds = 350; Name = '01-start-0350ms.png' },
    @{ Milliseconds = 1050; Name = '02-ring-1050ms.png' },
    @{ Milliseconds = 1800; Name = '03-lock-1800ms.png' },
    @{ Milliseconds = 2700; Name = '04-wordmark-2700ms.png' },
    @{ Milliseconds = 3500; Name = '05-menu-opening-0100ms.png' },
    @{ Milliseconds = 3660; Name = '06-menu-opening-0260ms.png' },
    @{ Milliseconds = 4050; Name = '07-menu-settled.png' }
)
foreach ($capture in $captures) {
    $wait = $capture.Milliseconds - $clock.ElapsedMilliseconds
    if ($wait -gt 0) { Start-Sleep -Milliseconds $wait }
    Save-WindowCapture $window (Join-Path $evidence $capture.Name)
}

[Win32Capture]::PostMessage($window, 0x0100, [IntPtr] 0x74, [IntPtr] 0) | Out-Null
[Win32Capture]::PostMessage($window, 0x0101, [IntPtr] 0x74, [IntPtr] 0) | Out-Null
Start-Sleep -Milliseconds 850
Save-WindowCapture $window (Join-Path $evidence '08-f5-replay-0850ms.png')

[Win32Capture]::PostMessage($window, 0x0100, [IntPtr] 0x1B, [IntPtr] 0) | Out-Null
[Win32Capture]::PostMessage($window, 0x0101, [IntPtr] 0x1B, [IntPtr] 0) | Out-Null
Start-Sleep -Milliseconds 120
Save-WindowCapture $window (Join-Path $evidence '09-escape-menu-opening.png')
Start-Sleep -Milliseconds 500
Save-WindowCapture $window (Join-Path $evidence '10-escape-menu-settled.png')

Click-Client $window 370 250
Start-Sleep -Milliseconds 35
Save-WindowCapture $window (Join-Path $evidence '11-dropdown-opening.png')
Start-Sleep -Milliseconds 240
Save-WindowCapture $window (Join-Path $evidence '12-dropdown-open.png')
Click-Client $window 370 282
Start-Sleep -Milliseconds 35
Save-WindowCapture $window (Join-Path $evidence '13-dropdown-closing.png')
Start-Sleep -Milliseconds 240
Save-WindowCapture $window (Join-Path $evidence '14-dropdown-selected.png')

Send-Mouse $window 0x0200 420 104
Start-Sleep -Milliseconds 180
Save-WindowCapture $window (Join-Path $evidence '15-toggle-hover.png')
Click-Client $window 420 104
Start-Sleep -Milliseconds 35
Save-WindowCapture $window (Join-Path $evidence '16-toggle-transition.png')
Start-Sleep -Milliseconds 220
Save-WindowCapture $window (Join-Path $evidence '17-toggle-settled.png')

Send-Mouse $window 0x0200 374 289
Send-Mouse $window 0x0201 374 289 1
foreach ($x in 365, 355, 345, 335) {
    Send-Mouse $window 0x0200 $x 289 1
    Start-Sleep -Milliseconds 18
}
Send-Mouse $window 0x0202 335 289
Start-Sleep -Milliseconds 35
Save-WindowCapture $window (Join-Path $evidence '18-slider-chasing.png')
Start-Sleep -Milliseconds 300
Save-WindowCapture $window (Join-Path $evidence '19-slider-settled.png')

Click-Client $window 70 550
Start-Sleep -Milliseconds 45
Save-WindowCapture $window (Join-Path $evidence '20-popover-closing.png')
Start-Sleep -Milliseconds 300
Save-WindowCapture $window (Join-Path $evidence '21-popover-closed.png')
Click-Client $window 70 550
Start-Sleep -Milliseconds 45
Save-WindowCapture $window (Join-Path $evidence '22-popover-opening.png')
Start-Sleep -Milliseconds 300
Save-WindowCapture $window (Join-Path $evidence '23-popover-settled.png')

Send-Mouse $window 0x0200 60 205
Start-Sleep -Milliseconds 160
Save-WindowCapture $window (Join-Path $evidence '24-navigation-hover.png')
Click-Client $window 60 205
Start-Sleep -Milliseconds 35
Save-WindowCapture $window (Join-Path $evidence '25-navigation-transition.png')
Start-Sleep -Milliseconds 220
Save-WindowCapture $window (Join-Path $evidence '26-navigation-selected.png')
Send-Mouse $window 0x0200 370 250
Start-Sleep -Milliseconds 160
Save-WindowCapture $window (Join-Path $evidence '27-select-hover.png')

$process.Refresh()
[pscustomobject]@{ Pid = $process.Id; Exited = $process.HasExited; Window = $window; Evidence = $evidence }
Get-ChildItem $evidence -File | Select-Object Name, Length, LastWriteTime
