$ErrorActionPreference = 'Stop'
$compiler = Join-Path $PSScriptRoot '..\build\mycc.exe'
if (-not (Test-Path $compiler)) { throw "Build mycc before running smoke tests." }
$out = Join-Path $PSScriptRoot 'parser_smoke.s'
& $compiler -S (Join-Path $PSScriptRoot 'parser_smoke.c') -o $out
if ($LASTEXITCODE -ne 0) { throw "compiler rejected parser smoke input" }
if (-not (Test-Path $out)) { throw "assembly output was not created" }
Remove-Item $out -Force
Write-Output 'smoke tests passed'
