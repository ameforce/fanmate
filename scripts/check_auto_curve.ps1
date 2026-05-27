param(
  [string]$ConfigPath = "include/config.h",
  [string]$MainPath = "src/main.cpp"
)

$ErrorActionPreference = "Stop"

function Get-ConstantNumber {
  param(
    [string]$Text,
    [string]$Name
  )

  $pattern = "constexpr\s+(?:float|uint8_t|int)\s+$Name\s*=\s*([-+]?\d+(?:\.\d+)?)(?:f)?\s*;"
  $match = [regex]::Match($Text, $pattern)
  if (-not $match.Success) {
    throw "Missing constant $Name"
  }
  return [double]$match.Groups[1].Value
}

function Assert-Close {
  param(
    [double]$Actual,
    [double]$Expected,
    [double]$Tolerance,
    [string]$Label
  )

  if ([math]::Abs($Actual - $Expected) -gt $Tolerance) {
    throw "$Label expected $Expected +/- $Tolerance, got $Actual"
  }
}

function Interpolate-Percent {
  param(
    [double]$Temperature,
    [array]$Points
  )

  if ($Temperature -le $Points[0].Temp) {
    return [int]$Points[0].Percent
  }
  for ($i = 1; $i -lt $Points.Count; $i++) {
    $left = $Points[$i - 1]
    $right = $Points[$i]
    if ($Temperature -le $right.Temp) {
      $ratio = ($Temperature - $left.Temp) / ($right.Temp - $left.Temp)
      return [int][math]::Floor(($left.Percent + ($right.Percent - $left.Percent) * $ratio) + 0.5)
    }
  }
  return [int]$Points[$Points.Count - 1].Percent
}

$config = Get-Content -Raw -Path $ConfigPath
$main = Get-Content -Raw -Path $MainPath

Assert-Close (Get-ConstantNumber $config "DHT_TEMP_OFFSET_C") -2.6 0.001 "DHT_TEMP_OFFSET_C"

$points = @(
  [pscustomobject]@{ Temp = Get-ConstantNumber $config "AUTO_TEMP_IDLE_C"; Percent = Get-ConstantNumber $config "AUTO_FAN_IDLE_PERCENT" },
  [pscustomobject]@{ Temp = Get-ConstantNumber $config "AUTO_TEMP_COMFORT_C"; Percent = Get-ConstantNumber $config "AUTO_FAN_COMFORT_PERCENT" },
  [pscustomobject]@{ Temp = Get-ConstantNumber $config "AUTO_TEMP_WARM_C"; Percent = Get-ConstantNumber $config "AUTO_FAN_WARM_PERCENT" },
  [pscustomobject]@{ Temp = Get-ConstantNumber $config "AUTO_TEMP_HOT_C"; Percent = Get-ConstantNumber $config "AUTO_FAN_HOT_PERCENT" }
)

Assert-Close $points[0].Temp 22.0 0.001 "AUTO_TEMP_IDLE_C"
Assert-Close $points[0].Percent 0 0.001 "AUTO_FAN_IDLE_PERCENT"
Assert-Close $points[1].Temp 26.0 0.001 "AUTO_TEMP_COMFORT_C"
Assert-Close $points[1].Percent 13 0.001 "AUTO_FAN_COMFORT_PERCENT"
Assert-Close $points[2].Temp 34.0 0.001 "AUTO_TEMP_WARM_C"
Assert-Close $points[2].Percent 50 0.001 "AUTO_FAN_WARM_PERCENT"
Assert-Close $points[3].Percent 100 0.001 "AUTO_FAN_HOT_PERCENT"

$checks = @(
  [pscustomobject]@{ Temp = 21.9; Expected = 0; Tolerance = 0; Label = "below idle" },
  [pscustomobject]@{ Temp = 22.0; Expected = 0; Tolerance = 0; Label = "idle" },
  [pscustomobject]@{ Temp = 26.0; Expected = 13; Tolerance = 1; Label = "comfort" },
  [pscustomobject]@{ Temp = 34.0; Expected = 50; Tolerance = 2; Label = "warm" },
  [pscustomobject]@{ Temp = $points[3].Temp; Expected = 100; Tolerance = 0; Label = "hot endpoint" }
)

foreach ($check in $checks) {
  $actual = Interpolate-Percent $check.Temp $points
  Assert-Close $actual $check.Expected $check.Tolerance $check.Label
}

if ($main -notmatch "AUTO_TEMP_COMFORT_C" -or $main -notmatch "AUTO_FAN_COMFORT_PERCENT") {
  throw "fanPercentFromTemperature must use named AUTO curve points"
}

Write-Output "AUTO curve check passed"
