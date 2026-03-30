# Network Schemas

AI Agent 서버와의 통신을 위한 HTTP 및 WebSocket 스키마 정의 폴더입니다.

## 📂 폴더 구조

```
Schemas/
├── README.md                    # 이 문서
├── NPCSchemas.h                 # NPC 관련 스키마 (레거시)
├── HttpV1/                      # HTTP API 스키마
│   ├── RequestHttp.h           # HTTP 요청 구조체 v1.1
│   ├── ResponseHttp.h          # HTTP 응답 구조체 v1.1
│   ├── HttpRequest2.h          # HTTP 요청 구조체 v1.2.0 ✨ NEW
│   ├── HttpRequest2.cpp        # 구현 파일
│   ├── HttpResponse2.h         # HTTP 응답 구조체 v1.2.0 ✨ NEW
│   └── HttpResponse2.cpp       # 구현 파일
├── WebSocketV1/                 # WebSocket API 스키마
│   ├── SendWebSocket.h         # WebSocket 전송 메시지 v1.1
│   ├── ReceiveWebSocket.h      # WebSocket 수신 메시지 v1.1
│   ├── SendWebSocket2.h        # WebSocket 전송 메시지 v1.2.0 ✨ NEW
│   ├── SendWebSocket2.cpp      # 구현 파일
│   ├── ReceiveWebSocket2.h     # WebSocket 수신 메시지 v1.2.0 ✨ NEW
│   └── ReceiveWebSocket2.cpp   # 구현 파일
└── JsonExamples/               # JSON 예시 파일들
```

---

## 📡 HTTP API v1.1 (`HttpV1/`)

AI 서버와의 **HTTP 통신**을 위한 요청/응답 구조체입니다.

### 📄 RequestHttp.h

**AI 서버로 전송할 HTTP Request 구조체**

#### 주요 구조체

| 구조체 | 설명 | 엔드포인트 |
|--------|------|------------|
| `FMatchStartRequest` | 매치 시작 요청 | `POST /api/v1/match/start` |
| `FMatchEndRequest` | 매치 종료 요청 | `POST /api/v1/match/end` |
| `FGetDecisionRequest` | **의사결정 요청 (핵심)** | `POST /api/v1/get_decision` |

#### 공통 구조체

- `FWebVectorRequest`: 3D 위치 벡터 (x, y, z) - Request 전용
- `FWebVelocityRequest`: 3D 속도 벡터 (x, y, z) - Request 전용
- `FGlobalContext`: 전역 게임 컨텍스트 (남은 시간, 경기 단계 등)
- `FAISquadContext`: AI 스쿼드 컨텍스트 (적 본체 + 미니언)
- `FPlayerTeamContext`: 플레이어/팀 컨텍스트
- `FCommanderContext`: 지휘관 컨텍스트
- `FMapContext`: 맵 컨텍스트 (P-Pellet 위치 등)

### 📄 ResponseHttp.h

**AI 서버로부터 수신할 HTTP Response 구조체**

#### 주요 구조체

| 구조체 | 설명 | 상태 코드 |
|--------|------|-----------|
| `FMatchStartResponse` | 매치 시작 응답 | 200 |
| `FMatchEndResponse` | 매치 종료 응답 | 200 |
| `FGetDecisionResponse` | **의사결정 응답 (핵심)** | 200 |
| `FHealthResponse` | 헬스체크 응답 | 200 |
| `FFailedResponse` | 실패 응답 | 400, 408, 500 |

#### 공통 구조체

- `FWebVectorResponse`: 3D 위치 벡터 (x, y, z) - Response 전용

#### 의사결정 응답 (`FGetDecisionResponse`)

- `FMetaInfo`: AI 서버 메타 정보 (버전, 레이턴시, LLM 토큰 수 등)
- `FDecisionInfo`: 의사결정 정보 (스쿼드 목표, 추론, 유닛 명령)
- `FBrainCamData`: AI 사고 과정 시각화 (Perception → Reasoning → Decision)

### 📄 Tactic.h

**AI 전술 데이터 구조체**

- 전술 문서 및 RAG(Retrieval-Augmented Generation) 관련 구조체

---

## 📡 HTTP API v1.2.0 (`HttpV1/`) ✨ NEW

AI 서버와의 **HTTP 통신 v1.2.0**을 위한 요청/응답 구조체입니다.

### 📄 HttpRequest2.h

**AI 서버로 전송할 HTTP Request 구조체 v1.2.0**

#### 주요 구조체

| 구조체 | 설명 | 엔드포인트 | 비고 |
|--------|------|------------|------|
| `FMatchStartRequest2` | 매치 시작 요청 | `POST /api/v1/match/start` | player_types[] 추가 |
| `FMatchEndRequest2` | 매치 종료 요청 | `POST /api/v1/match/end` | v1.1과 동일 |
| `FGetDecisionRequest2` | **의사결정 요청 (핵심)** | `POST /api/v1/get_decision` | NavMesh 필드 추가 |
| `FOverseerTTSRequest2` | **Overseer TTS 요청** | `POST /api/v1/overseer/tts` | 🆕 신규 |

#### v1.2.0 주요 변경사항

**1. Steam ID 기반 유저 시스템**
- `player_id`: Steam ID (고유 식별자)
- `player_name`: 스팀 닉네임/인게임 닉네임 (표시 이름)
- `player_type`: "COMMANDER" | "DRIVER" (역할 구분)

**2. AI 스쿼드 컨텍스트 변경**
- `enemy_main` → `pacman_main` (명칭 변경)
- `minions` → `clones` (명칭 변경)
- `FPacmanMain2.speed` 필드 추가 (현재 이동 속도 m/s)

**3. NavMesh 경로 필드 추가** (`FPlayerTeamContext2`)
- `distance_to_pacman`: 유클리드(직선) 거리 (m)
- `navmesh_cost_to_pacman`: NavMesh 최단 경로 길이 (m) - **핵심 필드**
- `navmesh_path_valid`: 경로 계산 성공 여부 (bool)

**4. 이벤트 구조 보강**
- `FPlayerEvent2.event_code` 추가 (int32, 공통 enum 테이블 기준)

**5. 기존 v1.1 구조체 재사용**
- `FWebVectorRequest`, `FWebVelocityRequest`
- `FEquippedRune`, `FCommanderInteraction`
- `FCashStatus`, `FCommanderRecentAction`, `FManagementStats`

### 📄 HttpResponse2.h

**AI 서버로부터 수신할 HTTP Response 구조체 v1.2.0**

#### 주요 구조체

| 구조체 | 설명 | 상태 코드 | 비고 |
|--------|------|-----------|------|
| `FGetDecisionResponse2` | 의사결정 응답 | 200 | overseer_tts_trigger 추가 |
| `FOverseerTTSResponse2` | **Overseer TTS 응답** | 200 | 🆕 신규 |
| `FCommanderReportResponse2` | **Commander 리포트 응답** | 200 | 🆕 신규 |

#### 신규 기능

**1. Overseer TTS API** (`FOverseerTTSResponse2`)
- **LOCAL_CLIP 모드**: 로컬 사운드 클립 재생 (1~20)
  - `local_clip_index`, `cue_id` 사용
- **SERVER_AUDIO 모드**: 서버에서 Opus 인코딩된 음성 전송
  - `audio_format: "OPUS"`, `audio_base64` (Base64 인코딩)

**2. Commander Report API** (`FCommanderReportResponse2`)
- 매치 종료 후 언제든 재조회 가능
- 강점/약점 분석, 통계, 전체 점수 포함

**3. Overseer TTS Trigger** (`FOverseerTTSTrigger2`)
- `/get_decision` 응답에 포함
- `has_pending_tts`가 true이면 언리얼이 `/overseer/tts` 호출

---

## 🔌 WebSocket API v1.1 (`WebSocketV1/`)

AI 서버와의 **실시간 WebSocket 통신**을 위한 메시지 구조체입니다.

### 📄 SendWebSocket.h

**AI 서버로 전송할 WebSocket 메시지**

#### 주요 구조체

| 구조체 | 타입 | 설명 |
|--------|------|------|
| `FWebSocketPongMessage` | JSON | Heartbeat PONG 응답 |
| `FAudioPCMChunk` | Binary | PCM 오디오 청크 데이터 (STT용) |

#### 오디오 포맷

- **샘플레이트**: 16,000 Hz
- **채널**: Mono (1채널)
- **포맷**: 16-bit PCM, little-endian (signed int16)
- **청크 크기**: 100~200ms 분량 (3,200~6,400 bytes)

#### 연결 파라미터

- `FWebSocketSTTConnectionParams`: WebSocket STT 연결 파라미터
  - URL: `ws://{server}:8000/ws/v1/overseer/stt?room_id={room_id}&commander_id={commander_id}`

### 📄 ReceiveWebSocket.h

**AI 서버로부터 수신할 WebSocket 메시지**

#### 주요 구조체

| 구조체 | 타입 | 설명 |
|--------|------|------|
| `FWebSocketPingMessage` | JSON | Heartbeat PING 메시지 |
| `FWebSocketSTTTranscript` | JSON | STT 최종 인식 결과 |
| `FWebSocketSTTPartial` | JSON | STT 중간 인식 결과 (선택적) |
| `FWebSocketCloseInfo` | - | 연결 종료 정보 |
| `FWebSocketErrorInfo` | - | 연결 오류 정보 |

#### Heartbeat 메커니즘

1. 서버가 30초 동안 오디오 프레임을 받지 못하면 **PING** 전송
2. 클라이언트는 **5초 이내에 PONG**으로 응답
3. 무응답 시 10초 후 서버가 연결 종료

#### WebSocket 종료 코드 (`EWebSocketCloseCode`)

- `1000`: Normal Closure (정상 종료)
- `1001`: Going Away (서버/클라이언트 종료)
- `1002`: Protocol Error (프로토콜 오류)
- `1006`: Abnormal Closure (비정상 종료)
- `1011`: Internal Server Error (서버 내부 오류)

---

## 🔌 WebSocket API v1.2.0 (`WebSocketV1/`) ✨ NEW

AI 서버와의 **실시간 WebSocket 통신 v1.2.0**을 위한 메시지 구조체입니다.

### 📄 SendWebSocket2.h

**AI 서버로 전송할 WebSocket 메시지 v1.2.0**

#### 주요 구조체

| 구조체 | 타입 | 설명 | 비고 |
|--------|------|------|------|
| `FCommanderSTTEvent2` | JSON | Commander STT 이벤트 | 🆕 신규 |
| `FCommanderWebSocketConnectionParams2` | - | Commander WebSocket 연결 파라미터 | 🆕 신규 |

#### v1.2.0 주요 변경사항

**1. Commander STT Event** (`FCommanderSTTEvent2`)
- **phase 구조**: `start` / `partial` / `final`
  - `start`: 발화 시작
  - `partial`: 중간 인식 결과
  - `final`: 최종 인식 결과
- **speaker 정보**: `player_id`, `player_name`, `player_type`
- **audio 정보**: Opus 바이너리를 Base64로 인코딩하여 전송
  - `format: "OPUS"`
  - `base64`: Base64 인코딩된 Opus 데이터

**2. 오디오 포맷**
- **입력**: Steam에서 받은 Opus 바이너리 그대로
- **전송**: Base64 인코딩하여 JSON에 포함
- **AI 서버 내부**: Base64 Decode → Opus Decompress → PCM → STT

**3. 연결 파라미터**
- `FCommanderWebSocketConnectionParams2`: Commander WebSocket 연결 파라미터
  - URL: `ws://{server}:8000/ws/v1/commander?room_id={room_id}&commander_id={commander_id}`

**4. 기존 v1.1 구조체 재사용**
- `FWebSocketPongMessage` (Heartbeat PONG)

### 📄 ReceiveWebSocket2.h

**AI 서버로부터 수신할 WebSocket 메시지 v1.2.0**

#### 주요 구조체

| 구조체 | 타입 | 설명 | 비고 |
|--------|------|------|------|
| `FCommanderSTTResult2` | JSON | STT 인식 결과 | phase별 응답 |
| `FTTSPushEvent2` | JSON | **TTS Push 이벤트** | 🆕 신규 |

#### v1.2.0 주요 변경사항

**1. STT Result** (`FCommanderSTTResult2`)
- **phase 구조**: `start` / `partial` / `final`
- **speaker 정보** 포함
- **신뢰도** (confidence): 0~1

**2. TTS Push Event** (`FTTSPushEvent2`)
- **두 가지 모드**:
  - **LOCAL_CLIP**: 로컬 사운드 클립 재생 (1~20)
    - `local_clip_index`, `cue_id` 사용
  - **SERVER_AUDIO**: 서버에서 Opus 인코딩된 음성 전송
    - `audio_format: "OPUS"`
    - `audio_base64`: Base64 인코딩된 Opus 데이터
- **target 정보**: 대상 플레이어 (Commander)
- **display_text**: HUD에 표시할 텍스트

**3. Heartbeat 메커니즘**
- 서버가 30초 비활동 시 PING 전송
- 클라이언트는 10초 이내에 PONG 응답
- 무응답 시 재연결

**4. 기존 v1.1 구조체 재사용**
- `FWebSocketPingMessage` (Heartbeat PING)
- `FWebSocketCloseInfo`, `FWebSocketErrorInfo` (연결 관리)

---

## 🔄 주요 변경 사항

### v1.2.0 업데이트 (2025-12-05) ✨ NEW

#### 1. Steam ID 기반 유저 시스템

- **player_id**: Steam ID로 변경 (고유 식별자)
- **player_name**: 스팀 닉네임/인게임 닉네임 추가
- **player_types[]**: "COMMANDER" | "DRIVER" 배열 추가
- **설계 사유**: 고유 식별자와 표시 이름 분리, 닉네임 변경에도 데이터 일관성 유지

#### 2. NavMesh 경로 코스트/거리 필드 추가

- **distance_to_pacman**: 유클리드(직선) 거리 (m)
- **navmesh_cost_to_pacman**: NavMesh 최단 경로 길이 (m) - **핵심 필드**
- **navmesh_path_valid**: 경로 계산 성공 여부 (bool)
- **설계 사유**: AI가 "직선거리"와 "실제 주행 경로 길이"를 모두 활용해 전술 판단 정확도 향상

#### 3. AI 스쿼드 컨텍스트 명칭 변경

- **enemy_main → pacman_main** (의미상 더 명확)
- **minions → clones** (분신 유닛)
- **pacman_main.speed**: 현재 이동 속도 필드 추가 (m/s)

#### 4. Overseer TTS HTTP API 추가

- **POST /api/v1/overseer/tts**: 언리얼 → AI 서버로 TTS 지시 조회
- **두 가지 모드**:
  - **LOCAL_CLIP**: 로컬 사운드 클립 재생 (1~20)
  - **SERVER_AUDIO**: 서버에서 Opus 인코딩된 음성 전송 (Base64)
- **설계 사유**: 단기적으로 로컬 클립으로 빠른 데모, 장기적으로 동적 TTS 확장 (OCP)

#### 5. Commander STT WebSocket JSON 스키마 추가

- **FCommanderSTTEvent2**: stt_event (start/partial/final)
- **오디오 포맷**: Steam Opus → Base64 → JSON
- **AI 서버 내부**: Base64 Decode → Opus Decompress → PCM → STT
- **설계 사유**: 발화 경계 명확히 하여 지휘관 지시 분석 품질 향상

#### 6. Commander 리포트 API 추가

- **GET /api/v1/commander/report**: room_id 기반 리포트 조회
- **매치 종료 후 언제든 재호출 가능**
- **설계 사유**: 리플레이/코칭/교육 용도로 재사용

#### 7. 이벤트 구조 보강

- **event_code** 필드 추가 (int32, 공통 enum 테이블 기준)

#### 영향받은 파일

- **HttpV1/**: `HttpRequest2.h/cpp`, `HttpResponse2.h/cpp` (신규)
- **WebSocketV1/**: `SendWebSocket2.h/cpp`, `ReceiveWebSocket2.h/cpp` (신규)
- **기존 v1.1 파일**: 유지 (하위 호환성)

---

### v1.1 업데이트 (2025-11-25)

#### 1. 타입 이름 변경 (엔진 충돌 방지)

- **Before**: `FWebVector`, `FWebVelocity`
- **After**: `FWebVectorRequest`, `FWebVectorResponse`, `FWebVelocityRequest`
- **이유**: 엔진 소스의 `FWebVector`와 이름 충돌 방지를 위해 Request/Response 접미사 유지

#### 영향받은 파일

- `RequestHttp.h`: 구조체 정의 및 주석 업데이트
- `ResponseHttp.h`: 구조체 정의 및 주석 업데이트
- `SendWebSocket.h`: 수정 없음 (WebSocket STT 전용)
- `ReceiveWebSocket.h`: 수정 없음 (WebSocket STT 전용)

---

## 🎯 VoiceWebSocket Component v1.2.0 지원 ✨

`UVoiceWebSocketComponent`가 v1.2.0 프로토콜을 완벽하게 지원합니다!

### 주요 변경사항

#### 1. STT Event 전송 방식 변경

**v1.1 (기존)**:
- `session_start` (JSON) → Binary audio chunks → `session_end` (JSON)

**v1.2.0 (신규)**:
- `stt_event` (JSON) with `phase` (start/partial/final) + `audio_base64` (Opus)
- 모든 오디오 데이터를 Base64로 인코딩하여 JSON에 포함

#### 2. Phase 기반 발화 추적

```cpp
// StartTransmissionTo() 호출 시
→ phase="start" (첫 번째 오디오 청크)

// 오디오 데이터 전송 중
→ phase="partial" (중간 오디오 청크들)

// StopTransmission() 호출 시
→ phase="final" (빈 오디오, 발화 종료 표시)
```

#### 3. 화자 정보 추가

모든 STT Event에 화자 정보가 포함됩니다:
```cpp
speaker.player_id = "STEAM_ID";        // Steam ID
speaker.player_name = "Commander";     // 닉네임
speaker.player_type = "COMMANDER";     // 역할
```

#### 4. STT Result 수신 변경

**v1.1**: `FWebSocketSTTTranscript` (final만), `FWebSocketSTTPartial` (partial)

**v1.2.0**: `FCommanderSTTResult2` (phase 통합)
```cpp
void HandleSTTResult(const FCommanderSTTResult2& Result)
{
    if (Result.IsFinal())
    {
        // 최종 인식 결과 처리
    }
    else if (Result.IsPartial())
    {
        // 중간 인식 결과 처리 (선택적)
    }
}
```

#### 5. TTS Push 이벤트 수신 (NEW!)

Overseer가 지휘관에게 음성/텍스트 알림을 보낼 수 있습니다:

```cpp
void HandleTTSPush(const FTTSPushEvent2& Push)
{
    if (Push.IsLocalClipMode())
    {
        // LOCAL_CLIP 모드: 로컬 사운드 재생
        PlayLocalSoundClip(Push.local_clip_index);
    }
    else if (Push.IsServerAudioMode())
    {
        // SERVER_AUDIO 모드: Base64 디코딩 후 Opus 재생
        TArray<uint8> OpusData;
        FBase64::Decode(Push.audio_base64, OpusData);
        PlayOpusAudio(OpusData);
    }

    // HUD에 텍스트 표시
    ShowHUDMessage(Push.display_text);
}
```

#### 6. Blueprint Delegate 변경

**v1.1**:
```cpp
UPROPERTY(BlueprintAssignable)
FOnSTTTranscriptReceived OnTranscriptReceived;  // Text, Confidence
```

**v1.2.0**:
```cpp
UPROPERTY(BlueprintAssignable)
FOnSTTResultReceived OnSTTResultReceived;  // Phase, Text, Confidence

UPROPERTY(BlueprintAssignable)
FOnTTSPushReceived OnTTSPushReceived;  // TTSMode, DisplayText, LocalClipIndex
```

### 연결 파라미터 변경

**v1.1**:
```cpp
FWebSocketSTTConnectionParams ConnectionParams;
ConnectionParams.ServerIP = "127.0.0.1";
ConnectionParams.ServerPort = 8000;
ConnectionParams.Version = 1;
ConnectionParams.Service = "overseer/stt";
```

**v1.2.0**:
```cpp
FCommanderWebSocketConnectionParams2 ConnectionParams;
ConnectionParams.ServerIP = "127.0.0.1";
ConnectionParams.ServerPort = 8000;
ConnectionParams.RoomID = "ROOM_001";        // 게임 방 ID
ConnectionParams.CommanderID = "STEAM_ID";   // Steam ID

// 자동 URL 생성: ws://127.0.0.1:8000/ws/v1/commander?room_id=ROOM_001&commander_id=STEAM_ID
```

### PCM 지원 중단

v1.2.0에서는 **Opus 형식만** 지원합니다:
- `SendAudioChunk_Opus()`: 정상 동작 (Base64 인코딩하여 JSON 전송)
- `SendAudioChunk_PCM()`: 에러 로그 출력 (Opus 인코딩 필요)

**권장**: `USteamVoiceComponent`를 사용하여 Steam에서 Opus 인코딩된 데이터를 직접 받으세요.

### 사용 예시

```cpp
// 1. Component 생성 및 초기화
VoiceWebSocket = CreateDefaultSubobject<UVoiceWebSocketComponent>(TEXT("VoiceWebSocket"));
VoiceWebSocket->ConnectionParams.ServerIP = "127.0.0.1";
VoiceWebSocket->ConnectionParams.ServerPort = 8000;

// 2. Delegate 바인딩
VoiceWebSocket->OnSTTResultReceived.AddDynamic(this, &AMyCommander::OnSTTResult);
VoiceWebSocket->OnTTSPushReceived.AddDynamic(this, &AMyCommander::OnTTSPush);

// 3. 연결
VoiceWebSocket->Create();
VoiceWebSocket->Connect();

// 4. 음성 전송 시작
VoiceWebSocket->StartTransmissionTo(ETargetRacer::Racer1);

// 5. 음성 전송 중지
VoiceWebSocket->StopTransmission();

// 6. STT Result 처리
void AMyCommander::OnSTTResult(const FString& Phase, const FString& Text, float Confidence)
{
    if (Phase == TEXT("final") && Confidence >= 0.8f)
    {
        ProcessVoiceCommand(Text);
    }
}

// 7. TTS Push 처리
void AMyCommander::OnTTSPush(const FString& TTSMode, const FString& DisplayText, int32 LocalClipIndex)
{
    ShowHUDMessage(DisplayText);

    if (TTSMode == TEXT("LOCAL_CLIP"))
    {
        PlayLocalSoundClip(LocalClipIndex);
    }
}
```

### 마이그레이션 가이드 (v1.1 → v1.2.0)

1. **Include 변경**:
   ```cpp
   // Before
   #include "SendWebSocket.h"
   #include "ReceiveWebSocket.h"

   // After
   #include "SendWebSocket2.h"
   #include "ReceiveWebSocket2.h"
   ```

2. **ConnectionParams 타입 변경**:
   ```cpp
   // Before
   FWebSocketSTTConnectionParams ConnectionParams;

   // After
   FCommanderWebSocketConnectionParams2 ConnectionParams;
   ```

3. **Delegate 시그니처 변경**:
   ```cpp
   // Before
   void OnTranscriptReceived(FString Text, float Confidence);

   // After
   void OnSTTResultReceived(FString Phase, FString Text, float Confidence);
   void OnTTSPushReceived(FString TTSMode, FString DisplayText, int32 LocalClipIndex);
   ```

4. **필요한 모듈 추가** (`UE_CITRUSH.Build.cs`):
   ```csharp
   PublicDependencyModuleNames.AddRange(new string[] {
       "Json",
       "JsonUtilities",
       // ... 기타 모듈
   });
   ```

---

## 🌐 AIDataManagerComponent v1.2.0 지원 ✨

`UAIDataManagerComponent`가 v1.2.0 프로토콜을 완벽하게 지원합니다!

### 주요 변경사항

#### 1. HTTP Request/Response 타입 변경

**v1.1 (기존)**:
- `FGetDecisionRequest` / `FGetDecisionResponse`

**v1.2.0 (신규)**:
- `FGetDecisionRequest2` / `FGetDecisionResponse2`
- NavMesh 경로 필드 추가 (distance_to_pacman, navmesh_cost_to_pacman, navmesh_path_valid)
- player_types 필드 추가 (Steam ID 기반 유저 시스템)

#### 2. 자동 Overseer TTS 요청

`GetDecision()` 응답에서 TTS Trigger를 자동으로 감지하여 처리합니다:

```cpp
// GetDecision() 응답 처리 중
if (DecisionResponse.overseer_tts_trigger.has_pending_tts)
{
    // 자동으로 TTS 요청
    FOverseerTTSRequest2 TTSRequest;
    TTSRequest.room_id = DecisionResponse.overseer_tts_trigger.room_id;
    TTSRequest.event_id = DecisionResponse.overseer_tts_trigger.event_id;
    GetOverseerTTS(TTSRequest);
}
```

#### 3. 새로운 API 엔드포인트 추가

| API | Method | 엔드포인트 | 설명 | 버전 |
|-----|--------|------------|------|------|
| GetOverseerTTS | POST | /api/v1/overseer/tts | Overseer TTS 요청 | v1.2.0 신규 |
| GetCommanderReport | GET | /api/v1/commander/report | Commander 리포트 조회 | v1.2.0 신규 |

#### 4. Overseer TTS 모드

**LOCAL_CLIP 모드**:
```cpp
FOverseerTTSResponse2 Response;
if (Response.tts_mode == "LOCAL_CLIP")
{
    int32 ClipIndex = Response.local_clip_index;  // 1~20
    FString CueID = Response.cue_id;
    // 로컬 사운드 클립 재생
}
```

**SERVER_AUDIO 모드**:
```cpp
if (Response.tts_mode == "SERVER_AUDIO")
{
    FString AudioFormat = Response.audio_format;  // "OPUS"
    FString AudioBase64 = Response.audio_base64;
    // Base64 디코딩 → Opus 디코딩 → 재생
}
```

#### 5. Commander 리포트 조회

매치 종료 후 언제든 리포트를 재조회할 수 있습니다:

```cpp
// room_id로 리포트 조회
HttpComponent->GetCommanderReport("ROOM_001");

// 응답 처리
void OnCommanderReport(bool bSuccess, FCommanderReportResponse2 Response)
{
    FString Grade = Response.grade;  // S, A, B, C, D
    float Score = Response.overall_score;
    TArray<FCommanderStrengthWeakness2> Strengths = Response.strengths;
    TArray<FCommanderStrengthWeakness2> Weaknesses = Response.weaknesses;
}
```

#### 6. Delegate 변경

**v1.1**:
```cpp
UPROPERTY(BlueprintAssignable)
FOnDecisionResponse OnDecisionResponse;  // FGetDecisionResponse
```

**v1.2.0**:
```cpp
UPROPERTY(BlueprintAssignable)
FOnDecisionResponse2 OnDecisionResponse;  // FGetDecisionResponse2

UPROPERTY(BlueprintAssignable)
FOnOverseerTTSResponse OnOverseerTTSResponse;  // 신규

UPROPERTY(BlueprintAssignable)
FOnCommanderReportResponse OnCommanderReportResponse;  // 신규
```

### 사용 예시

#### 1. Component 초기화 및 Delegate 바인딩

```cpp
void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Component 가져오기
    UAIDataManagerComponent* AIDataManager = FindComponentByClass<UAIDataManagerComponent>();

    // v1.2.0 Delegates 바인딩
    AIDataManager->OnDecisionResponse.AddDynamic(this, &AMyGameMode::OnDecisionReceived);
    AIDataManager->OnOverseerTTSResponse.AddDynamic(this, &AMyGameMode::OnTTSReceived);
    AIDataManager->OnCommanderReportResponse.AddDynamic(this, &AMyGameMode::OnReportReceived);

    // 서버 URL 설정
    AIDataManager->ServerURL = "127.0.0.1";
    AIDataManager->PortURL = 8000;
}
```

#### 2. 의사결정 요청 (v1.2.0)

```cpp
void AMyGameMode::RequestDecision()
{
    FGetDecisionRequest2 Request;
    Request.room_id = "ROOM_001";

    // Global Context 설정
    Request.global_context.remaining_time = 180.0f;
    Request.global_context.game_phase = "MID_GAME";

    // Player Team Context 설정 (v1.2.0 - NavMesh 필드 추가)
    for (APlayerController* PC : PlayerControllers)
    {
        FPlayerTeamContext2 PlayerContext;
        PlayerContext.player_id = PC->GetSteamID();
        PlayerContext.player_name = PC->GetPlayerName();
        PlayerContext.player_type = "DRIVER";
        PlayerContext.distance_to_pacman = 500.0f;
        PlayerContext.navmesh_cost_to_pacman = 750.0f;  // 실제 경로 길이
        PlayerContext.navmesh_path_valid = true;

        Request.player_team_context.Add(PlayerContext);
    }

    AIDataManager->GetDecision(Request);
}
```

#### 3. 의사결정 응답 처리

```cpp
void AMyGameMode::OnDecisionReceived(bool bSuccess, FGetDecisionResponse2 Response)
{
    if (!bSuccess) return;

    // 유닛 명령 실행
    for (const FUnitCommand& Cmd : Response.decision.unit_commands)
    {
        ApplyUnitCommand(Cmd.unit_id, Cmd.directive_code, Cmd.params);
    }

    // v1.2.0: Overseer TTS Trigger는 자동으로 처리됨
    // (GetDecision 내부에서 has_pending_tts 확인 후 자동 요청)
}
```

#### 4. Overseer TTS 응답 처리

```cpp
void AMyGameMode::OnTTSReceived(bool bSuccess, FOverseerTTSResponse2 Response)
{
    if (!bSuccess) return;

    // HUD에 텍스트 표시
    ShowHUDMessage(Response.display_text);

    if (Response.tts_mode == "LOCAL_CLIP")
    {
        // 로컬 사운드 클립 재생
        PlayLocalSoundClip(Response.local_clip_index, Response.cue_id);
    }
    else if (Response.tts_mode == "SERVER_AUDIO")
    {
        // Base64 디코딩
        TArray<uint8> OpusData;
        FBase64::Decode(Response.audio_base64, OpusData);

        // Opus 디코딩 후 재생
        PlayOpusAudio(OpusData);
    }
}
```

#### 5. Commander 리포트 조회

```cpp
void AMyGameMode::EndMatch()
{
    // 매치 종료
    AIDataManager->EndMatch();

    // 리포트 조회
    AIDataManager->GetCommanderReport("ROOM_001");
}

void AMyGameMode::OnReportReceived(bool bSuccess, FCommanderReportResponse2 Response)
{
    if (!bSuccess) return;

    UE_LOG(LogTemp, Log, TEXT("Commander Grade: %s, Score: %.2f"),
        *Response.grade, Response.overall_score);

    // 강점 출력
    for (const FCommanderStrengthWeakness2& Strength : Response.strengths)
    {
        UE_LOG(LogTemp, Log, TEXT("Strength: %s - %s"),
            *Strength.category, *Strength.detail);
    }

    // 약점 출력
    for (const FCommanderStrengthWeakness2& Weakness : Response.weaknesses)
    {
        UE_LOG(LogTemp, Log, TEXT("Weakness: %s - %s"),
            *Weakness.category, *Weakness.detail);
    }
}
```

### 마이그레이션 가이드 (v1.1 → v1.2.0)

#### 1. Request/Response 타입 변경

```cpp
// Before (v1.1)
FGetDecisionRequest Request;
void OnDecision(bool bSuccess, FGetDecisionResponse Response);

// After (v1.2.0)
FGetDecisionRequest2 Request;
void OnDecision(bool bSuccess, FGetDecisionResponse2 Response);
```

#### 2. Delegate 시그니처 변경

```cpp
// Before (v1.1)
AIDataManager->OnDecisionResponse.AddDynamic(this, &AMyClass::OnDecision);
void AMyClass::OnDecision(bool bSuccess, FGetDecisionResponse Response) { }

// After (v1.2.0)
AIDataManager->OnDecisionResponse.AddDynamic(this, &AMyClass::OnDecision);
void AMyClass::OnDecision(bool bSuccess, FGetDecisionResponse2 Response) { }
```

#### 3. 새로운 필드 설정

```cpp
// v1.2.0: NavMesh 필드 추가
FPlayerTeamContext2 PlayerContext;
PlayerContext.distance_to_pacman = CalculateEuclideanDistance();
PlayerContext.navmesh_cost_to_pacman = CalculateNavMeshPathCost();  // 중요!
PlayerContext.navmesh_path_valid = IsPathValid();

// v1.2.0: player_types 추가
PlayerContext.player_id = GetSteamID();
PlayerContext.player_name = GetPlayerName();
PlayerContext.player_type = "COMMANDER";  // or "DRIVER"
```

#### 4. 새로운 API 사용

```cpp
// Overseer TTS (자동 요청됨, 수동 호출도 가능)
AIDataManager->OnOverseerTTSResponse.AddDynamic(this, &AMyClass::OnTTS);

// Commander Report
AIDataManager->OnCommanderReportResponse.AddDynamic(this, &AMyClass::OnReport);
AIDataManager->GetCommanderReport("ROOM_001");
```

### 자동 TTS 요청 제어

기본적으로 `GetDecision()` 내부에서 TTS Trigger를 자동으로 처리합니다.
수동으로 제어하려면 AIDataManagerComponent.cpp:170-180 주석 처리 후:

```cpp
void OnDecisionReceived(bool bSuccess, FGetDecisionResponse2 Response)
{
    if (!bSuccess) return;

    // 수동 TTS 요청
    if (Response.overseer_tts_trigger.has_pending_tts)
    {
        if (ShouldPlayTTS())
        {
            FOverseerTTSRequest2 TTSRequest;
            TTSRequest.room_id = Response.overseer_tts_trigger.room_id;
            TTSRequest.event_id = Response.overseer_tts_trigger.event_id;
            AIDataManager->GetOverseerTTS(TTSRequest);
        }
    }
}
```

---

## 📖 사용 방법

### 1. HTTP Request 전송 예시

```cpp
// 1. 의사결정 요청 생성
FGetDecisionRequest Request;
Request.room_id = TEXT("room_001");

// 2. 전역 컨텍스트 설정
Request.global_context.remaining_time = 180.0f;
Request.global_context.game_phase = TEXT("MID_GAME");

// 3. AI 스쿼드 컨텍스트 설정
Request.ai_squad_context.enemy_main.position = FWebVectorRequest{100.0f, 200.0f, 0.0f};
Request.ai_squad_context.enemy_main.hp = 80.0f;

// 4. HTTP 요청 전송
SendHttpRequest(Request);
```

### 2. HTTP Response 수신 예시

```cpp
void OnDecisionReceived(const FGetDecisionResponse& Response)
{
    if (Response.status == TEXT("success"))
    {
        // 의사결정 처리
        for (const FUnitCommand& Cmd : Response.decision.unit_commands)
        {
            ApplyCommand(Cmd.unit_id, Cmd.directive_code, Cmd.params);
        }

        // 브레인캠 데이터 시각화
        if (Response.brain_cam_data.perception.threat_level == TEXT("CRITICAL"))
        {
            ShowWarning();
        }
    }
}
```

### 3. WebSocket 전송 예시

```cpp
// PONG 메시지 전송
FString JsonString;
FWebSocketSendUtils::SerializePongMessage(JsonString);
WebSocket->Send(JsonString);

// PCM 오디오 데이터 전송
TArray<uint8> PCMData;
CaptureMicrophoneAudio(PCMData);
WebSocket->Send(PCMData.GetData(), PCMData.Num(), true);  // Binary Frame
```

### 4. WebSocket 수신 예시

```cpp
void OnWebSocketMessage(const FString& JsonString)
{
    FWebSocketReceiveUtils::AutoParseAndHandle(
        JsonString,
        // PING 수신 콜백
        [this](const FWebSocketPingMessage& Ping)
        {
            // PONG 응답
            SendPongMessage();
        },
        // Transcript 수신 콜백
        [this](const FWebSocketSTTTranscript& Transcript)
        {
            if (Transcript.IsConfidentEnough(0.8f))
            {
                ProcessVoiceCommand(Transcript.text);
            }
        }
    );
}
```

---

## 📌 참고 문서

- **프로토콜 문서**: `AgentLog/Protocol.md`
- **네트워크 모듈**: `AgentLog/NetworkModule.md`
- **API 명세**: AI 서버 API 문서 참조

---

## 🛠️ 개발 가이드

### 새로운 구조체 추가 시 주의사항

1. **USTRUCT 매크로 필수**
   ```cpp
   USTRUCT(BlueprintType)
   struct FYourStructName
   {
       GENERATED_BODY()
       // ...
   };
   ```

2. **UPROPERTY 지정자 사용**
   - Request: `BlueprintReadWrite, EditAnywhere`
   - Response: `BlueprintReadOnly`

3. **기본값 설정**
   ```cpp
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   float hp = 100.0f;  // 기본값 지정
   ```

4. **주석 작성**
   - 구조체 위: 용도 및 엔드포인트 명시
   - 필드 위: 필수/선택 여부 및 설명

### JSON 직렬화/역직렬화

언리얼 엔진의 `FJsonObjectConverter`를 사용합니다:

```cpp
// 직렬화 (UStruct → JSON)
FString JsonString;
FJsonObjectConverter::UStructToJsonObjectString(Request, JsonString);

// 역직렬화 (JSON → UStruct)
FGetDecisionResponse Response;
FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &Response, 0, 0);
```

---

## ⚠️ 주의사항

1. **타입 일관성 유지**
   - Request용 벡터: `FWebVectorRequest`, `FWebVelocityRequest` 사용
   - Response용 벡터: `FWebVectorResponse` 사용
   - **중요**: 엔진 소스의 `FWebVector`와 혼동하지 말 것!

2. **ISO 8601 타임스탬프 사용**
   ```cpp
   FString timestamp = FDateTime::UtcNow().ToIso8601();
   ```

3. **WebSocket Binary 전송**
   - PCM 오디오는 **반드시 Binary Frame**으로 전송
   - JSON 메시지는 **Text Frame**으로 전송

4. **Heartbeat 응답 필수**
   - PING 수신 시 5초 이내에 PONG 응답
   - 미응답 시 연결 종료됨

---

## 📝 변경 이력

| 날짜 | 버전 | 변경 내용                                                                                       |
|------|------|---------------------------------------------------------------------------------------------|
| 2025-12-11 | v1.2.0 | **AIDataManagerComponent v1.2.0 지원 완료** - 자동 TTS 요청, Overseer TTS/Commander Report API 구현 ✨ |
| 2025-12-08 | v1.2.0 | **VoiceWebSocketComponent v1.2.0 지원 완료** - Phase 기반 STT Event, TTS Push 수신, Base64 오디오 전송 ✨ |
| 2025-12-05 | v1.2.0 | **Json Schema 업데이트** - Steam ID 유저 시스템, NavMesh 필드, Overseer TTS/Commander Report API 추가 ✨  |
| 2025-11-25 | v1.1 | FWebVectorRequest/Response로 변경 (엔진 충돌 방지)                                                   |
| 2025-11-24 | v1.0 | HttpV1, WebSocketV1 초기 구조 생성                                                                |
| 2025-11-11 | v0.1 | NPCSchemas.h 기본 구조 생성                                                                       |

---

**Last Updated**: 2025-12-11
**Protocol Version**: v1.2.0
**Components**:
- VoiceWebSocketComponent v1.2.0 ✨
- AIDataManagerComponent v1.2.0 ✨

**Author**: UE_CITRUSH Development Team
