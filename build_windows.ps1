$ErrorActionPreference = 'Stop'

if (-not (Get-Command xmake -ErrorAction SilentlyContinue)) {
    throw 'xmake 3.x를 찾을 수 없습니다. xmake와 Visual Studio 2026 C++ x64 빌드 도구를 설치한 뒤 다시 실행하세요.'
}

xmake f -m releasedbg -a x64 --vs=2026 -y
if ($LASTEXITCODE -ne 0) { throw "xmake configure 실패: $LASTEXITCODE" }

xmake build -j6 HairPositionerUI
if ($LASTEXITCODE -ne 0) { throw "HairPositionerUI 빌드 실패: $LASTEXITCODE" }

$dll = Join-Path $PSScriptRoot 'build/bin/HairPositionerUI.dll'
if (-not (Test-Path $dll)) { throw "빌드가 끝났지만 DLL이 없습니다: $dll" }
Write-Host "빌드 성공: $dll"
Write-Host '이 DLL은 게임에서 검증되지 않은 개발 수정본입니다. 원본 DLL을 백업한 뒤 별도 MO2 프로필에서 시험하세요.'
