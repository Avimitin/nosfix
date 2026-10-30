param(
    [string]$Path = '',
    [switch]$ListDrivers,
    [switch]$SelfTest
)

$ErrorActionPreference = 'Stop'

function Get-AsioDrivers {
    $result = @()
    foreach ($root in @('HKLM:\SOFTWARE\ASIO', 'HKLM:\SOFTWARE\WOW6432Node\ASIO')) {
        if (-not (Test-Path -LiteralPath $root)) { continue }
        foreach ($key in @(Get-ChildItem -LiteralPath $root -ErrorAction SilentlyContinue)) {
            $props = Get-ItemProperty -LiteralPath $key.PSPath -ErrorAction SilentlyContinue
            if (-not $props) { continue }
            $clsid = [string]$props.CLSID
            if ([string]::IsNullOrWhiteSpace($clsid)) { continue }
            $result += [pscustomobject]@{
                Name        = $key.PSChildName
                Description = [string]$props.Description
                Clsid       = $clsid
                Registry    = $root
            }
        }
    }
    return @($result | Sort-Object Name, Clsid -Unique)
}

if ($ListDrivers) {
    Get-AsioDrivers | Format-Table Name, Description, Clsid -AutoSize
    return
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

$script:Lines = [System.Collections.Generic.List[string]]::new()

function Read-IniFile {
    param([string]$FilePath)
    if (-not (Test-Path -LiteralPath $FilePath)) {
        throw "ini not found: $FilePath"
    }
    $script:IniPath = (Resolve-Path -LiteralPath $FilePath).Path
    $script:Lines.Clear()
    foreach ($line in [IO.File]::ReadAllLines($script:IniPath)) {
        $script:Lines.Add($line)
    }
}

function Get-IniValue {
    param([string]$Key)
    $pattern = '^\s*' + [regex]::Escape($Key) + '\s*=\s*(.*)$'
    for ($i = $script:Lines.Count - 1; $i -ge 0; --$i) {
        $m = [regex]::Match($script:Lines[$i], $pattern)
        if ($m.Success) { return $m.Groups[1].Value.Trim() }
    }
    return ''
}

function Set-IniValue {
    param([string]$Key, [string]$Value)
    $pattern = '^(\s*' + [regex]::Escape($Key) + '\s*=\s*).*$'
    for ($i = 0; $i -lt $script:Lines.Count; ++$i) {
        $m = [regex]::Match($script:Lines[$i], $pattern)
        if ($m.Success) {
            $script:Lines[$i] = $m.Groups[1].Value + $Value
            return
        }
    }
    $script:Lines.Add("$Key=$Value")
}

function Convert-Bool {
    param([string]$Value)
    return $Value -in @('1', 'true', 'yes', 'on')
}

$script:Drivers = @(Get-AsioDrivers)
$script:SelectedDriver = $null

if ([string]::IsNullOrWhiteSpace($Path)) {
    $candidates = @(
        (Join-Path $PSScriptRoot 'nosfix.ini'),
        (Join-Path (Split-Path -Parent $PSScriptRoot) 'nosfix.ini')
    )
    $Path = $candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}
if (-not $Path) {
    $Path = Join-Path $PSScriptRoot 'nosfix.ini'
}
Read-IniFile $Path

$form = New-Object System.Windows.Forms.Form
$form.Text = 'nosfix configuration'
$form.StartPosition = 'CenterScreen'
$form.ClientSize = New-Object System.Drawing.Size(760, 700)
$form.FormBorderStyle = 'FixedDialog'
$form.MaximizeBox = $false
$form.MinimizeBox = $false
$form.Font = New-Object System.Drawing.Font('Segoe UI', 9)

function New-CheckBox {
    param($Parent, [string]$Text, [int]$X, [int]$Y)
    $box = New-Object System.Windows.Forms.CheckBox
    $box.Text = $Text
    $box.Location = New-Object System.Drawing.Point($X, $Y)
    $box.AutoSize = $true
    $Parent.Controls.Add($box)
    return $box
}

$timing = New-Object System.Windows.Forms.GroupBox
$timing.Text = 'Runtime and timing'
$timing.Location = New-Object System.Drawing.Point(12, 8)
$timing.Size = New-Object System.Drawing.Size(676, 150)
$form.Controls.Add($timing)

$enabledBox = New-CheckBox $timing 'enabled' 12 22
$preciseBox = New-CheckBox $timing 'precise_audio_clock' 180 22
$skipBox = New-CheckBox $timing 'skip_monitor_check' 390 22
$timelineLabel = New-Object System.Windows.Forms.Label
$timelineLabel.Text = 'microsecond timeline: always on; no host prediction'
$timelineLabel.Location = New-Object System.Drawing.Point(12, 52)
$timelineLabel.AutoSize = $true
$timing.Controls.Add($timelineLabel)
$captureBox = New-CheckBox $timing 'capture_input' 12 78
$nativeShadowBox = New-CheckBox $timing 'native_shadow' 180 78
$probeBox = New-CheckBox $timing 'diagnostic_probe' 12 106

$hzLabel = New-Object System.Windows.Forms.Label
$hzLabel.Text = 'target_hz'
$hzLabel.Location = New-Object System.Drawing.Point(390, 108)
$hzLabel.AutoSize = $true
$timing.Controls.Add($hzLabel)
$hzCombo = New-Object System.Windows.Forms.ComboBox
$hzCombo.DropDownStyle = 'DropDownList'
$hzCombo.Location = New-Object System.Drawing.Point(470, 104)
$hzCombo.Size = New-Object System.Drawing.Size(100, 24)
foreach ($hz in @('auto', '60', '90', '120', '144', '165', '240', '360')) {
    [void]$hzCombo.Items.Add($hz)
}
$timing.Controls.Add($hzCombo)

$asioGroup = New-Object System.Windows.Forms.GroupBox
$asioGroup.Text = 'Native ASIO output (experimental)'
$asioGroup.Location = New-Object System.Drawing.Point(12, 166)
$asioGroup.Size = New-Object System.Drawing.Size(676, 205)
$form.Controls.Add($asioGroup)

$asioBox = New-CheckBox $asioGroup 'asio_output' 12 24
$refreshButton = New-Object System.Windows.Forms.Button
$refreshButton.Text = 'Refresh drivers'
$refreshButton.Location = New-Object System.Drawing.Point(540, 20)
$refreshButton.Size = New-Object System.Drawing.Size(120, 28)
$asioGroup.Controls.Add($refreshButton)

$driverLabel = New-Object System.Windows.Forms.Label
$driverLabel.Text = 'ASIO driver'
$driverLabel.Location = New-Object System.Drawing.Point(12, 58)
$driverLabel.AutoSize = $true
$asioGroup.Controls.Add($driverLabel)
$driverCombo = New-Object System.Windows.Forms.ComboBox
$driverCombo.DropDownStyle = 'DropDownList'
$driverCombo.Location = New-Object System.Drawing.Point(100, 54)
$driverCombo.Size = New-Object System.Drawing.Size(460, 24)
$asioGroup.Controls.Add($driverCombo)

$clsidLabel = New-Object System.Windows.Forms.Label
$clsidLabel.Text = 'CLSID: (none)'
$clsidLabel.Location = New-Object System.Drawing.Point(100, 84)
$clsidLabel.Size = New-Object System.Drawing.Size(460, 20)
$clsidLabel.AutoEllipsis = $true
$asioGroup.Controls.Add($clsidLabel)

$note = New-Object System.Windows.Forms.Label
$note.Text = 'The selected driver is saved by CLSID; the name is left empty to avoid a cross-check mismatch.'
$note.Location = New-Object System.Drawing.Point(12, 106)
$note.Size = New-Object System.Drawing.Size(650, 20)
$note.ForeColor = [System.Drawing.Color]::DimGray
$asioGroup.Controls.Add($note)

$chLabel = New-Object System.Windows.Forms.Label
$chLabel.Text = 'Output channels'
$chLabel.Location = New-Object System.Drawing.Point(12, 136)
$chLabel.AutoSize = $true
$asioGroup.Controls.Add($chLabel)
$channelsText = New-Object System.Windows.Forms.TextBox
$channelsText.Location = New-Object System.Drawing.Point(120, 132)
$channelsText.Size = New-Object System.Drawing.Size(120, 24)
$asioGroup.Controls.Add($channelsText)

$rateLabel = New-Object System.Windows.Forms.Label
$rateLabel.Text = 'Sample rate'
$rateLabel.Location = New-Object System.Drawing.Point(270, 136)
$rateLabel.AutoSize = $true
$asioGroup.Controls.Add($rateLabel)
$rateText = New-Object System.Windows.Forms.TextBox
$rateText.Location = New-Object System.Drawing.Point(350, 132)
$rateText.Size = New-Object System.Drawing.Size(80, 24)
$asioGroup.Controls.Add($rateText)

$bufferLabel = New-Object System.Windows.Forms.Label
$bufferLabel.Text = 'Buffer frames'
$bufferLabel.Location = New-Object System.Drawing.Point(455, 136)
$bufferLabel.AutoSize = $true
$asioGroup.Controls.Add($bufferLabel)
$bufferText = New-Object System.Windows.Forms.TextBox
$bufferText.Location = New-Object System.Drawing.Point(545, 132)
$bufferText.Size = New-Object System.Drawing.Size(80, 24)
$asioGroup.Controls.Add($bufferText)

$pathLabel = New-Object System.Windows.Forms.Label
$pathLabel.Text = 'File'
$pathLabel.Location = New-Object System.Drawing.Point(12, 388)
$pathLabel.AutoSize = $true
$form.Controls.Add($pathLabel)
$pathText = New-Object System.Windows.Forms.TextBox
$pathText.Location = New-Object System.Drawing.Point(55, 384)
$pathText.Size = New-Object System.Drawing.Size(565, 24)
$pathText.ReadOnly = $true
$form.Controls.Add($pathText)
$browseButton = New-Object System.Windows.Forms.Button
$browseButton.Text = 'Browse...'
$browseButton.Location = New-Object System.Drawing.Point(630, 382)
$browseButton.Size = New-Object System.Drawing.Size(90, 28)
$form.Controls.Add($browseButton)

$saveButton = New-Object System.Windows.Forms.Button
$saveButton.Text = 'Save'
$saveButton.Location = New-Object System.Drawing.Point(480, 640)
$saveButton.Size = New-Object System.Drawing.Size(90, 32)
$form.Controls.Add($saveButton)
$reloadButton = New-Object System.Windows.Forms.Button
$reloadButton.Text = 'Reload'
$reloadButton.Location = New-Object System.Drawing.Point(580, 640)
$reloadButton.Size = New-Object System.Drawing.Size(90, 32)
$form.Controls.Add($reloadButton)
$closeButton = New-Object System.Windows.Forms.Button
$closeButton.Text = 'Close'
$closeButton.Location = New-Object System.Drawing.Point(680, 640)
$closeButton.Size = New-Object System.Drawing.Size(70, 32)
$form.Controls.Add($closeButton)

$status = New-Object System.Windows.Forms.Label
$status.Location = New-Object System.Drawing.Point(12, 650)
$status.Size = New-Object System.Drawing.Size(450, 20)
$status.Text = ''
$form.Controls.Add($status)

function Refresh-DriverCombo {
    param([string]$PreferredClsid = '')
    $driverCombo.Items.Clear()
    $script:Drivers = @(Get-AsioDrivers)
    $selectedIndex = -1
    for ($i = 0; $i -lt $script:Drivers.Count; ++$i) {
        $d = $script:Drivers[$i]
        $label = $d.Name
        if ($d.Description -and $d.Description -ne $d.Name) {
            $label += ' - ' + $d.Description
        }
        [void]$driverCombo.Items.Add($label)
        if ($PreferredClsid -and $d.Clsid -ieq $PreferredClsid) {
            $selectedIndex = $i
        }
    }
    if ($selectedIndex -ge 0) {
        $driverCombo.SelectedIndex = $selectedIndex
    } elseif ($driverCombo.Items.Count -gt 0) {
        $driverCombo.SelectedIndex = 0
    }
    if ($driverCombo.Items.Count -eq 0) {
        $script:SelectedDriver = $null
        $clsidLabel.Text = 'CLSID: (no ASIO drivers found)'
    }
}

function Update-SelectedDriver {
    $index = $driverCombo.SelectedIndex
    if ($index -lt 0 -or $index -ge $script:Drivers.Count) {
        $script:SelectedDriver = $null
        $clsidLabel.Text = 'CLSID: (none)'
        return
    }
    $script:SelectedDriver = $script:Drivers[$index]
    $clsidLabel.Text = 'CLSID: ' + $script:SelectedDriver.Clsid
}

function Load-UiFromIni {
    $enabledBox.Checked = Convert-Bool (Get-IniValue 'enabled')
    $preciseBox.Checked = Convert-Bool (Get-IniValue 'precise_audio_clock')
    $skipBox.Checked = Convert-Bool (Get-IniValue 'skip_monitor_check')
    $captureBox.Checked = Convert-Bool (Get-IniValue 'capture_input')
    $nativeShadowBox.Checked = Convert-Bool (Get-IniValue 'native_shadow')
    $probeBox.Checked = Convert-Bool (Get-IniValue 'diagnostic_probe')
    $asioBox.Checked = Convert-Bool (Get-IniValue 'asio_output')

    $hz = Get-IniValue 'target_hz'
    if ([string]::IsNullOrWhiteSpace($hz)) { $hz = 'auto' }
    if ($hzCombo.Items.Contains($hz)) { $hzCombo.SelectedItem = $hz }
    else { $hzCombo.SelectedIndex = 0 }

    $channelsText.Text = Get-IniValue 'asio_output_channels'
    if ([string]::IsNullOrWhiteSpace($channelsText.Text)) { $channelsText.Text = '0,1' }
    $rateText.Text = Get-IniValue 'asio_sample_rate'
    if ([string]::IsNullOrWhiteSpace($rateText.Text)) { $rateText.Text = '44100' }
    $bufferText.Text = Get-IniValue 'asio_buffer_frames'
    if ([string]::IsNullOrWhiteSpace($bufferText.Text)) { $bufferText.Text = '128' }
    $pathText.Text = $script:IniPath
    Refresh-DriverCombo (Get-IniValue 'asio_driver_clsid')
    Update-SelectedDriver
}

$closeButton.Add_Click({ $form.Close() })

$refreshButton.Add_Click({
    Refresh-DriverCombo
    Update-SelectedDriver
})
$driverCombo.Add_SelectedIndexChanged({ Update-SelectedDriver })

$browseButton.Add_Click({
    $dialog = New-Object System.Windows.Forms.OpenFileDialog
    $dialog.Filter = 'nosfix.ini|nosfix.ini|INI files (*.ini)|*.ini|All files (*.*)|*.*'
    $dialog.FileName = 'nosfix.ini'
    if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
        try {
            Read-IniFile $dialog.FileName
            Load-UiFromIni
            $status.Text = 'Loaded ' + $script:IniPath
        } catch {
            [void][System.Windows.Forms.MessageBox]::Show($_.Exception.Message,
                'nosfix configuration', 'OK', 'Error')
        }
    }
})

$reloadButton.Add_Click({
    try {
        Read-IniFile $script:IniPath
        Load-UiFromIni
        $status.Text = 'Reloaded'
    } catch {
        [void][System.Windows.Forms.MessageBox]::Show($_.Exception.Message,
            'nosfix configuration', 'OK', 'Error')
    }
})

$saveButton.Add_Click({
    try {
        if ($asioBox.Checked) {
            $channels = $channelsText.Text.Trim()
            if ($channels -notmatch '^\d+(,\d+)*$') {
                throw 'ASIO output channels must be comma-separated numbers, e.g. 0,1'
            }
            $rate = 0
            if (-not [int]::TryParse($rateText.Text.Trim(), [ref]$rate) -or
                $rate -lt 8000 -or $rate -gt 384000) {
                throw 'ASIO sample rate must be 8000..384000'
            }
            $buffer = 0
            if (-not [int]::TryParse($bufferText.Text.Trim(), [ref]$buffer) -or
                $buffer -lt 16 -or $buffer -gt 8192) {
                throw 'ASIO buffer frames must be 16..8192'
            }
            if (-not $script:SelectedDriver) {
                throw 'Select an ASIO driver before enabling asio_output'
            }
        }

        Set-IniValue 'enabled' $(if ($enabledBox.Checked) { 'true' } else { 'false' })
        Set-IniValue 'precise_audio_clock' $(if ($preciseBox.Checked) { 'true' } else { 'false' })
        Set-IniValue 'skip_monitor_check' $(if ($skipBox.Checked) { 'true' } else { 'false' })
        Set-IniValue 'capture_input' $(if ($captureBox.Checked) { 'true' } else { 'false' })
        Set-IniValue 'native_shadow' $(if ($nativeShadowBox.Checked) { 'true' } else { 'false' })
        Set-IniValue 'diagnostic_probe' $(if ($probeBox.Checked) { 'true' } else { 'false' })
        Set-IniValue 'target_hz' ([string]$hzCombo.SelectedItem)
        Set-IniValue 'asio_output' $(if ($asioBox.Checked) { 'true' } else { 'false' })
        if ($asioBox.Checked) {
            Set-IniValue 'asio_driver_clsid' $script:SelectedDriver.Clsid
            Set-IniValue 'asio_driver_name' ''
            Set-IniValue 'asio_output_channels' $channelsText.Text.Trim()
            Set-IniValue 'asio_sample_rate' $rateText.Text.Trim()
            Set-IniValue 'asio_buffer_frames' $bufferText.Text.Trim()
        }

        $backup = $script:IniPath + '.bak-' + (Get-Date -Format 'yyyyMMdd-HHmmss')
        Copy-Item -LiteralPath $script:IniPath -Destination $backup -Force
        [IO.File]::WriteAllText($script:IniPath, ($script:Lines -join "`r`n") + "`r`n",
            (New-Object System.Text.UTF8Encoding($false)))
        $status.Text = 'Saved; backup: ' + (Split-Path -Leaf $backup)
    } catch {
        [void][System.Windows.Forms.MessageBox]::Show($_.Exception.Message,
            'nosfix configuration', 'OK', 'Error')
    }
})

$form.Add_Shown({
    Load-UiFromIni
    if (-not $asioBox.Checked) {
        $status.Text = 'ASIO is off; enable it and select a driver to configure the native route.'
    } else {
        $status.Text = 'Loaded ' + $script:IniPath
    }
})

if ($SelfTest) {
    Load-UiFromIni
    $form.Dispose()
    Write-Output ('self-test OK: ' + $script:IniPath)
    return
}

[void]$form.ShowDialog()
