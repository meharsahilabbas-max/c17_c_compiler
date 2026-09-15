param(
    [int]$Iterations = 1000,
    [string]$Compiler = ""
)
$ErrorActionPreference = 'Stop'
if (-not $Compiler) { $Compiler = Join-Path $PSScriptRoot '..\build-release\mycc.exe' }
if (-not (Test-Path $Compiler)) { throw "Compiler not found: $Compiler" }
$temp = Join-Path $env:TEMP "mycc-fuzz-$PID.c"
$random = [System.Random]::new(17)
try {
    for ($i = 0; $i -lt $Iterations; $i++) {
        $parts = @('int main(void) { return 0; }', 'int main(void) { return 1 + 2 * 3; }', 'int main(void) { int x = 4; while (x > 0) x = x - 1; return x; }', 'int main(void) { return ; }')
        $text = $parts[$random.Next($parts.Count)]
        if ($random.Next(4) -eq 0) { $text = "/* fuzz */ $text // trailing" }
        Set-Content -Path $temp -Value $text -NoNewline
        & $Compiler -E $temp *> $null
        if ($LASTEXITCODE -gt 2) { throw "unexpected crash-like exit code $LASTEXITCODE at iteration $i" }
    }
    Write-Output "fuzz smoke passed: $Iterations inputs"
} finally { Remove-Item $temp -Force -ErrorAction SilentlyContinue }
