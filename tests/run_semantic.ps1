$ErrorActionPreference = 'Stop'
$compiler = Join-Path $PSScriptRoot '..\build\mycc.exe'
if (-not (Test-Path $compiler)) { throw "Build mycc before running semantic tests." }
$source = Join-Path $PSScriptRoot 'semantic_errors.c'
$assembly = Join-Path $PSScriptRoot 'semantic_errors.s'
& $compiler -S $source -o $assembly 2> (Join-Path $PSScriptRoot 'semantic_errors.log')
if ($LASTEXITCODE -eq 0) { throw "semantic invalid input was accepted" }
$diagnostics = Get-Content (Join-Path $PSScriptRoot 'semantic_errors.log') -Raw
if ($diagnostics -notmatch 'undeclared identifier') { throw "expected undeclared identifier diagnostic" }
Remove-Item $assembly -ErrorAction SilentlyContinue
Remove-Item (Join-Path $PSScriptRoot 'semantic_errors.log') -ErrorAction SilentlyContinue
Write-Output 'semantic tests passed'
