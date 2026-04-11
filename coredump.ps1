param (
    [Parameter(Mandatory=$true)]
    [string]$IP,

    [Parameter(Mandatory=$true)]
    [string]$FirmwareElf
)

# --- 1. Environment Setup ---
$env:IDF_PATH            = "C:\esp\v6.0\esp-idf"
$env:IDF_TOOLS_PATH      = "C:\Espressif\tools"
$env:IDF_PYTHON_ENV_PATH = "C:\Espressif\tools\python\v6.0\venv"

$PythonExe    = "$($env:IDF_PYTHON_ENV_PATH)\Scripts\python.exe"
$CoreDumpTool = "$($env:IDF_PATH)\components\espcoredump\espcoredump.py"
$CoreFile     = "core_dump.bin"

# --- 2. Hardcoded GDB Path ---
$GdbExe = "C:\Espressif\tools\xtensa-esp-elf-gdb\16.3_20250913\xtensa-esp-elf-gdb\bin\xtensa-esp32-elf-gdb.exe"

if (-not (Test-Path $GdbExe)) {
    Write-Host "Error: GDB not found at $GdbExe" -ForegroundColor Red
    exit 1
}

# --- 3. Download the core dump ---
Write-Host "--- Downloading core dump from http://$IP/crashes ---" -ForegroundColor Cyan
try {
    Invoke-WebRequest -Uri "http://$IP/crashes" -OutFile $CoreFile -ErrorAction Stop -UseBasicParsing
} catch {
    Write-Host "Error: Download failed. Ensure the ESP32 is reachable." -ForegroundColor Red
    exit 1
}

# --- 4. Run the Tool with Explicit GDB Path ---
Write-Host "--- Running Analysis ---" -ForegroundColor Cyan

# We use the explicit path to GDB to bypass the PlatformIO project environment issues
& $PythonExe $CoreDumpTool info_corefile `
    --gdb "$GdbExe" `
    -m "$FirmwareElf" `
    --core "$CoreFile"

Write-Host "--- Process Complete ---" -ForegroundColor Green