param(
  [string]$Compiler = "g++",
  [string]$OutputPath = (Join-Path $env:TEMP "fanmate_fan_button_steps_test.exe")
)

$ErrorActionPreference = "Stop"

$outputDir = Split-Path -Parent $OutputPath
if ($outputDir -and -not (Test-Path $outputDir)) {
  New-Item -ItemType Directory -Path $outputDir | Out-Null
}

& $Compiler -std=c++17 -Wall -Wextra -Werror -Iinclude tests/fan_button_steps_test.cpp -o $OutputPath
if ($LASTEXITCODE -ne 0) {
  throw "fan button step test compile failed"
}

& $OutputPath
if ($LASTEXITCODE -ne 0) {
  throw "fan button step test failed"
}
