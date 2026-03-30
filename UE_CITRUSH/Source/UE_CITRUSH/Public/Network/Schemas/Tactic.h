#pragma once

//Ai에서 넘어오는 데이터 처리값

enum class EAITactic : int32
{
	AMBUSH = 1,              // 매복
	MOVE_TO_LOCATION = 2,    // 위치로 이동
	INTERCEPT = 3,           // 요격
	CHASE = 4,               // 추격
	RETREAT = 5,             // 후퇴
	PATROL = 6,              // 순찰
	CONSUME_PELLET = 7,      // 파워펠렛 섭취
	GUARD = 8,               // 방어
	FLANK = 9,               // 측면 우회
	SPLIT_FORMATION = 10,    // 분산 대형
	REGROUP = 11,            // 재집결
	FAKE_RETREAT = 12        // 후퇴 위장
};