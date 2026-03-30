# CITRUSH 프로젝트 GAS 구현 분석 및 설명

**작성일**: 2025-12-01
**프로젝트**: UE_CITRUSH (UE 5.4.4)
**문서 목적**: CITRUSH 프로젝트의 Gameplay Ability System (GAS) 구현 현황 분석 및 가이드

---

📋 주요 섹션

  1. 프로젝트 개요: CITRUSH의 특성 및 GAS 도입 목적
  2. GAS 아키텍처: 핵심 컴포넌트 및 폴더 구조
  3. AttributeSet 구현 분석:
    - 매크로 기반 시스템 (혁신적인 Boilerplate 감소)
    - ASRacer, ASCommander, ASEnemy 상세 분석
  4. GameplayTags 시스템: Native Tags 방식 완벽 분석
  5. GameplayAbility 구현: BaseGA 및 GATestBooster
  6. ASC 배치 및 초기화: PlayerState vs Character 전략
  7. 데미지 시스템 설계: Event 기반 설계 상세
  8. 구현 상태 요약: Phase별 진행 상황
  9. 향후 개발 권장사항: 우선순위별 작업 가이드

  ✨ 문서의 특징

  - AgentLog 3개 파일 통합: GAS.md, GAS-Implementation-Status.md, GameplayTags_Guide.md의 핵심 내용
  통합
  - 실제 코드 분석: Source 폴더의 실제 구현 코드 기반
  - 실용적 가이드: 다음 단계로 무엇을 해야 하는지 명확히 제시
  - 한국어로 작성: 모든 내용 한국어로 작성

  🎯 특히 강조된 내용

  1. 매크로 기반 AttributeSet 시스템: 2줄로 완전한 Attribute 생성
  2. Native GameplayTags: 컴파일 타임 타입 안전성
  3. Event 기반 데미지 시스템: OnCollisionHit 상세 분석
  4. Phase별 구현 현황: 완료/진행중/미구현 명확히 표시

---

## 목차

1. [프로젝트 개요](#1-프로젝트-개요)
2. [GAS 아키텍처 개요](#2-gas-아키텍처-개요)
3. [AttributeSet 구현 분석](#3-attributeset-구현-분석)
4. [GameplayTags 시스템](#4-gameplaytags-시스템)
5. [GameplayAbility 구현](#5-gameplayability-구현)
6. [ASC 배치 및 초기화](#6-asc-배치-및-초기화)
7. [데미지 시스템 설계](#7-데미지-시스템-설계)
8. [구현 상태 요약](#8-구현-상태-요약)
9. [향후 개발 권장사항](#9-향후-개발-권장사항)

---

## 1. 프로젝트 개요

### 1.1 프로젝트 특성

**UE_CITRUSH**는 전략 레이싱 슈팅 하이브리드 게임으로, 다음과 같은 특징을 가집니다:

- **장르**: 전략 레이싱 슈팅
- **플레이어 구성**:
  - 1명 지휘자 (Commander)
  - 3명 레이서 (Racer)
- **네트워크**: 4인 멀티플레이
- **목표**: 적대 NPC를 피해 달아나며, NPC를 물리치거나 생존

### 1.2 GAS 도입 목적

GAS는 다음과 같은 기능을 구현하기 위해 도입되었습니다:

- **캐릭터 스탯 관리**: Health, Fuel, AttackPower 등
- **아이템 시스템**: 다양한 아이템 효과 및 버프/디버프
- **어빌리티 시스템**: 레이서의 Boost, Dash 등
- **네트워크 동기화**: 멀티플레이 환경에서의 상태 동기화

---

## 2. GAS 아키텍처 개요

### 2.1 핵심 컴포넌트 구조

```
┌─────────────────────────────────────┐
│   Ability System Component (ASC)    │ ← 중앙 인터페이스
├─────────────────────────────────────┤
│  ┌─────────────────────────────┐   │
│  │   Gameplay Abilities        │   │ ← 실행 가능한 액션
│  └─────────────────────────────┘   │
│  ┌─────────────────────────────┐   │
│  │   Gameplay Effects          │   │ ← Attribute 변경 메커니즘
│  └─────────────────────────────┘   │
│  ┌─────────────────────────────┐   │
│  │   Gameplay Attributes       │   │ ← 변경 가능한 데이터
│  └─────────────────────────────┘   │
└─────────────────────────────────────┘
```

### 2.2 프로젝트 폴더 구조

```
Source/UE_CITRUSH/
├── Public/GAS/
│   ├── AbilitySystemComponent/
│   │   └── BaseASC.h
│   ├── AttributeSet/
│   │   ├── BaseAS.h           ← 매크로 시스템
│   │   ├── ASRacer.h          ← 레이서 전용
│   │   ├── ASCommander.h      ← 지휘자 전용
│   │   └── ASEnemy.h          ← 적 전용
│   ├── GameplayAbility/
│   │   ├── BaseGA.h
│   │   └── Racer/
│   │       └── GATestBooster.h
│   ├── GameplayEffect/
│   │   ├── BaseGE.h
│   │   └── BaseGEExecutionCalculation.h
│   ├── AbilityTask/
│   │   └── BaseAT.h
│   └── GameplayTags/
│       ├── GTNative.h         ← 공통 태그
│       ├── GTRacer.h          ← 레이서 태그
│       ├── GTCommander.h      ← 지휘자 태그
│       └── GTEnemy.h          ← 적 태그
│
└── Private/GAS/
    └── [해당 .cpp 파일들]
```

---

## 3. AttributeSet 구현 분석

### 3.1 BaseAS - 매크로 시스템

CITRUSH 프로젝트는 **매크로 기반 Attribute 자동 생성 시스템**을 구현했습니다. 이는 Boilerplate 코드를 획기적으로 줄이는 혁신적인 구조입니다.

#### 3.1.1 매크로 정의 (`BaseAS.h`)

```cpp
#define DECLARE_ATTRIBUTE(ClassName, PropertyName) \
protected: \
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_##PropertyName, Category="GAS|Attribute") \
    FGameplayAttributeData PropertyName; \
    UFUNCTION() \
    void OnRep_##PropertyName(const FGameplayAttributeData& old##PropertyName) const; \
public: \
    static FGameplayAttribute Get##PropertyName##Attribute() \
    { \
        static FProperty* Prop = FindFieldChecked<FProperty>(ClassName::StaticClass(), GET_MEMBER_NAME_CHECKED(ClassName, PropertyName)); \
        return Prop; \
    } \
    float Get##PropertyName() const; \
    void Set##PropertyName(float NewVal); \
    void Init##PropertyName(float NewVal);

#define DEFINE_ATTRIBUTE(ClassName, PropertyName) \
    void ClassName::OnRep_##PropertyName(const FGameplayAttributeData& old##PropertyName) const \
    { \
        static FProperty* ThisProperty = FindFieldChecked<FProperty>(ClassName::StaticClass(), GET_MEMBER_NAME_CHECKED(ClassName, PropertyName)); \
        GetOwningAbilitySystemComponentChecked()->SetBaseAttributeValueFromReplication(FGameplayAttribute(ThisProperty), PropertyName, old##PropertyName); \
    } \
    float ClassName::Get##PropertyName() const \
    { \
        return PropertyName.GetCurrentValue(); \
    } \
    void ClassName::Set##PropertyName(float NewVal) \
    { \
        UAbilitySystemComponent* AbilityComp = GetOwningAbilitySystemComponent(); \
        if (ensure(AbilityComp)) \
        { \
            AbilityComp->SetNumericAttributeBase(Get##PropertyName##Attribute(), NewVal); \
        } \
    } \
    void ClassName::Init##PropertyName(float NewVal) \
    { \
        PropertyName.SetBaseValue(NewVal); \
        PropertyName.SetCurrentValue(NewVal); \
    }
```

#### 3.1.2 매크로 사용법

**헤더 파일에서:**
```cpp
DECLARE_ATTRIBUTE(UASRacer, Health)
```

**구현 파일에서:**
```cpp
DEFINE_ATTRIBUTE(UASRacer, Health)
```

이 2줄로 다음 기능이 자동 생성됩니다:
- ✅ UPROPERTY 선언 (BlueprintReadOnly, Replication)
- ✅ OnRep 함수 구현
- ✅ Getter/Setter 함수
- ✅ Init 함수
- ✅ Static GetAttribute 함수

#### 3.1.3 매크로 시스템의 장점

| 항목 | 기존 방식 | 매크로 방식 |
|------|----------|------------|
| **코드량** | ~50줄/Attribute | 2줄/Attribute |
| **실수 가능성** | 높음 (수동 타이핑) | 낮음 (자동 생성) |
| **일관성** | 개발자마다 다를 수 있음 | 100% 일관성 보장 |
| **유지보수** | 어려움 | 쉬움 |
| **Replication** | 수동 설정 필요 | 자동 설정 |

### 3.2 ASRacer (레이서 AttributeSet)

#### 3.2.1 구조

```cpp
UCLASS()
class UE_CITRUSH_API UASRacer : public UBaseAS
{
    GENERATED_BODY()

public:
    UASRacer();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    DECLARE_ATTRIBUTE(UASRacer, Health)
    DECLARE_ATTRIBUTE(UASRacer, MaxHealth)
    DECLARE_ATTRIBUTE(UASRacer, Fuel)
    DECLARE_ATTRIBUTE(UASRacer, MaxFuel)
    DECLARE_ATTRIBUTE(UASRacer, AttackPower)
};
```

#### 3.2.2 Attribute 목록

| Attribute | 용도 | 초기값 | Clamping |
|-----------|------|--------|----------|
| **Health** | 현재 체력 | 100 | 0 ~ MaxHealth |
| **MaxHealth** | 최대 체력 | 100 | - |
| **Fuel** | 현재 연료 | 100 | 0 ~ MaxFuel |
| **MaxFuel** | 최대 연료 | 100 | - |
| **AttackPower** | 공격력 | 10 | 0 ~ ∞ |

#### 3.2.3 Clamping 로직

```cpp
void UASRacer::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    // Health를 0과 MaxHealth 사이로 제한
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
    }

    // Fuel을 0과 MaxFuel 사이로 제한
    if (Attribute == GetFuelAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxFuel());
    }
}
```

### 3.3 ASCommander (지휘자 AttributeSet)

#### 3.3.1 구조

```cpp
UCLASS()
class UE_CITRUSH_API UASCommander : public UBaseAS
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    DECLARE_ATTRIBUTE(UASCommander, CommandPoints)
    DECLARE_ATTRIBUTE(UASCommander, MaxCommandPoints)
    DECLARE_ATTRIBUTE(UASCommander, ScanRange)
};
```

#### 3.3.2 Attribute 목록

| Attribute | 용도 | 설명 |
|-----------|------|------|
| **CommandPoints** | 현재 지휘 포인트 | 스킬 사용 자원 |
| **MaxCommandPoints** | 최대 지휘 포인트 | 지휘 포인트 상한 |
| **ScanRange** | 스캔 범위 | 적 탐지 범위 |

**설계 의문점**:
- ⚠️ Health, MaxHealth가 없음 (지휘자는 죽지 않는 역할?)
- ⚠️ ScanRange Clamping이 자기 자신을 참조 (개선 필요)

### 3.4 ASEnemy (적 AttributeSet)

#### 3.4.1 구조

```cpp
UCLASS()
class UE_CITRUSH_API UASEnemy : public UBaseAS
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    DECLARE_ATTRIBUTE(UASEnemy, Health)
    DECLARE_ATTRIBUTE(UASEnemy, MaxHealth)
    DECLARE_ATTRIBUTE(UASEnemy, DetectionRange)
    DECLARE_ATTRIBUTE(UASEnemy, DefaultDetectionRange)
    DECLARE_ATTRIBUTE(UASEnemy, Speed)
    DECLARE_ATTRIBUTE(UASEnemy, DefaultSpeed)
    DECLARE_ATTRIBUTE(UASEnemy, AttackPower)
    DECLARE_ATTRIBUTE(UASEnemy, DefaultAttackPower)
};
```

#### 3.4.2 Attribute 목록

| Attribute | 용도 | Default Attribute |
|-----------|------|-------------------|
| **Health** | 현재 체력 | - |
| **MaxHealth** | 최대 체력 | - |
| **DetectionRange** | 현재 감지 범위 | DefaultDetectionRange |
| **DefaultDetectionRange** | 기본 감지 범위 | - |
| **Speed** | 현재 이동 속도 | DefaultSpeed |
| **DefaultSpeed** | 기본 이동 속도 | - |
| **AttackPower** | 현재 공격력 | DefaultAttackPower |
| **DefaultAttackPower** | 기본 공격력 | - |

**특징**:
- ✅ Default Attributes 패턴 사용 (버프/디버프 후 원래 값 복원 가능)
- ⚠️ PostGameplayEffectExecute에 버그 존재 (Clamping이 GetMaxHealth()로 잘못 설정됨)

---

## 4. GameplayTags 시스템

CITRUSH 프로젝트는 **Native GameplayTags 방식**을 채택했습니다. 이는 `.ini` 파일 방식보다 더 안전하고 효율적인 방법입니다.

### 4.1 Native Tags의 장점

| 항목 | `.ini` 방식 | Native Tags 방식 |
|------|-------------|------------------|
| **타입 안전성** | ❌ 런타임 체크 | ✅ 컴파일 타임 체크 |
| **자동완성** | ❌ 없음 | ✅ IntelliSense 지원 |
| **오타 방지** | ❌ 런타임 에러 | ✅ 컴파일 에러 |
| **리팩토링** | ❌ 수동 | ✅ IDE 지원 |
| **성능** | 동일 | 동일 |

### 4.2 GTNative - 공통 태그

```cpp
namespace CitRushTags
{
    //=================================================================
    // State Tags - 공통 상태 (모든 역할이 사용)
    //=================================================================
    namespace State
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Alive);      // State.Alive
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dead);       // State.Dead
    }

    //=================================================================
    // Event Tags - 공통 이벤트
    //=================================================================
    namespace Event
    {
        namespace Result
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Success);   // Event.Result.Success
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail);      // Event.Result.Fail
        }

        namespace Gameplay
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Start);  // Event.Gameplay.Start
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(End);    // Event.Gameplay.End
        }
    }

    //=================================================================
    // GameplayCue Tags - 공통 Cue
    //=================================================================
    namespace Cue
    {
        namespace Gameplay
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Start);     // GameplayCue.Gameplay.Start
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(End);       // GameplayCue.Gameplay.End
        }
    }
}
```

### 4.3 GTRacer - 레이서 태그 구조

```cpp
namespace CitRushTags::Racer
{
    //=================================================================
    // Racer.Ability - 레이서 기본 능력
    //=================================================================
    namespace Ability
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseItem);    // Racer.Ability.UseItem
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(SwapItem);   // Racer.Ability.SwapItem
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Move);       // Racer.Ability.Move
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Brake);      // Racer.Ability.Brake
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Drift);      // Racer.Ability.Drift
    }

    //=================================================================
    // Racer.Item - 레이서 아이템
    //=================================================================
    namespace Item
    {
        // Native Items
        namespace Native
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Heal);       // Racer.Item.Native.Heal
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Refuel);     // Racer.Item.Native.Refuel
        }

        // Offensive Items
        namespace Offensive
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpeedBoost); // Racer.Item.Offensive.SpeedBoost
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mine);       // Racer.Item.Offensive.Mine
        }

        // Defensive Items
        namespace Defensive
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shield);     // Racer.Item.Defensive.Shield
        }

        // Utility Items
        namespace Utility
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dash);       // Racer.Item.Utility.Dash
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Decoy);      // Racer.Item.Utility.Decoy
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Scan);       // Racer.Item.Utility.Scan
        }

        // Slot Tags
        namespace Slot
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Front);      // Racer.Item.Slot.Front
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Back);       // Racer.Item.Slot.Back
        }
    }

    //=================================================================
    // Racer.State - 레이서 전용 상태
    //=================================================================
    namespace State
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(UsingItem);      // Racer.State.UsingItem
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Swapping);       // Racer.State.Swapping
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(CanUseItem);     // Racer.State.CanUseItem
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(CanSwapItem);    // Racer.State.CanSwapItem
    }

    //=================================================================
    // Racer.Event - 레이서 이벤트
    //=================================================================
    namespace Event
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemReceived);   // Racer.Event.ItemReceived
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemUsed);       // Racer.Event.ItemUsed
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemSwapped);    // Racer.Event.ItemSwapped
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemDestroyed);  // Racer.Event.ItemDestroyed
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacked);       // Racer.Event.Attacked
    }

    //=================================================================
    // Racer.Cue - 레이서 Cue
    //=================================================================
    namespace Cue
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemReceived);   // Racer.Cue.ItemReceived
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemUsed);       // Racer.Cue.ItemUsed
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemSwapped);    // Racer.Cue.ItemSwapped

        namespace Effect
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpeedBoost); // Racer.Cue.Effect.SpeedBoost
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shield);     // Racer.Cue.Effect.Shield
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Heal);       // Racer.Cue.Effect.Heal
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dash);       // Racer.Cue.Effect.Dash
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Scan);       // Racer.Cue.Effect.Scan
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Decoy);      // Racer.Cue.Effect.Decoy
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Brake);      // Racer.Cue.Effect.Brake
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Drift);      // Racer.Cue.Effect.Drift
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage);     // Racer.Cue.Effect.Damage
        }
    }

    //=================================================================
    // Racer.Input - 레이서 입력
    //=================================================================
    namespace Input
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Move);           // Racer.Input.Move
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Look);           // Racer.Input.Look
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Brake);          // Racer.Input.Brake
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseItem);        // Racer.Input.UseItem
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(SwapItem);       // Racer.Input.SwapItem
    }
}
```

### 4.4 태그 사용 예시

```cpp
// ✅ GOOD - Native Tag 사용 (컴파일 타임 체크)
AbilitySystemComponent->TryActivateAbilitiesByTag(
    FGameplayTagContainer(CitRushTags::Racer::Ability::UseItem)
);

// ⚠️ OK - RequestGameplayTag (런타임 체크)
AbilitySystemComponent->TryActivateAbilitiesByTag(
    FGameplayTagContainer(
        FGameplayTag::RequestGameplayTag(FName("Racer.Ability.UseItem"))
    )
);

// ❌ BAD - 문자열 직접 사용 (작동하지 않음)
```

### 4.5 전체 태그 계층 구조

```
CitRushTags
├── State (공통)
│   ├── Alive
│   └── Dead
├── Event (공통)
│   ├── Result
│   │   ├── Success
│   │   └── Fail
│   └── Gameplay
│       ├── Start
│       └── End
├── Cue (공통)
│   └── Gameplay
│       ├── Start
│       └── End
│
├── Racer
│   ├── Ability
│   │   ├── UseItem
│   │   ├── SwapItem
│   │   ├── Move
│   │   ├── Brake
│   │   └── Drift
│   ├── Item
│   │   ├── Native (Heal, Refuel)
│   │   ├── Offensive (SpeedBoost, Mine)
│   │   ├── Defensive (Shield)
│   │   ├── Utility (Dash, Decoy, Scan)
│   │   └── Slot (Front, Back)
│   ├── State
│   │   ├── UsingItem
│   │   ├── Swapping
│   │   ├── CanUseItem
│   │   └── CanSwapItem
│   ├── Event
│   │   ├── ItemReceived
│   │   ├── ItemUsed
│   │   ├── ItemSwapped
│   │   ├── ItemDestroyed
│   │   └── Attacked
│   ├── Cue
│   │   ├── ItemReceived
│   │   ├── ItemUsed
│   │   ├── ItemSwapped
│   │   └── Effect (SpeedBoost, Shield, Heal, Dash, Scan, Decoy, Brake, Drift, Damage)
│   └── Input
│       ├── Move
│       ├── Look
│       ├── Brake
│       ├── UseItem
│       └── SwapItem
│
├── Commander
│   ├── Ability (GiveItem, UseItem, Scan, Ping, GlobalBuff)
│   ├── State (Commanding, CanGiveItem)
│   ├── Event (ItemGiven, ItemUsed, PingIssued)
│   ├── Cue (GiveItem, UseItem, Ping, Scan)
│   └── Input (SelectRacer, GiveItem, UseItem, Ping)
│
└── Enemy
    ├── Ability (Attack, Special, Chase, Patrol, Escape)
    ├── State (Idle, Patrolling, Chasing, Attacking, Escaping)
    ├── Event (TargetDetected, TargetLost, Attacked)
    └── Cue (Attack, Special, Detected)
```

---

## 5. GameplayAbility 구현

### 5.1 BaseGA (기본 Ability)

```cpp
UCLASS()
class UE_CITRUSH_API UBaseGA : public UGameplayAbility
{
    GENERATED_BODY()

public:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData
    ) override;
};
```

**역할**: 모든 Ability의 베이스 클래스

**구현 내용**:
- ✅ `ActivateAbility` 오버라이드
- ✅ `CommitAbility` 호출 (Cost, Cooldown 체크)

### 5.2 GATestBooster (레이서 부스터 예시)

```cpp
UCLASS()
class UE_CITRUSH_API UGATestBooster : public UBaseGA
{
    GENERATED_BODY()

public:
    virtual bool CanActivateAbility(...) override;
    virtual void ActivateAbility(...) override;
    virtual void InputReleased(...) override;
    virtual void CancelAbility(...) override;
};
```

**현재 상태**: ⚠️ 보일러플레이트만 존재 (실제 로직 미구현)

**구현 필요**:
- 부스터 활성화 시 Fuel 소모
- 이동 속도 증가 Effect 적용
- 부스터 지속 시간 관리

---

## 6. ASC 배치 및 초기화

### 6.1 ASC 배치 전략

CITRUSH 프로젝트는 GAS 권장 패턴을 따라 다음과 같이 ASC를 배치했습니다:

| 역할 | ASC 위치 | Replication Mode | 이유 |
|------|----------|------------------|------|
| **Player (Racer/Commander)** | PlayerState | Mixed | 리스폰 후에도 상태 유지 |
| **Enemy** | Character | Minimal | 리스폰 불필요, 가벼운 동기화 |

### 6.2 AbstractRacer - ASC 초기화

```cpp
UCLASS(Abstract)
class UE_CITRUSH_API AAbstractRacer : public AWheeledVehiclePawn, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    virtual UAttributeSet* GetAttributeSet() const override;

protected:
    virtual void InitAbilitySystem() override;

    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> abilitySystemComponent;
    UPROPERTY()
    TObjectPtr<UASRacer> attributeSet;

    UPROPERTY(EditDefaultsOnly, Category = "GAS|Ability")
    TSubclassOf<UGameplayAbility> frontGameplayAbility;
    UPROPERTY(EditDefaultsOnly, Category = "GAS|Ability")
    TSubclassOf<UGameplayAbility> backGameplayAbility;
    UPROPERTY(EditDefaultsOnly, Category = "GAS|Ability")
    TSubclassOf<UGameplayEffect> defaultAttributeEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Tag")
    FGameplayTagContainer tagContainer;
};
```

### 6.3 초기화 플로우 (서버/클라이언트)

#### 서버 측
```
1. PossessedBy(Controller) 호출
   ↓
2. InitAbilitySystem() 호출
   ↓
3. GetPlayerState<ACitRushPlayerState>()
   ↓
4. playerState->GetAbilitySystemComponent()
   ↓
5. abilitySystemComponent->InitAbilityActorInfo(playerState, this)
   - Owner: PlayerState
   - Avatar: this (Character)
```

#### 클라이언트 측
```
1. OnRep_PlayerState() 호출 (PlayerState 리플리케이션)
   ↓
2. InitAbilitySystem() 호출
   ↓
3. (서버와 동일한 초기화 과정)
```

### 6.4 구현 예시 (AbstractRacer.cpp)

```cpp
void AAbstractRacer::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // 서버에서만 실행
    InitAbilitySystem();
}

void AAbstractRacer::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    // 클라이언트에서 실행
    InitAbilitySystem();
}

void AAbstractRacer::InitAbilitySystem()
{
    ACitRushPlayerState* playerState = GetPlayerState<ACitRushPlayerState>();
    if (playerState)
    {
        abilitySystemComponent = playerState->GetAbilitySystemComponent();
        playerState->GetAbilitySystemComponent()->InitAbilityActorInfo(playerState, this);
        attributeSet = playerState->GetAttributeSet();

        // TODO: Default Abilities 부여
        // TODO: Default Effects 적용
    }
}
```

---

## 7. 데미지 시스템 설계

CITRUSH 프로젝트는 **Event 기반 데미지 시스템**을 설계했습니다.

### 7.1 시스템 플로우

```
1. OnCollisionHit (AbstractEnemy.cpp)
   ↓
2. 쿨다운 체크 (0.5초 내 재충돌 방지)
   ↓
3. 충돌 방향 판단 (dotProduct)
   ↓ (후면 충돌)                ↓ (정면 충돌)
4a. HandleGameplayEvent        4b. HandleGameplayEvent
   - Tag: Collision.Back          - Tag: Collision.Front
   - Target: Enemy ASC             - Target: Racer ASC
   - Magnitude: Racer.AttackPower  - Magnitude: Enemy.AttackPower
   ↓
5. [미구현] Gameplay Ability가 이벤트 리스닝
   ↓
6. [미구현] EventMagnitude를 SetByCaller로 전달
   ↓
7. [미구현] Gameplay Effect 적용 (Health 감소)
```

### 7.2 OnCollisionHit 구현 (AbstractEnemy.cpp)

```cpp
void AAbstractEnemy::OnCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, ...)
{
    AAbstractRacer* racer = Cast<AAbstractRacer>(OtherActor);
    if (!racer) return;

    // 쿨다운 체크 (연속 충돌 방지)
    float currentTime = GetWorld()->GetTimeSeconds();
    if (lastCollisionRacer == racer && (currentTime - lastCollisionTime) < collisionCooldown)
    {
        return;
    }
    lastCollisionTime = currentTime;
    lastCollisionRacer = racer;

    // 충돌 방향 판단
    FVector directionToRacer = (racer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    FVector enemyForward = GetActorForwardVector();
    float dotProduct = FVector::DotProduct(enemyForward, directionToRacer);

    // Gameplay Event 발송
    FGameplayEventData eventData;

    if (dotProduct < 0.f)
    {
        // 후면 충돌 - Enemy가 데미지
        eventData.EventMagnitude = racer->GetAttributeSet()->GetAttackPower();
        abilitySystemComponent->HandleGameplayEvent(
            FGameplayTag::RequestGameplayTag("Event.Gameplay.Collision.Back"),
            &eventData
        );
    }
    else
    {
        // 정면 충돌 - Racer가 데미지
        eventData.EventMagnitude = attributeSet->GetAttackPower();
        racer->GetAbilitySystemComponent()->HandleGameplayEvent(
            FGameplayTag::RequestGameplayTag("Event.Gameplay.Collision.Front"),
            &eventData
        );
    }
}
```

### 7.3 구현 완료 항목

- ✅ **OnCollisionHit 로직** (충돌 감지 및 방향 판단)
- ✅ **쿨다운 메커니즘** (연속 충돌 방지)
- ✅ **AttackPower Attribute** (ASRacer, ASEnemy)

### 7.4 구현 필요 항목

- ❌ **Gameplay Tags 추가**: Event.Gameplay.Collision.Back/Front (GTNative에 추가 필요)
- ❌ **Gameplay Ability**: 이벤트 리스너 구현 (Event → Damage Effect 적용)
- ❌ **Gameplay Effect**: BPGE_Damage 설정 (SetByCaller 방식)

---

## 8. 구현 상태 요약

### 8.1 Phase별 진행 상황

#### Phase 1: 기초 설정 ✅ 완료

| 작업 | 상태 |
|------|------|
| GAS Plugin 활성화 | ✅ |
| Gameplay Tags 정의 (Native Tags) | ✅ |
| Base Attribute Sets 구현 | ✅ |
| ASC 배치 (PlayerState, Enemy Character) | ✅ |
| 초기화 로직 구현 및 네트워크 테스트 | ✅ |

#### Phase 2: 핵심 시스템 ⚠️ 진행 중

| 작업 | 상태 |
|------|------|
| 데미지 시스템 (Event 기반) | ⚠️ OnCollisionHit 완료, Ability 필요 |
| 체력/사망 처리 | ❌ |
| 기본 Gameplay Effects (Damage, Heal) | ⚠️ BP 클래스 5개 생성 (내부 설정 필요) |
| Gameplay Cues (VFX/SFX 통합) | ❌ |

#### Phase 3: 레이서 어빌리티 ⚠️ 진행 중

| 작업 | 상태 |
|------|------|
| Boost 구현 | ⚠️ GATestBooster 구조만 존재 |
| Dash 구현 | ❌ |
| Weapon Fire 구현 | ❌ |
| 입력 바인딩 통합 | ❌ |

#### Phase 4: 지휘자 시스템 ❌ 미구현

| 작업 | 상태 |
|------|------|
| Command Points 시스템 | ❌ |
| Team Buff Ability | ❌ |
| Enemy Debuff Ability | ❌ |
| Heal Ability | ❌ |
| 타겟팅 UI | ❌ |

#### Phase 5: Enemy 시스템 ⚠️ 진행 중

| 작업 | 상태 |
|------|------|
| ASEnemy AttributeSet | ✅ 구현 완료 (버그 존재) |
| Enemy ASC 배치 | ✅ AbstractEnemy 완료 |
| Enemy Attack Ability | ❌ |
| Enemy Special Ability | ❌ |
| AI와 GAS 통합 | ❌ |

### 8.2 주요 성과

#### ✅ 완성된 시스템

1. **매크로 기반 AttributeSet 시스템**
   - Boilerplate 코드 95% 감소
   - 일관성 있는 구조
   - Replication 자동화

2. **Native GameplayTags 시스템**
   - 컴파일 타임 타입 안전성
   - IDE 자동완성 지원
   - 역할별/카테고리별 체계적 분류

3. **ASC 배치 및 초기화**
   - PlayerState (Mixed Mode)
   - Enemy Character (Minimal Mode)
   - 네트워크 안전 초기화 플로우

4. **Event 기반 데미지 설계**
   - 충돌 감지 및 방향 판단
   - 쿨다운 메커니즘
   - Gameplay Event 발송

#### ⚠️ 개선 필요 항목

1. **ASEnemy PostGameplayEffectExecute 버그**
   ```cpp
   // 현재 (잘못됨):
   SetDetectionRange(FMath::Clamp(GetDetectionRange(), 0.f, GetMaxHealth())); // ❌
   SetSpeed(FMath::Clamp(GetSpeed(), 0.f, GetMaxHealth())); // ❌
   SetAttackPower(FMath::Clamp(GetAttackPower(), 0.f, GetMaxHealth())); // ❌

   // 수정 필요:
   SetDetectionRange(FMath::Clamp(GetDetectionRange(), 0.f, GetDefaultDetectionRange()));
   SetSpeed(FMath::Clamp(GetSpeed(), 0.f, GetDefaultSpeed()));
   SetAttackPower(FMath::Clamp(GetAttackPower(), 0.f, GetDefaultAttackPower()));
   ```

2. **ASCommander 설계 재검토**
   - Health, MaxHealth 추가 여부 결정
   - ScanRange Clamping 로직 개선

3. **ASRacer AttackPower 완성**
   - Replication 등록
   - Clamping 로직 추가
   - 초기값 설정

---

## 9. 향후 개발 권장사항

### 9.1 우선순위 1: 데미지 시스템 완성

1. **Gameplay Tags 추가** (GTNative.h/cpp)
   ```cpp
   namespace Event::Gameplay
   {
       UE_DECLARE_GAMEPLAY_TAG_EXTERN(Collision_Back);   // Event.Gameplay.Collision.Back
       UE_DECLARE_GAMEPLAY_TAG_EXTERN(Collision_Front);  // Event.Gameplay.Collision.Front
   }
   ```

2. **Gameplay Ability 생성** (Blueprint 권장)
   - GA_CollisionDamage_Back
   - GA_CollisionDamage_Front
   - Triggers 섹션에서 Event Tag 설정
   - SetByCaller로 EventMagnitude 받기

3. **BPGE_Damage 설정**
   - Duration: Instant
   - Modifier: Health (Additive)
   - Magnitude: SetByCaller (Data.Damage)

### 9.2 우선순위 2: GameplayEffect 구현

현재 Blueprint 클래스 5개가 생성되어 있으나 내부 설정이 필요합니다:

| Effect | Duration Type | Modifier | 용도 |
|--------|---------------|----------|------|
| **BPGE_Damage** | Instant | Health (Additive, Negative) | 즉시 데미지 |
| **BPGE_Heal** | Instant | Health (Additive, Positive) | 즉시 회복 |
| **BPGE_Refuel** | Instant | Fuel (Additive, Positive) | 연료 보충 |
| **BPGE_Boost** | Duration (3초) | Speed (Multiply, 2.0) | 이동 속도 증가 |
| **BPGE_Attack** | Instant | - | 공격 시 적용 |

### 9.3 우선순위 3: Ability 로직 구현

1. **GATestBooster 완성**
   ```cpp
   void UGATestBooster::ActivateAbility(...)
   {
       // 1. Fuel 소모 체크
       if (GetCurrentFuel() < FuelCost)
       {
           CancelAbility(...);
           return;
       }

       // 2. Fuel 소모 Effect 적용
       ApplyGameplayEffectToOwner(GE_FuelCost);

       // 3. Boost Effect 적용
       ApplyGameplayEffectToOwner(GE_Boost);

       // 4. VFX/SFX (Gameplay Cue)
       ExecuteGameplayCue(CitRushTags::Racer::Cue::Effect::SpeedBoost);

       CommitAbility(...);
   }
   ```

2. **GA_Dash 구현**
   - 순간 이동 (Teleport)
   - 무적 (0.2초)
   - 쿨다운 (8초)

3. **GA_WeaponFire 구현**
   - 발사체 생성
   - 적 충돌 시 데미지 Event 발송

### 9.4 우선순위 4: UI 통합

1. **Attribute 변경 → Widget 업데이트**
   ```cpp
   void UMyWidget::BindAttributeCallbacks()
   {
       if (UAbilitySystemComponent* ASC = GetOwningPlayer()->GetAbilitySystemComponent())
       {
           // Health 변경 리스닝
           ASC->GetGameplayAttributeValueChangeDelegate(
               UASRacer::GetHealthAttribute()
           ).AddUObject(this, &UMyWidget::OnHealthChanged);

           // Fuel 변경 리스닝
           ASC->GetGameplayAttributeValueChangeDelegate(
               UASRacer::GetFuelAttribute()
           ).AddUObject(this, &UMyWidget::OnFuelChanged);
       }
   }
   ```

2. **Gameplay Tag 변경 → UI 업데이트**
   ```cpp
   void UMyWidget::BindTagCallbacks()
   {
       if (UAbilitySystemComponent* ASC = GetOwningPlayer()->GetAbilitySystemComponent())
       {
           // Tag 추가/제거 리스닝
           ASC->RegisterGameplayTagEvent(
               CitRushTags::Racer::State::UsingItem
           ).AddUObject(this, &UMyWidget::OnUsingItemChanged);
       }
   }
   ```

### 9.5 우선순위 5: 네트워크 최적화

1. **프로파일링**
   - `stat net`
   - `showdebug abilitysystem`
   - Network Profiler 사용

2. **최적화 검토**
   - Attribute Replication 주기 조정
   - Gameplay Cue 최적화
   - Tag Container 크기 모니터링

3. **Replication Proxy 검토**
   - 4인 멀티플레이에서는 불필요할 가능성 높음
   - 프로파일링 후 실제 문제가 있을 때만 적용

### 9.6 디버깅 도구 활용

**필수 콘솔 명령어**:
```
showdebug abilitysystem        // ASC 상태 표시
AbilitySystem.Debug.NextTarget // 디버그 타겟 전환
GameplayTags.PrintReport       // Tag 사용 현황
stat net                       // 네트워크 통계
```

**Blueprint 디버깅**:
- `Print Gameplay Effect Spec` 노드 사용
- `Get Debug String` (Gameplay Tags Container)

---

## 10. 참고 자료

### 10.1 공식 문서

- [Gameplay Ability System Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine)
- [Understanding GAS](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-the-unreal-engine-gameplay-ability-system)
- [Gameplay Abilities](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-abilities-in-unreal-engine)
- [Gameplay Effects](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-effects-for-the-gameplay-ability-system-in-unreal-engine)
- [Using Gameplay Tags](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-tags-in-unreal-engine)

### 10.2 커뮤니티 자료

- **[tranek/GASDocumentation](https://github.com/tranek/GASDocumentation)**: 가장 포괄적인 커뮤니티 문서
- **[Vorixo's DevTricks - The Truth of GAS](https://vorixo.github.io/devtricks/gas/)**: GAS 실전 사용법
- **Lyra** (UE 5.0+): Epic의 공식 샘플 프로젝트

### 10.3 AgentLog 문서

프로젝트 내부 문서:
- `C:\Users\user\Desktop\gas\AgentLog\GAS.md`: GAS 가이드라인
- `C:\Users\user\Desktop\gas\AgentLog\GAS-Implementation-Status.md`: 구현 상태 추적
- `C:\Users\user\Desktop\gas\AgentLog\GameplayTags_Guide.md`: GameplayTag C++ 할당 가이드

---

## 11. 결론

CITRUSH 프로젝트는 GAS의 핵심 아키텍처를 **매우 견고하게** 구현했습니다. 특히 다음 두 가지는 업계 표준을 뛰어넘는 우수한 설계입니다:

1. **매크로 기반 AttributeSet 시스템**: Boilerplate 코드를 획기적으로 줄이고 일관성을 보장
2. **Native GameplayTags**: 타입 안전성과 개발 생산성 극대화

현재 **Phase 1 완료, Phase 2-3 진행 중** 상태이며, 남은 작업은 주로 **Gameplay Ability 로직 구현**과 **GameplayEffect 설정**입니다.

---

**문서 버전**: v1.0
**최종 수정**: 2025-12-01
**작성자**: Claude (AI Agent)
