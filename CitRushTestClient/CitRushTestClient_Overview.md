# CitRushTestClient 개요

## 목적

CitRushTestClient는 **AI 적(Enemy) 전술 결정 서버**를 테스트하고 검증하기 위한 **Windows MFC 기반 데스크탑 테스트 클라이언트**다.

- AI 서버의 `/api/v1/get_decision` 엔드포인트 호출 테스트
- 팩맨 본체 + 클론 편대 상태 및 플레이어 위치를 요청 데이터로 전송
- 서버로부터 전술 지시(Directive)를 수신하고 결과 확인
- 서버 헬스 체크, 매치 시작/종료 등 라이프사이클 검증
- 다양한 게임 상황 시나리오를 빠르게 재현하여 AI 행동 검증

CITRUSH-UE (Unreal Engine 게임 프로젝트)의 개발/QA 보조 도구다.

---

## 아키텍처 개요

```
CitRushTestClient (MFC Application)
├── UI Layer          → MFC 다이얼로그 기반 GUI
├── Network Layer     → WinHTTP 기반 HTTP 클라이언트
└── Data Layer        → 요청/응답 스키마 + JSON 직렬화
```

### 핵심 설계 원칙
- 외부 라이브러리 없음 (HTTP: Windows WinHTTP, JSON: 자체 구현)
- 동기 HTTP 호출 (테스트 도구 특성상 허용)
- UE 매크로 없이 Unreal Engine 타입을 순수 C++ 구조체로 미러링

---

## 주요 파일 구성

| 파일 | 역할 |
|------|------|
| `CitRushTestClient.h/cpp` | MFC 앱 진입점, 메인 다이얼로그 초기화 |
| `CTestClientDlg.h/cpp` | 메인 UI 다이얼로그, 이벤트 핸들러, 테스트 로직 전체 |
| `Network/CHttpClient.h/cpp` | WinHTTP API 래퍼 (동기 HTTP 클라이언트) |
| `Schemas/EnemySchemas.h` | 요청/응답 데이터 구조체 (UE 타입 미러) |
| `Schemas/JsonHelper.h` | JSON 직렬화/역직렬화 (외부 라이브러리 없이 자체 구현) |

---

## 사용 방법

### 1. 서버 연결 설정

UI 상단에서 입력:
- **Server URL**: `http://34.64.141.47` (기본값)
- **Port**: `8000` (기본값)
- **Room ID**: `TEST_ROOM_001` (기본값, 변경 가능)

### 2. API 엔드포인트

| 엔드포인트 | 메서드 | 기능 |
|-----------|--------|------|
| `/api/v1/health` | GET | 서버 헬스 체크 |
| `/api/v1/match/start` | POST | 매치 세션 초기화 |
| `/api/v1/match/end` | POST | 매치 종료 및 결과 기록 |
| `/api/v1/get_decision` | POST | AI 전술 결정 요청 (핵심) |

### 3. Get Decision 흐름

```
1. UI에서 게임 상태 입력
   - 팩맨: 위치(X,Y,Z), HP, 속도, 무적 여부
   - 클론 1/2: 위치, HP, 생존 여부
   - 플레이어(Commander): 위치, HP, 팩맨까지 거리

2. "Get Decision" 버튼 클릭

3. 내부 처리
   BuildDecisionRequest()
     → JsonHelper::BuildDecisionRequest()  // JSON 직렬화
     → CHttpClient::Post("/api/v1/get_decision", json)
     → JsonHelper::ParseDecisionResponse()  // 응답 파싱
     → DisplayDecisionResponse()           // UI 출력

4. 결과 확인
   - 전술 목표 (Squad Objective)
   - 유닛별 지시 (Directive Code + Name + 목표 위치)
   - 신뢰도(Confidence), 레이턴시, LLM 토큰 사용량 등 메타 정보
```

### 4. 시나리오 프리셋 (빠른 테스트)

| 프리셋 버튼 | 상황 설정 | 예상 전술 |
|------------|----------|----------|
| RETREAT | 팩맨 HP 낮음(12), 플레이어 근접(80m) | RETREAT (5) |
| INTERCEPT | 팩맨 HP 중간(80), 플레이어 원거리(450m) | INTERCEPT (3) |
| CONSUME_PELLET | 상태 양호, 펠릿 쿨다운 없음 | CONSUME_PELLET (7) |
| SPLIT_FORMATION | 클론 분산, 플레이어 원거리 | SPLIT_FORMATION (10) |

---

## 데이터 구조 요약

### 요청 (FGetDecisionRequest)
- `room_id`, `request_num`
- `global_context`: 게임 시간, 점수, 남은 시간
- `ai_squad_context`: 팩맨 + 클론 상태 (위치, HP, 속도, 생존 여부)
- `player_team_context`: 플레이어 목록 (위치, HP, 팩맨까지 거리)
- `map_context`: 스폰 위치, 펠릿/포인트 위치

### 응답 (FGetDecisionResponse)
- `success`, `status`, `timestamp`
- `meta`: 레이턴시, 버전, fallback 여부, LLM 토큰 수
- `decision`: 편대 목표, 추론 근거, 신뢰도
- `unit_commands`: 유닛별 전술 지시 배열

### AI 전술 코드 (EAITactic)
| 코드 | 이름 |
|------|------|
| 1 | AMBUSH |
| 2 | MOVE_TO_LOCATION |
| 3 | INTERCEPT |
| 4 | CHASE |
| 5 | RETREAT |
| 6 | PATROL |
| 7 | CONSUME_PELLET |
| 8 | GUARD |
| 9 | FLANK |
| 10 | SPLIT_FORMATION |
| 11 | REGROUP |
| 12 | FAKE_RETREAT |

---

## 빌드 환경

- **플랫폼**: Windows (Win32)
- **툴셋**: MSVC v143 (Visual Studio 2022)
- **프레임워크**: MFC (Dynamic)
- **링크 라이브러리**: `winhttp.lib` (시스템 라이브러리)
- **문자 인코딩**: 네트워크 UTF-8 / MFC UI UTF-16 (CT2A/CA2W 변환)

---

## 구현 참고 사항

- JSON 파싱은 단순 문자열 검색 방식 (서버 응답이 제어된 환경이므로 충분)
- 싱글 스레드 동기 HTTP (UI 블로킹 허용 — 테스트 도구 특성)
- 로그: 타임스탬프 포함, 최대 300개 순환 버퍼
- 오류 처리: UI 리스트박스에 에러 메시지 출력
