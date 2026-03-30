# CCTV 시스템 문서

## 개요

CCTV 시스템은 Commander가 레이서와 Enemy의 카메라 뷰를 모니터링할 수 있도록 하는 시스템입니다. 월드에 배치된 MonitorActor를 통해 CCTV UI를 열고, 각 타겟의 카메라를 실시간으로 확인할 수 있습니다.

## 시스템 아키텍처

### 컴포넌트 구조

```
MonitorActor (월드에 배치된 Actor)
├── CCTVFeedComponent (CCTV Feed 관리)
│   ├── FeedSlots[4] (레이서 3명 + Enemy 1명)
│   │   ├── SceneCaptureComponent2D (렌더링)
│   │   ├── TextureRenderTarget2D (렌더 타겟)
│   │   ├── TargetPawn (타겟 Pawn)
│   │   └── CurrentCameraSlot (현재 카메라 슬롯 0~2)
│   └── FocusIndex (현재 포커스된 Feed 인덱스)
└── InteractableComponent (상호작용)

Racer/Enemy (타겟 Pawn)
├── CCTVCameraComponent (CCTV 카메라 관리)
│   └── GetCCTVCamera(SlotIndex) (카메라 가져오기)
└── Camera Components (실제 카메라)
    ├── SpringArm (레이서: Front/Back/Side, Enemy: 단일)
    └── Camera (SpringArm에 Attach)

PixelEnemy (Enemy 전용)
├── SpringArm (bInheritYaw = true로 회전 상속)
├── Camera (SpringArm에 Attach)
├── SceneCaptureComponent2D (CCTV용, Camera에 Attach)
└── CCTVRenderTarget (Blueprint에서 설정)
```

## 주요 컴포넌트

### 1. MonitorActor

월드에 배치되는 CCTV 모니터 Actor입니다.

**위치**: `Source/UE_CITRUSH/Public/Player/CCTV/MonitorActor.h`

**주요 기능**:
- Commander가 F키로 상호작용하여 CCTV UI 열기/닫기
- CCTVFeedComponent를 포함하여 Feed 관리
- InteractableComponent를 통한 상호작용 처리

**주요 함수**:
- `ToggleCCTV()`: CCTV UI 토글
- `OpenCCTV()`: CCTV UI 열기
- `CloseCCTV()`: CCTV UI 닫기
- `GetCCTVFeedComponent()`: CCTVFeedComponent 가져오기

### 2. CCTVFeedComponent

CCTV Feed를 관리하는 핵심 컴포넌트입니다.

**위치**: `Source/UE_CITRUSH/Public/Player/CCTV/CCTVFeedComponent.h`

**주요 기능**:
- 레이서 3명과 Enemy 1명의 카메라 Feed 관리 (총 4개 슬롯)
- 포커스 이동 (Q/E 키)
- 카메라 슬롯 전환 (각 Feed당 0~2 슬롯)
- 확대/복귀 토글 (T 키)
- 성능 최적화 (포커스된 Feed만 실시간 업데이트)

**주요 함수**:
- `InitializeFeeds()`: Feed 초기화 (레이서와 Enemy 찾기)
- `MoveFocus(bool bNext)`: 포커스 이동 (Q/E)
- `SwitchCameraSlot(bool bNext)`: 카메라 슬롯 전환
- `ToggleExpand()`: 확대/복귀 토글 (T)
- `SetCCTVUIActive(bool bActive)`: CCTV UI 활성화 상태 설정

**Feed 슬롯 구조**:
```cpp
struct FCCTVFeedSlot
{
    USceneCaptureComponent2D* SceneCapture;  // 렌더링 컴포넌트
    UTextureRenderTarget2D* RenderTarget;    // 렌더 타겟
    APawn* TargetPawn;                       // 타겟 Pawn
    int32 CurrentCameraSlot;                 // 현재 카메라 슬롯 (0~2)
};
```

**초기화 흐름**:
1. 레이서 3명 찾기 (Player Index 순서대로)
2. 각 레이서의 기존 SceneCaptureComponent 확인
   - 있으면: 기존 RenderTarget 사용
   - 없으면: 새 SceneCaptureComponent 생성 및 RenderTarget 생성
3. Enemy 찾기 (PixelEnemyBPClass 필터 적용)
   - CCTVRenderTarget이 있으면 사용
   - 없으면 새로 생성
4. 각 Feed의 SceneCapture를 타겟 카메라에 Attach

### 3. CCTVCameraComponent

타겟 Pawn에 붙는 CCTV 카메라 관리 컴포넌트입니다.

**위치**: `Source/UE_CITRUSH/Public/Player/CCTV/CCTVCameraComponent.h`

**주요 기능**:
- Pawn의 카메라를 SlotIndex로 가져오기
- SpringArm을 자동으로 찾아서 카메라 연결
- 레이서와 Enemy에 따라 다른 카메라 반환

**카메라 슬롯 매핑**:
- **레이서 (UE_CITRUSHPawn)**:
  - Slot 0: BackSpringArm의 Camera
  - Slot 1: FrontSpringArm의 Camera
  - Slot 2: Player Index에 따라 Left/Right/BackSide SpringArm의 Camera
- **Enemy (PixelEnemy)**:
  - 모든 Slot: 단일 Camera 반환 (SpringArm에 Attach된 Camera)

**주요 함수**:
- `GetCCTVCamera(int32 SlotIndex)`: CCTV 카메라 가져오기
- `AutoSetupCameras()`: BeginPlay에서 자동으로 카메라 설정

### 4. PixelEnemy (CCTV 관련)

Enemy의 CCTV 설정입니다.

**위치**: `Source/UE_CITRUSH/Public/Enemy/PixelEnemy.h`

**CCTV 관련 컴포넌트**:
- `SpringArm`: Camera를 위한 SpringArm (bInheritYaw = true로 Enemy 회전 상속)
- `Camera`: CCTV용 Camera (SpringArm에 Attach)
- `SceneCaptureComponent`: CCTV 렌더링용 (Camera에 Attach)
- `CCTVRenderTarget`: Blueprint에서 설정하는 RenderTarget
- `CCTVCameraComponent`: CCTV 카메라 관리 컴포넌트

**주요 설정**:
- SpringArm의 `bInheritYaw = true`: Enemy 회전 시 Camera도 회전
- SceneCaptureComponent가 Camera에 Attach되어 Transform 자동 동기화
- PostProcessSettings: Auto Exposure 활성화로 밝기 조정

**주요 함수**:
- `CaptureCCTVScene()`: CCTV 씬 캡처 (타이머로 주기적 호출)
- `GetCCTVCamera(int32 SlotIndex)`: CCTV 카메라 가져오기

## 동작 흐름

### 1. 초기화 단계

```
BeginPlay
├── MonitorActor::BeginPlay()
│   └── CCTVFeedComponent 초기화 대기
│
├── CCTVFeedComponent::InitializeFeeds() (외부에서 호출)
│   ├── 레이서 3명 찾기 (Player Index 순서)
│   ├── 각 레이서의 SceneCaptureComponent 확인
│   │   ├── 있으면: 기존 RenderTarget 사용
│   │   └── 없으면: 새로 생성
│   ├── Enemy 찾기 (PixelEnemy)
│   │   ├── CCTVRenderTarget 확인
│   │   └── SceneCaptureComponent 생성
│   └── UpdateFeedSlot() 호출하여 카메라 Attach
│
└── CCTVCameraComponent::BeginPlay() (각 Pawn에서)
    └── AutoSetupCameras() (카메라 자동 설정)
```

### 2. CCTV UI 열기

```
Commander가 MonitorActor와 상호작용 (F키)
├── MonitorActor::ToggleCCTV()
│   └── CCTVFeedComponent::SetCCTVUIActive(true)
│       ├── 모든 슬롯에 대해 ForceCaptureSlot() 호출
│       └── UpdateCaptureSettings() 호출
│
└── CCTV UI 위젯 생성 및 표시
    └── 각 Feed의 RenderTarget을 UI에 바인딩
```

### 3. 실시간 업데이트

```
타이머 (CCTVSyncInterval, 기본 0.033초)
└── PixelEnemy::CaptureCCTVScene()
    ├── Camera Transform 확인 (Attach 관계 유지)
    ├── FOV, ProjectionType 동기화
    ├── PostProcessSettings 동기화
    └── SceneCaptureComponent->CaptureScene()
```

### 4. 포커스 이동 (Q/E)

```
Q/E 키 입력
└── CCTVFeedComponent::MoveFocus(bool bNext)
    ├── FocusIndex 변경
    ├── UpdateCaptureSettings() 호출
    │   └── 포커스된 Feed만 실시간 업데이트 활성화
    └── OnFocusIndexChanged 델리게이트 브로드캐스트
```

### 5. 카메라 슬롯 전환

```
카메라 슬롯 전환 입력
└── CCTVFeedComponent::SwitchCameraSlot(bool bNext)
    ├── CurrentCameraSlot 변경
    ├── UpdateFeedSlot() 호출
    │   └── SceneCapture를 새로운 카메라에 Attach
    └── OnCameraSlotChanged 델리게이트 브로드캐스트
```

## 렌더링 설정

### SceneCaptureComponent 설정

**ShowFlags**:
- `SetGame(true)`: 게임 모드 활성화
- `SetMaterials(true)`: 머티리얼 렌더링
- `SetLighting(true)`: 라이팅 활성화
- `SetPostProcessing(true)`: 포스트 프로세싱 활성화
- `SetBounds(false)`: 바운딩 박스 비활성화
- `SetCollision(false)`: 충돌 표시 비활성화
- `SetFog(false)`: 안개 비활성화

**CaptureSource**: `SCS_FinalColorLDR` (최종 컬러 LDR)

**PostProcessSettings**:
- `AutoExposureMethod`: `AEM_Manual` (고정 노출 사용)
- `AutoExposureBias`: `1.0f` (밝기 조정 값, 0.2 단위로 튜닝 가능)
- `AutoExposureMinBrightness`: `1.0f` (고정)
- `AutoExposureMaxBrightness`: `1.0f` (고정)
- **중요**: CCTV는 "연출 카메라"이므로 고정 노출이 가장 안정적입니다.
  - Auto Exposure는 Game Camera의 히스토리를 공유하지 않아 어두워지는 문제 발생
  - Manual Exposure로 고정하면 밤/실내/역광에서도 안정적이고 "모니터로 보는 느낌"에 잘 맞음
  - MinBrightness/MaxBrightness를 1.0으로 고정하여 남아있는 적응 효과를 원천 차단

**밝기 조정 방법**:
- 너무 어두움 → `AutoExposureBias`를 +0.2씩 올림 (1.0 → 1.2 → 1.4 …)
- 너무 밝음 → `AutoExposureBias`를 -0.2씩 내림 (1.0 → 0.8 → 0.6 …)

### RenderTarget 설정

**포맷**: `RTF_RGBA8` (RGBA 8비트)
**기본 해상도**: 1024x576 (16:9 비율)
**ClearColor**: `(0, 0, 0, 1)` (검은색, 알파 1)

## 성능 최적화

### 포커스 기반 업데이트

`bUsePerformanceOptimization = true`일 때:
- 포커스된 Feed만 `bCaptureEveryFrame = true`
- 포커스되지 않은 Feed는 저주기 업데이트 (`LowFrequencyUpdateInterval`)

### CCTV UI 활성화 상태

- CCTV UI가 열려있을 때만 SceneCapture 활성화
- UI가 닫히면 SceneCapture 비활성화하여 성능 절약

## Enemy 카메라 회전 동기화

### 설정

```cpp
// PixelEnemy 생성자
SpringArm->bInheritYaw = true;  // Enemy 회전 시 Camera도 회전

// BeginPlay
SceneCaptureComponent->AttachToComponent(Camera, 
    FAttachmentTransformRules::SnapToTargetIncludingScale);
```

### 동작 원리

1. Enemy가 회전하면 → SpringArm이 Yaw 회전 상속 (`bInheritYaw = true`)
2. SpringArm이 회전하면 → Camera가 회전 (Attach 관계)
3. Camera가 회전하면 → SceneCaptureComponent가 회전 (Attach 관계)
4. `CaptureCCTVScene()`에서 Transform을 강제로 설정하지 않아 Attach 관계 유지

**⚠️ 핵심 원칙**:
- SceneCapture는 절대 직접 Transform을 만지지 않음
- Attach는 BeginPlay에서 단 한 번만 수행
- 이후에는 Attach 관계를 통해 자동으로 동기화됨
- `SetWorldTransform()`을 호출하면 Attach 관계가 깨져서 위치가 어긋남

## 주요 이슈 및 해결

### 1. 어두운 화면 문제

**원인**: 
- SceneCaptureComponent2D는 Game Camera의 Auto Exposure 히스토리를 공유하지 않음
- 실제 화면은 노출이 안정된 상태이지만, CCTV는 매 프레임 "처음 보는 화면"처럼 계산되어 어둡게 표시됨

**해결**:
- `AutoExposureMethod`를 `AEM_Manual`로 설정 (고정 노출 사용)
- `AutoExposureBias`를 `1.0f`로 설정 (0.2 단위로 튜닝)
- `AutoExposureMinBrightness`와 `MaxBrightness`를 `1.0f`로 고정하여 남아있는 적응 효과 차단
- CCTV는 "연출 카메라"이므로 고정 노출이 가장 안정적이며, 밤/실내/역광에서도 일관된 밝기 유지

**추가 진단 방법**:
- 여전히 밝기가 다르면 `CaptureSource`를 `SCS_FinalColorHDR`로 변경하여 비교
  - HDR로 바꿨는데 더 비슷해지면 → LDR 톤매핑 차이가 원인
  - HDR로 바꿨는데 더 이상해지면 → RT 감마/머티리얼 표시 방식 문제
- UI에서만 어두우면 → UMG/브러시 감마 중복 문제 의심

### 2. 카메라 위치 불일치

**원인**: 
- `CaptureCCTVScene()`에서 `SetWorldTransform`을 호출하여 Attach 관계가 깨짐
- Attach 관계가 한 번이라도 깨지면 SpringArm → Camera → SceneCapture 체인이 미세하게 어긋남
- Enemy는 AI 회전이 Tick 기반이므로 회전 타이밍 + 캡처 타이밍 불일치 발생

**해결**:
- **핵심 원칙**: SceneCapture는 절대 직접 Transform을 만지지 않음
  - ❌ 절대 사용하지 말 것: `SceneCaptureComponent->SetWorldTransform(...)`
- Attach는 BeginPlay에서 단 한 번만 수행
- `CaptureCCTVScene()`에서는 Attach 복구 로직을 디버그용으로만 유지 (정상 동작 시에는 필요 없어야 함)
- Transform은 Attach 관계를 통해 자동으로 동기화되도록 유지

### 3. 카메라 회전 미반영

**원인**: SpringArm의 `bInheritYaw = false`

**해결**:
- `bInheritYaw = true`로 설정하여 Enemy 회전 시 Camera도 회전

## 파일 구조

```
Source/UE_CITRUSH/
├── Public/Player/CCTV/
│   ├── CCTVFeedComponent.h          # CCTV Feed 관리 컴포넌트
│   ├── CCTVCameraComponent.h       # CCTV 카메라 관리 컴포넌트
│   └── MonitorActor.h              # CCTV 모니터 Actor
├── Private/Player/CCTV/
│   ├── CCTVFeedComponent.cpp
│   └── CCTVCameraComponent.cpp
└── Private/Enemy/
    └── PixelEnemy.cpp               # Enemy CCTV 설정
```

## 사용 예시

### Blueprint에서 설정

1. **MonitorActor 배치**:
   - 월드에 MonitorActor 배치
   - CCTVFeedComponent의 `PixelEnemyBPClass` 설정 (선택사항)

2. **PixelEnemy 설정**:
   - `CCTVRenderTarget` 프로퍼티에 RenderTarget 할당
   - `CCTVCameraComponent` 컴포넌트 확인

3. **CCTV UI 위젯**:
   - CCTVFeedComponent의 `GetRenderTarget()` 함수로 RenderTarget 가져오기
   - Image 위젯에 RenderTarget 바인딩

### C++에서 사용

```cpp
// CCTV Feed 초기화
if (UCCTVFeedComponent* CCTVFeed = MonitorActor->GetCCTVFeedComponent())
{
    CCTVFeed->InitializeFeeds();
}

// 포커스 이동
CCTVFeed->MoveFocus(true);  // 다음 Feed

// 카메라 슬롯 전환
CCTVFeed->SwitchCameraSlot(true);  // 다음 슬롯

// 확대 토글
CCTVFeed->ToggleExpand();
```

## 로그

모든 CCTV 관련 로그는 `LogCCTVFeed` 카테고리로 통일되어 있으며, `[CCTVLog]` 태그가 붙습니다.

**로그 레벨**:
- `Log`: 일반 정보
- `Warning`: 경고 (예: Enemy를 찾을 수 없음)
- `Error`: 오류 (예: RenderTarget 생성 실패)

## 참고사항

- CCTV UI가 열려있을 때만 SceneCapture가 활성화되어 성능을 절약합니다.
- Enemy의 Camera는 SpringArm에 Attach되어 있어 Enemy 회전 시 자동으로 회전합니다.
- RenderTarget 해상도는 VRAM 절약을 위해 기본값이 1024x576으로 설정되어 있습니다.
- 포커스 기반 업데이트를 사용하면 포커스되지 않은 Feed는 저주기 업데이트됩니다.

## 핵심 설계 원칙

### 1. SceneCapture Transform 관리

**✅ 올바른 방법**:
- BeginPlay에서 한 번만 Attach: `AttachToComponent(Camera, SnapToTargetIncludingScale)`
- 이후에는 Attach 관계를 통해 자동 동기화
- `CaptureCCTVScene()`에서는 FOV와 ProjectionType만 업데이트

**❌ 절대 하지 말 것**:
- `SetWorldTransform()` 호출 (Attach 관계가 깨짐)
- 매 프레임 Attach 재설정
- Transform을 직접 계산하여 설정

### 2. PostProcessSettings 관리

**✅ 올바른 방법**:
- Manual Exposure 사용 (`AEM_Manual`)
- 고정 노출 값 사용 (`AutoExposureBias = 1.3f`)
- BeginPlay에서 한 번만 설정

**❌ 피해야 할 것**:
- Auto Exposure (`AEM_Histogram`) - Game Camera 히스토리 공유 안 됨
- 매 프레임 PostProcessSettings 재설정
- MinBrightness/MaxBrightness 조정 (Manual에서는 불필요)

### 3. Attach 관계 유지

**정상 동작 시**:
- BeginPlay에서 Attach 한 번만 수행
- `CaptureCCTVScene()`에서 Attach 복구 로직은 실행되지 않아야 함

**디버그/안전장치**:
- Attach 관계 확인 로직은 유지하되, 정상 동작 시에는 실행되지 않아야 함
- Attach 복구가 발생하면 Warning 로그 출력하여 문제 감지

