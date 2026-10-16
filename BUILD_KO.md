# HairPositioner UI 1.1.0 개발 수정본 빌드

HairPositioner UI 소스는 GPL-3.0-or-later입니다.

- CommonLibSSE-NG: https://github.com/alandtse/CommonLibVR, ng 브랜치
- 확인한 커밋: 39f9d07a6ffabea8fb559eee87ab7d27cd463e8a
- SKSE Menu Framework API: https://github.com/QTR-Modding/SKSE-Menu-Framework-3-API
  include/SKSEMenuFramework.h와 LGPL-2.1 라이선스를 포함합니다.
- xmake 3.x, MSVC x64, C++23. 이 로컬 빌드는 Visual Studio 2026을 사용했습니다.
- xmake 의존성: directxmath 2024.02, directxtk 24.2.0, spdlog 1.16.0

CommonLib 소스를 lib/commonlibsse-ng에 둡니다. 의존성 소스는 수정하지 않았습니다.

```
xmake f -m releasedbg -a x64 --vs=2026 -y
xmake build -j6 HairPositionerUI
```

결과는 build/bin/HairPositionerUI.dll입니다.
HPUIBridge.psc는 Creation Kit PapyrusCompiler로 컴파일합니다.
원본 HairPositioner 2.5의 HairPositioner.psc, RaceMenu BSA에 포함된 NiOverride.psc,
게임/SKSE의 스크립트 소스를 import 경로에 넣습니다.
MO2에 있는 오래된 loose NiOverride.psc에는 BodyMorph 함수가 없을 수 있으므로,
실제 RaceMenu BSA의 소스를 우선하는 것이 필요했습니다. 설치된 소스 파일은 수정하지 않았습니다.

package/SKSE/Plugins/HairPositionerUI.dll과 package/Scripts/HPUIBridge.pex를 배포합니다.
HairPositioner.psc/pex, NiOverride.psc/pex 및 기존 DLL은 이 패키지에 재배포하지 않습니다.

## 1.1.0 수정 사항

- Papyrus 값 조회를 매초 반복하지 않고, 사용자가 요청했거나 선택/목록 갱신이 끝났을 때만 현재값을 다시 읽습니다.
- 읽기 전용 조회와 실제 쓰기 작업을 분리해, 백그라운드 조회 중 UI 조작이 불필요하게 비활성화되지 않도록 했습니다.
- 대상 종류를 유지하는 선택 복구 로직을 추가했습니다.
- 가발/부위별 11개 편집값을 `Data/SKSE/Plugins/HairPositionerUI.ini`의 `Target_<part>_<item>` 섹션에 저장하고, 게임 세션에서 HairPositioner가 인식하는 대상에 백그라운드로 복원합니다.

이 환경에는 Windows MSVC/xmake와 Creation Kit PapyrusCompiler가 없으므로 이 소스는 여기서 DLL로 컴파일하거나 게임에서 검증하지 못했습니다. 아래 명령은 해당 Windows 개발 환경에서 실행해야 합니다. 기존 `HPUIBridge.pex`와 호환되는 Papyrus 함수만 호출하므로 PSC/PEX 자체 변경은 없습니다.
