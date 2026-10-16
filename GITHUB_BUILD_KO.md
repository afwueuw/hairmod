# GitHub Actions로 DLL 빌드하기

이 수정본은 소스 코드만 포함하며, 컴파일된 DLL은 포함하지 않습니다. Papyrus 브리지(`HPUIBridge.psc`)는 변경하지 않았으므로 이미 설치된 `HPUIBridge.pex`를 그대로 사용합니다.

## GitHub Desktop 이용 (명령어 입력 불필요)

1. [GitHub Desktop](https://desktop.github.com/)을 설치하고 GitHub 계정으로 로그인합니다.
2. 이 ZIP을 PC의 새 폴더에 압축 해제합니다.
3. GitHub Desktop에서 **File → Add Local Repository**를 선택하고 압축 해제한 폴더를 지정합니다. 아직 Git 저장소가 아니라고 나오면 화면의 **create a repository** 링크를 선택해 그 폴더에 저장소를 생성합니다. 기존 소스 파일을 삭제하거나 다른 폴더로 옮기지 마세요.
4. **Publish repository**를 눌러 GitHub 계정에 저장소를 게시합니다. 개인 계정에서만 쓸 경우 저장소를 Private으로 설정해도 Actions는 실행할 수 있습니다.
5. GitHub 웹사이트에서 저장소를 열고 **Actions → Build HairPositionerUI → Run workflow**를 누릅니다.
6. 빌드가 성공하면 실행 결과 페이지의 **Artifacts**에서 `HairPositionerUI-DLL`을 내려받고 ZIP을 풉니다. 그 안의 `HairPositionerUI.dll`을 기존 모드 DLL과 교체합니다.

## 설치 주의

- 교체 전에 원본 `HairPositionerUI.dll`을 백업하세요.
- 이 저장소의 Actions가 생성하는 것은 `HairPositionerUI.dll`뿐입니다. 기존 `Scripts/HPUIBridge.pex`는 그대로 둡니다.
- GitHub Actions 빌드 성공은 실제 Skyrim 실행 테스트와 같지 않습니다. 게임에서 테스트하지 않은 상태이므로, 문제가 생기면 `Documents/My Games/Skyrim Special Edition/SKSE/HairPositionerUI.log`를 확인하세요.
- 모드 파일을 공개 저장소로 게시하면 소스가 공개됩니다. 혼자 사용하려면 Private 저장소를 선택하세요.

## 이미 GitHub 저장소를 만들어 둔 경우 (1.1.2 업데이트)

새 저장소를 만들 필요가 없습니다. `HairPositionerUI-1.1.2-github-patch.zip`을 압축 해제한 뒤, GitHub Desktop으로 열어 둔 기존 저장소 폴더 안에 같은 경로로 파일을 덮어씁니다. 필수 변경 파일은 `src/main.cpp`와 `xmake.lua`입니다. 안내 문서도 최신화하려면 ZIP의 Markdown 파일을 함께 덮어써도 됩니다.

1. GitHub Desktop에서 변경 파일 목록을 확인합니다.
2. Summary에 `Fix UI refresh flicker`라고 입력하고 **Commit to main**을 누릅니다.
3. **Push origin**을 눌러 GitHub에 올립니다.
4. 워크플로는 `src/**` 또는 `xmake.lua` 변경으로 자동 실행되어야 합니다. 실행되지 않으면 **Actions → Build HairPositionerUI → Run workflow**를 선택합니다.
5. 성공한 실행의 `HairPositionerUI-DLL` 아티팩트를 다운로드합니다. 기존 DLL을 백업한 다음 새 DLL로 교체하세요.

이 수정은 소스 코드 변경입니다. 이 ZIP에는 사전 컴파일된 DLL이 포함되어 있지 않습니다.
