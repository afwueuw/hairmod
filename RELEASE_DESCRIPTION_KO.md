# HairPositioner UI 1.1

**RaceMenu를 열지 않고 헤어와 파츠를 조절하세요.**

HairPositioner UI는 HairPositioner의 조절 기능을 SKSE Menu Framework의 작은 창에서 사용할 수 있게 해 주는 추가 모드입니다. 캐릭터를 보면서 헤어, 수염, 추가 머리 파츠, 착용 중인 가발·장비의 위치와 모양을 조절할 수 있습니다.

기본 핫키 **F3**으로 창을 열고 대상을 선택하면 바로 편집을 시작할 수 있습니다. 마우스 휠과 숫자 입력으로 세밀하게 맞추고, 실행 취소와 시작 상태 복원으로 변경 전 상태를 되찾을 수 있습니다.

## 주요 기능

### 부위와 아이템별 조절

- **헤어·수염:** 각 부위를 따로 선택해 편집합니다.
- **추가 머리 파츠:** HairPositioner가 인식하는 귀·뿔 등의 커스텀 파츠를 조절합니다.
- **착용 가발·장비:** 원본이 제공하는 착용품을 실제 이름으로 선택합니다.
- **위치·회전·크기:** 각각 3개 축을 개별적으로 조절합니다.
- **피벗:** 머리 본, 선택 대상 중심, 모델 원점 중 변형의 기준점을 선택합니다.

지원 대상은 원본 HairPositioner에서 활성화하고 인식한 플레이어의 부위·파츠입니다. NPC 편집이나 모든 장비를 자동으로 지원하는 기능은 포함하지 않습니다.

### 마우스 휠과 정밀도

숫자 입력칸 위에 마우스를 올리고 휠을 돌리면 값이 증감합니다. 값을 조절하는 동안에는 창이 함께 스크롤되지 않으며, 다른 영역에서는 휠로 창을 스크롤할 수 있습니다.

상단의 **정밀도**에서 조절 단위를 선택할 수 있습니다. 선택한 정밀도는 휠 증감 단위와 드래그 속도에 적용되고, 게임을 다시 실행해도 유지됩니다.

| 정밀도 | 위치·크기 | 회전 |
|---|---:|---:|
| 미세 | 0.001 | 0.01° |
| 보통 | 0.01 | 0.1° |
| 크게 | 0.1 | 1° |

### 세이브와 별개인 외부 프리셋

- 슬라이더·휠·초기화로 대상값을 바꾸면 외부 프리셋도 자동으로 갱신됩니다. 연속 조작 중에는 디스크에 매번 쓰지 않고, 마지막 변경 뒤 약 0.75초가 지나면 백그라운드에서 저장합니다.
- 새 게임이나 세이브 로드 후 HairPositioner가 해당 대상을 제공하면 저장값을 자동 적용합니다. 대상 목록은 게임 중에도 5초 간격으로 다시 확인합니다.
- 가발·장비는 플러그인 파일명과 로컬 FormID를 사용해 저장하므로, 단순한 로드 오더 번호 변경에 덜 민감합니다. 모드를 제거하거나 이름/FormID가 달라진 아이템은 별도 대상이 될 수 있습니다.
- 파일 경로: `Data\SKSE\Plugins\HairPositionerUI_presets.txt`
- 조절 창의 체크박스로 자동 저장·적용을 끌 수 있습니다. 선택 대상 프리셋을 수동으로 저장·적용·삭제할 수 있고, 현재 목록에 표시된 대상값을 한꺼번에 외부 파일로 옮길 수도 있습니다.

**기존 세이브에서 이미 조절해 둔 모든 가발이 자동으로 한꺼번에 복구되는 것은 아닙니다.** 일괄 저장은 현재 HairPositioner UI 목록에서 읽을 수 있는 대상만 가져옵니다. 앞으로 조절하는 값은 자동으로 외부 파일에 저장되며, 기존 세이브의 보이지 않는/현재 목록에 없는 가발값은 해당 대상이 목록에 나타나는 상태에서 가져와야 합니다.

### 실행 취소와 시작 상태 복원

**실행 취소**는 선택한 부위의 직전 변경을 되돌립니다. 드래그나 숫자 편집 한 번을 하나의 작업으로 묶고, 같은 값에 대한 짧은 연속 휠 조작도 한 작업으로 처리합니다. 기록은 부위별로 최대 64개까지 유지됩니다.

**조절 시작 상태 복원**은 이번에 창을 연 뒤 해당 부위의 값을 처음 읽었을 때로 돌아갑니다. 여러 부위를 오가며 조절해도 각 부위의 시작값을 따로 기억합니다. 시작 상태 복원과 기본값 초기화도 실행 취소할 수 있습니다.

창을 닫았다 다시 열거나 세이브를 불러오면 새로운 편집 세션이 시작됩니다. 실행 취소 기록과 시작값은 세이브에 저장되지 않습니다.

## 기본 사용법

1. 세이브를 불러온 뒤, 조절할 부위가 잘 보이도록 카메라 구도를 잡습니다.
2. **F3**을 눌러 조절 창을 엽니다.
3. 상단 **조절 대상**에서 헤어·수염·파츠·착용품을 선택합니다.
4. 숫자를 좌우로 드래그하거나 숫자 위에서 휠을 돌려 조절합니다. 정확한 값을 넣으려면 **Ctrl+클릭**으로 직접 입력합니다.
5. 필요한 경우 정밀도와 피벗을 바꿔 모양을 맞춥니다.
6. F3 또는 **닫기**로 창을 닫습니다. 변경 상태를 보존하려면 게임을 저장하세요.

핫키 대신 **Mod Control Panel → HairPositioner → 부위·파츠 조절 → 작은 조절 창 열기**로도 사용할 수 있습니다. 창을 조작하는 동안 게임 입력이 차단되므로 구도는 먼저 잡아 주세요.

## 버튼과 조작 안내

| 조작·버튼 | 동작 |
|---|---|
| F3 | 조절 창 열기·닫기. 기본 핫키이며 변경 가능 |
| 숫자 좌우 드래그 | 해당 축의 값을 연속 조절 |
| 숫자 위에서 마우스 휠 | 현재 정밀도 단위로 값 증감 |
| Ctrl+클릭 | 숫자 직접 입력 |
| 각 행의 **기본** | 해당 축만 기본값으로 초기화 |
| **선택 대상 초기화** | 확인 후 선택한 대상의 위치·회전·크기·피벗을 초기화 |
| **실행 취소** | 선택한 부위의 이전 변경을 되돌림 |
| **조절 시작 상태 복원** | 선택한 부위를 이번 편집의 시작값으로 복원 |
| **부위 목록 새로 읽기** | 장착 변경 등을 반영해 대상 목록 갱신 |
| **현재값 새로 읽기** | 선택한 대상의 현재값을 다시 조회 |

값 전달 중에는 대상 전환이나 실행 취소·복원이 잠시 비활성화될 수 있습니다. 숫자를 직접 입력 중일 때는 휠이 값을 바꾸지 않습니다.

목록은 창을 열 때와 편집하지 않는 동안 주기적으로 갱신됩니다. 장착을 바꿨는데 목록에 바로 나타나지 않으면 **부위 목록 새로 읽기**를 누르세요.

## 핫키 설정

조절 창 또는 Mod Control Panel의 HairPositioner 페이지에서 설정할 수 있습니다.

- **핫키 변경:** 버튼을 누른 뒤 원하는 키보드 키를 누릅니다. Esc로 등록을 취소합니다.
- **핫키 끄기:** 단축키를 비활성화합니다. Mod Control Panel에서는 계속 창을 열 수 있습니다.
- **기본 F3:** 기본 키로 되돌립니다.

단일 키보드 키를 지원합니다. 조합키·마우스·게임패드 버튼 등록은 지원하지 않습니다.

## 요구 사항과 설치

- **HairPositioner 2.5** 및 원본의 요구 사항
- **SKSE Menu Framework** — 개발 기준 환경: 3.1.3 hotfix2
- **RaceMenu / NiOverride**
- 게임 버전에 맞는 **SKSE64**, **Address Library**
- 대상 환경: **Skyrim Special Edition / Anniversary Edition**, VR 미지원

1. 요구 모드를 설치하고 활성화합니다.
2. 이 모드의 ZIP을 MO2에 별도 모드로 설치합니다.
3. MO2 왼쪽 목록에서 **HairPositioner 아래**에 두고 활성화합니다.
4. SKSE로 게임을 실행합니다.

원본 HairPositioner를 계속 활성화해야 합니다. 이 모드는 원본의 기능을 호출하는 추가 UI이며, 별도 ESP가 없어 오른쪽 플러그인 목록에서 정렬할 항목은 없습니다.

### 이전 버전에서 업데이트

게임을 완전히 종료한 뒤 이전 UI 모드의 DLL을 이 버전의 `HairPositionerUI.dll`로 교체하세요. 이 수정본은 기존 `HPUIBridge.pex`와 호환되도록 브리지 스크립트를 변경하지 않았습니다. 기존 `.pex`는 그대로 두면 됩니다. 구버전 DLL이 덮어쓰고 있지 않은지도 확인해 주세요.

사용자 지정 핫키·정밀도를 유지하려면 기존 `SKSE/Plugins/HairPositionerUI.ini`를 보관하세요. MO2 환경에서는 변경된 INI가 Overwrite에 생성될 수 있습니다.

## 저장과 원본 연동

변경값은 원본 HairPositioner의 조절 API로 적용하고, 원본 RaceMenu 연동에 사용되는 부위·아이템별 저장값에도 반영합니다. 별도로 UI 프리셋 파일에도 저장되므로 새 게임에 적용할 수 있습니다. 게임 저장은 기존처럼 세이브별 상태를 보존하고, 외부 프리셋은 여러 세이브에서 공통으로 사용할 조절값을 보존합니다.

원본이 가발 슬롯 동반 이동을 제공하는 경우에는 헤어 조절 시 해당 체크박스도 표시됩니다. 대상별 지원 여부와 조절 범위는 원본 기능을 따릅니다.

## 자주 묻는 질문

**원하는 가발이나 파츠가 목록에 없어요.**

원본 HairPositioner가 해당 대상을 지원하고 활성화했는지 확인해 주세요. 착용품은 장착 상태도 확인한 뒤 목록을 새로 읽어 보세요. 이 UI가 원본에 없는 대상 지원을 추가하지는 않습니다.

**수염은 목록에 있는데 조절할 수 없어요.**

캐릭터에 실제 수염 부위가 없으면 조절이 비활성화됩니다.

**초기화와 시작 상태 복원은 무엇이 다른가요?**

초기화는 원본의 기본값으로 되돌립니다. 시작 상태 복원은 이번 편집을 시작하기 전에 사용하던 값으로 돌아갑니다.

**실행 취소 기록이 사라졌어요.**

기록은 현재 창의 편집 세션 안에서만 유지됩니다. 창을 다시 열거나 세이브를 불러오면 새로 시작합니다.

**모드를 제거하면 적용한 모양도 사라지나요?**

UI를 제거해도 적용한 값은 원본 HairPositioner에 남습니다. 기본 모양으로 되돌리고 싶다면 제거 전에 원하는 대상을 초기화하고 게임을 저장하세요.

## 크레딧과 소스

- HairPositioner 원본: https://github.com/Iwillbewarlock/HairPositioner
- SKSE Menu Framework API: https://github.com/QTR-Modding/SKSE-Menu-Framework-3-API
- CommonLibSSE-NG: https://github.com/alandtse/CommonLibVR

이 UI의 수정 소스는 `HairPositionerUI-1.1.2-source.zip`으로 제공합니다. `.github/workflows/build.yml`이 Windows GitHub Actions에서 DLL을 빌드합니다. 라이선스와 관련 고지는 배포본의 `licenses` 폴더를 확인해 주세요. 원본 HairPositioner, RaceMenu, SKSE Menu Framework는 별도로 설치해야 하며 배포 ZIP에 포함되지 않습니다.


## 1.1.2 event-driven value refresh

Value snapshots are no longer polled on a timer. They are requested when the UI opens, when the selected target changes, when the user presses the manual refresh button, or when a catalog scan discovers that the current target changed. A successful user edit returns the current values directly. Equipped-item catalog polling remains at about every 10 seconds so the list can detect equipment changes; after equipping or removing an item, use the manual list refresh for an immediate update. During that scan, only the target selector and value-edit controls that depend on the catalog are briefly locked; unrelated UI actions remain available. If another mod changes values externally, use the manual value refresh button.
