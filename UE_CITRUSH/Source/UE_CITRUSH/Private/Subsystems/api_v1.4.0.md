# 5.AI 에이전트]언리얼팀 통합 연동 가이드_v1.4.0_overseer_commander제거버전

| 항목 | 내용 |
| --- | --- |
| 문서명 | 5 AI 에이전트] 언리얼팀 통합 연동 가이드 |
| 문서 ID | DOC-AI-NW-001 |
| 버전 | v1.4.0 |
| 상태 | Beta Working |
| 문서 책임자 | 강경란 |
| 최초 수정일 | 2025-12-23 |
| 최종 수정일 | 2025-12-28 |

---

# CHANGELOG

## v1.4.0 (2025-12-23)

- **변경 요약**
    - **베타 스코프 조정(분신 미사용 유지)**
        - **ai_squad_context.clones** 필드는 **유지하되 항상 빈 배열([])** 로만 전달/수신(베타 정책)
        - **decision.unit_commands**는 **pacman_main**에 대한 명령만 생성/반환(베타 정책)
    - **명령어(Directive) 정리 및 재번호**
        - **11, 12 제거**
        - **기존 13(FAKE_RETREAT) → 11로 재번호**
        - 유효 범위: **1~11**
        - **priority / emergency_priority는 integer**, **미지정 시 0**, 권장 범위 **0~3**
    - **P-Point 시스템 추가**
        - **map_context.p_point_locations[]** 추가(P-Point 스폰 위치)
        - P-Point 습득 시 스킬은 **랜덤 자동 발동**(효과 타입은 서버로 전달하지 않음, 위치만 전달)
    - **P-Pellet은 “삭제 아님”, 베타에서도 계속 사용**
        - **map_context.p_pellet_locations[] 유지**
        - Directive에도 **CONSUME_PELLET(코드 8)** 유지
        - **ai_squad_context의 p_pellet 관련 값 (p_pellet_cooldown, last_pellet_consumed_at 등)도 유지**(베타에서 사용)
    - **UE NavSystemLLMData(nav_frame) 매핑 추가**
        - **global_context.nav_frame** 추가
        - **timestamp_sec, delta_time_sec, inter_driver_avg_distance, delta_inter_driver_avg_distance**
    - **팩맨 방향 정보 추가**
        - **ai_squad_context.pacman_main.rotation_yaw_deg, forward_vector**
    - **NavMesh “경로 거리(PathDistance)” 슬롯 추가**
        - 모든 플레이어에 **navmesh_distance_to_pacman** 추가(직선거리/코스트와 분리)
    - **DRIVER 전용 내비게이션 델타(racer_nav_delta) 추가**
        - DRIVER에만 **racer_nav_delta** 추가(각도/델타/속도 변화 등)
        - 거리/코스트는 중복 제거(기존 distance_to_pacman / navmesh_* 사용)
    - **match/end 응답 정리(report 비활성)**
        - meta에 **cleanup_latency_ms**, **report_enabled: false**
        - **commander_report: null** 고정
- **세부 변경(문서 섹션 기준)**
    - **1. 통합 시스템 구성 / 2. 시스템 개요**
        - “분신 미사용(베타)”, “P-Point 추가”, “nav_frame/방향/경로거리/델타”, “명령어 재번호” 반영
    - **3.3 Request Body**
        - ai_squad_context: clones는 항상 [], pacman_main 방향 필드 포함, P-Pellet 관련 필드 유지
        - map_context: p_point_locations 추가, p_pellet_locations 유지
        - global_context: nav_frame 추가
        - player_team_context: navmesh_distance_to_pacman, DRIVER racer_nav_delta 추가
    - **3.4 Response Body - decision**
        - unit_commands는 pacman_main만, directive_code는 1~11(11=FAKE_RETREAT)
- **설계 사유(SRP/OCP)**
    - **베타 단계 MVP 집중**: 핵심 의사결정 파이프라인/연동 안정성 검증에 집중
    - **점진적 기능 확장**: 베타 검증 완료 후 분신 기능 등 단계적 확장 예정

## v1.3.0 (2025-12-14)

- **변경요약**
    - **3.3 Request 필드 상세 설명에 map_context 정의 보강**
        - P-Point(p_point_locations)로 추가
    - **3.x Directive 코드 정의 보강**
        - directive_code=7 "CONSUME_P_POINT" 명령 사용을 알파(1~7) 범위로 명시
    - **7.4 BrainCam UI 페이로드 예시**
        - P-Point 기반 UI 필드로 교체
    - **POST /api/v1/overseer/tts 삭제**
        - Overseer TTS는 **Commander STT WebSocket에서 type: "tts_push"만 사용**
        - 문서 내 Overseer TTS pull 방식(큐 조회) 관련 설명 제거
        - overseer tts trigger 삭제
    - **commander_context “필드명 유지 + 설명만 정정”**
        - cash_status, management_stats는 **리포트 평가 데이터**
        - recent_actions는 게임 중 **AI 팩맨 의사결정 참고용(지휘관 행동 로그)**
    - **STT 음성 포맷 변경: Base64-Opus → Base64-PCM**
        - WebSocket stt_event.audio_base64는 **PCM(16kHz, mono, S16LE) base64 문자열**로 전송

## **v1.2.0 (2025-12-02) - Steam ID·닉네임, NavMesh 코스트, TTS HTTP, STT WebSocket/리포트 정리**

- **변경 요약**:
    - **Steam ID 기반 유저 시스템**으로 전환 (player_id 의미 명확화)
    - **player_name / player_type / player_types[]** 필드 추가
    - **NavMesh 경로 코스트/거리 필드** 추가 (팩맨 기준)
    - **TTS HTTP API(/api/v1/overseer/tts)** 설계 정리
        - 동적 TTS 모드 (서버에서 음성 생성, audio_base64=Opus)
    - **Commander STT WebSocket JSON 스키마(stt_event)** 추가 (start/partial/final)
    - **Commander 리포트 API(GET /api/v1/commander/report)** 호출 타이밍/구조 명시
- **세부 변경**:
    - **1.1 주요 구성 요소 / 통신 테이블**
        - **/api/v1/overseer/tts**가 **HTTP POST, 언리얼 → AI 서버** 방향임을 명확히 표기
        - “텍스트 → 음성 스트림” 표현 제거, **HTTP 요청/응답 기반 TTS 구조**로 수정
    - **1.2 상호 동작 개요**
        - 언리얼이 **/api/v1/get_decision** 응답에서 **TTS 트리거 힌트(overseer_tts_trigger, 가칭)** 를 확인한 뒤, 필요 시점에 **POST /api/v1/overseer/tts**(언리얼 → AI)를 호출해 TTS 지시를 받아가는 흐름으로 정리
        - AI Overseer는 내부에서 **overseer_events**를 계산하여 TTS 큐에 쌓아두고, **/overseer/tts** 호출 시 해당 room_id에 대한 TTS 지시를 하나 꺼내서 응답
        - **/api/v1/commander/report** 는 매치 종료 직후뿐 아니라 **수 분 후에도 재조회 가능**하도록 설계 (room_id 기반 DB 저장)
    - **3.2 /api/v1/get_decision Request**
        - **ai_squad_context.pacman_main.speed:** pacman_main의 현재 이동 속도(스칼라 값, m/s 가정)
        - **player_team_context[].player_id** = **/match/start** 의 **player_ids[]** 와 동일(스팀 ID).
        - **player_team_context[].player_name** 추가: 스팀 닉네임/인게임 닉네임 (옵션, 있으면 로그/리포트에 사용).
        - **player_team_context[].player_type** 추가: **"COMMANDER"** / **"DRIVER"** (역할 구분용).
        - 기존 **player_team_context[].role** 유지: 내부 코드/호환성 목적 (player_type과 의미 일치).
        - **NavMesh 관련 필드** 추가
            - **player_team_context[].distance_to_pacman**: 유클리드(직선) 거리 (m, optional).
            - **player_team_context[].navmesh_cost_to_pacman**: NavMesh 최단 경로 길이 (m, path cost 핵심 필드).
            - **player_team_context[].navmesh_path_valid**: 경로 계산 성공 여부/경로 유효 여부 (bool, optional).
        - **player_team_context[].events[]** 이벤트 구조 보강
            - **events[].event_code**(int) + **events[].event_type**(string) 조합으로 정의 (공통 enum 테이블 기준)
    - **3.6 /match/start 보조 API**
        - *player_ids[]**를 **Steam ID 배열**로 정의
        - **player_names[]**: 스팀 닉네임/인게임 닉네임
        - **player_types[]**: **"COMMANDER" | "DRIVER"**
        - 인덱스 규칙: **0 = COMMANDER, 1~3 = DRIVER**
    - **4. Overseer API 명세서**
        - **4.1 개요**: Overseer 역할(이벤트 기반 지휘관 알림, TTS 큐 관리) 및 **/api/v1/overseer/tts** 관계 설명
        - **4.2 /api/v1/overseer/tts (언리얼 → AI HTTP POST)** 를 TTS 전용 API로 규정
            - Request: 최소한 **room_id** 기반으로 TTS 지시를 조회
            - Response
                - **임시 모드 (LOCAL_CLIP)**: **tts_mode: "LOCAL_CLIP"**, **local_clip_index(1~20)**, **cue_id**, **display_text** 등
                - **최종 모드 (SERVER_AUDIO)**: **tts_mode: "SERVER_AUDIO"**, **audio_format: "OPUS"**, **audio_base64**(Base64 인코딩된 Opus), **display_text**
            - TTS 큐가 비어 있는 경우 **status: "no_tts"** 등을 반환하는 패턴 명시
    - **5. Commander API 명세서**
        - **5.2 Commander STT WebSocket**에 **stt_event** JSON 스키마 추가
            - **type: "stt_event"**, **phase: "start" | "partial" | "final"**
            - **speaker.player_id/player_name/player_type**
            - 오디오 포맷: 언리얼 → AI 는 **Opus 바이너리** (Steam 포맷 그대로 전송)
            - AI 서버 내부에서 Opus → PCM(16kHz, mono) 디코딩 후 STT 엔진에 전달한다고 명시
        - **5.3 GET /api/v1/commander/report** 추가
            - **room_id** 기준 리포트 조회
            - 매치 종료 후 **몇 분 뒤에도 호출 가능**, 1회성/지연 호출 허용
            - 매치 종료 직후뿐 아니라 **5~10분 후, 혹은 그 이후에도** 동일 리포트를 재조회할 수 있도록 DB에 저장/보존하는 정책 설명
            - 최종 점수, 강점/약점, 통계(statistics) 등을 포함하는 리포트 JSON 스켈레톤 정의
        - **5.4 Commander WebSocket Heartbeat**:
            - 기존 heartbeat(클라이언트 PING, 서버 PONG)를 “계획”에서 실제 스펙으로 승격
            - 30초 비활동 시 PING, 10초 응답 타임아웃 시 재연결 규칙 명시
    - **(섹션 번호 이동)**
        - 기존 문서의 **4. WebSocket 비활동 시 기본 Request/Response 정의**,**5. 지휘관 리포트 지표 확장** 내용은 **6.x, 7.x 섹션**으로 한 칸씩 뒤로 이동
    - **9. 테스트/검증 포인트**
        - **/match/start** 의 **player_ids/player_names/player_types**와 **/get_decision.player_team_context** 가 일관되게 매핑되는지 검증
        - NavMesh 코스트 필드가 없거나 null인 경우에도 기존 v1.1.0 로직이 문제 없이 동작하는지 확인(Optional handling)
        - Commander STT WebSocket에서 Opus 스트림 수신 후 디코딩/인식 성능 및 지연 시간 측정
        - **/api/v1/overseer/tts** HTTP 지연 시간, 실패 시 재시도/로그 처리
        - **/api/v1/commander/report** 를 매치 종료 후 즉시/5분 후/1시간 후 호출해 동일 리포트가 반환되는지 확인
        - **overseer_audio_cues** 임시 구조로 20개 로컬 음성이 기대대로 재생되는지 확인, 동일 프레임에 여러 cue 가 왔을 때 우선순위 규칙이 의도대로 적용되는지.
        - Commander WebSocket Heartbeat: 30초 비활동 후 PING 전송/10초 타임아웃 시 재연결 동작이 기대대로 수행되는지 확인
- **설계 사유(SRP/OCP)**:
    - **Steam ID + player_name 분리**: 고유 식별자와 표시 이름을 분리해 통계를 안정적으로 쌓고, 닉네임 변경에도 데이터 일관성 유지 (리포트/로그/DB에서 추적 용이)
    - **player_type / player_types[]**: 지휘관/드라이버 역할 구분을 명시해 리포트/알림/룬 효과 계산에 재사용 (역할 기반 확장 용이)
    - **NavMesh 기반 거리/코스트 필드**: AI가 “단순 직선거리”와 “실제 주행 경로 길이”를 모두 활용해 전술 판단 정확도 향상
    - **TTS HTTP 이원 구조**:
        - 단기적으로는 로컬 클립 재생만으로 빠르게 데모 가능 (LOCAL_CLIP)
        - 장기적으로는 동일 API에 동적 TTS(SERVER_AUDIO)를 얹어 자연스럽게 확장 (OCP)
    - **STT WebSocket 스키마**: start/partial/final 구조로 발화 경계를 명확히 하여, 지휘관 지시 분석과 리포트 작성 품질을 높임
    - **Commander 리포트 조회 API**: 매치 종료 후 언제든 재호출 가능하게 설계해, 리플레이/코칭/교육 용도로 재사용
- **TODO 및 향후 과제**
    - Commander WebSocket heartbeat 실제 구현 & 엣지 케이스 테스트
    (네트워크 단절, 서버 재시작, 장시간 비활동 시나리오)
    - Steam ID 기반 유저 시스템 전환
    (player_id/player_name/player_type 을 DB/리포트/로그까지 일관되게 사용)
    - 룬 효과가 AI 의사결정/전술 카드 선택에 반영되는 로직 구현
    (**equipped_runes[]** → 위협도/전술 카드 선택 가중치. 실제 알고리즘은 서버 구현 TODO)
    - STT 로그와 Commander 리포트 연동
    (지휘관 콜 빈도, 음성 vs 버튼 지시 비율 등 리포트 지표와 연결)
    - Directive 파라미터 상세 명세 언리얼팀과 최종 협의
    (각 directive_code 별 **params** 구조 확정)
    - 12/8 이후, **/api/v1/overseer/tts** 의 **LOCAL_CLIP 모드 정리 및 SERVER_AUDIO 모드 확장 계획 수립**
    (임시 데모 로직/매핑 테이블 정리, 본격 서비스용 TTS로 이행)

---

## **v1.1.0 (2025-11-22) - 지휘자 컨텍스트, 룬 시스템, 명령어 정의, Heartbeat 프로토콜 통합**

- **변경 요약**: 지휘관 행동 추적, 룬 시스템, 역할 구분, WebSocket heartbeat, 명령어 정의 추가
- **세부 변경**:
    - **player_team_context[].role** 추가: 플레이어 역할 구분 (COMMANDER/DRIVER)
    - **player_team_context[].equipped_runes[]** 추가: 룬(버프/아이템) 장착 정보 (rune_id, rune_name, rune_type, effect_value, duration_remaining)
    - **player_team_context[].commander_interaction** 추가: 플레이어별 마지막 음성/버튼 지시 시각 추적
    - **commander_context.management_stats** 추가: 음성/버튼 지시 비율, 플레이어별 관심 분포, 아이템 효율성, 평균 대응 시간 통계
    - **3.5 Directive 코드 정의 추가**: 12개 표준 명령어 enum (AMBUSH, MOVE_TO_LOCATION, INTERCEPT, CHASE, RETREAT 등) 및 파라미터 명세
    - **4.2 WebSocket Heartbeat 프로토콜** 추가: PING/PONG 메커니즘, 30초 비활동 시 heartbeat, 무응답 시 재연결 로직
    - **차후 변경사항 섹션** 업데이트: 구현 완료된 항목(룬, 명령어, heartbeat) 제거, 남은 작업만 유지
- **설계 사유(SRP/OCP)**:
    - **역할(role) 필드**: 향후 동일 유저가 지휘관/드라이버 역할을 바꿔가며 플레이 시 데이터 분리 용이 (확장성)
    - **룬 시스템**: AI 의사결정에 버프/디버프 정보 반영하여 전술 다양성 확보 (AI 품질 향상)
    - **management_stats**: 지휘관 평가를 정량화(음성 vs 버튼 비율, 플레이어별 관심 분포)하여 객관적 리포트 제공 (리포트 품질)
    - **Directive enum**: 명령어 표준화로 언리얼↔︎AI 간 통신 일관성 확보, 새 명령어 추가 용이 (OCP 원칙)
    - **Heartbeat 프로토콜**: WebSocket 장기 연결 안정성 확보, 침묵 기간에도 연결 유지 (안정성)
- **테스트/검증**:
    - 기존 v1.0 API와 하위 호환성 유지 (신규 필드는 모두 선택적/optional)
    - role 필드는 현재 “DRIVER”로 고정, 추후 동적 할당 예정
    - equipped_runes[] 빈 배열 허용
    - 검증/협의/테스트 필요 항목
        - management_stats 누적 계산 로직 검증 필요
        - Directive 파라미터 언리얼팀과 상세 협의 필요 (Phase 1-F)
        - Heartbeat 프로토콜 실제 구현 및 재연결 테스트 필요 (Phase 1-G)
- **추가 TODO**:
    - Directive 파라미터 상세 명세 언리얼팀과 최종 협의
    - WebSocket heartbeat 실제 구현 및 엣지 케이스 테스트
    - 스팀 ID 기반 유저 시스템 전환
    - 룬 효과가 AI 의사결정에 반영되는 로직 구현

---

## v1.0 (언리얼팀과 공유한 시점의 버전)

- **변경 요약**: Overseer STT 통신 방식 변경(WebSocket) 및 팩맨 데이터 연동 로직 추가
- **세부 변경**:
    - **Overseer API 섹션에 WebSocket STT 명세 (ws://...) 추가**
    - **get_decision** API 호출 시 Overseer 이벤트 트리거 연동 설명 추가
    - 시스템 구성도 및 데이터 흐름 업데이트
- **설계 사유(SRP/OCP)**:
    - 실시간 음성 처리를 위해 단방향 HTTP보다 WebSocket 스트리밍이 지연 시간 측면에서 유리함.
    - 팩맨과 Overseer가 동일한 게임 상황(Context)을 공유해야 하므로, 잦은 빈도(2-3초)로 갱신되는 팩맨의 결정 데이터를 Overseer의 관찰 입력으로 재사용하여 데이터 일관성 확보 및 트래픽 효율화.

---

## 차후 변경/추가 예정 사항 (v1.4.0 이후 / 참고)

- **전술카드/룰 확장**: 신규 카드 추가 및 룰 기반 폴백 정교화
- **로그/지표 강화**: decision latency, fallback rate 등 운영 지표 확장
- **리포트(선택 기능) 재도입**: match/end에서 commander_report 생성/반환은 품질 확보 후 재개 (현재는 report_enabled=false, commander_report=null)
- **Overseer/Commander(보류)**: STT/TTS WebSocket 및 관련 API는 베타 범위에서 제외, 별도 문서로 관리

---

## 목차

I. [기본 시스템 구성](https://www.notion.so/5-AI-_v1-4-0_overseer_commander-2d7867e905cd80878408cd716e98b514?pvs=21)

II. 시스템 개요

III. AI 팩맨 API 명세

IV. 통신 프로토콜

V. 브레인캠 데이터

VI. 에러 처리

VII. 테스트 가이드

VIII. 문제 해결

IX부록

---

## API 버전 관리 정책

본 API는 양 팀(AI-언리얼)의 안정적인 서비스 운영을 위해 **버전 관리(예: `api/v1/...`)**를 필수 정책으로 합니다.

**버전을 사용하는 이유:**

1. **언리얼 서버의 안정성 보장**
    - 향후 AI팀에서 API의 기능을 대폭 수정/변경하더라도, 기존에 약속된 **`v1`** API는 **중단 없이 동일하게 작동**합니다.
2. **안전한 기능 업그레이드:**
    - 기존 API와 호환되지 않는(Breaking Change) 새로운 기능은 **`v2`** 등 새 버전으로 추가됩니다.
    - 이를 통해 언리얼팀은 **원하는 시점에** 준비가 되었을 때 새로운 **`v2`** 기능으로 업그레이드할 수 있습니다.

결론: 버전 관리는 AI API가 변경되어도 이미 개발된 언리얼서버가 '먹통'이 되는 것을 방지하는 핵심 안전장치

---

# I. 통합 시스템 구성

## 1. 주요 구성 요소

1. **AI 팩맨**
    - 팩맨 본체(**pacman_main**)의 의사결정(전술 선택, 유닛 명령 생성)을 담당
    - *(분신(clones)은 베타 이후 스코프)*
- **Protocol**
    - **Command/Query (AI 팩맨)**: HTTP/1.1 (RESTful)
- **AI 서버 Base URL**: **http://{ai_server_ip}:8000/api/v1**

| 시스템 | 역할 | 주요 통신 방식 | 호출/전송 주기 |
| --- | --- | --- | --- |
| **AI 팩맨** | 팩맨 의사결정
(명령 생성) | **POST /api/v1/get_decision** | 게임 중 2~3초마다 주기 호출 |

## 2. 상호 동작 개요

1. **언리얼 → AI: 팩맨 의사결정 요청**
    - 언리얼 서버는 2~3초 주기로 **POST /api/v1/get_decision**을 호출합니다.
    - Request에는 다음 정보가 포함됩니다.
        - **global_context**: 남은 시간, 게임 페이즈 등
            - **global_context.nav_frame**: UE 네비게이션 프레임 데이터(UE FNavSystemLLMData 매핑)
                - timestamp_sec, delta_time_sec, inter_driver_avg_distance, delta_inter_driver_avg_distance
        - **ai_squad_context**: 팩맨 본체 상태 + P-Pellet 쿨타임 등
            - **pacman_main.rotation_yaw_deg / forward_vector**: 팩맨 방향 정보(옵션)
            - **clones: []** (베타 스코프: 분신 미사용)
        - **player_team_context[]**: 각 플레이어의 위치/속도/HP/이벤트 목록
            - **navmesh_distance_to_pacman**: NavMesh 경로 거리(UE PathDistance)
            - DRIVER에 한해 **racer_nav_delta** 추가(각도/델타/속도 변화)
        - **commander_context**: 지휘관 캐시/행동 로그(필드명 유지)
            - **cash_status, management_stats**는 **리포트 평가용**
            - **recent_actions**는 **게임 중 AI 팩맨 참고용(지휘관 행동 로그)**
        - **map_context**: 맵/지형 정보
            - **p_pellet_locations[]**
            - **p_point_locations[]**: P-Point 위치 정보*(P-Point 습득 시 스킬은 랜덤 자동 발동 → 효과 타입/종류는 서버로 전달하지 않음)*
2. **AI 팩맨 내부 처리**
    - AI 팩맨
        - 위 상태를 바탕으로 위협도/기회 분석 → 전술 선택 → **decision.unit_commands** 생성
        - 지휘관(적) 정보는 **commander_context.recent_actions(지휘관 행동 로그)** 중심으로 참고할 수 있습니다.
3. **AI → 언리얼: get_decision 응답**
    - AI 서버는 다음 정보를 응답합니다.
        - **decision.unit_commands**: **pacman_main**에 대한 명령만 반환(베타)
        - **decision.squad_objective**, **reasoning**, **confidence**
        - **meta.version**, **meta.latency_ms** 등 메타 정보
4. **언리얼: 명령 적용**
    - 언리얼은 **decision.unit_commands** 를 파싱해 팩맨을 제어

## 3. 서버 정보

## 3. 서버 정보

| 항목 | 내용 |
| --- | --- |
| **팩맨 의사결정 API 서버** | **http://{ai_server_ip}:8000** |
| **프로토콜** | **HTTP(REST)** |
| **메인 API** | - **/api/v1/health** (GET)
- **/api/v1/match/start** (POST)
- **/api/v1/get_decision** (POST)
- **/api/v1/match/end** (POST) |
| **타임아웃(권장)** | - **/api/v1/health**: 1~2s
- **/api/v1/get_decision**: 2s (1회 재시도 가능)
- **/api/v1/match/start, /api/v1/match/end**: 5s (1회 재시도 가능) |
| **베타 범위 제외
(미사용)** | - **Commander/Overseer WebSocket(STT/TTS)**
- **Commander Report 조회 API** |

> 참고: v1.4.0(베타)에서는 게임 진행에 필수인 팩맨 의사결정 HTTP API만 사용합니다.
> 

---

# II. 시스템 개요

AI 서버는 **FastAPI 기반 비동기 서버**로,

- **HTTP** 요청(주기적 **/api/v1/get_decision, /api/v1/match/start, /api/v1/match/end**)
1. **AI 팩맨 시스템**
    
    → 팩맨 본체(**pacman_main**)의 **전술 의사결정** 담당 (**/get_decision** HTTP)
    
    *(분신은 베타 이후 스코프)*
    

---

## 1. AI 팩맨 시스템

AI 팩맨은 **팩맨/분신 유닛의 전술적 의사결정**만 담당하는 “두뇌”입니다.

- 입력
    - **global_context** (nav_frame 포함)
    - **ai_squad_context** (pacman_main 상태/방향, clones: [])
    - **player_team_context[]** (NavMesh 코스트/거리, 이벤트, 룬, DRIVER racer_nav_delta 등)
    - **commander_context** (지휘관 캐시, 행동 로그 등 / 필드명 유지)
        - **recent_actions**는 게임 중 **적 지휘관 행동 로그로 참고 가능**
    - **map_context** (p_pellet_locations, p_point_locations)
- 출력:
    - **/api/v1/get_decision** Response 안의
        - **decision.unit_commands[]** (**pacman_main** 명령만)
        - **decision.squad_objective, decision.reasoning, decision.confidence**
        - **brain_cam_data** (Perception / Reasoning / Decision 3단계 설명)

### 1) 처리 흐름 (요약)

```json
언리얼 게임 서버 (약 2~3초마다)
			↓ (room_id + global_context + ai_squad_context
					+ player_team_context[] + commander_context + map_context)
AI 팩맨 서버 (FastAPI)
- 분석: 위협도/기회 요소 계산
- 전술 선택: 전략/데이터 기반 전술 선택
- 명령 생성: unit_commands (MOVE_TO_LOCATION, INTERCEPT, RETREAT 등)
- 설명 생성: brain_cam_data (Perception/Reasoning/Decision)
			↓
언리얼 게임 서버
- unit_commands 파싱 후 팩맨 제어
- brain_cam_data 선택적으로 디버그/관전 UI에 표시
```

### 2) 주요 특징

- **2~3초 주기**로 반복되는 **짧은 의사결정 루프**
- **명령 중심 출력**
    - **unit_id + directive_code + directive_name + params**
    - 예: **MOVE_TO_LOCATION, INTERCEPT, RETREAT** 등
- **설명 가능한 AI**
    - **brain_cam_data**로
        - “왜 이 전술을 선택했는지”
        - “어떤 위협/기회를 봤는지”를 텍스트로 제공
- **팩맨/클론만 제어(지휘관(사람)과 드라이버 플레이어는 AI 팩맨의 적(enemy)임)**
    - 언리얼의 플레이어/지휘관 조작은 건드리지 않고, **팩맨 진영 유닛 행동만** 책임집니다.

---

# III. AI 팩맨 API 명세

## 1. 의사결정 요청 (Get Decision)

본 API는 언리얼 게임 서버가 일정 주기(예: 3~5초)로 AI 서버에 **현재 게임 상태(room_id 기준)** 를 전달하고,
AI 서버로부터 **AI 팩맨 의사결정 결과(unit_commands)** 를 수신하기 위한 핵심 엔드포인트입니다.

- 베타(v1.4.0) 범위에서는 **AI 팩맨 의사결정(HTTP)** 만 사용합니다.
- Overseer/Commander(WebSocket STT/TTS, Commander Report 등)는 **베타 범위에서 제외**되며,
해당 기능은 향후 버전에서 별도 스펙으로 복구/확장될 수 있습니다.

게임의 핵심 루프에서 호출되는 API입니다.

- **Endpoint**: **/api/v1/get_decision**
- **Method**: **POST**
- **Content-Type**: **application/json**
- **호출 시점**: 게임 진행 중 **2~3초마다**
- **타임아웃 권장**: 2초 이내

## **2. Request Body (JSON) 예시**

```json
{
  "room_id": "GAME_12345",

  "global_context": {
    "remaining_time": 420.5,
    "game_phase": "MID_GAME",
    "match_start_time": "2025-11-08T15:20:00.000Z",

    "nav_frame": {
      "timestamp_sec": null,
      "delta_time_sec": null,
      "inter_driver_avg_distance": null,
      "delta_inter_driver_avg_distance": null
    }
  },

  "ai_squad_context": {
    "pacman_main": {
      "position": { "x": 150.0, "y": 300.0, "z": 0.0 },
      "hp": 100.0,
      "speed": 12.5,
      "capture_gauge": 45.0,
      "is_invulnerable": false,

      "rotation_yaw_deg": null,
      "forward_vector": null
    },

    "clones": [],

    "p_pellet_cooldown": 5.0,
    "last_pellet_consumed_at": "2025-11-08T15:29:30.000Z"
  },

  "player_team_context": [
    {
      "player_id": "steam_76561198000000001",
      "player_name": "지휘관_닉네임",
      "player_type": "COMMANDER",
      "role": 1,

      "position": { "x": 0.0, "y": 0.0, "z": 0.0 },
      "velocity": { "x": 0.0, "y": 0.0 },

      "rotation_yaw_deg": null,

      "hp": 100.0,
      "is_boosting": false,
      "recent_action": "issuing_orders",

      "distance_to_pacman": 250.0,
      "navmesh_cost_to_pacman": 280.0,
      "navmesh_distance_to_pacman": null,
      "navmesh_path_valid": true,

      "events": [],
      "equipped_runes": [],
      "commander_interaction": {
        "last_voice_command_at": "2025-11-08T15:29:40.000Z",
        "last_button_command_at": "2025-11-08T15:29:45.000Z"
      }
    },

    {
      "player_id": "steam_76561198000000002",
      "player_name": "드라이버1_닉네임",
      "player_type": "DRIVER",
      "role": 2,

      "position": { "x": 120.0, "y": 250.0, "z": 0.0 },
      "velocity": { "x": 20.0, "y": 3.0 },

      "rotation_yaw_deg": null,

      "hp": 72.5,
      "is_boosting": true,
      "recent_action": "back_attack_attempt",

      "distance_to_pacman": 35.2,
      "navmesh_cost_to_pacman": 42.7,
      "navmesh_distance_to_pacman": null,
      "navmesh_path_valid": true,

      "racer_nav_delta": {
        "player_index": 1,
        "relative_angle_to_pacman_deg": null,
        "delta_position": { "x": 0.0, "y": 0.0, "z": 0.0 },
        "delta_straight_distance": 0.0,
        "delta_path_distance": 0.0,
        "delta_path_cost": 0.0,
        "movement_direction_change_deg": 0.0,
        "average_speed": 0.0,
        "relative_bearing_change_deg": 0.0
      },

      "events": [
        {
          "event_code": 100,
          "event_type": "FUEL_LOW",
          "severity": "HIGH",
          "value": 15.0,
          "threshold": 20.0
        }
      ],
      "equipped_runes": [
        {
          "rune_id": "RUNE_SPEED_01",
          "name": "질주의 룬",
          "duration_remaining": 25.0
        }
      ],
      "commander_interaction": {
        "last_voice_command_at": "2025-11-08T15:29:40.000Z",
        "last_button_command_at": "2025-11-08T15:29:45.000Z"
      }
    },

    {
      "player_id": "steam_76561198000000003",
      "player_name": "드라이버2_닉네임",
      "player_type": "DRIVER",
      "role": 3,

      "position": { "x": 80.0, "y": 200.0, "z": 0.0 },
      "velocity": { "x": 15.0, "y": -2.0 },

      "rotation_yaw_deg": null,

      "hp": 95.0,
      "is_boosting": false,
      "recent_action": "patrolling",

      "distance_to_pacman": 80.0,
      "navmesh_cost_to_pacman": 95.5,
      "navmesh_distance_to_pacman": null,
      "navmesh_path_valid": true,

      "racer_nav_delta": {
        "player_index": 2,
        "relative_angle_to_pacman_deg": null,
        "delta_position": { "x": 0.0, "y": 0.0, "z": 0.0 },
        "delta_straight_distance": 0.0,
        "delta_path_distance": 0.0,
        "delta_path_cost": 0.0,
        "movement_direction_change_deg": 0.0,
        "average_speed": 0.0,
        "relative_bearing_change_deg": 0.0
      },

      "events": [
        {
          "event_code": 210,
          "event_type": "RISK_HIGH",
          "severity": "MEDIUM",
          "value": 0.78
        }
      ],
      "equipped_runes": [],
      "commander_interaction": {
        "last_voice_command_at": null,
        "last_button_command_at": "2025-11-08T15:29:20.000Z"
      }
    },

    {
      "player_id": "steam_76561198000000004",
      "player_name": "드라이버3_닉네임",
      "player_type": "DRIVER",
      "role": 4,

      "position": { "x": 60.0, "y": 180.0, "z": 0.0 },
      "velocity": { "x": 10.0, "y": 1.0 },

      "rotation_yaw_deg": null,

      "hp": 40.0,
      "is_boosting": false,
      "recent_action": "retreating",

      "distance_to_pacman": 100.0,
      "navmesh_cost_to_pacman": 120.0,
      "navmesh_distance_to_pacman": null,
      "navmesh_path_valid": true,

      "racer_nav_delta": {
        "player_index": 3,
        "relative_angle_to_pacman_deg": null,
        "delta_position": { "x": 0.0, "y": 0.0, "z": 0.0 },
        "delta_straight_distance": 0.0,
        "delta_path_distance": 0.0,
        "delta_path_cost": 0.0,
        "movement_direction_change_deg": 0.0,
        "average_speed": 0.0,
        "relative_bearing_change_deg": 0.0
      },

      "events": [
        {
          "event_code": 120,
          "event_type": "HP_LOW",
          "severity": "HIGH",
          "value": 40.0,
          "threshold": 50.0
        }
      ],
      "equipped_runes": [
        {
          "rune_id": "RUNE_SHIELD_01",
          "name": "방어의 룬",
          "duration_remaining": 10.0
        }
      ],
      "commander_interaction": {
        "last_voice_command_at": "2025-11-08T15:29:10.000Z",
        "last_button_command_at": null
      }
    }
  ],

  "commander_context": {
    "commander_id": "steam_76561198000000001",
    "cash_status": {
      "current_cash": 320,
      "total_cash_spent": 180,
      "total_cash_collected": 210
    },
    "recent_actions": [
      {
        "timestamp": "2025-11-08T15:29:40.000Z",
        "action_type": "GIVE_ITEM",
        "target_player_id": "steam_76561198000000002",
        "item_id": "FUEL_PACK_SMALL",
        "cash_delta": -50
      },
      {
        "timestamp": "2025-11-08T15:29:50.000Z",
        "action_type": "COLLECT_CASH",
        "target_player_id": null,
        "item_id": null,
        "cash_delta": 100
      }
    ]
  },

  "map_context": {
    "pacman_spawn": { "x": 400.0, "y": 400.0, "z": 0.0 },
    "p_pellet_locations": [
      { "id": "pellet_A", "x": -71.0, "y": -24895.0, "z": 170.0, "available": true, "cooldown": 0.0 }
    ],
    "p_point_locations": [
      { "id": "ppoint_1", "x": 234.1, "y": 678.9, "z": 0.2, "available": false },
      { "id": "ppoint_2", "x": 600.0, "y": 200.0, "z": 0.8, "available": true }
    ]
  }
}
```

## 3. Request 필드 상세 설명 (v1.4.0)

**권장 이벤트 타입 예시**

- **FUEL_LOW**: 연료가 특정 임계치 미만
- **HP_LOW**: HP가 일정 이하
- **RISK_HIGH**: AI가 계산한 위험도 지수가 높음
- 향후 필요 시 새로운 **event_type**을 합의 후 추가

### 1) 최상위 필드

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **room_id** | string | ✓ | 게임 방 ID. 같은 매치 동안 **/match/start, /get_decision, /match/end**에서 공통으로 사용하는 키. 예: **"GAME_12345"** |
| **global_context** | object | ✓ | 경기 전체 전역 정보(남은 시간, 페이즈, 프레임/내비 프레임 등). |
| **ai_squad_context** | object | ✓ | 팩맨 본체 상태 + (향후 확장용) 분신 슬롯 + P-Pellet 관련 쿨타임/사용 이력. |
| **player_team_context** | array | ✓ | 플레이어(지휘관/드라이버) 상태 목록. 위치/속도/HP/이벤트 + (추가) 경로거리/델타/각도 슬롯 포함. |
| **commander_context** | object | ✗ | 상대(적) 지휘관의 캐시/행동 로그(리포트 평가 + 팩맨 판단 보조). |
| **map_context** | object | ✓ | 맵/오브젝트 좌표(팩맨 스폰, P-Pellet 위치, P-Point 위치). 없으면 서버에서 기본값/내부 맵 데이터를 사용할 수 있음. |

### **2) global_context**

```json
"global_context": {
  "remaining_time": 420.5,
  "game_phase": "MID_GAME",
  "match_start_time": "2025-11-08T15:20:00.000Z",
  "nav_frame": {
    "timestamp_sec": null,
    "delta_time_sec": null,
    "inter_driver_avg_distance": null,
    "delta_inter_driver_avg_distance": null
  }
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **remaining_time** | number | ✓ | 남은 경기 시간(초). 예: **420.5**  → 7분 0.5초 남음. |
| **game_phase** | string | ✓ | 경기 진행 단계. 예: **"EARLY_GAME", "MID_GAME", "LATE_GAME"** |
| **match_start_time** | string | ✗ | 경기 시작 시각(ISO 8601). 리포트/로그 용도. |
| **nav_frame** | object | null | ✗ | UE **FNavSystemLLMData** 매핑 프레임. UE가 채워서 전달(미지원/미구현 시 null 허용). |

**2-1) nav_frame**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **timestamp_sec** | number | null | ✗ | UE Timestamp(초). 예: 매치/플레이 경과 시간(Elapsed Time). |
| **delta_time_sec** | number | null | ✗ | UE DeltaTime(초). 직전 프레임 대비 경과. |
| **inter_driver_avg_distance** | number | null | ✗ | 플레이어(드라이버) 간 평균 거리(cm). 뭉침/분산 판단용. |
| **delta_inter_driver_avg_distance** | number | null | ✗ | 평균 거리의 변화량(cm). “빠르게 뭉치는 중/흩어지는 중” 판단용. |

### 3) **ai_squad_context** (팩맨/분신 상태)

```json
"ai_squad_context": {
  "pacman_main": {
    "position": {"x": 150.0, "y": 300.0, "z": 0.0},
    "hp": 100.0,
    "speed": 12.5,
    "capture_gauge": 45.0,
    "is_invulnerable": false,
    "rotation_yaw_deg": null,
    "forward_vector": null
  },
  "clones": [],
  "p_pellet_cooldown": 5.0,
  "last_pellet_consumed_at": "2025-11-08T15:29:30.000Z"
}
```

**3-1) pacman_main**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **position** | object | ✓ | 팩맨 본체 위치. **{x, y, z}** (게임 월드 좌표계) |
| **position.x** | number | ✓ | X 좌표 |
| **position.y** | number | ✓ | Y 좌표 |
| **position.z** | number | ✓ | Z 좌표 |
| **hp** | number | ✓ | 팩맨 본체 체력 |
| **speed** | number | ✓ | 현재 이동 속도(스칼라). 단위는 게임/UE 정의값을 따름 |
| **capture_gauge** | number | ✓ | 포획 게이지(0~100 등). 일정 수치 이상이면 위기 판단 가능 |
| **is_invulnerable** | bool | ✓ | 무적 상태 여부,  **true**이면 공격/포획에 면역인 상태. |
| **rotation_yaw_deg** | number | null | ✗ | 팩맨 바라보는 방향 Yaw(도). UE에서 제공 가능 시 채움 |
| **forward_vector** | object | null | ✗ | 전방 벡터(정규화 권장). 예: **{x,y,z}** |

**3-2) clones[] (분신 슬롯)**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **p_pellet_cooldown** | number | ✓ | P-Pellet 재사용 대기 시간(초). **0**이면 즉시 사용 가능 |
| **last_pellet_consumed_at** | string | null | ✗ | 마지막 P-Pellet 사용 시각(ISO 8601). 전략/로그/리포트에 사용. 서버가 “얼마나 오래 안 썼는지”를 계산해서 전략에 반영 가능. |

**3-3) P-Pellet 관련 필드**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **clones** | array | ✓ | v1.4.0 베타에서는 **분신 미사용 정책**으로 **항상 []**. (스키마/확장성 유지 목적) |

### 4) **player_team_context** (지휘관/드라이버 상태 + 이벤트 + 네비 경로/델타)

> 공통: COMMANDER/DRIVER 모두 동일한 기본 슬롯을 사용합니다.
> 
> 
> **추가**: DRIVER는 **racer_nav_delta**를 통해 “각도/델타” 정보를 추가로 전달할 수 있습니다.
> 

```json
"player_team_context": [
  {
    "player_id": "steam_76561198000000001",
    "player_name": "지휘관_닉네임",
    "player_type": "COMMANDER",
    "role": 1,

    "position": { "x": 0.0, "y": 0.0, "z": 0.0 },
    "velocity": { "x": 0.0, "y": 0.0 },

    "rotation_yaw_deg": null,

    "hp": 100.0,
    "is_boosting": false,
    "recent_action": "issuing_orders",

    "distance_to_pacman": 250.0,
    "navmesh_cost_to_pacman": 280.0,
    "navmesh_distance_to_pacman": null,
    "navmesh_path_valid": true,

    "events": [],
    "equipped_runes": [],
    "commander_interaction": {
      "last_voice_command_at": "2025-11-08T15:29:40.000Z",
      "last_button_command_at": "2025-11-08T15:29:45.000Z"
    }
  },

  {
    "player_id": "steam_76561198000000002",
    "player_name": "드라이버1_닉네임",
    "player_type": "DRIVER",
    "role": 2,

    "position": { "x": 120.0, "y": 250.0, "z": 0.0 },
    "velocity": { "x": 20.0, "y": 3.0 },

    "rotation_yaw_deg": null,

    "hp": 72.5,
    "is_boosting": true,
    "recent_action": "back_attack_attempt",

    "distance_to_pacman": 35.2,
    "navmesh_cost_to_pacman": 42.7,
    "navmesh_distance_to_pacman": null,
    "navmesh_path_valid": true,

    "racer_nav_delta": {
      "player_index": 1,
      "relative_angle_to_pacman_deg": null,
      "delta_position": { "x": 0.0, "y": 0.0, "z": 0.0 },
      "delta_straight_distance": 0.0,
      "delta_path_distance": 0.0,
      "delta_path_cost": 0.0,
      "movement_direction_change_deg": 0.0,
      "average_speed": 0.0,
      "relative_bearing_change_deg": 0.0
    },

    "events": [
      {
        "event_code": 100,
        "event_type": "FUEL_LOW",
        "severity": "HIGH",
        "value": 15.0,
        "threshold": 20.0
      }
    ],
    "equipped_runes": [
      {
        "rune_id": "RUNE_SPEED_01",
        "name": "질주의 룬",
        "duration_remaining": 25.0
      }
    ],
    "commander_interaction": {
      "last_voice_command_at": "2025-11-08T15:29:40.000Z",
      "last_button_command_at": "2025-11-08T15:29:45.000Z"
    }
  },

  {
    "player_id": "steam_76561198000000003",
    "player_name": "드라이버2_닉네임",
    "player_type": "DRIVER",
    "role": 3,

    "position": { "x": 80.0, "y": 200.0, "z": 0.0 },
    "velocity": { "x": 15.0, "y": -2.0 },

    "rotation_yaw_deg": null,

    "hp": 95.0,
    "is_boosting": false,
    "recent_action": "patrolling",

    "distance_to_pacman": 80.0,
    "navmesh_cost_to_pacman": 95.5,
    "navmesh_distance_to_pacman": null,
    "navmesh_path_valid": true,

    "racer_nav_delta": {
      "player_index": 2,
      "relative_angle_to_pacman_deg": null,
      "delta_position": { "x": 0.0, "y": 0.0, "z": 0.0 },
      "delta_straight_distance": 0.0,
      "delta_path_distance": 0.0,
      "delta_path_cost": 0.0,
      "movement_direction_change_deg": 0.0,
      "average_speed": 0.0,
      "relative_bearing_change_deg": 0.0
    },

    "events": [
      {
        "event_code": 210,
        "event_type": "RISK_HIGH",
        "severity": "MEDIUM",
        "value": 0.78
      }
    ],
    "equipped_runes": [],
    "commander_interaction": {
      "last_voice_command_at": null,
      "last_button_command_at": "2025-11-08T15:29:20.000Z"
    }
  },

  {
    "player_id": "steam_76561198000000004",
    "player_name": "드라이버3_닉네임",
    "player_type": "DRIVER",
    "role": 4,

    "position": { "x": 60.0, "y": 180.0, "z": 0.0 },
    "velocity": { "x": 10.0, "y": 1.0 },

    "rotation_yaw_deg": null,

    "hp": 40.0,
    "is_boosting": false,
    "recent_action": "retreating",

    "distance_to_pacman": 100.0,
    "navmesh_cost_to_pacman": 120.0,
    "navmesh_distance_to_pacman": null,
    "navmesh_path_valid": true,

    "racer_nav_delta": {
      "player_index": 3,
      "relative_angle_to_pacman_deg": null,
      "delta_position": { "x": 0.0, "y": 0.0, "z": 0.0 },
      "delta_straight_distance": 0.0,
      "delta_path_distance": 0.0,
      "delta_path_cost": 0.0,
      "movement_direction_change_deg": 0.0,
      "average_speed": 0.0,
      "relative_bearing_change_deg": 0.0
    },

    "events": [
      {
        "event_code": 120,
        "event_type": "HP_LOW",
        "severity": "HIGH",
        "value": 40.0,
        "threshold": 50.0
      }
    ],
    "equipped_runes": [
      {
        "rune_id": "RUNE_SHIELD_01",
        "name": "방어의 룬",
        "duration_remaining": 10.0
      }
    ],
    "commander_interaction": {
      "last_voice_command_at": "2025-11-08T15:29:10.000Z",
      "last_button_command_at": null
    }
  }
]
```

**4-1) 플레이어 공통 필드**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **player_id** | string | ✓ | Steam ID. **/match/start**의 player_ids[]와 동일 |
| **player_name** | string | ✓ | 인게임 닉네임. **/match/start**의 player_names[]와 동일 |
| **player_type** | string | ✓ | **"COMMANDER"** 또는 **"DRIVER"** |
| **role** | number | ✓ | 기존 역할 코드 (1=COMMANDER, 2=DRIVER1, 3=DRIVER2, 4=DRIVER3 …) |
| **position** | object | ✓ | 플레이어 위치 **{x, y, z}** |
| **position.x** | number | ✓ | X 좌표 |
| **position.y** | number | ✓ | Y 좌표 |
| **position.z** | number | ✓ | Z 좌표 |
| **velocity** | object | ✓ | 속도 벡터(2D) **{x, y}** (단위는 UE/게임 정의값) |
| **velocity.x** | number | ✓ | X 방향 속도 |
| **velocity.y** | number | ✓ | Y 방향 속도 |
| **rotation_yaw_deg** | number | null | ✗ | 플레이어 바라보는 방향 Yaw(도). 제공 가능 시 채움 |
| **hp** | number | ✓ | 플레이어 체력(0~100). 0이면 전투불능/탈락 상태. |
| **is_boosting** | bool | ✓ | 현재 부스터(가속)를 사용 중인지 여부 |
| **recent_action** | string | ✓ | 최근 행동 라벨. 예: **"back_attack_attempt", "normal_drive", "patrolling"** 등. AI가 플레이어의 의도/스타일을 파악하는 데 사용. |
| **distance_to_pacman** | number | ✓ | 팩맨 ↔ 플레이어 **직선 거리**(UE StraightDistance 대응). 단위는 UE/게임 정의값 |
| **navmesh_cost_to_pacman** | number | null | ✓ | 팩맨까지의 **경로 비용**(UE PathCost 대응). **navmesh_path_valid=false**면 null 허용 |
| **navmesh_distance_to_pacman** | number | null | ✗ | 팩맨까지의 **경로 거리**(UE PathDistance 대응). **navmesh_path_valid=false**면 null 권장 |
| **navmesh_path_valid** | bool | ✓ | 경로 계산 성공 여부. 실패 시 cost/distance는 null 권장 |
| **events** | array | ✓ | 현재 적용 이벤트 목록(없으면 **[]**). |
| **equipped_runes** | array | ✓ | 룬 장착 상태(없으면 **[]**) |
| **commander_interaction** | object | ✓ | 지휘관의 최근 지시 타이밍(음성/버튼) |
- **role 필드 (추후 모드에 따른 변경 가능성 있음)**

| role | 설명 |
| --- | --- |
| 1 | **COMMANDER:** 전체를 보는 지휘관 |
| 2 | **DRIVER1:** 실제 차량/유닛을 조작하는 플레이어1 |
| 3 | **DRIVER2:** 실제 차량/유닛을 조작하는 플레이어2 |
| 4 | **DRIVER3:** 실제 차량/유닛을 조작하는 플레이어3 |

**4-2) racer_nav_delta (DRIVER 전용 추가 슬롯)**

- **위치**: **player_team_context[]** 중 **player_type == "DRIVER"** 인 원소에만 포함 가능
- **설명**: UE에서 계산한 “상대각/델타/변화량” 정보 묶음
- **중복 정책**: **distance_to_pacman**, **navmesh_cost_to_pacman**, **navmesh_distance_to_pacman**는 **상위 공통 필드에만** 유지하고, **racer_nav_delta**에는 **중복으로 넣지 않습니다.**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **racer_nav_delta** | object | ✗ | DRIVER 전용 델타 정보 묶음 |
| **player_index** | number | ✓ | 드라이버 인덱스(예: 1=DRIVER1, 2=DRIVER2, 3=DRIVER3) |
| **relative_angle_to_pacman_deg** | number | null | ✗ | 팩맨 기준 상대각(도) |
| **delta_position** | object | ✓ | 직전 프레임 대비 위치 변화량 **{x,y,z}** |
| **delta_position.x** | number | ✓ | ΔX |
| **delta_position.y** | number | ✓ | ΔY |
| **delta_position.z** | number | ✓ | ΔZ |
| **delta_straight_distance** | number | ✓ | 직선거리 변화량(cm) |
| **delta_path_distance** | number | ✓ | 경로거리 변화량(cm) |
| **delta_path_cost** | number | ✓ | 경로비용 변화량(cost) |
| **movement_direction_change_deg** | number | ✓ | 이동 방향 변화량(도) |
| **average_speed** | number | ✓ | 평균 속도(cm/s) |
| **relative_bearing_change_deg** | number | ✓ | 상대 방위 변화량(도) |

**4-3) events[] 상세**

- **위치**: **player_team_context[].events**
- **타입**: 배열(없으면 **[]**)

```json
"events": [
  {
	  "event_code": 100,
    "event_type": "FUEL_LOW",
    "severity": "HIGH",
    "value": 15.0,
    "threshold": 20.0
  },
  {
	  "event_code": 110,
    "event_type": "RISK_HIGH",
    "severity": "MEDIUM",
    "value": 0.78,
    "threshold": null
  }
]
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **event_code** | number | ✓ | 정수형 코드. 예: 100=FUEL_LOW, 120=HP_LOW, 210=RISK_HIGH. 공통 enum 테이블로 관리 |
| **event_type** | string | ✓ | 이벤트 종류. 예: **"FUEL_LOW", "HP_LOW", "RISK_HIGH", "STATION_CLOSED"** 등. |
| **severity** | string | ✓ | 심각도**. "INFO", "LOW", "MEDIUM", "HIGH", "CRITICAL"** 등 사전 정의된 값 사용. |
| **value** | number | ✗ | 현재 수치 값. 예: 연료 15%, 위험도 스코어 0.78 등. |
| **threshold** | number | ✗ | 경고 기준 값. 예: 경고 기준 연료 20%. **value < threshold**일 때 경고. |
- 이 **events** 정보는:
    - **AI 팩맨**: “누가 지금 가장 위태로운지” 판단하는 데 사용
    - **OVERSEER**: “어떤 순간에 TTS 알림을 줄지” 결정하는 데 사용

**권장 이벤트 enum 예시 (event_code / event_type)**

| event_code | event_type | 설명 |
| --- | --- | --- |
| 100 | FUEL_LOW | 연료가 임계치 미만 |
| 101 | HP_LOW | HP가 일정 이하 |
| 102 | RISK_HIGH | AI 계산 위험도가 HIGH 이상 |
| 103 | CAPTURE_GAUGE_SPIKE | 포획 게이지가 짧은 시간에 급상승 |
| 104 | PPELLET_AVAILABLE | 가까운 P-Pellet 사용 가능 |
| 105 | COMMANDER_CALLOUT | 지휘관이 해당 플레이어를 콜 |
- 새로운 이벤트가 필요하면
    - **코드 범위만 합의** 후 (**200+** 등**) event_code / event_type** 를 표에 추가.

**4-4) equipped_runes[] 상세**

지휘관/플레이어의 **룬 장착 상태**를 전달합니다.

```json
"equipped_runes": [
  {
    "rune_id": "RUNE_SPEED_01",
    "name": "질주의 룬",
    "duration_remaining": 25.0
  }
]
```

| 필드 | 타입 | 설명 |
| --- | --- | --- |
| **rune_id** | string | 룬 식별자 (내부 ID) |
| **name** | string | 룬 이름 (UI 표시용) |
| **duration_remaining** | number | 남은 지속시간(초). 0 이하이면 효과 없음 |

**4-5) commander_interaction 상세**

> 플레이어별로 지휘관의 관심/지시 타이밍을 추적하기 위한 필드입니다.
> 

```json
"commander_interaction": {
  "last_voice_command_at": "2025-11-08T15:29:40.000Z",
  "last_button_command_at": "2025-11-08T15:29:45.000Z"
}
```

| 필드 | 타입 | 설명 |
| --- | --- | --- |
| **last_voice_command_at** | string | null | 해당 플레이어에게 **마지막 음성(STT) 지시**가 내려간 시각 |
| **last_button_command_at** | string | null | 마지막 UI/버튼 기반 지시 시각 |

**활용 예시**

1. **TTS 중복 방지**
    - FUEL_LOW 이벤트가 계속 떠도, **last_voice_command_at**이 10초 이내라면 **같은 경고를 반복하지 않음**.
2. **방치 플레이어 감지**
    - 60초 이상 두 필드가 모두 오래되거나 null이면 → **NEGLECTED** 이벤트 발생.
3. **리포트 통계**
    - 플레이어별 **지휘관의 관심 분포/시간** 계산에 사용.

### **5) commander_context 필드 상세**

> commander_context는 상대(적) 지휘관의 캐쉬/아이템 사용 기록/행동 정보를 담으며, 용도에 따라 아래처럼 사용합니다.
> 
> - **cash_status, management_stats**: 매치 종료 후 **지휘관 리포트(평가 데이터)** 산출에 사용
> - **recent_actions**: 게임 진행 중 **AI 팩맨이 참고하는 “지휘관 행동 로그”** (전술/위험도 판단 보조)
> 
> ※ recent_actions는 “전지적/비공개 정보 제공”으로 오해되지 않도록, **게임 규칙상 공유 가능한 이벤트 범위**로만 구성하는 것을 원칙으로 합니다..
> 

```json
"commander_context": {
  "commander_id": "C1",
  "cash_status": {
    "current_cash": 320,
    "total_cash_spent": 180,
    "total_cash_collected": 210
  },
  "recent_actions": [
    {
      "timestamp": "2025-11-08T15:29:40.000Z",
      "action_type": "GIVE_ITEM",
      "target_player_id": "P1",
      "item_id": "FUEL_PACK_SMALL",
      "cash_delta": -50
    }
  ],
  "management_stats": {
    "voice_command_count": 23,
    "button_command_count": 22,
    "player_attention_distribution": {
      "P1": 18,
      "P2": 15,
      "P3": 12
    },
    "item_efficiency_score": 0.78,
    "average_response_time_seconds": 8.5
  }
}
```

5-1) **commander_context**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **commander_id** | string | ✓ | 지휘관 플레이어 Steam ID. |
| **cash_status** | object | ✓ | 지휘관 캐시 상태 요약. |
| **recent_actions** | array | ✗ | 최근 캐시 관련 액션(GIVE_ITEM, COLLECT_CASH 등) 타임라인. |

**5-2) cash_status**

| 필드 | 타입 | 설명 |
| --- | --- | --- |
| **current_cash** | number | 현재 사용 가능한 캐쉬 |
| **total_cash_spent** | number | 이번 매치에서 사용한 캐쉬 총합 |
| **total_cash_collected** | number | 이번 매치에서 회수/획득한 캐쉬 총합 |

**5-3) recent_actions[]**

| 필드 | 타입 | 설명 |
| --- | --- | --- |
| **timestamp** | string | 액션 발생 시각(ISO8601) |
| **action_type** | string | **"GIVE_ITEM", "SPEND_CASH", "COLLECT_CASH", "REFUND_CASH"** 등 |
| **target_player_id** | string | null | 대상 플레이어 ID, 없는 경우 null |
| **item_id** | string | null | 관련 아이템 ID. 없는 경우 null |
| **cash_delta** | number | 캐쉬 변화량 (사용 시 음수, 회수 시 양수) |

**5-4) management_stats**

- Commander 리포트(지휘관 리포트)의 **지휘관 평가 지표** 계산에 사용됩니다.

| 필드 | 타입 | 설명 |
| --- | --- | --- |
| **voice_command_count** | number | 음성(STT) 지시 횟수 |
| **button_command_count** | number | UI/버튼 지시 횟수 |
| **player_attention_distribution** | object | 플레이어별 관심 횟수/비율 |
| **item_efficiency_score** | number | 아이템 효율 지수 (0~1) |
| **average_response_time_seconds** | number | 평균 대응 속도(초) |

### 6) **map_context** (맵/지형 + P-POINT 위치)

- 목적: 에이전트가 맵 상의 “고정 오브젝트 좌표/가용 상태”를 동일한 스키마로 참조하기 위함
- 베타 기준 사용 항목:
    - pacman_spawn: 사용
    - p_point_locations: 사용(추가)
    - **p_pellet_locations**: 사용(그대로 유지)
- **pacman_spawn (필수)**
    - Pacman 최초 스폰 좌표
- **p_point_locations (필수)**
    - P-Point(점수/목표 포인트) 목록
    - 각 항목: id, x, y, z, available
    - id 규칙: trim 적용(양끝 공백 금지)

```json
"map_context": {
  "pacman_spawn": { "x": 0.0, "y": 0.0, "z": 0.0 },
  "p_pellet_locations": [
    {
      "id": "pellet_A",
      "x": -71.0,
      "y": -24895.0,
      "z": 170.0,
      "available": true,
      "cooldown": 0.0
    }
  ],
  "p_point_locations": [
    { 
	    "id": "p_point_1",
		  "x": -2721.180664,
		  "y": -22485.201172,
		  "z": 170.0,
		  "available": true 
    },{ 
	    "id": "p_point_2",
	    "x":   -71.180664,
	    "y": -22485.201172,
	    "z": 170.0,
	    "available": true 
    },{
	    "id": "p_point_3",
	    "x":   -71.180664,
	    "y": -23835.201172,
	    "z": 170.0,
	    "available": true 
    },{
	    "id": "p_point_4",
	    "x":   -71.180664,
	    "y": -21935.201172,
	    "z": 170.0,
	    "available": true 
    },{
	    "id": "p_point_5", 
	    "x":   -71.180664,
	    "y": -23485.201172,
	    "z": 170.0, "available": true 
    },{
	    "id": "p_point_6", 
	    "x":   -71.180664,
	    "y": -23835.201172,
	    "z": 170.0,
	    "available": true 
    },{
	    "id": "p_point_7", 
	    "x":   -71.180664,
	    "y": -25935.201172,
	    "z": 170.0,
	    "available": true 
    },{
	    "id": "p_point_8", 
	    "x":   -71.180664,
	    "y": -22485.201172,
	    "z": 170.0, "available": true 
    },{
	    "id": "p_point_9",
	    "x":   -71.180664,
	    "y": -26835.201172,
	    "z": 170.0,
	    "available": true 
    },{
	    "id": "p_point_10",
	    "x":   -71.180664,
	    "y": -27935.201172,
	    "z": 170.0,
	    "available": true 
    }
  ]
}
```

**6-1) pacman_spawn**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **pacman_spawn** | object | ✗ | 팩맨의 기본 스폰 지점(리필/리셋 기준 위치). |
| **pacman_spawn.x** | number | ✗ | X 좌표. |
| **pacman_spawn.y** | number | ✗ | Y 좌표. |
| **pacman_spawn**.**z** | number | ✗ | Z좌표. |

**6-2) p_pellet_locations[]**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **id** | string | ✓ | P-Pellet 개체 ID. 예: **"pellet_1", "pellet_2"**. 서버 내부에서 P-Pellet 객체와 매핑할 때 사용. |
| **x** | number | ✓ | P-Pellet X 좌표. |
| **y** | number | ✓ | P-Pellet Y 좌표. |
| **z** | number | ✓ | P-Pellet Z 좌표. 고가도로/지형 높이 판단에 쓰일 수 있음. |
| **available** | bool | ✓ | 현재 이 P-Pellet이 활성(먹을 수 있는) 상태인지 여부. |
| **cooldown** | number | ✓ | 이 P-Pellet이 다시 등장할 때까지 남은 시간(초). **0**이면 이미 리스폰된 상태. |
- **available == true**인 P-Pellet 중에서 거리, 위치, 위험도 등을 보고 **“어디를 우선 먹으러 갈지”** 판단 가능.
- **cooldown** 정보로 “좀만 더 버티면 풀리는 P-Pellet”을 고려한 전략도 가능.

**6-3) p_point_locations[] (추가)**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **id** | string | ✓ | **P-Point 개체 ID.** 예: **"p_point_1", "p_point_2"** |
| **x** | number | ✓ | X |
| **y** | number | ✓ | Y |
| **z** | number | ✓ | Z |
| **available** | bool | ✓ | 현재 습득 가능 여부 |
- 참고: P-Point 습득 시 스킬/효과는 랜덤 자동 발동이므로(HP/주유/시야감소/속도 등), 연동 데이터에는 **“위치/가용 여부”만** 포함합니다.

## 4. Response Body (성공 예시) *(v1.4.0 / 분신 명령 제거)*

```json
{
  "status": "success",
  "timestamp": "2025-11-21T10:15:30.123Z",
  "room_id": "room_ue",

  "meta": {
    "version": "1.4.0",
    "ontology_version": "v1.0.0",
    "latency_ms": 50.0,
    "llm_tokens": 0,
    "fallback_used": false,
    "agent_pipeline": ["analyst", "decision_gate", "planner", "rule_based_tactical", "executor", "logger"],
    "mode": "LIGHT"
  },

  "decision": {
    "squad_objective": "LOW 상황 대응 - P-Point 직행 확보 - 점수 획득",
    "reasoning": "Rule-based + card=tactic_049 (kw=['CONSUME_P_POINT','RETREAT','MOVE','AMBUSH','CHASE'])",
    "confidence": 0.8,
    "unit_commands": [
      {
        "unit_id": "pacman_main",
        "directive_code": 5,
        "directive_name": "RETREAT",
        "params": {
          "safe_zone_position": { "x": 29563.9140625, "y": 47321.50390625, "z": 622.2418823242188 }
        }
      }
    ]
  },

  "brain_cam_data": {
    "perception": {
      "summary": "위협 0명, 리스크 LOW, 포획 게이지 0%",
      "threat_level": "LOW",
      "key_signals": [
        { "type": "OBJECTIVE", "name": "P_POINT", "target_id": "p_point_07" }
      ]
    },
    "reasoning": {
      "retrieved_docs": [
        { "tactic_id": "tactic_049", "title": "P-Point 직행 확보 - 점수 획득", "similarity": null }
      ],
      "selected_tactic_id": "tactic_049",
      "notes": "LLM 미사용(LIGHT). P-Point 우선순위 규칙 적용. 안전지대 후퇴로 리스크 최소화."
    },
    "decision": {
      "final_choice": "LOW 상황 대응 - P-Point 직행 확보 - 점수 획득",
      "unit_commands_summary": "PACMAN RETREAT (safe_zone_position)"
    }
  },

  "message": "Decision generated successfully",
  "request_id": "req_room_ue_0000000000"
}
```

핵심: **title 필드 제거**, **retrieved_docs**는 **문자열 배열 금지** → **{tactic_id,title,similarity}** 객체 배열로 통일

---

## 5. Response 필드 상세 설명

### 1) 최상위 필드

| 필드 | 타입 | 설명 |
| --- | --- | --- |
| **status** | string | 응답 상태. **"success"** 또는 **"error"** |
| **timestamp** | string | 응답 생성 시각(ISO8601, 서버 기준). 예: **"2025-11-08T15:30:00.123Z"** |
| **room_id** | string | 요청에 사용된 게임 방 ID. 예: **"GAME_12345"** |
| **meta** | object | 디버깅·모니터링 메타데이터(버전, 지연 시간, 토큰 수 등) |
| **decision** | object | 이번 틱에서 AI 팩맨이 내린 최종 의사결정(목표·유닛 명령·신뢰도) |
| **brain_cam_data** | object | 디버깅/설명용 Brain Cam 데이터(Perception/Reasoning/Decision 요약) |

**status == "error"** 인 응답 구조는 별도 에러 섹션에서 정의 (이 문단은 **성공 응답** 기준 설명).

---

### 2) **meta** 필드

| 필드 | 타입 | 설명 |
| --- | --- | --- |
| **version** | string | API 응답 스펙 버전. 예: **"1.4.0"** |
| **ontology_version** | string | 위협/기회 분석에 사용된 규칙/온톨로지 버전. 예: **"1.0.0"** |
| **latency_ms** | number | 요청 수신 ~ 응답 생성까지 서버 처리 지연(ms) |
| **fallback_used** | boolean | LLM 대신 룰 기반 폴백을 사용했는지 여부 |
| **agent_pipeline** | string[] | 이번 요청에서 실제로 거친 처리 모듈 이름 목록 |
| **llm_tokens** | number | LLM 호출 시 사용 토큰 수(입력+출력 합산 기준, 구현 선택) |

```json
"meta": {
  "version": "1.4.0",
  "ontology_version": "1.0.0",
  "latency_ms": 420.5,
  "fallback_used": false,
  "agent_pipeline": [
    "analyst",
    "planner",
    "llm_tactical",
    "executor",
    "logger"
  ],
  "llm_tokens": 1250
}
```

### 3) **decision** 필드

| 필드 | 타입 | 설명 |
| --- | --- | --- |
| **squad_objective** | string | 이번 틱에서 AI 팩맨이 추구하는 상위 목표(예: “P-Point 확보”, “P-Pellet 확보 후 역습”) |
| **reasoning** | string | 목표/명령을 선택한 이유에 대한 자연어 설명 |
| **unit_commands** | object[] | 실행해야 할 명령 목록 |
| **confidence** | number | 0~1 신뢰도 |

v1.4.0 베타에서는 **분신 미사용 정책**이므로, **unit_commands[]**는 **pacman_main**에 대한 명령만 생성하는 것을 note로 둡니다. (스키마/확장성은 유지)

---

### 4) **decision.unit_commands[]** 필드

```json
"unit_commands": [
  {
    "unit_id": "pacman_main",
    "directive_code": 7,
    "directive_name": "CONSUME_P_POINT",
    "params": {
      "p_point_id": "ppoint_2",
      "emergency_priority": 1
    }
  }
]
```

| 필드 | 타입 | 설명 |
| --- | --- | --- |
| **unit_id** | string | 명령을 적용할 유닛 ID. v1.4.0 베타는 **"pacman_main"**만 사용 |
| **directive_code** | int | 언리얼팀과 합의한 숫자 명령 코드(예: 1=AMBUSH, 2=MOVE_TO_LOCATION, 7=CONSUME_P_POINT …) |
| **directive_name** | string | 사람이 읽기 위한 이름(디버깅용). 실제 로직은 **directive_code** 기준 |
| **params** | object | 명령별 파라미터(목표 좌표, 타겟 ID, p_point_id 등) |
- 언리얼 C++/블루프린트에서는 **directive_code** 기준으로 **switch/enum** 처리
- **directive_name**은 로그/디버그에만 쓰면 됩니다.

---

### **예시 directive 매핑** *(v1.4.0 / 11,12 제거 + 13→11)*

- **원칙**
    - **필수 파라미터는 언리얼 실행에 꼭 필요한 것만** 둔다.
    - 문서/구현이 엇갈리는 필드는 **별칭(aliases)로 허용**하고, 서버 validator에서 표준 키로 정규화한다.
    - **params**는 directive_name 별로 “필수/선택 파라미터” 규격을 엄격히 맞춘다.

| directive_code | directive_name | 설명 | 선택 파라미터 | 필수 파라미터 |
| --- | --- | --- | --- | --- |
| 1 | AMBUSH | 매복 – 특정 위치에서 대기 후 급습 | trigger_distance, wait_duration | target_position |
| 2 | MOVE_TO_LOCATION | 지정 위치로 이동 | speed_factor, priority, tolerance | target_position |
| 3 | INTERCEPT | 요격 – 플레이어 진로 차단 | approach_angle, intercept_distance, aggressiveness | target_player_id |
| 4 | CHASE | 추격 – 플레이어 압박 | max_chase_duration, speed_factor | target_player_id |
| 5 | RETREAT | 후퇴 – 안전 지대로 이탈 | safe_zone_position, speed_factor | - |
| 6 | PATROL | 순찰 – 구역 반복 이동 | patrol_speed, waypoints[] | patrol_zone |
| 7 | CONSUME_P_POINT | P-Point 섭취/획득 | emergency_priority | p_point_id |
| 8 | CONSUME_PELLET | 파워펠릿(P-Pellet) 섭취 | emergency_priority | pellet_id |
| 9 | GUARD | 방어 – 특정 위치/대상 방어 | guard_radius, duration | guard_target |
| 10 | FLANK | 측면 우회 – 플레이어 측면 공략 | flank_distance | target_player_id, flank_direction |
| 11 | FAKE_RETREAT | 후퇴 위장 – 후퇴인 척 역습 | counter_attack_position | fake_retreat_duration |

> NOTE
> 
> - RETREAT / REGROUP처럼 “필수 파라미터가 없는” directive는, Unreal이 기본 정책(안전지대/리더 위치 등)을 사용한다.
> - **기존 13(FAKE_RETREAT)이 v1.4.0에서 11로 재번호**되었습니다. (11,12는 삭제)

---

### **공통 타입 정의 (params에서 재사용)**

- **Vector3**
    - x: number
    - y: number
    - z: number
- **priority**
    - integer
    - 기본 0, 높을수록 우선 (예: 0~3 또는 0~10은 팀 합의 범위)
    
    | 항목 | 값 |
    | --- | --- |
    | **priority 정책** | **integer**, **기본값 0(미지정 시 0으로 처리)**, **권장 범위 0~3** 
    *(**0=기본, 1=높음, 2=긴급, 3=최우선**; 
    범위 확장 필요 시 0~10으로 확장 가능하나 v1.4.0 기본은 0~3로 고정)* |
- emergency_priority:
    - integer (기본 0, 위기 시 1~3 정도 사용 권장)
- speed_factor
    - number (기본 1.0)
    - **directive_name: "BLOCK_ROUTE"** 의 **params**

**NOTE**

- (v1.4.0) **BLOCK_ROUTE 명령은 제거**되어 더 이상 params 재사용 항목으로 언급하지 않습니다.
- 문서/구현이 엇갈리는 키는 서버 validator에서 정규화(aliases) 가능하되, **문서 표준 키는 아래로 고정**합니다.

---

### **directive별 params 예시**

- **1 - AMBUSH params**

```json
{
  "target_position": { "x": 100.0, "y": 200.0, "z": 0.0 },
  "trigger_distance": 30.0,
  "wait_duration": 2.0
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| target_position | Vector3 | Y | 매복 위치 |
| trigger_distance | number | N | 트리거 거리(미만이면 급습) |
| wait_duration | number | N | 대기 시간(초) |
- **2 - MOVE_TO_LOCATION params**

```json
{
  "target_position": { "x": 234.5, "y": 678.9, "z": 0.0 },
  "tolerance": 50.0,
  "speed_factor": 1.2,
  "priority": 1
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| target_position | Vector3 | Y | 이동 목표 위치 |
| tolerance | number | N | 도착 판정 반경(기본 50) |
| speed_factor | number | N | 이동 속도 배수(기본 1.0) |
| priority | integer | N | 우선순위 |
- **3 - INTERCEPT params**

```json
{
  "target_player_id": "steam_76561198000000002",
  "intercept_distance": 80.0,
  "approach_angle": 30.0,
  "aggressiveness": "MEDIUM"
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| target_player_id | string | Y | 목표 플레이어 ID |
| intercept_distance | number | N | 요격 개시 거리(기본 80) |
| approach_angle | number | N | 접근 각도(도 단위, 선택) |
| aggressiveness | string | N | LOW/MEDIUM/HIGH |
- **4 - CHASE params**

```python
{
  "target_player_id": "steam_76561198000000002",
  "max_chase_duration": 3.0,
  "speed_factor": 1.15
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| target_player_id | string | Y | 추격 대상 |
| max_chase_duration | number | N | 최대 추격 시간(초) |
| speed_factor | number | N | 추격 속도 배수 |
- **5 - RETREAT params**

```python
{
  "safe_zone_position": { "x": 10.0, "y": 20.0, "z": 0.0 },
  "speed_factor": 1.25
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| safe_zone_position | Vector3 | N | 지정 안전지대(없으면 기본 안전지대 사용) |
| speed_factor | number | N | 후퇴 속도 배수 |
- **6 - PATROL params**

```python
{
  "patrol_zone": "zone_mid_01",
  "patrol_speed": 0.9,
  "waypoints": [
    { "x": 100.0, "y": 100.0, "z": 0.0 },
    { "x": 150.0, "y": 120.0, "z": 0.0 }
  ]
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| patrol_zone | string | Y | 순찰 구역 ID |
| patrol_speed | number | N | 순찰 속도 배수 |
| waypoints | Vector3[] | N | 순찰 경유점(있으면 zone보다 우선) |
- **7 - CONSUME_P_POINT params**

```json
{
  "p_point_id": "ppoint_2",
  "emergency_priority": 1
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| p_point_id | string | Y | 섭취 대상 P-Point ID |
| emergency_priority | integer | N | 위기 우선순위(0~3 권장) |
- 8 - CONSUME_PELLET params

```json
{
  "pellet_id": "pellet_A",
  "emergency_priority": 3
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| pellet_id | string | Y | 섭취 대상 P-Pellet ID |
| emergency_priority | integer | N | 위기 우선순위(0~3 권장) |
- **9 - GUARD params**

```json
{
  "guard_target": "pacman_main",
  "guard_radius": 120.0,
  "duration": 2.5
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| guard_target | string | Y | 방어 대상(유닛ID/오브젝트ID/위치ID 등) |
| guard_radius | number | N | 방어 반경 |
| duration | number | N | 유지 시간(초) |
- **10 - FLANK params**

```json
{
  "target_player_id": "steam_76561198000000002",
  "flank_direction": "LEFT",
  "flank_distance": 60.0
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| target_player_id | string | Y | 목표 플레이어 |
| flank_direction | string | Y | LEFT/RIGHT |
| flank_distance | number | N | 우회 거리 |
- **11 - FAKE_RETREAT params *(기존 13 → v1.4.0에서 11)***

```json
{
  "fake_retreat_duration": 1.8,
  "counter_attack_position": { "x": 210.0, "y": 640.0, "z": 0.0 }
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| fake_retreat_duration | number | Y | 후퇴 연출 시간(초) |
| counter_attack_position | Vector3 | N | 역습 목표 위치(없으면 전술 기본값) |

---

### 5) **brain_cam_data** 필드

brain_cam_data:

- 브레인캠(XAI) 데이터. Unreal 디버그/관전자 UI 표시용.
- 구조: perception(summary, threat_level, key_signals[]) /
reasoning(retrieved_docs[{tactic_id,title,similarity}], selected_tactic_id, notes) /
decision(final_choice, unit_commands_summary)
- title 기반 구버전 예시는 사용하지 않음(삭제)
(상세 스키마/예시는 V. 브레인캠 데이터 섹션 참조)

**구조 요약**

```json
"brain_cam_data": {
  "perception": {
    "summary": "DRIVER1 근접 + 부스터, 포획 게이지 45%(위기), ppoint_2 사용 가능(available=true), 경로 유효(navmesh_path_valid=true)",
    "threat_level": "HIGH",
    "key_signals": [
      {
        "type": "PLAYER_THREAT",
        "player_id": "steam_76561198000000002",
        "score": 14.0,
        "reason": "근거리 접근 + 부스터 사용"
      },
      {
        "type": "OBJECTIVE",
        "name": "P_POINT",
        "target_id": "ppoint_2",
        "available": true
      },
      {
        "type": "NAVMESH",
        "navmesh_path_valid": true
      },
      {
        "type": "CAPTURE_GAUGE",
        "value": 0.45
      }
    ]
  },
  "reasoning": {
    "retrieved_docs": [
      {
        "tactic_id": "tactic_021",
        "title": "위기 구간에서 P-Point 우선 확보로 랜덤 스킬 트리거",
        "similarity": null
      },
      {
        "tactic_id": "tactic_088",
        "title": "근접 위협 존재 시 최단/안전 경로로 목표 확보",
        "similarity": null
      }
    ],
    "selected_tactic_id": "tactic_021",
    "notes": "포획 게이지 45%로 위험 구간. ppoint_2가 가용(available=true)이며 navmesh 경로가 유효하므로, 단기 목표를 P-Point 확보로 설정."
  },
  "decision": {
    "final_choice": "pacman_main은 ppoint_2를 우선 확보(분신 미사용 정책으로 단일 유닛 명령)",
    "unit_commands_summary": "MOVE_TO_LOCATION (to ppoint_2)"
  }
}
```

각 필드 설명

| 필드 | 타입 | 설명 |
| --- | --- | --- |
| **perception** | object | 현재 상황 요약(위협/기회/자원 등) |
| **perception.summary** | string | 사람에게 보여주기 위한 요약 문장 |
| **perception.threat_level** | string | 위험 수준. **"CRITICAL" | "HIGH" | "MEDIUM" | "LOW"** |
| **perception.key_signals** | object[] | 핵심 신호 목록(위협 플레이어, 목표 오브젝트, 경로/게이지 등)을 구조화한 배열 |
| **reasoning** | object | 참고한 전술/지식 패턴 및 선택 근거 |
| **reasoning.retrieved_docs** | object[] | 검색/참고된 전술(카드) 목록(최대 3~5개 권장) |
| **reasoning.retrieved_docs[].tactic_id** | string | 전술(카드) ID |
| **reasoning.retrieved_docs[].title** | string | 전술(카드) 제목 |
| **reasoning.retrieved_docs[].similarity** | number | null | 유사도 점수(0~1). 산출 안 하면 null 또는 필드 생략 가능 |
| **reasoning.selected_tactic_id** | string | null | 최종 선택된 전술(카드) ID (없으면 null 또는 생략) |
| **reasoning.notes** | string | null | 선택 이유 요약(한두 문장). 없으면 null 또는 생략 |
| **decision** | object | 최종 전략 선택 요약 |
| **decision.final_choice** | string | 이번 틱 핵심 선택 한 문장 |
| **decision.unit_commands_summary** | string | null | unit_commands를 사람이 읽기 좋게 요약한 문장(없으면 null 또는 생략) |

 이 brain_cam_data는

- 언리얼 디버그 UI, 시연용, “AI 브레인 뷰어”, 발표용 화면 등에 그대로 쓸 수 있는 구조입니다.

---

**언리얼 처리 규칙(요약)**

1. **/api/v1/get_decision 응답 처리(v1.4.0)**
    - **decision.unit_commands[]**는 “**unit_id == "pacman_main"**만 들어온다고 가정하고 파싱/적용합니다. (분신 명령 제거 정책)

---

## 6. 보조 API: 매치 시작

- **Endpoint**: **POST /api/v1/match/start**
- **용도**: 방 생성/게임 시작 시 호출, 초기화 정보 전달
    - 언리얼 서버가 **게임 방이 생성되거나 실제 경기가 시작될 때 1회 호출**합니다.
    - AI 서버는 이 정보를 바탕으로 **room_id 단위 상태를 초기화**하고, 이후 **/api/v1/get_decision** 요청을 준비합니다.
- **필수 필드**: **room_id, player_ids, player_types**(배열 정렬 규칙 유지)

### **1) Request Body 예시**

```json
{
  "room_id": "GAME_12345",
  "player_ids": [
    "steam_76561198000000001",
    "steam_76561198000000002",
    "steam_76561198000000003",
    "steam_76561198000000004"
  ],
  "player_names": [
    "지휘관_닉네임",
    "드라이버1_닉네임",
    "드라이버2_닉네임",
    "드라이버3_닉네임"
  ],
  "player_types": [
    "COMMANDER",
    "DRIVER",
    "DRIVER",
    "DRIVER"
  ],
  "map_id": "BRIDGE_NIGHT",
  "mode": "RANKED",
  "match_start_time": "2025-11-16T10:20:00.000Z"
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **room_id** | string | ✓ | 게임 방 ID. 전체 API에서 공통으로 사용하는 키 (예: **"GAME_12345"**) |
| **player_ids** | string[] | ✓ | 이번 매치에 참여하는 플레이어 Steam ID 리스트. 인덱스 규칙: **0=COMMANDER, 1~3=DRIVER** | |
| **player_names[]** | string[] | ✓ | 스팀 닉네임/인게임 닉네임 |
| **player_types[]** | string[] | ✓ | 플레이어 타입 배열. 값: **"COMMANDER"** 또는 **"DRIVER"** (player_ids/player_names와 인덱스 정렬 동일) |
| **map_id** | string | ✗ | 맵/코스 ID (예: **"BRIDGE_NIGHT"**). 생략 시 기본 맵으로 처리 |
| **mode** | string | ✗ | 게임 모드 (예: **"RANKED", "CASUAL"**). 생략 가능 |
| **match_start_time** | string | ✗ | 매치 시작 시각. ISO 8601 형식 (예: **"2025-11-16T10:20:00.000Z"**) |

인덱스 규칙: **0 = COMMANDER, 1~3 = DRIVER**

**베타 스코프 주의:** v1.4.0 베타에서는 **COMMANDER 관련 STT/TTS/리포트 연동은 비활성**이며, AI 팩맨 의사결정은 **DRIVER 상태 중심으로 동작**합니다.

---

## 7. 보조 API: 매치 종료(팩맨 사망/시간초과 즉시 1회 전송)

### 1) 개요

- **Endpoint**: **POST /api/v1/match/end**
- **역할**
    - 경기 종료 시 AI 서버에 **“이 매치는 끝났다”** 를 알리고,
    - 서버는 **room 단위 정리(캐시/세션) + 최소 결과 저장**을 수행합니다.
- **호출 타이밍**
    - **팩맨이 죽는 순간** 또는 **경기 시간이 0이 되는 순간(시간 초과)** 에
        
        **언리얼 서버가 즉시 1번 호출**해야 함.
        
- **(중요) 베타 스코프**
    - v1.4.0 베타에서는 **Commander Report(지휘관 보고서) 생성/반환을 하지 않습니다.**
    - 즉, **match/end 응답에서 commander_report는 내려주지 않거나(null)** 처리합니다.

---

### 2) Request

- **Method**: **POST**
- **URL**: **/api/v1/match/end**
- **Content-Type**: **application/json**

**Request Body 예시**

```json
{
  "room_id": "GAME_12345",
  "result": "PLAYERS_WIN",
  "winner_team": "PLAYERS",
  "end_reason": "PACMAN_DEAD",
  "match_duration_seconds": 600,
  "match_end_time": "2025-11-08T15:30:00.000Z"
}
```

**Request 필드 설명**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **room_id** | string | ✓ | 매치 시작 시 사용한 게임 방 ID (**"/match/start"**와 동일 값) |
| **result** | string | ✓ | 매치 결과. 예: **"PLAYERS_WIN", "PACMAN_WIN", "DRAW", "ABORTED"** |
| **match_duration_seconds** | number | ✗ | 실제 경기 진행 시간(초). 예: 8분 30초 → **510** |
| **winner_team** | string | ✗ | 승리 팀 식별자 (예: **"PLAYERS", "PACMAN"**) |
| **end_reason** | string | ✗ | 종료 사유. 예: **"TIME_OVER", "PACMAN_DEAD", "PACMAN_CAPTURED", "SURRENDER", "DISCONNECT", "MANUAL_ABORT"** 등 |
| **match_end_time** | string | ✗ | 매치 종료 시각. ISO 8601 형식 (예: **"2025-11-16T10:20:00.000Z"**) |

### 3) Response – 베타(Report 비활성) 기본 구조

### 개요

- **역할**
    - 경기 종료 시점에 AI 서버에 “이 매치는 끝났다”는 사실을 알린다.
- **호출 타이밍**
    - **팩맨이 죽는 순간** 또는 **경기 시간이 0이 되는 순간(시간 초과)** 에 **언리얼 서버가 즉시 1회 호출**해야 함.

**성공 시**, AI 서버는 다음 두 가지를 수행합니다.

1. 내부적으로
    - **room_id** 기반 로그/지표/캐시 정리
2. 응답으로:
    - **status: "success"** 응답

**Response Body 예시**

```json
{
  "status": "success",
  "generated_at": "2025-12-01T10:40:05Z",
  "room_id": "GAME_12345",
  "message": "Match ended",

  "result": "PLAYERS_WIN",
  "winner_team": "PLAYERS",
  "end_reason": "PACMAN_DEAD",
  "game_duration_seconds": 600,

  "meta": {
    "version": "1.4.0",
    "cleanup_latency_ms": 120.0,
    "report_enabled": false
  },

  "commander_report": null,

  "statistics": {
    "total_ai_decisions": 170,
    "avg_decision_latency_ms": 380.5
  }
}
```

- **상위 필드 설명 (베타 기준)**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **status** | string | ✓ | **"success"** 또는 **"error"** |
| **generated_at** | string | ✓ | 보고서 생성 시각 (ISO8601). 기존 **timestamp**와 동일 의미 |
| **room_id** | string | ✓ | 매치/방 ID |
| **message** | string | ✗ | 상태 메시지(기본 **"Match ended"**) |
| **result** | string | ✗ | 최종 승패 결과 |
| **winner_team** | string | ✗ | 승리 팀 |
| **end_reason** | string | ✗ | 종료 사유 |
| **game_duration_seconds** | number | ✗ | 실제 경기 진행 시간(초) |
| **meta** | object | ✗ | match/end 처리 관련 메타데이터 묶음 |
| **commander_report** | object | null | ✗ | **베타에서는 null**(또는 필드 자체 생략 가능) |
| **statistics** | object | ✓ | 최소 통계(베타에서 유지) |
- **statistics 필드(베타 최소)**

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **total_ai_decisions** | integer | ✗ | 경기 동안 **/api/v1/get_decision** 이 호출된 총 횟수 (AI 의사결정 틱 수). 기존 **total_decisions.** |
| **avg_decision_latency_ms** | float | ✗ | **/get_decision** 요청당 평균 응답 지연 시간(ms). 기존 **average_response_time_ms**. |
- **meta 필드 상세**

```json
"meta": {
  "version": "1.4.0",
  "cleanup_latency_ms": 120.0,
  "report_enabled": false
}
```

| 필드 | 타입 | 필수 | 설명 |
| --- | --- | --- | --- |
| **version** | string | ✓ | **응답 스펙 버전**. 문서/클라이언트 파싱 기준 버전 (예: **"1.4.0"**) |
| **cleanup_latency_ms** | number | ✓ | **match/end 요청 처리 지연 시간(ms)**. 서버가 요청을 수신한 시점부터 “정리/저장/응답 생성” 완료까지 걸린 시간 |
| **report_enabled** | boolean | ✓ | **보고서 기능 활성 여부**. v1.4.0 베타에서는 false 고정 (보고서 생성/반환 안 함) |

---

# IV. 통신 프로토콜

본 장에서는 **언리얼 게임 서버 ↔ AI 서버(FastAPI)** 간의 통신 흐름과 프로토콜을 정리합니다.

※ **v1.4.0 베타에서는 Overseer/Commander(WebSocket, 리포트 재조회 API 등)를 문서/연동 스코프에서 제외**합니다. (코드는 존재하더라도 **언리얼 통합은 보류**)

## 1. 전체 통신 구조 요약

### 1) 채널별 역할

| 채널 | 프로토콜 | 방향 | 역할 |
| --- | --- | --- | --- |
| **/api/v1/health** | HTTP GET | 언리얼 → AI | AI 서버 상태 확인 (헬스체크) |
| **/api/v1/match/start** | HTTP POST | 언리얼 → AI | 매치 시작 알림 / room 초기화 |
| **/api/v1/get_decision** | HTTP POST | 언리얼 → AI | 게임 상태 전송 + **AI 팩맨 의사결정 요청** |
| **/api/v1/match/end** | HTTP POST | 언리얼 → AI | 매치 종료 알림 / room 정리 + 통계 집계 *(v1.4.0: report 비활성)* |

**핵심 요약 (v1.4.0 베타)**

- 본 통합은 **HTTP 4개 엔드포인트만 사용**합니다.
    - **/api/v1/health**
    - **/api/v1/match/start**
    - **/api/v1/get_decision**
    - **/api/v1/match/end**

### 2) 전체 흐름 개요 (텍스트 다이어그램)

```powershell
[매치 시작 전]
언리얼 → AI: GET  /api/v1/health         (헬스체크)
언리얼 → AI: POST /api/v1/match/start    (room_id, player_ids, ...)

[인게임 루프]
(약 2~3초마다, 튜닝 가능)
언리얼 → AI: POST /api/v1/get_decision
AI → 언리얼: unit_commands + brain_cam_data (v1.4.0 베타: pacman_main 중심)

[매치 종료]
언리얼 → AI: POST /api/v1/match/end
AI → 언리얼: status + meta + commander_report(null) + statistics
```

---

## 2. 매치 라이프사이클별 통신 흐름

### 1) 매치 시작 단계

- **헬스체크**
    - 언리얼 → AI
    - **GET /api/v1/health**
    - 목적
        - AI 서버 프로세스 동작 여부 확인
        - 배포 환경/버전 확인(필요 시)
- **매치 시작 알림**
    - 언리얼 → AI
    - **POST /api/v1/match/start**
    - 필수 필드:
        - **room_id, player_ids[], player_names[], player_types[]**
    - 선택 필드:
        - **map_id, mode, match_start_time** 등
    - AI 서버 동작:
        - **room_id** 기준으로 내부 상태/캐시 초기화
        - 이후 **/get_decision** 요청 처리 준비

---

### 2) 인게임 루프 단계

- **AI 팩맨 의사결정 루프**
    - 주기: 약 2~3초(튜닝 가능)
    - **의사결정 요청(**언리얼 → AI)
        - **POST /api/v1/get_decision**
        - Request Body
            - **room_id**
            - **global_context** *(remaining_time, game_phase, match_start_time, nav_frame 등)*
            - **ai_squad_context** *(pacman_main 상태 + 방향 정보, clones는 [] 유지, p_pellet 정보 등)*
            - **player_team_context[]** *(거리/경로/이벤트/룬/델타 등)*
            - **commander_context** *(요청 스키마 유지; 베타에서는 참고용/로그용으로만 취급 가능)*
            - **map_context** *(pacman_spawn, p_point_locations, p_pellet_locations 등)*
            - 그 외 3장에 정의된 필드들
    - **의사결정 응답(**AI → 언리얼)
        - **decision.unit_commands[]**
            - v1.4.0 베타: **분신 명령 제외**, **pacman_main 중심**으로 반환
        - **decision.squad_objective, decision.reasoning, decision.confidence**
        - **brain_cam_data** *(Perception / Reasoning / Decision)*
    - 언리얼
        - **unit_commands** 를 언리얼 AI/컨트롤러에 적용
        - **brain_cam_data** 는 디버그 UI/관전 모드에서 사용

> 중요 (v1.4.0 베타)
> 
> - 이 문서 범위에서 **WebSocket(STT/TTS) 통신은 사용하지 않습니다.**
> - 의사결정은 **HTTP /get_decision** 루프로만 통합합니다.

---

### 3) 매치 종료 단계

- **매치 종료 알림**
    - 언리얼 → AI
    - **POST /api/v1/match/end**
    - Request Body:
        - **room_id**
        - **result / winner_team / end_reason**
        - (선택) **game_duration_seconds / match_end_time** 등
- **AI 서버 동작 (v1.4.0 베타)**
    - **room_id** 기반으로 내부 리소스 정리(캐시/상태/집계)
    - **report_enabled = false** 정책:
        - **commander_report는 생성/반환하지 않고 null**로 고정
- **AI → 언리얼 Response**
    - **status, generated_at, room_id, message**
    - **result/winner_team/end_reason/game_duration_seconds**
    - **meta** *(version, cleanup_latency_ms, report_enabled=false)*
    - **commander_report: null**
    - **statistics** *(예: total_ai_decisions, avg_decision_latency_ms)*
- **리소스 정리**
    - 양쪽에서 **room_id** 관련 캐시/상태 정리
    - 동일 **room_id** 재사용은 비권장(새 게임은 새 **room_id** 권장)

---

## 3. 요청/응답 타임아웃 및 재시도 정책

### 1) HTTP 엔드포인트

- **/api/v1/get_decision**
    - 권장 타임아웃: **2초**
    - 권장 재시도: **1회** (네트워크 일시 장애 시)
    - 실패 시 언리얼 처리:
        - 직전 틱의 **unit_commands** 유지 또는
        - 안전한 기본 행동(예: **RETREAT, PATROL**)으로 폴백
    - 장시간 실패(연속 N회) 시:
        - 언리얼 쪽에서 AI 의존도를 줄이고 기본 AI로 동작하도록 스위칭 고려
- **/api/v1/match/start, /api/v1/match/end**
    - 권장 타임아웃: **5초**
    - 권장 재시도: **1회**
    - 재시도 실패 시:
        - 게임 플레이 자체는 계속 가능
        - 다만 **해당 매치의 서버 측 집계/정리 데이터**는 누락될 수 있으므로 로그에 기록 권장
- **/api/v1/health**
    - 타임아웃: **1~2초**
    - 재시도:
        - 초기 연결 단계에서만 1~2회 재시도
        - 지속 실패 시 AI 서버 장애로 판단

---

## 4. (v1.4.0 베타) WebSocket / Heartbeat 섹션 처리

- v1.4.0 베타 문서/통합 범위에서는 **WebSocket 채널을 사용하지 않으므로**,
    - **Heartbeat(ping/pong)**
    - **재연결 전략**
    - **STT 업로드 / TTS 푸시**
        
        위 항목은 **본 문서에서 제거(또는 “향후 범위”로 이관)**합니다.
        

---

## 5. 다중 매치 / 다중 룸 처리

- **room_id 단위 격리**
    - 모든 HTTP 요청(**match/start**, **get_decision**, **match/end**)은 **room_id** 를 포함
    - AI 서버는 이를 기준으로 상태를 분리 관리
- **로그/집계 저장(권장)**
    - /get_decision 관련 로그/지표는 모두 **room_id 기준으로 분리** 저장 권장
        - 예: **decision_log:{room_id}, metrics:{room_id}** 등
    - (베타 범위) STT/TTS 로그 키는 문서에서 제외

---

## 6. 에러 시 공통 처리 원칙

### 1) 언리얼 측 정책

- **/get_decision 실패**
    - 게임 플레이가 멈추지 않도록
        - 직전 **unit_commands** 유지 또는
        - 안전한 기본 행동(RETREAT, PATROL 등)으로 폴백
- **/match/start 실패**
    - 1회 재시도 후 실패 시:
        - 매치는 진행 가능하나, 서버 측 room 초기화 누락 가능
        - 가능하면 **재호출 성공 여부를 로그로 남김**
- **/match/end 실패**
    - 1회 재시도 후 실패 시:
        - 서버 측 정리/집계 누락 가능
        - 언리얼 로그에 남기고 운영 측에서 사후 확인 가능하도록 처리 권장

### 2) AI 서버 측 정책

- **클라이언트 오류(4xx)**
    - Body 검증 실패, 필드 누락 등
    - 재시도 없이 로그만 남김(입력 스키마 불일치 조기 발견 목적)
- **서버/네트워크 오류(5xx, 타임아웃)**
    - **/get_decision**
        - 내부적으로 가능하면 폴백 전략 수행(룰 기반 등)
        - 그래도 실패 시 명확한 에러 응답
    - **/match/start, /match/end**
        - 실패 시 room_id 기준으로 “정리/집계 실패” 로그 기록

---

# V. 브레인캠 데이터

## 1. 브레인캠 개요

- **브레인캠(BrainCam)**은 AI 팩맨이 어떤 생각을 했는지를 사람이 보기 쉽게 **요약·시각화**한 데이터입니다.

**목적**

- AI 팩맨의 **사고 과정을 시각화**
- 개발/디버깅 단계에서 **의사결정 근거 확인**
- 관전자 모드/리플레이/하이라이트에 **설명용 오버레이**로 활용

> 게임 규칙/판정은 **unit_commands**로만 결정되고, **brain_cam_data**는 **UI·분석용(비게임플레이)** 입니다.
성능 문제가 있으면 **brain_cam_data를 부분적으로 끄거나 축약**할 수 있습니다.
> 

## 2. BrainCam 구조 (응답 내 포함)

- **brain_cam_data**는 **/api/v1/get_decision 응**의 **decision**과 함께 제공됩니다.
- v1.4.0 베타 기준:
    - **분신(클론) 관련 문장/필드 사용 금지**
    - **pacman_main 중심**으로 서술

```json
{
  "brain_cam_data": {
    "perception": {
      "summary": "위협도 상승, P-Point 접근 가능, 포획 게이지 45%",
      "threat_level": "HIGH",
      "key_signals": [
        {
          "type": "PLAYER_THREAT",
          "player_id": "steam_76561198000000002",
          "score": 14.0,
          "reason": "근거리 접근 + 빠른 속도 유지"
        },
        {
          "type": "OBJECTIVE",
          "name": "P_POINT",
          "target_id": "ppoint_2",
          "available": true
        }
      ]
    },
    "reasoning": {
      "retrieved_docs": [
        {
          "tactic_id": "tactic_088",
          "title": "위기 구간: 안전 루트 확보 후 목표 전환",
          "similarity": 0.94
        },
        {
          "tactic_id": "tactic_031",
          "title": "근거리 위협 회피: 경로 기반 거리 우선",
          "similarity": 0.89
        }
      ],
      "selected_tactic_id": "tactic_088",
      "notes": "포획 게이지 45%로 위험 구간. P-Point가 가용이므로 단기 목표를 P-Point로 설정."
    },
    "decision": {
      "final_choice": "pacman_main은 ppoint_2로 이동 후 위협 수준에 따라 후퇴 또는 추격 회피로 전환",
      "unit_commands_summary": "MOVE_TO_LOCATION (priority=2)"
    }
  }
}
```

## 3. brain_cam_data.[] 필드 상세 설명

### 1) perception (상황 인식 레이어)

- 목적: **현재 틱의 핵심 상황을 구조적으로 요약**
- 주요 필드:
    - **summary** *(string)*: HUD/브레인캠에 그대로 노출 가능한 **한 줄 요약**
    - **threat_level** *(string)*: **CRITICAL / HIGH / MEDIUM / LOW**
    - **key_signals[]** *(array)*: 위협도 높은 플레이어/목표/타이머  정보 등 핵심 신호 목록

**key_signals[] 권장 타입 예시**

- **PLAYER_THREAT**: 특정 플레이어의 위협 요약
- **OBJECTIVE**: P-Point 등 목표 관련 상태

---

### 2) reasoning (근거/참조 레이어)

- 목적: **전략카드/온톨로지/RAG 결과를 사람이 이해할 수 있게 요약**
- 주요 필드:
    - **retrieved_docs[]** *(array)*: 참고한 전술/전략 문서 목록 (권장 2~5개)
        - 각 항목: **tactic_id, title, similarity**
    - **selected_tactic_id** *(string | null)*: 실제로 적용한 전술 카드 ID (없으면 null/생략 가능)
    - **notes** *(string)*: 선택 이유를 1~2문장으로 요약

---

### 3) decision (최종 선택 레이어)

- 목적: 사람이 보는 관점에서 **이번 틱의 결론을 한 문장으로 정리**
- 주요 필드:
    - **final_choice** *(string)*: 최종 결론 문장 (v1.4.0: **pacman_main 중심**)
    - **unit_commands_summary** *(string)*: unit_commands 내용을 짧게 요약한 텍스트

---

## 4. UI 렌더링 가이드(권장)

**브레인캠 UI 구성 요소(3단 패널)**

1. **상황 요약 (Perception) 패널**
    - 제목: **“상황 요약”**
    - 내용: **brain_cam_data**.**perception.summary** 텍스트 표시
    - 역할: 현재 판의 핵심 상황을 한눈에 보여주는 영역
        
        (누가 위험한지, 어떤 자원이 있는지 등)
        
    - 스타일: 밝은 배경, 약간 굵은 폰트로 가독성 강조
2. **참고 전술 (Reasoning) 패널**
    - 제목: **“참고 전술”**
    - 내용: **brain_cam_data.reasoning.retrieved_docs[]** 배열을 리스트로 표시
    - 각 항목을 bullet point(●, • 등)로 구분
    - 역할: AI가 참고한 과거 전술/지식 목록을 보여주는 영역
        
        (예: “전술 #088: 위기 시 P-Pellet 우선 확보”)
        
    - 스타일: 중간 톤 배경, 리스트 가독성 위주
3. **AI 최종 판단 (Decision) 패널**
    - 제목: **“AI 최종 판단”**
    - 내용: **brain_cam_data.decision.final_choice** 텍스트 표시
    - 역할: 이번 턴에 AI가 실제로 선택한 전략/플랜을 한 문장으로 보여주는 영역
    - 스타일: 강조 색상(포인트 컬러), 폰트 크기를 가장 크게

---

### 7.4 브레인캠 UI 레이아웃 예시

```powershell
┌─────────────────────────────────────────┐
│  AI 브레인캠                             │
├─────────────────────────────────────────┤
│  상황 요약:                              │
│  위협도 상승, P-Point 접근 가능,          │
│  포획 게이지 45%                         │
├─────────────────────────────────────────┤
│  참고 전술:                              │
│  • tactic_088: 위기 구간 안전 루트 확보   │
│  • tactic_031: 경로 기반 거리 우선        │
├─────────────────────────────────────────┤
│  AI 최종 판단:                           │
│  pacman_main은 ppoint_2로 이동 후        │
│  위협 수준에 따라 후퇴/회피로 전환         │
└─────────────────────────────────────────┘
```

- **UI 업데이트 로직 예시 (언리얼팀에서 어울리게 원하는 방향으로 해주시면 됩니다 )**
    1. **데이터 수신**
        - AI 서버 응답에서 **brain_cam_data** 필드 존재 여부 확인
    2. **필드별 파싱**
        - **brain_cam_data.perception.summary**
            
            → **“상황 요약” 패널 텍스트**
            
        - **brain_cam_data.reasoning.retrieved_docs[]**
            
            → **“참고 전술” 패널 리스트** (줄바꿈 또는 bullet으로 구분)
            
        - **brain_cam_data.decision.final_choice**
            
            → **“AI 최종 판단” 패널 텍스트**
            
    3. **NULL / 빈 값 처리**
        - 각 필드가 없거나 빈 문자열인 경우 기본 메시지 표시
        - 예:
            - 상황 요약: “상황 데이터 없음”
            - 참고 전술: “참고한 전술 없음”
            - AI 최종 판단: “분석 중…”
    4. **애니메이션 (선택)**
        - 새 데이터 수신 시 페이드 인/아웃 효과
        - 위험 수준에 따라 색상 강조 (예: 위험도 높음 → 붉은 계열 테두리)
    
    **레이아웃 권장**
    
    - 화면 우측 패널(투명도 약 70%)
    - 세로 3단 구조: **상황 요약 → 참고 전술 → AI 최종 판단**
    - /get_decision 응답 수신 시마다 갱신 (페이드 인/아웃은 선택)
    
    ---
    
    ## 6. 예외/폴백 처리
    
    1. **브레인캠 데이터 없음**
        - **brain_cam_data**가 없거나 null일 때:
            - 상단 패널에 **"분석 중..."** 정도의 텍스트 표시
            - 빈 상태(placeholder) 디자인 유지
        - 게임 플레이에는 영향 없음.
    2. **일부 필드만 존재**
        - **perception**만 있고 **reasoning/decision**이 비어 있는 경우 등:
            - 존재하는 필드만 순서대로 표시
            - 없는 영역은 “참고 전술 없음”, “AI 판단 대기 중” 등 기본 문구 사용
    3. **JSON 파싱 실패**
        - 잘못된 형식으로 들어와 UI 파싱에 실패하면:
            - 브레인캠 패널 비활성화 + 로그 남김
            - **/get_decision**에서 받은 **unit_commands**는 정상 처리 (게임 영향 없음)

---

# VI. 에러 처리

## 1. 에러 응답 규격

AI 서버(FastAPI)는 에러가 발생하면 **HTTP 상태 코드 + JSON 바디** 형태로 응답합니다.

성공 응답**(status: “success”)**과 형식을 맞추기 위해, 실패 시에는 아래 공통 포맷을 사용합니다.

### 실패 응답 포맷 (공통)

```json
{
  "status": "error",
  "error": {
    "code": "VALIDATION_ERROR",
    "message": "Missing required field: room_id",
    "details": {
      "missing_fields": ["room_id"],
      "timestamp": "2025-11-14T10:25:30.000Z"
    }
  }
}

```

- **status: "error"** 고정
- **error.code**: 에러 종류를 나타내는 문자열 코드
- **error.message**: 사람 읽기용 요약 메시지 (로그/디버깅용)
- **error.details**: 상황별 상세 정보(JSON 객체)
    - 예: 누락 필드 목록, 타임아웃 ms, 내부 에러 메시지, timestamp 등
- 적용 대상 엔드포인트 (v1.4.0 / 베타 기준)
    - **/api/v1/match/start**
    - **/api/v1/get_decision**
    - **/api/v1/match/end**

> NOTE (v1.4.0)
Overseer/Commander 관련 WebSocket(STT/TTS) 및
**/api/v1/commander/report**는 베타 범위에서 제외되어 본 문서의 에러 규격 적용 대상에서 제외
> 

---

## 2. 에러 코드 상세

### 1) HTTP 상태 코드

| HTTP 코드 | 에러 코드 | 설명 | 원인 | 조치 방법 |
| --- | --- | --- | --- | --- |
| **400** | **VALIDATION_ERROR** | 요청 검증 실패 | 필수 필드 누락, 타입 오류 | 요청 JSON 구조/타입 재확인 |
| **400** | **INVALID_JSON** | JSON 파싱 실패 | 잘못된 JSON 형식 | JSON 포맷 확인, 인코딩 확인 |
| **400** | **INVALID_FIELD_TYPE** | 필드 타입 오류 | 타입 불일치 (예: string → float) | 필드 타입 수정 |
| **400** | **MISSING_REQUIRED_FIELD** | 필수 필드 누락 | 필수 필드 없음 | 필드 추가 |
| **401** | **UNAUTHORIZED** | 인증 실패 | API 키 누락/오류 | API 키 확인
키/헤더 정책 확인 |
| **404** | **ENDPOINT_NOT_FOUND** | 엔드포인트 없음 | 잘못된 URL | URL 확인 |
| **408** | **REQUEST_TIMEOUT** | 요청 타임아웃 | 서버 처리 지연, 네트워크 문제 | 1회 재시도, 실패 시 폴백 전술 사용 |
| **429** | **RATE_LIMIT_EXCEEDED** | 요청 과다 | 너무 많은 요청 | 요청 주기 증가 (쿨타임) |
| **500** | **INTERNAL_ERROR** | 서버 내부 오류 | AI 서버 버그, LLM 오류 | 1회 재시도, 실패 시 폴백 전술 사용 |
| **503** | **SERVICE_UNAVAILABLE** | 서비스 불가 | 배포/유지보수 중, 과부하/점검 | 재시도(간격 두고), 상태 모니터링 |
| **504** | **GATEWAY_TIMEOUT** | 게이트웨이 타임아웃 | LLM/외부 API 지연 | 1회 재시도, 실패 시 폴백 전술 사용 |

### 2) AI 팩맨 전용 에러 코드(v1.4.0)

| 에러 코드 | 설명 | 대표 원인 | 언리얼 측 조치 |
| --- | --- | --- | --- |
| INVALID_GAME_STATE | 게임 상태 값이 비정상 | 좌표 NaN/Inf, 음수 HP, 말이 안 되는 시간값 | 요청 데이터 생성 로직 점검, 잘못된 값 필터링 |
| UNIT_NOT_FOUND | 유닛 ID 불일치 | 서버가 이해 못 하는 unit_id | 유닛 ID 매핑 재점검, 잘못된 ID 사용 금지 |
| PELLET_NOT_AVAILABLE | P-Pellet 사용 불가 | 쿨타임 중, **available: false** | 클라이언트/언리얼의 P-Pellet 상태 동기화 확인 |
| DECISION_TIMEOUT | 의사결정 시간 초과 | LLM 지연, 내부 파이프라인 과부하 | 해당 틱은 폴백 전술 사용, 서버 모니터링 요청
1회 재시도 → 실패 시 폴백 |
| LLM_GENERATION_FAILED | LLM 응답 실패 | 외부 LLM API 에러, 쿼터 초과 | 1회 재시도, 실패 시 룰 기반 폴백 전술 사용 |
| FALLBACK_USED | 폴백 전술로 대체 처리 | LLM/지식 검색 실패 | 심각한 에러는 아님, 디버그 로그 정도만 확인 |
| RATE_LIMIT_EXCEEDED | 과도한 호출(서버 보호) | tick 호출 주기 과다 | 호출 주기 증가 |

> NOTE
> 
> - **FALLBACK_USED**는 “에러”가 아니라 **성공 응답(meta.fallback_used=true)** 형태로 관측하는 것을 권장합니다.

---

## 3.  에러 처리 예시

### 1) 예시 1: VALIDATION_ERROR(400)

```json
{
  "status": "error",
  "error": {
    "code": "VALIDATION_ERROR",
    "message": "Missing required field: room_id",
    "details": {
      "missing_fields": ["room_id"],
      "timestamp": "2025-11-14T10:25:30.000Z"
    }
  }
}
```

- 얼리얼 조치: 요청 JSON에 **room_id**가 제대로 들어가 있는지 확인 후 다시 전송

### 2) 예시 2: REQUEST_TIMEOUT(408)

```json
{
  "status": "error",
  "error": {
    "code": "REQUEST_TIMEOUT",
    "message": "Request processing exceeded 2000ms",
    "details": {
      "timeout_ms": 2000,
      "elapsed_ms": 2150,
      "timestamp": "2025-11-14T10:25:30.000Z"
    }
  }
}
```

- 얼리얼 조치
    - 1회 재시도 **(/get_decision** 다시 호출)
    - 실패하면 **이전 unit_commands 유지** 또는 **안전 전술(RETREAT/PATROL 등)** 적용
    - room_id 기준으로 에러 로그 누적(디버그/리플레이 연동 대비)

### 3) 예시 3: LLM_GENERATION_FAILED (500)

```json
{
  "status": "error",
  "error": {
    "code": "LLM_GENERATION_FAILED",
    "message": "LLM API returned error",
    "details": {
      "llm_error": "overloaded_error",
      "retry_after_seconds": 5,
      "timestamp": "2025-11-14T10:25:30.000Z"
    }
  }
}
```

- 얼리얼 조치
    - **재시도는 선택 사항** (연속 오류 시 지연만 커짐)
    - **fallback_available: true**이면 AI 서버가 자체적으로 **룰 기반 폴백 전술로 이미 전환**했을 수 있으므로, 게임은 계속 진행하되 디버그용 로그만 남기면 충분.

---

## 4. 폴백 처리

### 1) AI 팩맨 폴백 전략

     **[타임아웃/에러 시 기본 순서]**

1. **이전 명령 유지**(가능하면)
2. **안전 명령 실행** (예: RETREAT)
3. **PATROL**로 전환(안전 구역/중립 루트)

     **[폴백 명령 우선순위]**

1. **포획 게이지(capture_gauge) > 70% → RETREAT 우선**
2. **포획 게이지(capture_gauge) 50-70% → PATROL(안전 루트)**
3. **포획 게이지(capture_gauge) < 50% → 이전 명령 유지**

> 실제 임계치는 밸런스/튜닝에 따라 변경 가능하며, “게임이 멈추지 않는 것”이 최우선입니다.
> 

---

## 5. 언리얼 측 에러 처리 유지 요청

- **게임 플레이는 멈추지 않는다**
    - **/get_decision** 실패/타임아웃이어도:
        - 이전 명령 유지 또는 안전 전술로 폴백
- **재시도는 1회까지만**
    - 무한 재시도 금지(프레임 드랍/렉 유발 방지)
- **room_id 기준으로 로그를 구조화**
    - room_id + timestamp + error.code 로 누적하여 추후 분석/재현 가능하게 유지

---

# VII. 테스트 가이드 (v1.4.0 / 베타 범위)

> 베타 범위(현재 문서 기준): HTTP 4종만 테스트
> 
- **GET /api/v1/health**
- **POST /api/v1/match/start**
- **POST /api/v1/get_decision**
- **POST /api/v1/match/end**
    
    ※ Overseer/Commander(WebSocket, report API 등)는 **축약/제외** 전제
    

## 1. 테스트 목표

| 목표 | 체크 포인트 |
| --- | --- |
| 연동 안정성 | 필수 필드/타입/널 처리, 응답 포맷 일치 |
| 게임 루프 안전성 | **/get_decision** 타임아웃/에러 시 언리얼 폴백 가능 |
| 매치 라이프사이클 완결성 | start → decision loop → end 순서로 정상 동작 |
| 스펙 준수 | directive_code 범위/params 규격/priority 정책 준수 |
| 베타 정책 준수 | **분신(clones) 미사용**, report 비활성(**report_enabled=false**) |

## 2. Smoke 테스트: Health

**Request**

```
GET /api/v1/health
```

**Expected (예시)**

```json
{
  "status": "ok",
  "timestamp": "2025-12-25T11:00:00.000Z",
  "environment": "dev",
  "app_name": "CITRUSH AI Server",
  "services": {
    "app": { "ok": true },
    "postgres": { "ok": true },
    "redis": { "ok": true },
    "qdrant": { "ok": true }
  }
}
```

체크:

- **status**가 **ok** 또는 **degraded**로 오더라도 **HTTP 200**이 내려오는지
- **services.*.ok**가 기대값인지

---

## 3. 매치 시작: /match/start

**Request Body 예시**

```json
{
  "room_id": "GAME_12345_TEST_001",
  "player_ids": [
    "steam_76561198000000001",
    "steam_76561198000000002",
    "steam_76561198000000003",
    "steam_76561198000000004"
  ],
  "player_names": [
    "지휘관_닉네임",
    "드라이버1_닉네임",
    "드라이버2_닉네임",
    "드라이버3_닉네임"
  ],
  "player_types": [
    "COMMANDER",
    "DRIVER",
    "DRIVER",
    "DRIVER"
  ],
  "map_id": "BRIDGE_NIGHT",
  "mode": "RANKED",
  "match_start_time": "2025-12-25T11:01:00.000Z"
}
```

체크:

- 인덱스 규칙: **0=COMMANDER, 1~3=DRIVER** 정렬 유지
- 누락 시 400 에러 포맷으로 떨어지는지(8장 규격)

---

## 4.  핵심 루프: /get_decision (요청/응답 통합 테스트)

### 1) Request Body (베타 기준 핵심 필드 포함 예시)

> 아래는 분신 미사용(clones 빈 배열) + 추가된 nav_frame / racer_nav_delta / navmesh_distance_to_pacman / pacman 방향을 포함한 최소 예시입니다.
> 

```json
{
  "room_id": "GAME_12345_TEST_001",
  "global_context": {
    "remaining_time": 420.5,
    "game_phase": "MID_GAME",
    "match_start_time": "2025-12-25T11:01:00.000Z",
    "nav_frame": {
      "timestamp_sec": null,
      "delta_time_sec": null,
      "inter_driver_avg_distance": null,
      "delta_inter_driver_avg_distance": null
    }
  },
  "ai_squad_context": {
    "pacman_main": {
      "position": { "x": 150.0, "y": 300.0, "z": 0.0 },
      "hp": 100.0,
      "speed": 12.5,
      "capture_gauge": 45.0,
      "is_invulnerable": false,
      "rotation_yaw_deg": null,
      "forward_vector": null
    },
    "clones": [],
    "p_pellet_cooldown": 5.0,
    "last_pellet_consumed_at": "2025-12-25T11:02:30.000Z"
  },
  "player_team_context": [
    {
      "player_id": "steam_76561198000000001",
      "player_name": "지휘관_닉네임",
      "player_type": "COMMANDER",
      "role": 1,
      "position": { "x": 0.0, "y": 0.0, "z": 0.0 },
      "velocity": { "x": 0.0, "y": 0.0 },
      "rotation_yaw_deg": null,
      "hp": 100.0,
      "is_boosting": false,
      "recent_action": "issuing_orders",
      "distance_to_pacman": 250.0,
      "navmesh_cost_to_pacman": 280.0,
      "navmesh_distance_to_pacman": null,
      "navmesh_path_valid": true,
      "events": [],
      "equipped_runes": [],
      "commander_interaction": {
        "last_voice_command_at": null,
        "last_button_command_at": null
      }
    },
    {
      "player_id": "steam_76561198000000002",
      "player_name": "드라이버1_닉네임",
      "player_type": "DRIVER",
      "role": 2,
      "position": { "x": 120.0, "y": 250.0, "z": 0.0 },
      "velocity": { "x": 20.0, "y": 3.0 },
      "rotation_yaw_deg": null,
      "hp": 72.5,
      "is_boosting": true,
      "recent_action": "back_attack_attempt",
      "distance_to_pacman": 35.2,
      "navmesh_cost_to_pacman": 42.7,
      "navmesh_distance_to_pacman": null,
      "navmesh_path_valid": true,
      "racer_nav_delta": {
        "player_index": 1,
        "relative_angle_to_pacman_deg": null,
        "delta_position": { "x": 0.0, "y": 0.0, "z": 0.0 },
        "delta_straight_distance": 0.0,
        "delta_path_distance": 0.0,
        "delta_path_cost": 0.0,
        "movement_direction_change_deg": 0.0,
        "average_speed": 0.0,
        "relative_bearing_change_deg": 0.0
      },
      "events": [],
      "equipped_runes": [],
      "commander_interaction": {
        "last_voice_command_at": null,
        "last_button_command_at": null
      }
    }
  ],
  "commander_context": null,
  "map_context": {
    "pacman_spawn": { "x": 400.0, "y": 400.0, "z": 0.0 },
    "p_pellet_locations": [],
    "p_point_locations": [
      { "id": "ppoint_2", "x": 600.0, "y": 200.0, "z": 0.8, "available": true }
    ]
  }
}
```

체크:

- priority는 **integer**로만 처리(문자열 금지)
- **clones**는 **항상 []** (베타 정책)
- **racer_nav_delta**는 **DRIVER에만 존재**(COMMANDER에는 없음)

### 2) Response (성공 예시 / 분신 명령 제거 + directive 재번호 반영)

> 요청하신 정책: 11,12 제거 → 13이 11이 됨
> 
> 
> 즉, directive_code 유효 범위는 **1~11**, 11은 **FAKE_RETREAT**
> 

```json
{
  "status": "success",
  "timestamp": "2025-12-25T11:02:10.123Z",
  "room_id": "GAME_12345_TEST_001",
  "meta": {
    "version": "1.4.0",
    "ontology_version": "1.0.0",
    "latency_ms": 180.5,
    "fallback_used": false,
    "agent_pipeline": ["analyst", "planner", "llm_tactical", "executor", "logger"],
    "llm_tokens": 950
  },
  "decision": {
    "squad_objective": "P-Point 확보",
    "reasoning": "위협도가 상승했으나 ppoint_2가 가용 상태이므로 단기 목표를 P-Point로 설정합니다.",
    "unit_commands": [
      {
        "unit_id": "pacman_main",
        "directive_code": 2,
        "directive_name": "MOVE_TO_LOCATION",
        "params": {
          "target_position": { "x": 600.0, "y": 200.0, "z": 0.8 },
          "speed_factor": null,
          "priority": 2
        }
      }
    ],
    "confidence": 0.86
  },
  "brain_cam_data": {
    "perception": {
      "summary": "위협도 상승, P-Point 접근 가능, 포획 게이지 45%",
      "threat_level": "HIGH",
      "key_signals": [
        {
          "type": "PLAYER_THREAT",
          "player_id": "steam_76561198000000002",
          "score": 14.0,
          "reason": "근거리 접근 + 빠른 속도 유지"
        },
        {
          "type": "OBJECTIVE",
          "name": "P_POINT",
          "target_id": "ppoint_2",
          "available": true
        }
      ]
    },
    "reasoning": {
      "retrieved_docs": [
        { "tactic_id": "tactic_088", "title": "위기 구간: 안전 루트 확보 후 목표 전환", "similarity": 0.94 },
        { "tactic_id": "tactic_031", "title": "근거리 위협 회피: 경로 기반 거리 우선", "similarity": 0.89 }
      ],
      "selected_tactic_id": "tactic_088",
      "notes": "포획 게이지 45%로 위험 구간. P-Point가 가용이므로 단기 목표를 P-Point로 설정."
    },
    "decision": {
      "final_choice": "pacman_main은 ppoint_2로 이동 후 위협 수준에 따라 후퇴 또는 추격 회피로 전환",
      "unit_commands_summary": "MOVE_TO_LOCATION (priority=2)"
    }
  },
  "message": "Decision generated.",
  "request_id": "req-20251225-110210-0001"
}
```

체크:

- **unit_commands[]**에 **clone_**관련 명령이 절대 나오지 않는지
- **priority**가 **정수**인지(예: 0~3 정책을 문서에 정의했다면 그 범위 내인지)
- **directive_code**가 **1~11** 범위인지

---

## 5. 매치 종료: /match/end (report 비활성 확인)

**Request Body 예시**

```json
{
  "room_id": "GAME_12345_TEST_001",
  "result": "PLAYERS_WIN",
  "winner_team": "PLAYERS",
  "end_reason": "PACMAN_DEAD",
  "match_duration_seconds": 600,
  "match_end_time": "2025-12-25T11:05:00.000Z"
}
```

**Expected Response (사용자 지정 스펙 그대로)**

```json
{
  "status": "success",
  "generated_at": "2025-12-01T10:40:05Z",
  "room_id": "GAME_12345",
  "message": "Match ended",
  "result": "PLAYERS_WIN",
  "winner_team": "PLAYERS",
  "end_reason": "PACMAN_DEAD",
  "game_duration_seconds": 600,
  "meta": {
    "version": "1.4.0",
    "cleanup_latency_ms": 120.0,
    "report_enabled": false
  },
  "commander_report": null,
  "statistics": {
    "total_ai_decisions": 170,
    "avg_decision_latency_ms": 380.5
  }
}
```

체크:

- **meta.report_enabled**가 **false**
- **commander_report**는 **항상 null**
- **statistics**는 최소 2필드만 유지

---

### 9.7 네거티브(실패) 테스트 체크리스트

1. **필수 필드 누락**: room_id 제거
2. **타입 오류**: priority를 **"2"**(string)로 넣기
3. **NaN/Inf 입력**: position.x에 NaN 유사 값이 들어가는지(언리얼 생성단에서 방지 필요)
4. **directive_code 범위 위반**: 서버가 12/13 같은 값을 내보내지 않는지(회귀)
5. **racer_nav_delta 잘못된 대상**: COMMANDER에 racer_nav_delta가 들어가지 않도록

---

# VIII. 문제해결 (Troubleshooting) (v1.4.0 / 베타 범위)

## 1. “/health는 ok인데 /get_decision이 자주 타임아웃”

- 증상: 408 REQUEST_TIMEOUT, 혹은 언리얼 타임아웃(2초) 발생
- 원인 후보
    - 서버 내부 LLM/검색 파이프라인 지연
    - **/get_decision** 호출 주기가 너무 짧음(과도한 요청)
- 조치
    1. 언리얼 타임아웃 2초 유지 + **1회 재시도만**
    2. 호출 주기를 2~3초 → 4~5초로 늘려 관측
    3. 실패 시 **이전 명령 유지 / RETREAT / PATROL** 폴백 적용

---

## 2. 400 VALIDATION_ERROR / MISSING_REQUIRED_FIELD가 뜬다

- 가장 흔한 원인
    1. **room_id** 누락
    2. **player_team_context**가 배열이 아닌 객체로 감
    3. **priority**가 string으로 들어감 (**"CRITICAL", "2"** 등)
    4. **emergency_priority**를 boolean으로 보냄 (**true/false**) — 문서상 integer 권장

**예시: priority 타입 오류(잘못된 요청)**

```json
{
  "params": {
    "target_position": { "x": 1.0, "y": 2.0, "z": 0.0 },
    "priority": "2"
  }
}
```

**해결:** **priority: 2** (정수)로 수정

---

## 3. “언리얼에서 directive가 실행되지 않는다 / 움직임이 이상하다”

체크 순서:

1. **directive_code 범위 확인**
    - v1.4.0 베타 기준: **1~11**
    - **11 = FAKE_RETREAT**
    - 12/13이 내려오면 문서/서버 중 하나가 구버전
2. **params 키 표준화**
    - MOVE_TO_LOCATION: **target_position, speed_factor, tolerance, priority**
    - CONSUME_P_POINT: **p_point_id, emergency_priority**(int)
3. **priority 정책**
    - 미지정 시 0 적용(서버/언리얼 양쪽 동일 정책 권장)
    - 범위(예: 0~3)를 문서/코드에 고정했다면, 언리얼도 같은 clamp 적용 권장

---

## 4. “racer_nav_delta가 없어서 AI가 이상하게 판단한다”

- 결론: **정상**입니다.
    - racer_nav_delta는 **DRIVER 전용**
    - COMMANDER에는 없어야 함
- 단, DRIVER에 대해서도 UE가 아직 못 채우면 null/0으로 와도 OK (서버가 참고만 하도록 설계)

---

## 5. “match/end에서 report가 비어있는데 괜찮나요?”

- v1.4.0 베타 정책이 **report_enabled=false** 이므로:
    - **commander_report: null**은 **정상**
    - 이 상태로도 매치 종료 정리/통계 집계는 수행 가능
- 장점: 스펙은 유지하면서도 기능은 보류(문서/클라 변경 최소화)

---

## 

## 6. 로그 레벨 설정 (예시)

**DefaultEngine.ini 설정 예시**

```
[Core.Log]
LogAIPacman=Display
LogBrainCam=Display
LogHTTP=Warning
```

- **LogAIPacman**
    - **/get_decision** 요청/응답, unit_commands 실행 상태 디버깅
- **LogBrainCam**
    - 브레인캠 데이터 수신/파싱/UMG 바인딩 관련 디버깅
- **LogHTTP**
    - HTTP 요청/응답 중 Warning 이상만 출력 (필요 시 Verbose로 조정 가능)

---

## 7. 성능 모니터링

**[응답 시간 측정 권장]**

1. 언리얼에서 요청 전 **시작 시간 기록**
2. 응답 수신 시 **경과 시간(ms)** 계산
3. 목표 성능 기준:
    - AI 팩맨 **/api/v1/get_decision**:
        - **< 2,000ms 권장**
        - 평균 500~800ms 수준을 목표 (환경에 따라 상이)
4. 느린 응답(슬로우 쿼리) 감지 시:
    - 일정 기준(예: **/get_decision** 2초 초과) 이상인 요청만 경고 로그로 남기고, 평균/최대 응답 시간 통계를 주기적으로 확인

**[통계 수집 권장 항목]**

- 평균 응답 시간 (API별)
- 최대/최소 응답 시간
- 타임아웃 발생 빈도
- 폴백 사용 빈도 (**fallback_used: true** 비율)

---