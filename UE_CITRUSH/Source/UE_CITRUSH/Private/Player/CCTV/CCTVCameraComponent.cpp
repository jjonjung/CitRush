// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CCTV/CCTVCameraComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Player/AbstractRacer.h"
#include "Enemy/PixelEnemy.h"
#include "VehicleTemplate/UE_CITRUSHPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogCCTVCameraComponent, Log, All);

UCCTVCameraComponent::UCCTVCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCCTVCameraComponent::BeginPlay()
{
	Super::BeginPlay();
}

UCameraComponent* UCCTVCameraComponent::GetCCTVCamera(int32 SlotIndex) const
{
	if (SlotIndex < 0)
	{
		return nullptr;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// Enemy인 경우: SlotIndex 무시하고 Camera 반환
	if (APixelEnemy* Enemy = Cast<APixelEnemy>(Owner))
	{
		return Enemy->Camera;
	}

	// 레이서인 경우
	if (AAbstractRacer* Racer = Cast<AAbstractRacer>(Owner))
	{
		if (AUE_CITRUSHPawn* VehiclePawn = Cast<AUE_CITRUSHPawn>(Racer))
		{
			// UE_CITRUSHPawn: 카메라 순서는 0~4까지만 (5개)
			// 0=Back, 1=Front, 2=Left, 3=Right, 4=BackSide
			if (SlotIndex < 0 || SlotIndex >= 5)
			{
				return nullptr;
			}
			
			// SpringArm 배열을 사용하여 카메라 찾기
			TArray<USpringArmComponent*> SpringArms;
			VehiclePawn->GetComponents<USpringArmComponent>(SpringArms);
			
			// SpringArm 이름으로 순서 정렬 (Back, Front, Left, Right, BackSide)
			// 이름 매칭을 더 정확하게 하기 위해 대소문자 구분 없이 비교
			TArray<USpringArmComponent*> SortedSpringArms;
			SortedSpringArms.SetNum(5);
			
			for (USpringArmComponent* SpringArm : SpringArms)
			{
				if (!SpringArm) continue;
				
				FString SpringArmName = SpringArm->GetName();
				SpringArmName = SpringArmName.ToUpper(); // 대소문자 무시
				
				// Back Spring Arm (BackSide 제외)
				if (SpringArmName.Contains(TEXT("BACK")) && SpringArmName.Contains(TEXT("SPRING")) && 
				    !SpringArmName.Contains(TEXT("BACKSIDE")) && !SpringArmName.Contains(TEXT("FRONT")))
				{
					SortedSpringArms[0] = SpringArm; // Back
				}
				// Front Spring Arm
				else if (SpringArmName.Contains(TEXT("FRONT")) && SpringArmName.Contains(TEXT("SPRING")))
				{
					SortedSpringArms[1] = SpringArm; // Front
				}
				// Left/Right Spring Arm: 이름과 위치로 확인
				// Left는 Y가 음수, Right는 Y가 양수
				else if (SpringArmName.Contains(TEXT("SPRING")) && 
				         (SpringArmName.Contains(TEXT("LEFT")) || SpringArmName.Contains(TEXT("RIGHT"))))
				{
					FVector RelativeLocation = SpringArm->GetRelativeLocation();
					// 이름과 위치 모두 확인하여 정확한 매칭
					if (SpringArmName.Contains(TEXT("LEFT")))
					{
						// Left는 Y가 음수여야 함 (위치 검증)
						// 아직 설정되지 않았거나, 위치가 맞으면 설정
						if (SortedSpringArms[2] == nullptr)
						{
							// 위치 검증: Y가 음수인지 확인 (경고만 출력)
							if (RelativeLocation.Y >= 0.0f)
							{
								UE_LOG(LogCCTVCameraComponent, Warning, TEXT("[GetCCTVCamera] Left SpringArm의 Y 위치가 양수입니다: %f"), RelativeLocation.Y);
							}
							SortedSpringArms[2] = SpringArm; // Left
						}
					}
					else if (SpringArmName.Contains(TEXT("RIGHT")))
					{
						// Right는 Y가 양수여야 함 (위치 검증)
						// 아직 설정되지 않았거나, 위치가 맞으면 설정
						if (SortedSpringArms[3] == nullptr)
						{
							// 위치 검증: Y가 양수인지 확인 (경고만 출력)
							if (RelativeLocation.Y <= 0.0f)
							{
								UE_LOG(LogCCTVCameraComponent, Warning, TEXT("[GetCCTVCamera] Right SpringArm의 Y 위치가 음수입니다: %f"), RelativeLocation.Y);
							}
							SortedSpringArms[3] = SpringArm; // Right
						}
					}
				}
				// BackSide Spring Arm
				else if (SpringArmName.Contains(TEXT("BACKSIDE")) && SpringArmName.Contains(TEXT("SPRING")))
				{
					SortedSpringArms[4] = SpringArm; // BackSide
				}
			}
			
			// SlotIndex에 해당하는 SpringArm 찾기
			if (SortedSpringArms.IsValidIndex(SlotIndex) && SortedSpringArms[SlotIndex])
			{
				USpringArmComponent* TargetSpringArm = SortedSpringArms[SlotIndex];
				UCameraComponent* FoundCamera = FindCameraAttachedToSpringArm(TargetSpringArm);
				if (FoundCamera)
				{
					// 디버깅: 매칭된 SpringArm 이름과 위치 확인
					FVector SpringArmLocation = TargetSpringArm->GetRelativeLocation();
					FVector CameraLocation = FoundCamera->GetComponentLocation();
					UE_LOG(LogCCTVCameraComponent, Log, TEXT("[GetCCTVCamera] SlotIndex %d -> SpringArm: %s, SpringArm Location: %s, Camera: %s, Camera Location: %s"), 
						SlotIndex, *TargetSpringArm->GetName(), *SpringArmLocation.ToString(), *FoundCamera->GetName(), *CameraLocation.ToString());
					return FoundCamera;
				}
				else
				{
					UE_LOG(LogCCTVCameraComponent, Warning, TEXT("[GetCCTVCamera] SlotIndex %d의 SpringArm %s에 연결된 Camera를 찾을 수 없습니다"), 
						SlotIndex, *TargetSpringArm->GetName());
				}
			}
			else
			{
				UE_LOG(LogCCTVCameraComponent, Warning, TEXT("[GetCCTVCamera] SlotIndex %d에 해당하는 SpringArm을 찾을 수 없습니다. SortedSpringArms 상태:"), SlotIndex);
				for (int32 i = 0; i < SortedSpringArms.Num(); ++i)
				{
					if (SortedSpringArms[i])
					{
						UE_LOG(LogCCTVCameraComponent, Warning, TEXT("  [%d] %s"), i, *SortedSpringArms[i]->GetName());
					}
					else
					{
						UE_LOG(LogCCTVCameraComponent, Warning, TEXT("  [%d] nullptr"), i);
					}
				}
			}
			
			// Fallback: Getter 함수 사용 (Back, Front만)
			if (SlotIndex == 0)
			{
				UCameraComponent* BackCamera = FindCameraAttachedToSpringArm(VehiclePawn->GetBackSpringArm());
				if (BackCamera) return BackCamera;
			}
			if (SlotIndex == 1)
			{
				UCameraComponent* FrontCamera = FindCameraAttachedToSpringArm(VehiclePawn->GetFrontSpringArm());
				if (FrontCamera) return FrontCamera;
			}
			
			// SlotIndex 2~4는 이름으로 찾지 못한 경우 null 반환
			UE_LOG(LogCCTVCameraComponent, Warning, TEXT("[GetCCTVCamera] SlotIndex %d에 해당하는 카메라를 찾을 수 없습니다"), SlotIndex);
			return nullptr;
		}
		else
		{
			// 다른 레이서 타입: SpringArm 인덱스로 찾기
			TArray<USpringArmComponent*> SpringArms;
			Owner->GetComponents<USpringArmComponent>(SpringArms);
			if (SpringArms.IsValidIndex(SlotIndex))
			{
				return FindCameraAttachedToSpringArm(SpringArms[SlotIndex]);
			}
		}
	}
	// 기타 Actor: SpringArm 인덱스로 찾기
	else
	{
		TArray<USpringArmComponent*> SpringArms;
		Owner->GetComponents<USpringArmComponent>(SpringArms);
		if (SpringArms.IsValidIndex(SlotIndex))
		{
			return FindCameraAttachedToSpringArm(SpringArms[SlotIndex]);
		}
	}

	return nullptr;
}


UCameraComponent* UCCTVCameraComponent::FindSlot2SpringArmForVehiclePawn(AAbstractRacer* Racer) const
{
	if (!Racer)
	{
		return nullptr;
	}

	TArray<USpringArmComponent*> SpringArms;
	Racer->GetComponents<USpringArmComponent>(SpringArms);

	int32 PlayerIndex = (int32)Racer->AutoPossessPlayer - 1;
	const TCHAR* SearchName = nullptr;

	if (PlayerIndex == 0)
	{
		SearchName = TEXT("Left");
	}
	else if (PlayerIndex == 1)
	{
		SearchName = TEXT("Right");
	}
	else if (PlayerIndex == 2)
	{
		SearchName = TEXT("BackSide");
	}

	if (SearchName)
	{
		for (USpringArmComponent* SpringArm : SpringArms)
		{
			if (SpringArm && SpringArm->GetName().Contains(SearchName))
			{
				return FindCameraAttachedToSpringArm(SpringArm);
			}
		}
	}

	// Fallback: 세 번째 SpringArm 사용
	if (SpringArms.Num() >= 3)
	{
		return FindCameraAttachedToSpringArm(SpringArms[2]);
	}

	return nullptr;
}

UCameraComponent* UCCTVCameraComponent::FindCameraAttachedToSpringArm(USpringArmComponent* SpringArm) const
{
	if (!SpringArm)
	{
		UE_LOG(LogCCTVCameraComponent, Warning, TEXT("[FindCameraAttachedToSpringArm] SpringArm이 null입니다"));
		return nullptr;
	}

	TArray<USceneComponent*> Children;
	SpringArm->GetChildrenComponents(false, Children);

	UE_LOG(LogCCTVCameraComponent, Log, TEXT("[FindCameraAttachedToSpringArm] SpringArm: %s, 자식 컴포넌트 개수: %d"), 
		*SpringArm->GetName(), Children.Num());

	for (USceneComponent* Child : Children)
	{
		if (!Child) continue;
		
		UE_LOG(LogCCTVCameraComponent, Log, TEXT("[FindCameraAttachedToSpringArm] 자식 컴포넌트 확인: %s (타입: %s)"), 
			*Child->GetName(), *Child->GetClass()->GetName());
		
		if (UCameraComponent* Camera = Cast<UCameraComponent>(Child))
		{
			UE_LOG(LogCCTVCameraComponent, Log, TEXT("[FindCameraAttachedToSpringArm] Camera 찾음: %s, Location: %s"), 
				*Camera->GetName(), *Camera->GetComponentLocation().ToString());
			return Camera;
		}
	}

	UE_LOG(LogCCTVCameraComponent, Warning, TEXT("[FindCameraAttachedToSpringArm] SpringArm %s에 연결된 Camera를 찾을 수 없습니다"), 
		*SpringArm->GetName());
	return nullptr;
}

