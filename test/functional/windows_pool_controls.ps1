param(
    [Parameter(Mandatory)][int]$WalletProcess,
    [Parameter(Mandatory)][ValidateSet('open', 'start', 'stop', 'close', 'exit', 'child')]$Action
)
$ErrorActionPreference = 'Stop'
if ($Action -eq 'child') {
    Get-CimInstance Win32_Process -Filter "ParentProcessId = $WalletProcess AND Name = 'cpuminer.exe'" |
        ForEach-Object { "$($_.ProcessId)|$($_.ExecutablePath)|$($_.CommandLine)" }
    exit
}

Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes, System.Windows.Forms
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
    throw "Could not find visible wallet control: $Name"
}

function Invoke-Control($Element) {
    $pattern = $null
    if ($Element.TryGetCurrentPattern([Windows.Automation.InvokePattern]::Pattern, [ref]$pattern)) {
        $pattern.Invoke()
    } elseif ($Element.TryGetCurrentPattern([Windows.Automation.ExpandCollapsePattern]::Pattern, [ref]$pattern)) {
        $pattern.Expand()
    } else {
        $Element.SetFocus()
        [Windows.Forms.SendKeys]::SendWait('{ENTER}')
    }
}

switch ($Action) {
    open {
        $window = Find-Control '*Agrarian*Windows mining QA*' -Window
        $window.SetFocus()
        Invoke-Control (Find-Control 'Mining')
        Invoke-Control (Find-Control 'Mine on All The Acronyms*')
        $null = Find-Control 'Start pool mining'
    }
    start { Invoke-Control (Find-Control 'Start pool mining') }
    stop { Invoke-Control (Find-Control 'Stop mining') }
    close {
        $dialog = Find-Control 'All The Acronyms*Pool mining' -Window
        $dialog.GetCurrentPattern([Windows.Automation.WindowPattern]::Pattern).Close()
    }
    exit {
        $window = Find-Control '*Agrarian*Windows mining QA*' -Window
        $window.GetCurrentPattern([Windows.Automation.WindowPattern]::Pattern).Close()
    }
}
