# 시나리오 기반 Mock Data 사용 가이드

## 📋 개요

이 CSV 파일들은 **AI 서버 없이도 게임 플레이 흐름을 테스트**할 수 있도록 설계된 Mock Data입니다.

### 목적
1. ✅ **시나리오 기반 전체 플레이 흐름 테스트** (주 목적)

---

## 📁 파일 구조

```
AgentLog/CSV_Samples/
├── DT_GameScenarios.csv          # 시나리오 정의 (전체 흐름)
├── DT_ScenarioSteps.csv          # 시나리오 스텝 (각 단계)
├── DT_HttpRequestTemplates.csv  # Request 템플릿
├── DT_HttpResponseSamples.csv   # Mock Response 샘플
└── README_시나리오_사용_가이드.md
```

---

## 🎮 제공되는 시나리오

### 1. **Scenario_EarlyGameNormal** (초반전 - 정상 진행)
- **설명**: 매치 시작 후 적 탐색 단계
- **난이도**: NORMAL
- **예상 시간**: 90초
- **흐름**:
  1. 매치 시작
  2. AI 의사결정 #1 (안전한 상태, 적 탐색)
  3. AI 의사결정 #2 (적 발견, 경계 태세)

### 2. **Scenario_MidGameCrisis** (중반전 - 위기 상황)
- **설명**: 적이 Driver1을 추격, 체력 낮음 경고 발생
- **난이도**: HARD
- **예상 시간**: 60초
- **흐름**:
  1. AI 의사결정 #3 (Driver1 추격당함)
  2. Overseer TTS #1 (체력 낮음 경고)
  3. AI 의사결정 #4 (회피 기동)
  4. AI 의사결정 #5 (안전 지역 도착)

### 3. **Scenario_LateGameVictory** (후반전 - 승리)
- **설명**: 적 체력 낮음, 최종 공격으로 승리
- **난이도**: NORMAL
- **예상 시간**: 45초
- **흐름**:
  1. AI 의사결정 #6 (적 체력 낮음 확인)
  2. AI 의사결정 #7 (최종 공격 개시)
  3. 매치 종료 (승리)
  4. 지휘관 리포트 조회

### 4. **Scenario_FullMatch** (전체 매치 - 정상 승리)
- **설명**: 매치 시작부터 종료까지 전체 플레이 흐름
- **난이도**: NORMAL
- **예상 시간**: 300초 (5분)
- **흐름**: 초반 → 중반 → 후반 → 승리 (총 9단계)

### 5. **Scenario_QuickDefeat** (빠른 패배 시나리오)
- **설명**: 초반에 플레이어들이 빠르게 제압당함 (테스트용)
- **난이도**: HARD
- **예상 시간**: 30초

### 6. **Scenario_MockOnly** (Mock 전용 테스트)
- **설명**: AI 서버 없이 완전 Mock 데이터로만 진행
- **난이도**: EASY
- **예상 시간**: 60초
- **특징**: `bForceMockMode = true` (항상 Mock Response 사용)

---

## 🔧 DataTable 생성 방법

### 1. 언리얼 에디터에서 DataTable 생성

#### Step 1: API 엔드포인트 설정
```
Content Browser → 우클릭 → Miscellaneous → Data Table
Row Structure: FHttpApiEndpointRow
이름: DT_HttpApiEndpoints
```

#### Step 2: Request 템플릿
```
Row Structure: FHttpRequestTemplateRow
이름: DT_HttpRequestTemplates
```

#### Step 3: Response 샘플
```
Row Structure: FHttpResponseSampleRow
이름: DT_HttpResponseSamples
```

#### Step 4: 시나리오 스텝
```
Row Structure: FScenarioStepRow
이름: DT_ScenarioSteps
```

#### Step 5: 게임 시나리오
```
Row Structure: FGameScenarioRow
이름: DT_GameScenarios
```

### 2. CSV Import

각 DataTable을 생성한 후:
1. DataTable 에셋 우클릭 → **Reimport**
2. 해당 CSV 파일 선택 (예: `DT_GameScenarios.csv`)
3. Import 완료!

---

## 💻 코드에서 사용하는 방법

### 시나리오 기반 흐름 예시 (Pseudo Code)

```cpp
// 1. 시나리오 로드
FGameScenarioRow* Scenario = GameScenariosTable->FindRow<FGameScenarioRow>(
    TEXT("Scenario_FullMatch"), TEXT(""));

// 2. 각 스텝 순차 실행
for (FName StepRowName : Scenario->StepRowNames)
{
    FScenarioStepRow* Step = ScenarioStepsTable->FindRow<FScenarioStepRow>(
        StepRowName, TEXT(""));

    // 3. 대기 시간
    if (Step->DelayBeforeExecutionSeconds > 0.0f)
    {
        Wait(Step->DelayBeforeExecutionSeconds);
    }

    // 4. API 호출 또는 Mock 사용
    if (Step->bForceMockMode || !IsServerConnected())
    {
        // Mock Response 사용
        FHttpResponseSampleRow* MockResponse =
            ResponseSamplesTable->FindRow<FHttpResponseSampleRow>(
                Step->MockResponseSampleRowName, TEXT(""));

        ProcessMockResponse(MockResponse->ResponseJsonSample);
    }
    else
    {
        // 실제 서버 호출
        FHttpRequestTemplateRow* RequestTemplate =
            RequestTemplatesTable->FindRow<FHttpRequestTemplateRow>(
                Step->RequestTemplateRowName, TEXT(""));

        SendHttpRequest(RequestTemplate->RequestJsonTemplate);
    }
}
```

### Mock 모드 vs 서버 모드

```cpp
// Mock 모드 전용 (서버 호출 안 함)
if (Step->bForceMockMode)
{
    UseMockResponse();
}

// 하이브리드 모드 (서버 연결 시 실제 호출, 실패 시 Mock)
else
{
    if (IsServerConnected())
    {
        TryRealServerCall_OnFailUseMock();
    }
    else
    {
        UseMockResponse();
    }
}
```

---

## 🎯 사용 시나리오별 추천

### 개발 초기 단계
- **Scenario_MockOnly** 사용
- AI 서버 없이 게임 로직 개발
- `bForceMockMode = true`로 완전 오프라인

### 통합 테스트 단계
- **Scenario_EarlyGameNormal** → **Scenario_MidGameCrisis** → **Scenario_LateGameVictory** 순차 실행
- 각 게임 페이즈별 테스트

### 전체 플레이 테스트
- **Scenario_FullMatch** 사용
- 매치 시작부터 종료까지 완전한 흐름 검증

### 엣지 케이스 테스트
- **Scenario_QuickDefeat** 사용
- 패배 시나리오 테스트
- 에러 처리 검증

---

## 📊 데이터 구조 이해하기

### FGameScenarioRow (시나리오 정의)
```
Scenario_FullMatch
  ├─ StepRowNames: [Step_Full_01, Step_Full_02, ...]
  ├─ GamePhase: FULL_MATCH
  └─ ExpectedDurationSeconds: 300.0
```

### FScenarioStepRow (시나리오 스텝)
```
Step_Full_01
  ├─ ApiEndpointRowName: MatchStart_V1_2
  ├─ RequestTemplateRowName: Req_MatchStart_Normal
  ├─ MockResponseSampleRowName: Res_MatchStart_Success
  ├─ DelayBeforeExecutionSeconds: 0.0
  └─ bForceMockMode: false
```

### FHttpRequestTemplateRow (Request 템플릿)
```
Req_MatchStart_Normal
  ├─ ApiEndpointRowName: MatchStart_V1_2
  └─ RequestJsonTemplate: { room_id: "test_room_001", ... }
```

### FHttpResponseSampleRow (Mock Response)
```
Res_MatchStart_Success
  ├─ ApiEndpointRowName: MatchStart_V1_2
  ├─ ResponseJsonSample: { status: "success", ... }
  └─ HttpStatusCode: 200
```

---

## ⚙️ 커스터마이징 가이드

### 새로운 시나리오 추가하기

#### 1. Request 템플릿 추가 (DT_HttpRequestTemplates.csv)
```csv
Name,TemplateName,Description,ApiEndpointRowName,RequestJsonTemplate,bEnabled,Tags
Req_Custom_MyTest,"내 테스트","커스텀 테스트",GetDecision_V1_2,"{...}",true,"(Custom,Test)"
```

#### 2. Response 샘플 추가 (DT_HttpResponseSamples.csv)
```csv
Name,SampleName,Description,ApiEndpointRowName,ResponseJsonSample,HttpStatusCode,bIsSuccess,bEnabled,Tags
Res_Custom_MyTest,"내 응답","커스텀 응답",GetDecision_V1_2,"{...}",200,true,true,"(Custom,Test)"
```

#### 3. 스텝 추가 (DT_ScenarioSteps.csv)
```csv
Name,StepName,Description,ApiEndpointRowName,RequestTemplateRowName,MockResponseSampleRowName,...
Step_Custom_01,"내 스텝","커스텀 스텝",GetDecision_V1_2,Req_Custom_MyTest,Res_Custom_MyTest,0.0,false,true,true
```

#### 4. 시나리오 추가 (DT_GameScenarios.csv)
```csv
Name,ScenarioName,Description,GamePhase,ExpectedDurationSeconds,Difficulty,StepRowNames,bEnabled,Tags
Scenario_Custom,"내 시나리오","커스텀",EARLY_GAME,60.0,NORMAL,"(Step_Custom_01,Step_Custom_02)",true,"(Custom)"
```

### Request JSON 커스터마이징

`DT_HttpRequestTemplates.csv`의 `RequestJsonTemplate` 컬럼에서 JSON 수정:

```json
{
  "room_id": "my_custom_room",
  "global_context": {
    "remaining_time": 120.0,
    "game_phase": "EARLY_GAME"
  },
  ...
}
```

### Response JSON 커스터마이징

`DT_HttpResponseSamples.csv`의 `ResponseJsonSample` 컬럼에서 JSON 수정:

```json
{
  "status": "success",
  "decision": {
    "squad_objective": "MY_CUSTOM_OBJECTIVE",
    ...
  }
}
```

---

## 🐛 트러블슈팅

### CSV Import 실패
**증상**: CSV 파일을 Import할 때 에러 발생

**해결**:
1. CSV 파일 인코딩을 **UTF-8**로 변경
2. Excel에서 열었다면 쉼표(`,`) 구분자 확인
3. 큰따옴표(`"`) 이스케이프 확인

### Mock Response가 적용되지 않음
**증상**: `bForceMockMode = true`인데 실제 서버 호출

**해결**:
1. `FScenarioStepRow`의 `bForceMockMode` 필드 확인
2. CSV Reimport 후 에디터 재시작
3. DataTable 캐시 클리어

### 시나리오 스텝 순서가 이상함
**증상**: 스텝이 의도한 순서대로 실행되지 않음

**해결**:
1. `FGameScenarioRow`의 `StepRowNames` 배열 순서 확인
2. CSV에서 배열은 `(Item1,Item2,Item3)` 형식
3. 공백 없이 정확히 입력

---

## 📝 체크리스트

DataTable 설정 완료 확인:

- [ ] `DT_HttpApiEndpoints` 생성 및 CSV Import
- [ ] `DT_HttpRequestTemplates` 생성 및 CSV Import
- [ ] `DT_HttpResponseSamples` 생성 및 CSV Import
- [ ] `DT_ScenarioSteps` 생성 및 CSV Import
- [ ] `DT_GameScenarios` 생성 및 CSV Import

코드 구현 확인:

- [ ] 시나리오 로드 코드 구현
- [ ] 스텝 순차 실행 코드 구현
- [ ] Mock Response 파싱 코드 구현
- [ ] 서버/Mock 모드 전환 로직 구현

테스트 확인:

- [ ] `Scenario_MockOnly` 실행 성공 (완전 오프라인)
- [ ] `Scenario_EarlyGameNormal` 실행 성공
- [ ] `Scenario_FullMatch` 실행 성공 (전체 흐름)
- [ ] 에러 시나리오 테스트 (Timeout, ServerError)

---

## 🎓 다음 단계

1. **Blueprint 통합**
   - Blueprint Function Library 만들기
   - 시나리오 실행 노드 만들기

2. **UI 개발**
   - 시나리오 선택 UI
   - 스텝 진행 상황 표시
   - Mock/Server 모드 토글

3. **자동화 테스트**
   - 모든 시나리오 자동 실행
   - 결과 검증 및 리포트 생성

4. **추가 시나리오 작성**
   - 팀 플레이 시나리오
   - 아이템 사용 시나리오
   - 네트워크 에러 시나리오

---

## 📚 참고 문서

- `AgentLog/ConstructDataTable.md` - DataTable 구축 가이드
- `CITRUSH-UE/Source/UE_CITRUSH/Public/Data/CitRushHttpData.h` - 구조체 정의
- `AgentLog/Protocol.md` - HTTP API v1.1 프로토콜
- `AgentLog/[AI 에이전트]언리얼팀 통합 연동 가이드_v1.2.0` - HTTP API v1.2.0 프로토콜

---

**작성일**: 2025-12-17
**버전**: 1.0
**작성자**: Claude (CITRUSH Project)
