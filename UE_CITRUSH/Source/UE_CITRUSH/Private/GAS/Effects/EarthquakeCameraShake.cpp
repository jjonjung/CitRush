// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Effects/EarthquakeCameraShake.h"


UEarthquakeCameraShake::UEarthquakeCameraShake()
{
    // 기본 타이밍
	OscillationDuration = 1.5f;
	OscillationBlendInTime = 0.15f;
	OscillationBlendOutTime = 0.7f;

	// Pitch (상하)
	RotOscillation.Pitch.Amplitude = 2.0f;
	RotOscillation.Pitch.Frequency = 6.0f;
	RotOscillation.Pitch.InitialOffset = EInitialOscillatorOffset::EOO_OffsetRandom;
	RotOscillation.Pitch.Waveform = EOscillatorWaveform::PerlinNoise;
    
	// Yaw (좌우)
	RotOscillation.Yaw.Amplitude = 2.0f;
	RotOscillation.Yaw.Frequency = 5.5f;
	RotOscillation.Yaw.InitialOffset = EInitialOscillatorOffset::EOO_OffsetRandom;
	RotOscillation.Yaw.Waveform = EOscillatorWaveform::PerlinNoise;
    
	// Roll (기울기)
	RotOscillation.Roll.Amplitude = 1.0f;
	RotOscillation.Roll.Frequency = 5.0f;
	RotOscillation.Roll.InitialOffset = EInitialOscillatorOffset::EOO_OffsetRandom;
	RotOscillation.Roll.Waveform = EOscillatorWaveform::PerlinNoise;

	// X축 (앞뒤)
	LocOscillation.X.Amplitude = 5.0f;
	LocOscillation.X.Frequency = 4.5f;
	LocOscillation.X.InitialOffset = EInitialOscillatorOffset::EOO_OffsetRandom;
	LocOscillation.X.Waveform = EOscillatorWaveform::PerlinNoise;
    
	// Y축 (좌우)
	LocOscillation.Y.Amplitude = 5.0f;
	LocOscillation.Y.Frequency = 4.0f;
	LocOscillation.Y.InitialOffset = EInitialOscillatorOffset::EOO_OffsetRandom;
	LocOscillation.Y.Waveform = EOscillatorWaveform::PerlinNoise;
    
	// Z축 (상하)
	LocOscillation.Z.Amplitude = 4.0f;
	LocOscillation.Z.Frequency = 5.0f;
	LocOscillation.Z.InitialOffset = EInitialOscillatorOffset::EOO_OffsetRandom;
	LocOscillation.Z.Waveform = EOscillatorWaveform::PerlinNoise;

	// FOV
	FOVOscillation.Amplitude = 1.0f;
	FOVOscillation.Frequency = 5.5f;
	FOVOscillation.InitialOffset = EInitialOscillatorOffset::EOO_OffsetRandom;
	FOVOscillation.Waveform = EOscillatorWaveform::PerlinNoise;
}
