#pragma once
#include <string>
#include <vector>

// ============================================================
// EnemySchemas.h
// HttpRequest2.h / HttpResponse2.h / EnemyTypes.h 의 MFC 미러
// UE 매크로(USTRUCT, UPROPERTY, GENERATED_BODY) 없이 순수 C++ 재정의
// ============================================================

// --- EAITactic (Tactic.h 미러) ---
enum class EAITactic : int {
    AMBUSH          = 1,
    MOVE_TO_LOCATION= 2,
    INTERCEPT       = 3,
    CHASE           = 4,
    RETREAT         = 5,
    PATROL          = 6,
    CONSUME_PELLET  = 7,
    GUARD           = 8,
    FLANK           = 9,
    SPLIT_FORMATION = 10,
    REGROUP         = 11,
    FAKE_RETREAT    = 12
};

inline const char* GetDirectiveName(int code) {
    switch (code) {
        case 1:  return "AMBUSH";
        case 2:  return "MOVE_TO_LOCATION";
        case 3:  return "INTERCEPT";
        case 4:  return "CHASE";
        case 5:  return "RETREAT";
        case 6:  return "PATROL";
        case 7:  return "CONSUME_PELLET";
        case 8:  return "GUARD";
        case 9:  return "FLANK";
        case 10: return "SPLIT_FORMATION";
        case 11: return "REGROUP";
        case 12: return "FAKE_RETREAT";
        default: return "UNKNOWN";
    }
}

// ============================================================
// 공통
// ============================================================
struct FWebVector { float x=0.f, y=0.f, z=0.f; };

// ============================================================
// Request 구조체 (FGetDecisionRequest2 미러)
// ============================================================

struct FPacmanMain {
    FWebVector position;
    float hp               = 100.f;
    float speed            = 0.f;
    float capture_gauge    = 0.f;
    bool  is_invulnerable  = false;
    float rotation_yaw_deg = 0.f;
};

struct FCloneUnit {
    std::string unit_id    = "clone_1";
    FWebVector  position;
    float hp               = 1.f;
    bool  is_alive         = true;
    float speed            = 0.f;
    float rotation_yaw_deg = 0.f;
};

struct FAISquadContext {
    FPacmanMain             pacman_main;
    std::vector<FCloneUnit> clones;
    float p_pellet_cooldown = 0.f;
};

struct FPlayerEvent {
    int         event_code = 0;
    std::string event_type;    // "FUEL_LOW","HP_LOW","RISK_HIGH"
    std::string severity;      // "INFO","LOW","MEDIUM","HIGH","CRITICAL"
    float       value     = 0.f;
    float       threshold = 0.f;
};

struct FPlayerTeamContext {
    std::string player_id   = "STEAM_TEST_001";
    std::string player_name = "TestDriver";
    std::string player_type = "DRIVER";   // "COMMANDER" or "DRIVER"
    int   role              = 2;          // 1=CMD, 2~4=DRIVER
    FWebVector  position;
    float hp                        = 100.f;
    float distance_to_pacman        = 500.f;
    float navmesh_cost_to_pacman    = 500.f;
    float navmesh_distance_to_pacman= 500.f;
    bool  navmesh_path_valid        = true;
    bool  is_boosting               = false;
    std::string recent_action       = "normal_drive";
    std::vector<FPlayerEvent> events;
};

struct FPPelletLocation {
    std::string id;
    float x=0.f, y=0.f, z=0.f;
    bool  available = true;
};

struct FPPointLocation {
    std::string id;
    float x=0.f, y=0.f, z=0.f;
    bool  available = true;
    float cooldown  = 0.f;
};

struct FMapContext {
    FWebVector pacman_spawn;
    std::vector<FPPelletLocation> p_pellet_locations;
    std::vector<FPPointLocation>  p_point_locations;
};

struct FGlobalContext {
    float game_time_seconds = 0.f;
    int   score             = 0;
    float time_remaining    = 300.f;
};

struct FGetDecisionRequest {
    std::string    room_id;
    std::string    request_num;
    int            current_directive_code = 0;
    FGlobalContext global_context;
    FAISquadContext ai_squad_context;
    std::vector<FPlayerTeamContext> player_team_context;
    FMapContext    map_context;
};

struct FMatchStartRequest {
    std::string room_id;
    std::vector<std::string> player_ids;
    std::vector<std::string> player_names;
    std::vector<std::string> player_types;
    std::string map_id = "default";
    std::string mode   = "CASUAL";
};

struct FMatchEndRequest {
    std::string room_id;
    std::string result;         // "PLAYERS_WIN","ENEMY_WIN","DRAW","ABORTED"
    int   final_score           = 0;
    int   match_duration_seconds= 0;
    std::string winner_team;    // "PLAYERS" or "ENEMY"
    std::string end_reason;     // "TIME_OVER","PACMAN_DEAD","SURRENDER"
};

// ============================================================
// Response 구조체 (FGetDecisionResponse2 미러)
// ============================================================

struct FUnitCommandParams {
    struct Vec3 { float x=0,y=0,z=0; };
    Vec3  target_position;
    Vec3  safe_zone_position;
    float speed_factor    = 1.f;
    int   priority        = 0;
    std::string aggressiveness;  // "HIGH","MEDIUM","LOW"
    std::string flank_direction;
    std::string formation_type;
    std::string pellet_id;
    std::string p_point_id;
};

struct FUnitCommand {
    std::string       unit_id;         // "enemy_main","clone_1","clone_2"
    int               directive_code  = 0;
    std::string       directive_name;  // "RETREAT","INTERCEPT" ...
    FUnitCommandParams params;
};

struct FDecisionInfo {
    std::string squad_objective;
    std::string reasoning;
    std::vector<FUnitCommand> unit_commands;
    float confidence = 0.f;
};

struct FMetaInfo {
    std::string version;
    std::string ontology_version;
    float latency_ms   = 0.f;
    bool  fallback_used= false;
    int   llm_tokens   = 0;
};

struct FGetDecisionResponse {
    std::string   status;
    std::string   timestamp;
    std::string   room_id;
    std::string   request_num;
    FMetaInfo     meta;
    FDecisionInfo decision;
    bool success          = false;
    bool has_pending_tts  = false;
    std::string raw_json; // 원본 JSON 보관
};
