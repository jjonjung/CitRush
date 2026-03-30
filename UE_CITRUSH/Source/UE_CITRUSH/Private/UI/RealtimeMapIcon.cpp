#include "UI/RealtimeMapIcon.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Player/Components/MinimapIconComponent.h"
#include "Engine/Texture2D.h"
#include "Slate/SlateBrushAsset.h"

URealtimeMapIcon::URealtimeMapIcon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DefaultColor(FLinearColor::White)
	, IconSize(FVector2D(32.0f, 32.0f))
{
}

void URealtimeMapIcon::NativeConstruct()
{
	Super::NativeConstruct();
	
	// IconImage 바인딩 확인
	if (!IconImage)
	{
		UE_LOG(LogTemp, Error, TEXT("[RealtimeMapIcon] NativeConstruct: IconImage가 바인딩되지 않았습니다! WBP_RealtimeMapIcon에 이름이 정확히 'IconImage'인 Image 위젯이 있는지 확인하세요."));
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[RealtimeMapIcon] NativeConstruct: IconImage 바인딩 완료"));
	}
}

void URealtimeMapIcon::SetupIcon(UMinimapIconComponent* InIconComponent)
{
	IconComponent = InIconComponent;
	
	if (!IconComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RealtimeMapIcon] SetupIcon: IconComponent가 nullptr입니다!"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[RealtimeMapIcon] SetupIcon 호출: Actor=%s, IconId=%d, TeamId=%d, bShowOnMap=%d"), 
		IconComponent->GetOwner() ? *IconComponent->GetOwner()->GetName() : TEXT("None"),
		(int32)IconComponent->IconId, IconComponent->TeamId, IconComponent->bShowOnMap ? 1 : 0);

	if (!IconImage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RealtimeMapIcon] SetupIcon: IconImage가 없습니다! WBP_RealtimeMapIcon에 IconImage 위젯이 바인딩되어 있는지 확인하세요."));
	}

	// 아이콘 ID 설정
	UpdateIconId(IconComponent->IconId);
	
	// 팀 색상 설정
	UpdateTeamColor(IconComponent->TeamId);
}

void URealtimeMapIcon::UpdatePosition(const FVector2D& InPosition)
{
	// CanvasPanelSlot을 통해 위치 설정
	// InPosition은 이미 맵 위의 정확한 위치(아이콘 중심)이므로, Alignment를 (0,0)으로 설정하고 오프셋 적용
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		// Alignment를 왼쪽 상단 (0, 0)으로 설정하여 일관된 위치 계산 보장
		CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
		
		// 아이콘 크기도 설정 (아직 설정되지 않은 경우)
		if (CanvasSlot->GetSize().IsNearlyZero())
		{
			CanvasSlot->SetSize(IconSize);
			UE_LOG(LogTemp, Log, TEXT("[RealtimeMapIcon] UpdatePosition: 아이콘 크기 설정 (%.1f, %.1f)"), IconSize.X, IconSize.Y);
		}
		
		// 아이콘 크기의 절반만큼 오프셋을 빼서 중심점을 맞춤
		// InPosition은 아이콘의 중심 위치이므로, 왼쪽 상단 모서리 위치로 변환
		FVector2D Offset = CanvasSlot->GetSize() * 0.5f;
		FVector2D FinalPosition = InPosition - Offset;
		CanvasSlot->SetPosition(FinalPosition);
		
		UE_LOG(LogTemp, Verbose, TEXT("[RealtimeMapIcon] UpdatePosition: InPosition=(%.1f, %.1f), Offset=(%.1f, %.1f), FinalPosition=(%.1f, %.1f), Size=(%.1f, %.1f)"), 
			InPosition.X, InPosition.Y, Offset.X, Offset.Y, FinalPosition.X, FinalPosition.Y, 
			CanvasSlot->GetSize().X, CanvasSlot->GetSize().Y);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RealtimeMapIcon] UpdatePosition: CanvasPanelSlot을 찾을 수 없습니다! 위젯이 CanvasPanel의 자식인지 확인하세요."));
	}
}

void URealtimeMapIcon::UpdateRotation(float InYaw)
{
	if (IconImage)
	{
		// Yaw를 회전 각도로 변환 (도 단위)
		IconImage->SetRenderTransformAngle(InYaw);
	}
}

void URealtimeMapIcon::UpdateIconId(ERealtimeMapIconId InIconId)
{
	if (!IconImage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RealtimeMapIcon] UpdateIconId: IconImage가 없습니다!"));
		return;
	}

	// 아이콘 ID에 맞는 텍스처 찾기
	UE_LOG(LogTemp, Log, TEXT("[RealtimeMapIcon] UpdateIconId 호출: IconId=%d (Racer1=%d, Racer2=%d, Racer3=%d, Enemy=%d)"), 
		(int32)InIconId, (int32)ERealtimeMapIconId::Racer1, (int32)ERealtimeMapIconId::Racer2, 
		(int32)ERealtimeMapIconId::Racer3, (int32)ERealtimeMapIconId::Enemy);
	
	if (TObjectPtr<UTexture2D>* FoundTexture = IconIdTextures.Find(InIconId))
	{
		if (UTexture2D* Texture = *FoundTexture)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(Texture);
			IconImage->SetBrush(Brush);
			UE_LOG(LogTemp, Log, TEXT("[RealtimeMapIcon] UpdateIconId: IconId=%d, Texture=%s 설정 완료"), 
				(int32)InIconId, *Texture->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[RealtimeMapIcon] UpdateIconId: IconId=%d에 대한 텍스처가 nullptr입니다!"), (int32)InIconId);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RealtimeMapIcon] UpdateIconId: IconId=%d에 대한 텍스처를 찾을 수 없습니다! IconIdTextures 맵에 설정되어 있는지 확인하세요. (현재 맵 크기: %d)"), 
			(int32)InIconId, IconIdTextures.Num());
	}
}

void URealtimeMapIcon::UpdateTeamColor(int32 InTeamId)
{
	if (!IconImage)
	{
		return;
	}

	// 팀 색상 찾기
	FLinearColor Color = DefaultColor;
	if (const FLinearColor* FoundColor = TeamColors.Find(InTeamId))
	{
		Color = *FoundColor;
	}

	// 색상 적용
	IconImage->SetColorAndOpacity(Color);
}

