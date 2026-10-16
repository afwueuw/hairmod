# HairPositionerUI 1.1.0 — 외부 프리셋 저장 수정본

이 소스 수정본은 HairPositionerUI의 가발/장비 조절값을 현재 Skyrim 세이브와 별도로 저장해 다른 세이브나 새 게임에서도 다시 적용할 수 있도록 하는 기능을 추가합니다.

## 추가 기능

- 가발/장비·헤어 파츠 값을 조절하면 마지막 변경 뒤 약 0.75초 후 외부 프리셋 파일에 자동 저장
- 파일 쓰기는 별도 스레드에서 처리하고 임시 파일을 작성한 뒤 교체
- Skyrim을 불러오거나 새 게임을 시작한 뒤 HairPositioner가 대상을 목록에 제공하면 저장값을 순차 적용
- 개별 대상 프리셋 적용/삭제, 현재 목록값 일괄 가져오기
- 기본 저장 위치: `Data/SKSE/Plugins/HairPositionerUI_presets.txt`
- 기능 사용 여부: `Data/SKSE/Plugins/HairPositionerUI.ini`의 `[Persistence] Enabled=1`

## 중요한 제한

이 ZIP은 소스 코드이며 컴파일된 `HairPositionerUI.dll`과 `HPUIBridge.pex`는 포함하지 않습니다. 브리지 Papyrus 소스는 변경하지 않았으므로, 기존 설치의 `Scripts/HPUIBridge.pex`를 유지하는 것을 전제로 합니다.

기존 세이브에만 저장되어 있고 아직 외부 파일에 저장되지 않은 모든 예전 가발값을 자동으로 추출할 수 있는 것은 아닙니다. **현재 HairPositioner 목록에서 읽을 수 있는 대상**에 한해 UI의 `현재 목록의 모든 대상값을 외부 파일에 저장` 기능으로 가져올 수 있습니다. 앞으로 조절하는 값은 자동으로 외부 파일에 기록됩니다.

가발/장비는 플러그인 파일명과 로컬 FormID로 식별하도록 했습니다. HairPositioner 원본이 실제로 노출하는 대상만 적용할 수 있으므로, 착용/목록 갱신 시점에 따라 해당 가발이 목록에 나타난 뒤 적용될 수 있습니다.

## 빌드

Creation Kit 없이 Windows용 DLL을 빌드하도록 `.github/workflows/build.yml`을 포함했습니다. `GITHUB_BUILD_KO.md`의 단계를 따라 GitHub 저장소에 게시한 뒤 Actions에서 `Build HairPositionerUI` 워크플로를 실행하세요. 완료되면 `HairPositionerUI-DLL` 아티팩트를 내려받을 수 있습니다.

빌드가 성공하더라도 실제 Skyrim 게임 내 호환성과 동작은 별도로 확인해야 합니다. 문제가 생기면 `Documents/My Games/Skyrim Special Edition/SKSE/HairPositionerUI.log`를 확인하세요.
