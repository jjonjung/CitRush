# TutorialActor 설정 가이드

## 개요
`TutorialActor`는 레벨에 배치하여 튜토리얼 애니메이션과 사용 방법 위젯을 표시하는 액터입니다.

---

## 1. 기본 설정

### 1.1 Blueprint 생성
1. `TutorialActor`를 상속한 Blueprint 클래스 생성 (예: `BP_TutorialActor`)`
2. `SkeletalMeshComponent`에 Skeletal Mesh 할당
3. `Current Animation Sequence`에 재생할 애니메이션 시퀀스 할당

### 1.2 필수 컴포넌트 설정
- **Skeletal Mesh Component**: 튜토리얼 애니메이션을 표시할 메시
- **Widget Component**: 머리 위에 표시될 위젯

---

## 2. 위젯 설정 (Widget Component)

### 2.0 Tutorial Widget Blueprint 생성 (필수)

**위젯 Blueprint를 먼저 생성해야 합니다!**

#### Step 1: Tutorial Widget Blueprint 생성
1. Content Browser에서 우클릭 → `User Interface` → `Widget Blueprint` 선택
2. 부모 클래스 선택 창에서 `Tutorial Widget` 선택
3. 이름 지정 (예: `BP_TutorialWidget`)
4. 생성된 `BP_TutorialWidget` 더블클릭하여 열기

#### Step 2: 위젯 디자인
1. **Designer 탭**에서 위젯 디자인:
   - `Text Block` 추가 (멘트 표시용)
   - 원하는 UI 요소 추가 (배경, 아이콘 등)
   - 레이아웃 구성

2. **Designer 탭**에서 TextBlock 추가:
   - TextBlock을 추가하고 이름을 **`TutorialTextBlock`**으로 설정 (중요!)
   - `meta = (BindWidget)`으로 자동 바인딩되므로 이름이 정확히 일치해야 함
   - 디자인 원하는 대로 배치

3. **자동 텍스트 설정**:
   - C++ 코드에서 자동으로 `TutorialTextBlock`에 텍스트가 설정됨
   - Graph 탭에서 별도 구현 불필요!
   - 필요시 `On Message Changed` 이벤트를 오버라이드하여 추가 동작 구현 가능

#### Step 3: 멘트 배열 설정
1. `BP_TutorialWidget`의 **Details 패널**에서:
   - `Tutorial` 카테고리 → `Messages` 배열 찾기
   - `+` 버튼으로 멘트 추가

2. 각 멘트 항목 설정:
   ```
   [0]
     Message Name: "Message Index 0"
     Message Text: "WASD 키를 사용하여 이동하세요..."
   
   [1]
     Message Name: "Message Index 1"
     Message Text: "Space 키를 눌러 점프하세요..."
   ```

### 2.1 기본 위젯 설정

#### Tutorial Widget Class
- **위치**: `BP_TutorialActor` Details 패널 → `Tutorial|Widget` → `Tutorial Widget Class`
- **설명**: 머리 위에 표시할 위젯 클래스 할당
- **예시**: `BP_TutorialWidget` 할당
- **중요**: 위젯 Blueprint를 먼저 생성한 후 여기에 할당해야 함!
- **주의**: Widget Component의 Widget Class는 직접 설정하지 않아도 됩니다. `Tutorial Widget Class`만 설정하면 자동으로 연결됩니다!

#### Widget Offset
- **위치**: Details 패널 → `Tutorial|Widget` → `Widget Offset`
- **설명**: 머리 위 위젯의 상대 위치 (기본값: Z=150)
- **형식**: `(X, Y, Z)` 벡터
- **예시**: 
  - 기본: `(0, 0, 150)` - 머리 위 150cm
  - 앞쪽: `(0, 50, 150)` - 앞쪽 50cm
  - 높게: `(0, 0, 200)` - 머리 위 200cm

#### Widget Size
- **위치**: Details 패널 → `Tutorial|Widget` → `Widget Size`
- **설명**: 위젯 크기 배율 (기본값: 1.0 = 100%)
- **범위**: 0.1 ~ 10.0 (더 세밀한 조절 가능)
- **단위 설명**:
  - `0.1` = 기본 크기의 10% (51.2 x 25.6)
  - `0.5` = 기본 크기의 50% (256 x 128)
  - `1.0` = 기본 크기의 100% (512 x 256) - **기본값**
  - `2.0` = 기본 크기의 200% (1024 x 512)
  - `5.0` = 기본 크기의 500% (2560 x 1280)
  - `10.0` = 기본 크기의 1000% (5120 x 2560) - **최대값**
- **예시**:
  - 매우 작게: `0.3` (153.6 x 76.8)
  - 작게: `0.5` (256 x 128)
  - 기본: `1.0` (512 x 256)
  - 크게: `2.0` (1024 x 512)
  - 매우 크게: `5.0` (2560 x 1280)
  - 최대: `10.0` (5120 x 2560)

### 2.2 Widget Component 세부 설정

#### Widget Component 직접 접근
Blueprint에서 `Widget Component`를 직접 수정하려면:

1. **Event Graph**에서 `Get Widget Component` 노드 사용
2. **Widget Component** 노드의 출력 핀에서 설정 가능

#### 주요 설정 항목

##### Draw Size (위젯 렌더링 크기)
- **설정 방법**: `Widget Component` → `Draw Size`
- **설명**: 위젯이 렌더링되는 픽셀 크기
- **기본값**: `(512, 256)` (Width × Height)
- **권장값**:
  - 작은 위젯: `(256, 128)`
  - 기본 위젯: `(512, 256)`
  - 큰 위젯: `(1024, 512)`

**Blueprint에서 설정:**
```
Get Widget Component
  → Set Draw Size
    → Size X: 512
    → Size Y: 256
```

##### Widget Space (위젯 공간)
- **설정 방법**: `Widget Component` → `Widget Space`
- **옵션**:
  - `World`: 월드 공간 (3D 공간에서 표시, 기본값)
  - `Screen`: 스크린 공간 (화면에 고정)
- **권장**: `World` (머리 위 표시용)

**Blueprint에서 설정:**
```
Get Widget Component
  → Set Widget Space
    → Widget Space: World
```

##### Pivot (위젯 피벗)
- **설정 방법**: `Widget Component` → `Pivot`
- **설명**: 위젯의 중심점 위치 (0.0 ~ 1.0)
- **기본값**: `(0.5, 0.5)` (중앙)
- **예시**:
  - 중앙: `(0.5, 0.5)`
  - 상단 중앙: `(0.5, 0.0)`
  - 하단 중앙: `(0.5, 1.0)`

**Blueprint에서 설정:**
```
Get Widget Component
  → Set Pivot
    → Pivot: (0.5, 0.5)
```

##### Geometry Mode (지오메트리 모드)
- **설정 방법**: `Widget Component` → `Geometry Mode`
- **옵션**:
  - `Plane`: 평면 (기본값)
  - `Cylinder`: 원통형
  - `Sphere`: 구형
- **권장**: `Plane` (일반적인 경우)

##### Cylinder Arc Angle (원통형 각도)
- **설정 방법**: `Widget Component` → `Cylinder Arc Angle`
- **설명**: 원통형 모드일 때의 호 각도
- **범위**: 0 ~ 360도
- **기본값**: 180도

##### Tick Mode (틱 모드)
- **설정 방법**: `Widget Component` → `Tick Mode`
- **옵션**:
  - `Enabled`: 항상 업데이트
  - `Disabled`: 업데이트 안 함
- **권장**: `Enabled` (동적 위젯의 경우)

##### Redraw Time (재그리기 시간)
- **설정 방법**: `Widget Component` → `Redraw Time`
- **설명**: 위젯을 다시 그리는 시간 간격 (초)
- **기본값**: 0.0 (매 프레임)
- **최적화**: 값이 클수록 성능 향상, 반응성 감소

##### Two Sided (양면 렌더링)
- **설정 방법**: `Widget Component` → `Two Sided`
- **설명**: 위젯의 양면 렌더링 여부
- **기본값**: `false`
- **권장**: `true` (플레이어가 뒤에서 봐도 보이도록)

##### Background Color (배경색)
- **설정 방법**: `Widget Component` → `Background Color`
- **설명**: 위젯 배경색 (투명도 포함)
- **기본값**: `(0, 0, 0, 0)` (투명)
- **예시**: 반투명 배경 `(0, 0, 0, 128)`

##### Tint Color and Opacity (틴트 색상 및 투명도)
- **설정 방법**: `Widget Component` → `Tint Color and Opacity`
- **설명**: 위젯 전체에 적용되는 색상 틴트
- **기본값**: `(1, 1, 1, 1)` (무색, 불투명)
- **예시**: 반투명 `(1, 1, 1, 0.5)`

### 2.3 위젯 위치 및 회전 설정

#### Widget Face Player (플레이어를 바라보기)
- **위치**: Details 패널 → `Tutorial|Widget` → `Widget Face Player`
- **설명**: 위젯이 항상 플레이어를 바라보도록 회전
- **기본값**: `true`
- **권장**: `true` (머리 위 표시용)

#### Widget Visible (위젯 표시 여부)
- **위치**: Details 패널 → `Tutorial|Widget` → `Widget Visible`
- **설명**: 위젯 표시/숨김
- **기본값**: `true`

#### 런타임 위치 변경
**Blueprint에서:**
```
Get Widget Component
  → Set Relative Location
    → New Location: (0, 0, 200)  // 머리 위 200cm
```

---

## 3. 멘트 설정

### 3.1 멘트 이름으로 지정 (권장)

#### Message Name
- **위치**: Details 패널 → `Tutorial|Widget` → `Message Name`
- **설명**: 표시할 멘트의 이름
- **예시**: `"Message Index 0"`, `"Jump Tutorial"`

**사용 방법:**
1. `BP_TutorialWidget`의 `Messages` 배열에 멘트 추가
2. 각 멘트에 `Message Name` 설정 (예: "Message Index 0")
3. `TutorialActor` 인스턴스의 `Message Name`에 동일한 이름 입력

### 3.2 멘트 인덱스로 지정

#### Message Index
- **위치**: Details 패널 → `Tutorial|Widget` → `Message Index`
- **설명**: 표시할 멘트의 배열 인덱스 (0부터 시작)
- **기본값**: `0`
- **예시**: `0`, `1`, `2`

**우선순위:**
1. `Message Name`이 설정되어 있으면 → 이름으로 찾아서 표시
2. `Message Index`가 0 이상이면 → 인덱스로 표시
3. 둘 다 없으면 → `TutorialText` 직접 표시

### 3.3 직접 텍스트 설정 (기존 호환성)

#### Tutorial Text
- **위치**: Details 패널 → `Tutorial|Widget` → `Tutorial Text`
- **설명**: 위젯에 직접 표시할 텍스트
- **사용 시기**: `Message Name`과 `Message Index`가 모두 설정되지 않은 경우

---

## 4. 애니메이션 설정

### 4.1 Current Animation Sequence
- **위치**: Details 패널 → `Tutorial|Animation` → `Current Animation Sequence`
- **설명**: 재생할 애니메이션 시퀀스
- **필수**: 각 인스턴스마다 다른 애니메이션 할당 가능

### 4.2 Loop Animation
- **위치**: Details 패널 → `Tutorial|Animation` → `Loop Animation`
- **설명**: 애니메이션 루프 재생 여부
- **기본값**: `true`

### 4.3 Play Rate
- **위치**: Details 패널 → `Tutorial|Animation` → `Play Rate`
- **설명**: 재생 속도 배율 (현재는 경고만 출력, Montage 사용 권장)
- **범위**: 0.1 ~ 5.0
- **기본값**: 1.0

---

## 5. 완전한 설정 예시

### 예시 1: 기본 설정

**Step 1: 위젯 Blueprint 생성 (먼저 해야 함!)**
```
1. Content Browser에서 우클릭
   → User Interface → Widget Blueprint
   → 부모 클래스: Tutorial Widget 선택
   → 이름: BP_TutorialWidget

2. BP_TutorialWidget 열기
   → Designer 탭: TextBlock 추가
   → Graph 탭: On Message Changed 이벤트 구현
     - New Text → TextBlock의 Set Text 연결

3. Details 패널에서 Messages 배열 설정
   [0] Message Name: "Message Index 0"
       Message Text: "WASD로 이동하세요"
   [1] Message Name: "Message Index 1"
       Message Text: "Space로 점프하세요"
```

**Step 2: Tutorial Actor Blueprint 생성**
```
1. TutorialActor 상속한 Blueprint 생성: BP_TutorialActor
2. BP_TutorialActor 설정:
   ├─ Skeletal Mesh Component
   │  └─ Skeletal Mesh: Mannequin_Mesh
   ├─ Current Animation Sequence: Idle_Animation
   ├─ Tutorial Widget Class: BP_TutorialWidget  ← 위에서 만든 위젯 할당
   ├─ Message Name: "Message Index 0"  ← 위젯의 Messages에서 찾음
   ├─ Widget Offset: (0, 0, 150)
   ├─ Widget Size: 2.0
   ├─ Widget Face Player: true
   └─ Widget Visible: true
```

### 예시 2: 커스텀 위젯 크기
```
BP_TutorialActor 인스턴스:
├─ Widget Component (Blueprint에서 직접 설정)
│  ├─ Draw Size: (1024, 512)  // 큰 위젯
│  ├─ Widget Space: World
│  ├─ Pivot: (0.5, 0.0)  // 상단 중앙
│  ├─ Two Sided: true
│  └─ Tint Color and Opacity: (1, 1, 1, 0.9)  // 약간 투명
└─ Widget Offset: (0, 0, 200)  // 더 높게
```

### 예시 3: 여러 인스턴스 배치
```
레벨 배치:
├─ TutorialActor_1
│  ├─ Message Name: "Message Index 0"
│  └─ Animation: Walk_Animation
├─ TutorialActor_2
│  ├─ Message Name: "Message Index 1"
│  └─ Animation: Jump_Animation
└─ TutorialActor_3
   ├─ Message Name: "Jump Tutorial"
   └─ Animation: Jump_Animation
```

---

## 6. Blueprint에서 Widget Component 세부 설정

### 6.1 BeginPlay에서 설정
```blueprint
Event BeginPlay
  → Get Widget Component
    → Set Draw Size
      → Size X: 1024
      → Size Y: 512
    → Set Widget Space
      → Widget Space: World
    → Set Pivot
      → Pivot: (0.5, 0.5)
    → Set Two Sided
      → Two Sided: true
```

### 6.2 런타임 위치 변경
```blueprint
Custom Event: Change Widget Height
  → Get Widget Component
    → Set Relative Location
      → New Location: (0, 0, 200)  // 높이 변경
```

### 6.3 위젯 색상 변경
```blueprint
Custom Event: Set Widget Color
  → Get Widget Component
    → Set Tint Color and Opacity
      → Tint Color and Opacity: (1, 0.5, 0.5, 1)  // 빨간색 틴트
```

---

## 7. 문제 해결

### 위젯이 보이지 않을 때
1. `Widget Visible`이 `true`인지 확인
2. `Tutorial Widget Class`가 할당되었는지 확인
3. `Widget Component`의 `Draw Size`가 0이 아닌지 확인
4. 카메라가 위젯을 바라보고 있는지 확인 (`Two Sided` 활성화 권장)

### 위젯 위치가 이상할 때
1. `Widget Offset` 값 확인
2. `Widget Component`의 `Pivot` 설정 확인
3. `Widget Face Player`가 활성화되어 있으면 회전에 영향

### 멘트가 표시되지 않을 때
1. `Message Name` 또는 `Message Index` 설정 확인
2. `BP_TutorialWidget`의 `Messages` 배열에 해당 멘트가 있는지 확인
3. `Message Name`의 대소문자 및 공백 확인

---

## 8. 최적화 팁

1. **Redraw Time 설정**: 동적 위젯이 아닌 경우 `Redraw Time`을 증가시켜 성능 향상
2. **Draw Size 최적화**: 필요한 만큼만 크게 설정
3. **Two Sided 비활성화**: 한쪽에서만 보이는 경우 성능 향상
4. **Widget Space**: `Screen` 모드는 성능이 좋지만 3D 효과 없음

---

## 9. 참고 사항

- `Widget Component`는 `SkeletalMeshComponent`에 Attach되어 있음
- 위젯은 기본적으로 플레이어를 바라보도록 회전 (0.1초마다 업데이트)
- 멘트는 `BP_TutorialWidget`의 `Messages` 배열에서 관리
- 각 인스턴스마다 다른 애니메이션과 멘트 설정 가능

