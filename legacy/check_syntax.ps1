# Syntax-checks every legacy translation unit (from RailSim2.vcxproj) against the D3D8 shim headers.
# Usage: powershell -File legacy\check_syntax.ps1 [-Files a.cpp,b.cpp]
param([string[]]$Files)

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$vc = Get-ChildItem "C:\Program Files (x86)\Microsoft Visual Studio\*\*\VC\Auxiliary\Build\vcvars64.bat" |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $Files) {
    $Files = Select-String -Path RailSim2.vcxproj -Pattern 'ClCompile Include="([^"]+)"' |
        ForEach-Object { $_.Matches[0].Groups[1].Value }
}
$flags = '/nologo /Zs /EHsc /permissive /Zc:strictStrings- /std:c++14 /source-charset:utf-8 /execution-charset:.932 /DWIN32 /D_WINDOWS ' +
    '/D_CRT_SECURE_NO_WARNINGS /W1 /wd4996 /wd4244 /wd4305 /wd4267 /wd4018 /I legacy\shim /I lib /I .'
$out = Join-Path $env:TEMP 'rs2_syntax.txt'
cmd /c "call `"$vc`" >nul && cl $flags $($Files -join ' ') > `"$out`" 2>&1"
$errors = Select-String -Path $out -Pattern 'error [CD]\d+|fatal error'
"files: $($Files.Count)  errors: $(@($errors).Count)"
$errors | Select-Object -First 60 | ForEach-Object { $_.Line }


