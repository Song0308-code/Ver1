# ver1 실행 프로세스

UE 5.8 + unreal-mcp로 만드는 LoL 스타일 MOBA 프로토타입의 작업 순서.
각 단계는 "완료 기준"을 통과해야 다음 단계로 넘어간다.

## 공통 규칙

- **단위**: LoL 1단위 = 언리얼 1cm. 맵 한 변 약 15,000 (150m).
- **네트워크**: 리슨 서버, 서버 권위. 모든 기능은 PIE "2인 / Listen Server" 모드로 테스트한다.
- **코드**: 핵심 시스템은 C++, 수치·외형은 DataAsset / 블루프린트.
- **외형 분리**: VFX·사운드는 GameplayCue, 메시·애니메이션은 데이터로 지정해 나중에 교체한다.
- **작업 단위**: 한 줄 스펙 → 구현 → 2인 PIE 테스트 → 커밋.

## 조작

| 입력 | 동작 |
|---|---|
| 우클릭 | 적이면 공격, 아니면 이동. 누르고 있으면 커서를 따라간다 |
| A 후 좌클릭 | 적이면 공격, 지면이면 공격 이동(걸어가며 만나는 적을 공격) |
| Q / W / E / R | 스킬 시전 (누르는 즉시 커서 위치로) |
| Ctrl + Q / W / E / R | 스킬 단계 올리기 |
| Y | 카메라 고정/해제 |
| Space | 누르는 동안 내 챔피언으로 카메라 복귀 |
| 마우스 휠 | 줌 |
| Enter | 경기가 끝난 뒤 메뉴로 돌아가기 |

가만히 서 있으면 사거리 안의 적 챔피언·미니언을 자동으로 공격한다(건물 제외). 적을 클릭할 때는 커서가 정확히 겹치지 않아도 가장자리에서 90 이내면 잡힌다(`ClickSelectMargin`).

기본 공격은 근접·원거리 모두 팀 색 투사체로 표시한다. 근접은 애니메이션이 들어오기 전까지 아주 빠른 투사체를 쓴다.

## 0단계. 환경 준비

- [x] 언리얼 에디터 재시작 후 MCP 툴셋 로드 확인
- [x] 로드 실패한 툴셋 플러그인이 있으면 `ver1.uproject`에서 제거
- [x] `.gitignore` 점검 (Binaries, Intermediate, Saved, DerivedDataCache 제외)
- [x] Git LFS 설정 (`*.uasset`, `*.umap`)
- [x] GameplayAbilities, EnhancedInput 플러그인 활성화 및 `ver1.Build.cs` 모듈 추가

완료 기준: 에디터가 오류 없이 열리고 MCP로 레벨의 액터 목록을 조회할 수 있다.

## 1단계. 프로젝트 뼈대

- [x] 폴더 규칙 확정 (`Content/Maps`, `Characters`, `Abilities`, `Data`, `UI`, `Minions`, `Structures`)
- [x] C++ 기본 클래스: GameMode, GameState, PlayerController, PlayerState, Character
- [x] 팀 구분(블루/레드) enum과 팀 인터페이스
- [x] 빈 테스트 맵 생성 후 기본 맵·GameMode 지정

완료 기준: 2인 PIE에서 두 플레이어가 각자 캐릭터로 스폰된다.

## 2단계. 조작과 맵 그레이박스

- [x] 탑다운 고정 카메라 (화면 가장자리 스크롤, 캐릭터 고정 토글)
- [x] 우클릭 이동 (내비메시 경로, 클라이언트에서도 지연 없이 동작)
- [x] 협곡 전체 그레이박스: 기지 2곳, 3라인, 강, 정글 벽, 부쉬 위치
- [x] 타워·억제기·넥서스 자리 표시 (팀당 타워 11, 억제기 3, 넥서스 1)
- [x] 내비메시 빌드, 라인별 스플라인 배치

완료 기준: 2인 PIE에서 양쪽 모두 맵 끝에서 끝까지 클릭 이동이 되고 서로의 이동이 보인다.

메모: 맵은 `/Game/Maps/L_SummonersRift`. 벽·정글 구조는 실제 협곡을 근사한 것이고(블루 쪽 절반을 설계해 대칭 복사), 타워·억제기·넥서스 위치는 실제 좌표를 따랐다. LoL 좌표 (x, y)는 언리얼 (X = y, Y = x)로 옮긴다. 입력은 아직 에셋이 아니라 `MobaPlayerController`에서 코드로 구성한다.

## 3단계. 전투 기본

- [x] GAS 초기화 (AbilitySystemComponent는 PlayerState에 배치)
- [x] 스탯 AttributeSet: 체력, 마나, 공격력, 주문력, 방어력, 마법 저항력, 공격 속도, 이동 속도, 사거리
- [x] 대상 지정 (우클릭으로 적 선택, 사거리 밖이면 접근 후 공격)
- [x] 기본 공격 (근접 / 원거리 투사체)
- [x] 데미지 공식 (방어력·마법 저항력 적용)
- [x] 사망, 부활 타이머, 기지 부활
- [x] 머리 위 체력바

완료 기준: 2인 PIE에서 서로 기본 공격으로 처치하고 부활할 수 있다.

메모: 능력 시스템은 `MobaPlayerState`가 갖고 캐릭터는 아바타다. 피해는 `UMobaDamageEffect` 하나로 주고 피해량은 `Data.Damage.*` 태그로 넘긴다. 기본 공격은 `UMobaCombatComponent`가 처리한다(추격은 조종하는 클라이언트, 판정은 서버). 체력바는 임시로 `AMobaHUD`가 캔버스에 그린다. 공격 시 대상을 바라보는 회전은 아직 없다(8단계 애니메이션과 함께).

주의: MCP의 `set_actor_transform`에 위치만 넘기면 스케일이 1로 초기화된다. 또 내비메시 경계 볼륨은 이 방식으로 옮겨도 내비메시가 다시 구워지지 않으므로, 바꾼 뒤에는 레벨을 다시 로드해야 한다.

## 4단계. 스킬 (챔피언 1명)

- [x] 스킬 입력 QWER, 조준 표시기, 스마트캐스트
- [x] Q: 직선 투사체 (논타깃)
- [x] W: 대상 지정 스킬 또는 자기 강화
- [x] E: 이동기 (돌진 / 점멸)
- [x] R: 범위 스킬
- [x] 쿨다운, 마나 소모
- [x] 스킬 레벨업 (5단계에서 구현. 스킬은 0단계로 시작하고 Ctrl+Q/W/E/R로 올린다)
- [x] 상태 이상: 기절, 둔화
- [x] 스킬 HUD (아이콘, 쿨다운, 체력·마나 바)
- [x] 챔피언 정의 DataAsset (스탯, 스킬 목록, 메시)

완료 기준: 2인 PIE에서 네 가지 스킬이 양쪽 화면에 동일하게 보이고 판정이 서버 기준으로 일치한다.

메모: 스킬은 `UMobaGameplayAbility`를 상속하고 서버에서만 실행된다. 클라이언트는 `AMobaCharacter::CastAbility`로 요청한다. 쿨다운·마나·둔화·기절은 `MobaGameplayEffects`의 범용 이펙트 네 개로 처리하므로 스킬마다 이펙트 에셋을 만들 필요가 없다. 스킬 원형은 `MobaAbilityArchetypes`의 네 종류(직선 투사체, 자기 강화, 점멸, 범위 폭발)이며, 새 챔피언은 이 원형의 수치를 바꾼 블루프린트 자식을 `UMobaChampionData` 에셋에 넣어 만든다. 기본 챔피언은 `DefaultGame.ini`의 `[/Script/ver1.MobaGameMode] DefaultChampion`으로 지정한다. 스킬바와 조준 표시는 임시로 `AMobaHUD`가 캔버스에 그린다.

## 5단계. 미드 라인 가동

- [x] 미니언 (근접·원거리): 스플라인 따라 이동, 어그로 규칙 (StateTree 대신 C++ 타이머 로직으로 구현)
- [x] 웨이브 스포너 (30초 주기)
- [x] 타워: 대상 우선순위, 챔피언 공격 시 어그로 전환, 뒤 타워 보호 규칙
- [x] 넥서스 파괴 → 승패 판정과 결과 화면
- [x] 골드 (막타, 시간당 수급), 경험치, 레벨업, 스탯 성장
- [x] 기지 우물 회복

완료 기준: 2인 PIE에서 미드 라인만으로 한 판을 끝까지 진행해 승패가 난다.

메모: 미니언은 `AMobaMinion`(자체 능력 시스템, 서버 AI), 건물은 `AMobaStructure`와 그 자식 `AMobaTower`·`AMobaInhibitor`·`AMobaNexus`다. 건물은 레벨에 직접 배치돼 있고 `RequiresAllDestroyed`·`RequiresAnyDestroyed`로 보호 순서를 지정한다. 웨이브·보상·경기 종료는 `AMobaGameMode`가 맡고, 미니언이 나오는 라인은 `ActiveLanes`(현재 미드만)로 정한다. 레벨·경험치·골드·스킬 포인트는 `AMobaPlayerState`에 있다. 타워와 미니언은 `UMobaCombatEvents`로 "적 챔피언이 아군 챔피언을 공격함"을 듣고 대상을 바꾼다.

아직 없는 것: 타워 피해 누적 증가, 우물 포탑, 골드를 쓰는 상점(8단계), 억제기 재생성과 슈퍼 미니언(7단계).

## 6단계. 1대1 접속

- [x] 메인 메뉴: 방 만들기 / 혼자 연습 / IP로 접속 (`L_MainMenu`, `SMobaMainMenu`)
- [x] 팀 배정과 챔피언 선택 (고정 배정: 방을 만든 쪽이 블루, 접속한 쪽이 레드, 챔피언은 `DefaultChampion`)
- [x] 접속 끊김 처리, 게임 종료 후 메뉴 복귀
- [ ] 패키징 빌드로 두 PC 간 접속 테스트 (방화벽 포트 7777)

구현 메모

- 시작 맵은 `L_MainMenu`(빈 맵)이고 `GameModeMapPrefixes`로 `MobaMenuGameMode`가 붙는다. 에디터 시작 맵은 그대로 `L_SummonersRift`라서 PIE는 메뉴를 거치지 않는다.
- 방 만들기는 `L_SummonersRift?listen?Players=2`로 연다. `Players`가 경기 시작에 필요한 인원이고, 모일 때까지 미니언과 골드 지급이 멈춰 있다. 옵션이 없으면 1명이라 PIE와 혼자 연습은 바로 시작한다.
- 정원(`MaxPlayers` 2)이 찼거나 이미 끝난 경기에는 접속이 거절되고, 사유가 접속하려던 쪽 메뉴에 표시된다.
- 경기 중 한 팀이 모두 나가면 남은 팀의 승리로 끝난다. 호스트가 나가면 접속해 있던 쪽은 안내 문구와 함께 메뉴로 돌아간다(`MobaGameInstance`).
- 접속 시도는 10초 안에 응답이 없으면 실패로 처리한다(`InitialConnectTimeout`).
- 상대 프로그램이 강제 종료돼 응답이 끊기면 20초 뒤에 끊김으로 처리한다(`ConnectionTimeout`). 정상 종료는 바로 반영된다.
- 하드웨어 레이 트레이싱은 껐다(`r.RayTracing=False`). 켠 채로 패키징하면 이 PC(RTX 4060 Laptop, 드라이버 555.97)에서 실행 직후 컴퓨트 파이프라인 생성이 실패하고 NVIDIA 드라이버 안에서 크래시했다. 에디터에서는 재현되지 않았다. 드라이버를 올린 뒤 다시 켜 볼 수 있다.

패키징

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\UnrealProjects\ver1\ver1.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="C:\UnrealProjects\ver1\Saved\Packaged" -unattended -utf8output
```

결과물은 `Saved/Packaged/Windows/`에 생긴다. 이 폴더를 통째로 상대 PC에 복사한다. MCP·툴셋 플러그인은 `TargetAllowList`로 에디터에서만 켜지므로 패키지에 들어가지 않는다.

두 PC 접속 방법

1. 호스트: `ver1.exe` 실행 → 방 만들기. 처음 실행하면 Windows 방화벽 창이 뜨는데 허용해야 한다(UDP 7777).
2. 호스트 화면에 표시되는 주소(또는 `ipconfig`의 IPv4 주소)를 상대에게 알려 준다. 같은 공유기 안이면 그 주소 그대로, 인터넷 너머라면 공유기 포트 포워딩(UDP 7777)과 공인 IP가 필요하다.
3. 상대: `ver1.exe` 실행 → 주소 입력 → 접속.

한 PC에서 창 두 개로 확인하려면 `ver1.exe -windowed -ResX=1280 -ResY=720`으로 두 번 실행하고 한쪽에서 방을 만든 뒤 다른 쪽에서 `127.0.0.1`로 접속한다.

완료 기준: 서로 다른 PC 두 대에서 접속해 한 판을 끝낸다.

## 7단계. 맵 확장

- [ ] 탑·봇 라인 웨이브와 타워 가동
- [ ] 억제기, 슈퍼 미니언, 억제기 재생성
- [ ] 정글 캠프와 재생성 타이머, 블루·레드 버프
- [ ] 바위게, 드래곤, 바론
- [ ] 부쉬 은신, 전장의 안개, 와드
- [ ] 미니맵

완료 기준: 3라인과 정글이 모두 동작하고 한 판이 정상 종료된다.

## 8단계. 완성도

- [ ] 캐릭터 메시·애니메이션 교체 (이동, 공격, 스킬, 사망)
- [ ] 투사체·스킬 VFX, 사운드
- [ ] 맵 아트 교체 (지형, 건물, 라이팅)
- [ ] 두 번째 챔피언
- [ ] 아이템 상점, 귀환
- [ ] UI 다듬기 (점수판, 킬 로그, 핑)

## 역할

- **Claude**: C++ 작성과 빌드, MCP로 블루프린트·데이터 에셋 생성, 액터 배치, 설정 변경, 커밋.
- **사용자**: 에디터 재시작, 조작감·밸런스 확인, 에셋 선택과 임포트, 두 PC 접속 테스트.
