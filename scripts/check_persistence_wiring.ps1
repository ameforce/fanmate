param(
  [string]$ConfigPath = "include/config.h",
  [string]$MainPath = "src/main.cpp",
  [string]$ServoHeaderPath = "include/servo_control.h",
  [string]$ServoSourcePath = "src/servo_control.cpp",
  [string]$PersistenceHeaderPath = "include/app_persistence.h",
  [string]$PersistenceSourcePath = "src/app_persistence.cpp"
)

$ErrorActionPreference = "Stop"

function Require-File {
  param([string]$Path)
  if (-not (Test-Path $Path)) {
    throw "Missing file $Path"
  }
}

function Require-Pattern {
  param(
    [string]$Text,
    [string]$Pattern,
    [string]$Label
  )
  if ($Text -notmatch $Pattern) {
    throw "Missing $Label"
  }
}

Require-File $PersistenceHeaderPath
Require-File $PersistenceSourcePath

$config = Get-Content -Raw -Path $ConfigPath
$main = Get-Content -Raw -Path $MainPath
$servoHeader = Get-Content -Raw -Path $ServoHeaderPath
$servoSource = Get-Content -Raw -Path $ServoSourcePath
$persistenceHeader = Get-Content -Raw -Path $PersistenceHeaderPath
$persistenceSource = Get-Content -Raw -Path $PersistenceSourcePath

Require-Pattern $config "PERSIST_MANUAL_DEBOUNCE_MS\s*=\s*2000" "manual persistence debounce"
Require-Pattern $config "PERSIST_AUTO_SAVE_INTERVAL_MS\s*=\s*30000" "AUTO persistence throttle"
Require-Pattern $config "PERSIST_SERVO_SAVE_INTERVAL_MS\s*=\s*10000" "servo persistence throttle"

Require-Pattern $persistenceHeader "struct\s+PersistedAppState" "PersistedAppState"
Require-Pattern $persistenceHeader "autoFanPercent" "persisted AUTO fan percent"
Require-Pattern $persistenceSource "#include\s+<Preferences.h>" "Preferences-backed storage"
Require-Pattern $persistenceSource "PERSISTENCE_VERSION" "versioned storage"
Require-Pattern $persistenceSource "clampPercent" "restore value clamping"

Require-Pattern $servoHeader "bool\s+update\s*\(\s*uint32_t\s+nowMs\s*\)" "servo movement dirty signal"
Require-Pattern $servoHeader "restoreState\s*\(\s*int\s+angle\s*,\s*bool\s+sweepEnabled\s*\)" "servo restore API"
Require-Pattern $servoSource "ServoController::restoreState" "servo restore implementation"
Require-Pattern $servoSource "sweepEnabled_\s*=\s*sweepEnabled" "servo restore preserves sweep state"

Require-Pattern $main "#include\s+""app_persistence.h""" "main persistence include"
Require-Pattern $main "AppPersistence\s+appPersistence" "main persistence instance"
Require-Pattern $main "autoFanPercentProvisional" "provisional AUTO state"
Require-Pattern $main "appPersistence\.begin" "persistence initialization"
Require-Pattern $main "appPersistence\.load" "restore load path"
Require-Pattern $main "Config::PERSIST_AUTO_SAVE_INTERVAL_MS" "AUTO save throttle use"
Require-Pattern $main "Config::PERSIST_SERVO_SAVE_INTERVAL_MS" "servo save throttle use"
Require-Pattern $main "servoController\.restoreState" "main applies servo restore API"
Require-Pattern $main "autoFanPercentProvisional[\s\S]{0,240}return;" "failed DHT read preserves provisional AUTO output"

if ($main -match "writeFanPercent\s*\(\s*Config::BOOT_FAN_PERCENT\s*\)\s*;") {
  throw "Integrated setup must not overwrite restored fan output with BOOT_FAN_PERCENT"
}

Write-Output "Persistence wiring check passed"
