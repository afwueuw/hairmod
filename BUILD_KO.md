# 빌드

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


## GitHub Actions로 DLL 빌드하기 (Creation Kit 없이)

이 수정본은 `HPUIBridge.psc`를 변경하지 않았으므로 Papyrus 재컴파일이 필요하지 않습니다. ZIP의 파일을 새 GitHub 저장소에 업로드한 뒤 `Actions` 탭에서 `Build HairPositionerUI` 워크플로를 실행하면 Windows 러너가 DLL을 빌드하고 `HairPositionerUI-DLL` 아티팩트를 제공합니다. 아티팩트에는 `HairPositionerUI.dll`만 들어 있습니다.

설치할 때는 기존 UI 모드의 DLL만 교체하고, 원본 모드에 이미 설치된 `Scripts/HPUIBridge.pex`는 그대로 유지하세요. 교체 전 원본 DLL을 백업하는 것이 좋습니다. 워크플로가 성공적으로 끝날지는 GitHub 러너 환경과 외부 의존성 다운로드 결과에 달려 있으므로, 이 소스 패키지에 사전 컴파일된 DLL이 포함된 것은 아닙니다.

## 외부 프리셋 파일

게임에서 수정된 값은 `Data/SKSE/Plugins/HairPositionerUI_presets.txt`에 별도 저장됩니다. 저장은 마지막 편집 후 약 0.75초 동안 추가 변화가 없을 때 백그라운드 작업 스레드에서 실행되며, 종료 시 대기 중인 쓰기를 한 번 더 flush하도록 구성했습니다. 가발·장비의 외부 키에는 플러그인 파일명과 로컬 FormID를 사용합니다.


## 값 UI 갱신 (1.1.2)

`SnapshotV2`는 타이머로 반복 호출하지 않습니다. UI 열기, 대상 선택, 사용자의 수동 새로 읽기, 선택 대상이 바뀐 경우에 호출됩니다. 조작으로 변경한 값은 편집 요청 응답을 통해 UI에 반영됩니다. 착용 장비 목록은 장착/해제를 발견하기 위해 약 10초 간격으로 확인하며, 즉시 갱신이 필요하면 UI에서 `부위 목록 새로 읽기`를 사용합니다.
