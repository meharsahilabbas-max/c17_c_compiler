$ErrorActionPreference = 'Stop'
$compiler = Join-Path $PSScriptRoot '..\build\mycc.exe'
if (-not (Test-Path $compiler)) { throw "Build mycc before running preprocessor tests." }
$source = Join-Path $PSScriptRoot 'preprocessor.c'
$result = & $compiler -E $source
if ($LASTEXITCODE -ne 0) { throw "preprocessor rejected valid conditional input" }
if ($result -notmatch 'int selected = 42;') { throw "object-like macro expansion failed" }
if ($result -match 'ignored|also_ignored') { throw "conditional branch filtering failed" }
Write-Output 'preprocessor tests passed'
