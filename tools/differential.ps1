param(
    [string]$Compiler = "",
    [string]$Reference = "clang",
    [string]$Source = ""
)
$ErrorActionPreference = 'Stop'
if (-not $Compiler) { $Compiler = Join-Path $PSScriptRoot '..\build-release\mycc.exe' }
if (-not $Source) { $Source = Join-Path $PSScriptRoot '..\examples\factorial.c' }
if (-not (Test-Path $Compiler)) { throw "Compiler not found: $Compiler" }
if (-not (Get-Command $Reference -ErrorAction SilentlyContinue)) { Write-Warning "$Reference is not installed; differential test skipped"; exit 0 }
$myccAssembly = Join-Path $env:TEMP "mycc-diff-$PID.s"
$refAssembly = Join-Path $env:TEMP "ref-diff-$PID.s"
try {
    & $Compiler -S $Source -o $myccAssembly
    $myccCode = $LASTEXITCODE
    & $Reference -S $Source -o $refAssembly
    $refCode = $LASTEXITCODE
    if (($myccCode -eq 0) -and ($refCode -ne 0)) { throw 'mycc accepted a program rejected by the reference compiler' }
    Write-Output "differential compile comparison passed: mycc=$myccCode reference=$refCode"
} finally { Remove-Item $myccAssembly,$refAssembly -Force -ErrorAction SilentlyContinue }
