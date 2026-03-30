// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenu/RankingItemWidget.h"

void URankingItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void URankingItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	//TODO : Ranking이 담긴 Web Server에서 받아서 Setting
	// Tree View 열릴 때 멤버들 Steam NickName 표시
	
}

void URankingItemWidget::OnClickItem()
{
}
