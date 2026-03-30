#include "Player/Stats/PingMarkerManager.h"
#include "Player/Stats/PingMarkerActor.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"

UPingMarkerManager* UPingMarkerManager::Get(UWorld* World)
{
    // 기본 C++ 클래스 사용 (오버로드에 nullptr 전달)
    return Get(World, nullptr);
}

UPingMarkerManager* UPingMarkerManager::Get(UWorld* World, TSubclassOf<UPingMarkerManager> ManagerClassOverride)
{
    if (!World)
    {
        return nullptr;
    }

    // GameInstance에 저장하거나, 간단하게 월드 서브시스템으로 만들 수 있음
    // 여기서는 간단하게 월드별 로컬 매니저를 TWeakObjectPtr 로 관리한다.
    // (월드/매니저가 GC 로 파괴된 뒤에도 안전하게 무시되도록)
    static TMap<UWorld*, TWeakObjectPtr<UPingMarkerManager>> Instances;
    
    if (TWeakObjectPtr<UPingMarkerManager>* Existing = Instances.Find(World))
    {
        if (Existing->IsValid())
        {
            return Existing->Get();
        }
        
        // 약한 포인터가 더 이상 유효하지 않다면 엔트리를 제거하고 새로 만든다.
        Instances.Remove(World);
    }

    // 사용할 클래스 결정: Blueprint 서브클래스가 지정되어 있으면 그것을, 아니면 기본 C++ 클래스를 사용
    UClass* ClassToUse = UPingMarkerManager::StaticClass();
    if (ManagerClassOverride && ManagerClassOverride->IsChildOf(UPingMarkerManager::StaticClass()))
    {
        ClassToUse = ManagerClassOverride;
    }

    // 새 인스턴스 생성
    UPingMarkerManager* NewInstance = NewObject<UPingMarkerManager>(World, ClassToUse);
    NewInstance->WorldRef = World;
    Instances.Add(World, NewInstance);
    
    return NewInstance;
}

void UPingMarkerManager::UpdatePingMarker(const FPingData& PingData)
{
    // 여전히 호출되는 곳을 위해 1개짜리 리스트로 Refresh 에 위임
    TArray<FPingData> Single;
    if (PingData.IsValid())
    {
        Single.Add(PingData);
    }
    RefreshPingMarkers(Single);
}

void UPingMarkerManager::RefreshPingMarkers(const TArray<FPingData>& PingList)
{
    UE_LOG(LogTemp, Log, TEXT("[PingMarkerManager] RefreshPingMarkers 호출 - PingList 수: %d"), PingList.Num());
    
    // 월드가 없으면 아무 것도 하지 않음 (레벨 전환 중 등)
    if (!WorldRef || !IsValid(WorldRef))
    {
        UE_LOG(LogTemp, Warning, TEXT("[PingMarkerManager] RefreshPingMarkers: WorldRef가 유효하지 않습니다!"));
        return;
    }

    // 1) 새로운 PingId 집합 구성
    TSet<FGuid> NewIds;
    for (const FPingData& Data : PingList)
    {
        if (Data.IsValid())
        {
            NewIds.Add(Data.PingId);
        }
    }

    // 2) 더 이상 존재하지 않는 마커 제거
    for (auto It = MarkersById.CreateIterator(); It; ++It)
    {
        const FGuid& ExistingId = It->Key;
        TWeakObjectPtr<APingMarkerActor>& MarkerPtr = It->Value;

        if (!NewIds.Contains(ExistingId))
        {
            if (MarkerPtr.IsValid())
            {
                MarkerPtr->RemoveMarker();
            }
            It.RemoveCurrent();
        }
    }

    // 3) PingList 에 포함된 핑들에 대해 마커 생성/업데이트
    for (const FPingData& Data : PingList)
    {
        if (!Data.IsValid())
        {
            continue;
        }

        TWeakObjectPtr<APingMarkerActor>* FoundPtr = MarkersById.Find(Data.PingId);
        APingMarkerActor* MarkerActor = FoundPtr ? FoundPtr->Get() : nullptr;

        if (!MarkerActor)
        {
            UE_LOG(LogTemp, Log, TEXT("[PingMarkerManager] 새 핑 마커 스폰 시도 - Location: %s, Type: %d"), 
                *Data.WorldLocation.ToString(), (int32)Data.Type);
            
            // 새 마커 스폰
            TSubclassOf<APingMarkerActor> ClassToSpawn = MarkerClass;
            if (!ClassToSpawn || !ClassToSpawn->IsChildOf(APingMarkerActor::StaticClass()))
            {
                ClassToSpawn = APingMarkerActor::StaticClass();
                UE_LOG(LogTemp, Log, TEXT("[PingMarkerManager] MarkerClass가 없어 기본 클래스 사용: %s"), 
                    *ClassToSpawn->GetName());
            }

            if (!ClassToSpawn)
            {
                UE_LOG(LogTemp, Error, TEXT("[PingMarkerManager] RefreshPingMarkers: ClassToSpawn is null, aborting spawn"));
                continue;
            }

            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = nullptr;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            MarkerActor = WorldRef->SpawnActor<APingMarkerActor>(ClassToSpawn, Data.WorldLocation, FRotator::ZeroRotator, SpawnParams);
            if (!MarkerActor)
            {
                UE_LOG(LogTemp, Error, TEXT("[PingMarkerManager] RefreshPingMarkers: Failed to spawn PingMarkerActor at %s"), 
                    *Data.WorldLocation.ToString());
                continue;
            }

            MarkersById.Add(Data.PingId, MarkerActor);

            UE_LOG(LogTemp, Warning,
                TEXT("[PingMarkerManager] ✓✓✓ Spawned PingMarkerActor '%s' at %s (Class=%s)"),
                *MarkerActor->GetName(),
                *Data.WorldLocation.ToString(),
                *ClassToSpawn->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Verbose, TEXT("[PingMarkerManager] 기존 핑 마커 업데이트 - Location: %s"), 
                *Data.WorldLocation.ToString());
        }

        // 데이터 갱신
        MarkerActor->SetPingData(Data);
    }
}

void UPingMarkerManager::ClearPingMarker()
{
    // 모든 마커 제거
    for (auto& Pair : MarkersById)
    {
        if (Pair.Value.IsValid())
        {
            Pair.Value->RemoveMarker();
        }
    }
    MarkersById.Empty();
}
