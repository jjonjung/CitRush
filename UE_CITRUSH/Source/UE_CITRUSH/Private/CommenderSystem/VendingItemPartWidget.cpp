// VendingItemPartWidget.cpp

#include "CommenderSystem/VendingItemPartWidget.h"
#include "CommenderSystem/VendingItemListWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "CommenderSystem/VendingSlotUIData.h"

void UVendingItemPartWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ItemButton)
	{
		ItemButton->OnClicked.AddDynamic(this, &UVendingItemPartWidget::HandleButtonClicked);
	}
}

void UVendingItemPartWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	CurrentData = Cast<UVendingSlotUIData>(ListItemObject);

	if (!CurrentData)
	{
		if (ItemNameText)
			ItemNameText->SetText(FText::GetEmpty());
		if (ItemIcon)
			ItemIcon->SetBrushFromTexture(nullptr);
		return;
	}

	// ===== 빈 슬롯 처리 =====
	if (CurrentData->bIsEmpty)
	{
		// 버튼 비활성화
		if (ItemButton)
		{
			ItemButton->SetIsEnabled(false);
		}

		// 투명도 감소 (0.3)
		SetRenderOpacity(0.3f);

		// "SOLD OUT" 텍스트 표시
		if (ItemNameText)
		{
			ItemNameText->SetText(FText::FromString(TEXT("SOLD OUT")));
		}

		// 아이콘 회색으로
		if (ItemIcon)
		{
			ItemIcon->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
		}

		// 남은 개수 표시
		if (RemainingCountText)
		{
			RemainingCountText->SetText(FText::FromString(TEXT("0")));
		}

		return;
	}

	// ===== 아이템이 있는 경우 =====
	
	// 버튼 활성화
	if (ItemButton)
	{
		ItemButton->SetIsEnabled(true);
	}

	// 투명도 복원 (1.0)
	SetRenderOpacity(1.0f);

	// 아이템 이름 표시
	if (ItemNameText)
	{
		ItemNameText->SetText(CurrentData->ItemName);
		ItemNameText->SetAutoWrapText(true);
		ItemNameText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	}

	// 아이콘 표시 (원래 색상)
	if (ItemIcon)
	{
		ItemIcon->SetBrushFromTexture(CurrentData->Icon ? CurrentData->Icon : nullptr);
		ItemIcon->SetColorAndOpacity(FLinearColor::White);
	}

	// Coin 표시 (Optional)
	if (CoinText)
	{
		CoinText->SetText(FText::AsNumber(CurrentData->Coin));
	}

	// 남은 개수 표시 (Optional)
	if (RemainingCountText)
	{
		RemainingCountText->SetText(FText::AsNumber(CurrentData->RemainingCount));
	}
}

void UVendingItemPartWidget::HandleButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[VendingItemPartWidget] HandleButtonClicked 호출"));
	
	if (!CurrentData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VendingItemPartWidget] CurrentData가 없습니다"));
		return;
	}

	if (CurrentData->bIsEmpty)
	{
		UE_LOG(LogTemp, Log, TEXT("[VendingItemPartWidget] 슬롯이 비어있습니다"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[VendingItemPartWidget] 슬롯 %d 클릭: %s"), CurrentData->SlotIndex, *CurrentData->ItemName.ToString());

	// 부모 위젯으로 클릭 이벤트 전달
	if (UUserWidget* ParentWidget = GetTypedOuter<UUserWidget>())
	{
		if (UVendingItemListWidget* ListWidget = Cast<UVendingItemListWidget>(ParentWidget))
		{
			UE_LOG(LogTemp, Log, TEXT("[VendingItemPartWidget] ListWidget->OnItemEntryClicked 호출"));
			ListWidget->OnItemEntryClicked(CurrentData);
			return;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[VendingItemPartWidget] ListWidget로 캐스팅 실패"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[VendingItemPartWidget] ParentWidget을 찾을 수 없습니다"));
	}
	
	UE_LOG(LogTemp, Log, TEXT("[VendingItemPartWidget] OnEntryClicked.Broadcast 호출"));
	OnEntryClicked.Broadcast(CurrentData);
}