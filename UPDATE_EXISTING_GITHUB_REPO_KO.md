# 기존 GitHub 저장소에 1.1.2 수정 적용하기

이 패치는 주기적인 값 읽기로 메뉴가 비활성화되는 현상을 더 줄이기 위한 소스 업데이트입니다. 사전 빌드 DLL은 포함하지 않습니다.

1. `HairPositionerUI-1.1.2-github-patch.zip`을 압축 해제합니다.
2. 기존 GitHub 저장소 폴더에 `src/main.cpp`와 `xmake.lua`를 같은 상대 경로로 덮어씁니다.
3. 안내 문서도 최신화하려면 ZIP 안의 Markdown 파일들을 덮어써도 됩니다.
4. GitHub Desktop에서 변경 내용을 확인하고 `Reduce automatic value refresh` 같은 커밋 메시지로 커밋한 뒤 `Push origin`을 누릅니다.
5. `src/main.cpp`와 `xmake.lua`가 바뀌면 Actions 빌드가 자동 실행됩니다. 실행되지 않으면 Actions 탭에서 `Build HairPositionerUI`를 수동 실행합니다.
6. 초록색 체크로 빌드가 성공한 뒤 `HairPositionerUI-DLL` 아티팩트를 다운로드합니다.
7. 스카이림을 완전히 종료하고 기존 `HairPositionerUI.dll`을 백업한 뒤 새 DLL로 교체합니다. `HPUIBridge.pex`와 `HairPositionerUI_presets.txt`는 그대로 유지합니다.

## 변경 사항

- `SnapshotV2` 값 읽기의 타이머 기반 반복 갱신을 제거했습니다.
- 현재값은 UI를 열거나, 대상을 선택하거나, `현재값 새로 읽기`를 누를 때 확인합니다. 사용자가 직접 조절한 값은 조절 요청의 응답으로 바로 UI에 반영됩니다.
- 현재 선택한 대상이 목록에서 사라지거나 초기 선택이 필요한 경우에만 목록 갱신 뒤 추가로 값을 읽습니다.
- 착용/해제된 대상 확인은 약 10초 간격으로 유지합니다. 따라서 새 장비는 즉시가 아니라 다음 목록 확인 때 나타날 수 있으며, 급하면 `부위 목록 새로 읽기`를 누르면 됩니다.
- 값 읽기 타이머가 반복해서 UI 잠금을 유발하는 상황을 제거합니다. 목록 확인 중에는 대상 선택/값 조절처럼 대상 상태에 의존하는 컨트롤만 잠시 잠기며, 무관한 메뉴 조작은 유지됩니다.
- 다른 모드가 값 자체를 변경했을 경우 `현재값 새로 읽기`로 표시값을 동기화할 수 있습니다.

## 주의

DLL 전체 컴파일과 실제 게임 테스트는 GitHub Actions 및 게임에서 확인해야 합니다. 빌드 성공만으로 게임 내 동작을 보장하지는 않습니다. 오류가 생기면 `Documents/My Games/Skyrim Special Edition/SKSE/HairPositionerUI.log`를 확인하세요.
