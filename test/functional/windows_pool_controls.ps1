param(
    [Parameter(Mandatory)][int]$WalletProcess,
    [Parameter(Mandatory)][ValidateSet('open', 'start', 'stop', 'close', 'child')]$Action
)
$ErrorActionPreference = 'Stop'
if ($Action -eq 'child') {
    Get-CimInstance Win32_Process -Filter "ParentProcessId = $WalletProcess AND Name = 'cpuminer.exe'" |
        ForEach-Object { "$($_.ProcessId)|$($_.ExecutablePath)|$($_.CommandLine)" }
    exit
}

Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes, System.Windows.Forms
Add-Type @'
using System.Runtime.InteropServices;
public static class WalletInput {
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint x, uint y, uint data, System.UIntPtr extra);
    [DllImport("user32.dll")] public static extern System.IntPtr WindowFromPoint(Point point);
    [DllImport("user32.dll")] public static extern System.IntPtr GetAncestor(System.IntPtr window, uint flag);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowText(System.IntPtr window, System.Text.StringBuilder title, int capacity);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(System.IntPtr window, out uint process);
    [DllImport("user32.dll")] public static extern bool PostMessage(System.IntPtr window, uint message, System.IntPtr wparam, System.IntPtr lparam);
    [StructLayout(LayoutKind.Sequential)]
    public struct Point { public int X, Y; }
}
'@
$ae = [Windows.Automation.AutomationElement]
$root = $ae::RootElement
$byPid = [Windows.Automation.PropertyCondition]::new($ae::ProcessIdProperty, [object]$WalletProcess)
$children = [Windows.Automation.TreeScope]::Children
$scope = [Windows.Automation.TreeScope]::Descendants

function Find-Control([string]$Name, [switch]$Window) {
    for ($i = 0; $i -lt 35; $i++) {
        foreach ($surface in $root.FindAll($children, $byPid)) {
            if ($Window -and $surface.Current.Name -like $Name) { return $surface }
            if ($Window) { continue }
            foreach ($item in $surface.FindAll($scope, [Windows.Automation.Condition]::TrueCondition)) {
                if (($item.Current.Name -replace '&', '') -like $Name -and !$item.Current.IsOffscreen) { return $item }
            }
        }
        Start-Sleep -Milliseconds 200
    }
    $labels = foreach ($surface in $root.FindAll($children, $byPid)) {
        "Window: " + $surface.Current.Name
        foreach ($item in $surface.FindAll($scope, [Windows.Automation.Condition]::TrueCondition)) {
            if ($item.Current.Name -and !$item.Current.IsOffscreen) { $item.Current.Name }
        }
    }
    throw "Could not find visible wallet control: $Name. UI labels: $($labels -join ' | ')"
}

function Click-Control($Element) {
    $bounds = $Element.Current.BoundingRectangle
    if ($bounds.IsEmpty -or $bounds.Width -le 0 -or $bounds.Height -le 0) {
        throw "No visible bounds for $($Element.Current.Name)"
    }
    if (![WalletInput]::SetCursorPos([int]($bounds.X + $bounds.Width / 2), [int]($bounds.Y + $bounds.Height / 2))) {
        throw 'Could not move pointer on the Windows runner'
    }
    [WalletInput]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)
    [WalletInput]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
}

switch ($Action) {
    open {
        $window = Find-Control '*Agrarian*Windows mining QA*' -Window
        $window.SetFocus()
        Click-Control (Find-Control 'Mining')
        Click-Control (Find-Control 'Mine on All The Acronyms*')
        $null = Find-Control 'Start pool mining'
    }
    start { Click-Control (Find-Control 'Start pool mining') }
    stop { Click-Control (Find-Control 'Stop mining') }
    close {
        # UI Automation exposes this Qt dialog underneath its owner window.
        # Resolve and verify its native window before issuing the close event.
        $button = Find-Control 'Stop mining'
        $bounds = $button.Current.BoundingRectangle
        $point = New-Object WalletInput+Point
        $point.X = [int]($bounds.X + $bounds.Width / 2)
        $point.Y = [int]($bounds.Y + $bounds.Height / 2)
        $handle = [WalletInput]::GetAncestor([WalletInput]::WindowFromPoint($point), 2)
        $title = New-Object System.Text.StringBuilder 256
        [uint32]$owner = 0
        $null = [WalletInput]::GetWindowThreadProcessId($handle, [ref]$owner)
        $null = [WalletInput]::GetWindowText($handle, $title, $title.Capacity)
        if ($owner -ne $WalletProcess -or $title.ToString() -notlike 'All The Acronyms*Pool mining') {
            throw "Refusing to close unexpected window: $title"
        }
        if (![WalletInput]::PostMessage($handle, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)) {
            throw 'Could not close the verified pool dialog'
        }
    }
}
