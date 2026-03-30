#pragma once
#include "EnemySchemas.h"
#include <sstream>
#include <cstring>
#include <cctype>

namespace JsonHelper {

inline std::string EscapeStr(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else out += c;
    }
    return out;
}

inline std::string S(const std::string& v) { return "\"" + EscapeStr(v) + "\""; }
inline std::string B(bool v)               { return v ? "true" : "false"; }
inline std::string F(float v) {
    std::ostringstream o; o << std::fixed; o.precision(4); o << v; return o.str();
}
inline std::string I(int v) { return std::to_string(v); }
inline std::string Vec(float x, float y, float z) {
    return "{\"x\":" + F(x) + ",\"y\":" + F(y) + ",\"z\":" + F(z) + "}";
}

inline std::string BuildDecisionRequest(const FGetDecisionRequest& req) {
    std::ostringstream j;
    j << "{";
    j << "\"room_id\":"       << S(req.room_id) << ",";
    j << "\"request_num\":"   << S(req.request_num) << ",";
    j << "\"current_directive_code\":" << I(req.current_directive_code) << ",";

    j << "\"global_context\":{";
    j << "\"game_time_seconds\":" << F(req.global_context.game_time_seconds) << ",";
    j << "\"score\":"             << I(req.global_context.score) << ",";
    j << "\"time_remaining\":"    << F(req.global_context.time_remaining);
    j << "},";

    const auto& sq = req.ai_squad_context;
    j << "\"ai_squad_context\":{";
    j << "\"pacman_main\":{";
    j << "\"position\":"       << Vec(sq.pacman_main.position.x, sq.pacman_main.position.y, sq.pacman_main.position.z) << ",";
    j << "\"hp\":"             << F(sq.pacman_main.hp) << ",";
    j << "\"speed\":"          << F(sq.pacman_main.speed) << ",";
    j << "\"capture_gauge\":"  << F(sq.pacman_main.capture_gauge) << ",";
    j << "\"is_invulnerable\":" << B(sq.pacman_main.is_invulnerable) << ",";
    j << "\"rotation_yaw_deg\":" << F(sq.pacman_main.rotation_yaw_deg);
    j << "},";

    j << "\"clones\":[";
    for (size_t i = 0; i < sq.clones.size(); ++i) {
        const auto& c = sq.clones[i];
        if (i > 0) j << ",";
        j << "{";
        j << "\"unit_id\":"       << S(c.unit_id) << ",";
        j << "\"position\":"      << Vec(c.position.x, c.position.y, c.position.z) << ",";
        j << "\"hp\":"            << F(c.hp) << ",";
        j << "\"is_alive\":"      << B(c.is_alive) << ",";
        j << "\"speed\":"         << F(c.speed) << ",";
        j << "\"rotation_yaw_deg\":" << F(c.rotation_yaw_deg);
        j << "}";
    }
    j << "],";
    j << "\"p_pellet_cooldown\":" << F(sq.p_pellet_cooldown);
    j << "},";

    j << "\"player_team_context\":[";
    for (size_t i = 0; i < req.player_team_context.size(); ++i) {
        const auto& p = req.player_team_context[i];
        if (i > 0) j << ",";
        j << "{";
        j << "\"player_id\":"    << S(p.player_id) << ",";
        j << "\"player_name\":"  << S(p.player_name) << ",";
        j << "\"player_type\":"  << S(p.player_type) << ",";
        j << "\"role\":"         << I(p.role) << ",";
        j << "\"position\":"     << Vec(p.position.x, p.position.y, p.position.z) << ",";
        j << "\"velocity\":{\"vx\":0,\"vy\":0,\"vz\":0},";
        j << "\"hp\":"           << F(p.hp) << ",";
        j << "\"distance_to_pacman\":"         << F(p.distance_to_pacman) << ",";
        j << "\"navmesh_cost_to_pacman\":"     << F(p.navmesh_cost_to_pacman) << ",";
        j << "\"navmesh_distance_to_pacman\":" << F(p.navmesh_distance_to_pacman) << ",";
        j << "\"navmesh_path_valid\":"  << B(p.navmesh_path_valid) << ",";
        j << "\"is_boosting\":"         << B(p.is_boosting) << ",";
        j << "\"recent_action\":"       << S(p.recent_action) << ",";
        j << "\"events\":[],";
        j << "\"equipped_runes\":[],";
        j << "\"commander_interaction\":{\"last_interaction_time\":\"\",\"interaction_count\":0},";
        j << "\"racer_nav_delta\":{\"player_index\":0,\"relative_angle_to_pacman_deg\":0,\"delta_position\":{\"x\":0,\"y\":0,\"z\":0},\"delta_straight_distance\":0,\"delta_path_distance\":0,\"delta_path_cost\":0,\"movement_direction_change_deg\":0,\"average_speed_s\":0,\"relative_bearing_change_deg\":0}";
        j << "}";
    }
    j << "],";

    j << "\"commander_context\":{";
    j << "\"cash_status\":{\"current\":0,\"max\":0},";
    j << "\"recent_actions\":[],";
    j << "\"management_stats\":{\"total_commands\":0,\"commands_followed\":0}";
    j << "},";

    j << "\"map_context\":{";
    j << "\"pacman_spawn\":"      << Vec(req.map_context.pacman_spawn.x, req.map_context.pacman_spawn.y, req.map_context.pacman_spawn.z) << ",";
    j << "\"p_pellet_locations\":[],";
    j << "\"p_point_locations\":[]";
    j << "}";

    j << "}";
    return j.str();
}

inline std::string BuildMatchStartRequest(const FMatchStartRequest& req) {
    std::ostringstream j;
    j << "{";
    j << "\"room_id\":" << S(req.room_id) << ",";

    auto strArr = [&](const std::string& key, const std::vector<std::string>& arr) {
        j << "\"" << key << "\":[";
        for (size_t i = 0; i < arr.size(); ++i) { if (i) j << ","; j << S(arr[i]); }
        j << "]";
    };
    strArr("player_ids",   req.player_ids);   j << ",";
    strArr("player_names", req.player_names); j << ",";
    strArr("player_types", req.player_types); j << ",";
    j << "\"map_id\":"  << S(req.map_id) << ",";
    j << "\"mode\":"    << S(req.mode);
    j << "}";
    return j.str();
}

inline std::string BuildMatchEndRequest(const FMatchEndRequest& req) {
    std::ostringstream j;
    j << "{";
    j << "\"room_id\":"   << S(req.room_id) << ",";
    j << "\"result\":"    << S(req.result) << ",";
    j << "\"final_score\":" << I(req.final_score) << ",";
    j << "\"match_duration_seconds\":" << I(req.match_duration_seconds) << ",";
    j << "\"winner_team\":" << S(req.winner_team) << ",";
    j << "\"end_reason\":"  << S(req.end_reason);
    j << "}";
    return j.str();
}

inline std::string ExtractStr(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    std::string out;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) { ++pos; }
        out += json[pos++];
    }
    return out;
}

inline float ExtractFloat(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return 0.f;
    pos += search.size();
    while (pos < json.size() && json[pos] == ' ') ++pos;
    size_t end = pos;
    while (end < json.size() && (std::isdigit((unsigned char)json[end]) || json[end]=='.' || json[end]=='-' || json[end]=='e' || json[end]=='E')) ++end;
    if (end == pos) return 0.f;
    try { return std::stof(json.substr(pos, end - pos)); } catch (...) { return 0.f; }
}

inline int ExtractInt(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return 0;
    pos += search.size();
    while (pos < json.size() && json[pos] == ' ') ++pos;
    size_t end = pos;
    while (end < json.size() && (std::isdigit((unsigned char)json[end]) || json[end]=='-')) ++end;
    if (end == pos) return 0;
    try { return std::stoi(json.substr(pos, end - pos)); } catch (...) { return 0; }
}

inline bool ExtractBool(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return false;
    pos += search.size();
    while (pos < json.size() && json[pos] == ' ') ++pos;
    return (json.substr(pos, 4) == "true");
}

inline std::vector<FUnitCommand> ParseUnitCommands(const std::string& json) {
    std::vector<FUnitCommand> cmds;
    size_t arr = json.find("\"unit_commands\":[");
    if (arr == std::string::npos) return cmds;
    arr += std::strlen("\"unit_commands\":[");

    size_t pos = arr;
    int depth = 0;
    size_t blockStart = std::string::npos;
    while (pos < json.size()) {
        char ch = json[pos];
        if (ch == '{') { if (depth == 0) blockStart = pos; ++depth; }
        else if (ch == '}') {
            --depth;
            if (depth == 0 && blockStart != std::string::npos) {
                std::string blk = json.substr(blockStart, pos - blockStart + 1);
                FUnitCommand cmd;
                cmd.unit_id        = ExtractStr(blk, "unit_id");
                cmd.directive_code = ExtractInt(blk, "directive_code");
                cmd.directive_name = ExtractStr(blk, "directive_name");
                if (cmd.directive_name.empty() && cmd.directive_code > 0)
                    cmd.directive_name = GetDirectiveName(cmd.directive_code);

                size_t tp = blk.find("\"target_position\":{");
                if (tp != std::string::npos) {
                    size_t te = blk.find("}", tp);
                    if (te != std::string::npos) {
                        std::string tblk = blk.substr(tp, te - tp + 1);
                        cmd.params.target_position.x = ExtractFloat(tblk, "x");
                        cmd.params.target_position.y = ExtractFloat(tblk, "y");
                        cmd.params.target_position.z = ExtractFloat(tblk, "z");
                    }
                }
                cmd.params.aggressiveness  = ExtractStr(blk, "aggressiveness");
                cmd.params.flank_direction = ExtractStr(blk, "flank_direction");
                cmd.params.pellet_id       = ExtractStr(blk, "pellet_id");
                cmds.push_back(cmd);
                blockStart = std::string::npos;
            }
        } else if (ch == ']' && depth == 0) break;
        ++pos;
    }
    return cmds;
}

inline FGetDecisionResponse ParseDecisionResponse(const std::string& json) {
    FGetDecisionResponse resp;
    resp.raw_json    = json;
    resp.status      = ExtractStr(json, "status");
    resp.timestamp   = ExtractStr(json, "timestamp");
    resp.room_id     = ExtractStr(json, "room_id");
    resp.request_num = ExtractStr(json, "request_num");
    resp.success     = (resp.status == "success");

    resp.decision.squad_objective = ExtractStr(json, "squad_objective");
    resp.decision.reasoning       = ExtractStr(json, "reasoning");
    resp.decision.confidence      = ExtractFloat(json, "confidence");
    resp.decision.unit_commands   = ParseUnitCommands(json);

    resp.meta.latency_ms   = ExtractFloat(json, "latency_ms");
    resp.meta.version      = ExtractStr(json, "version");
    resp.meta.fallback_used= ExtractBool(json, "fallback_used");
    resp.meta.llm_tokens   = ExtractInt(json, "llm_tokens");

    resp.has_pending_tts = ExtractBool(json, "has_pending_tts");
    return resp;
}

} // namespace JsonHelper
