param([string]$Compiler = 'g++')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$out = Join-Path $root 'build/host'
New-Item -Path $out -ItemType Directory -Force | Out-Null
Push-Location $root
try {
    $cases = @(
        @{ Name = 'serial'; Sources = @('tests/test_serial.cpp', 'BSP/uart.cpp', 'Common/logger.cpp') },
        @{ Name = 'timer'; Sources = @('tests/test_timer.cpp', 'BSP/timer.cpp', 'BSP/pwm.cpp') },
        @{ Name = 'devices'; Sources = @('tests/test_devices.cpp', 'Module/mpu6050.cpp', 'Module/oled.cpp') }
    )
    foreach ($case in $cases) {
        $exe = Join-Path $out ($case.Name + '.exe')
        $sources = $case.Sources
        & $Compiler -std=c++17 -Wall -Wextra -Werror -Itests/support -I. @sources -o $exe
        if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $($case.Name)" }
        & $exe
        if ($LASTEXITCODE -ne 0) { throw "Test failed: $($case.Name)" }
    }
}
finally { Pop-Location }
