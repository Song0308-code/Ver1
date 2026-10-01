#include "UI/MobaHUD.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystem/MobaGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Player/MobaPlayerController.h"
#include "Player/MobaPlayerState.h"

void AMobaHUD::DrawHUD()
{
	Super::DrawHUD();

	for (TActorIterator<AMobaCharacter> It(GetWorld()); It; ++It)
	{
		DrawHealthBar(*It);
	}

	DrawRespawnCountdown();

	if (const AMobaCharacter* LocalCharacter = Cast<AMobaCharacter>(GetOwningPawn()))
	{
		DrawTargetingIndicator(LocalCharacter);
		DrawStatusBar(LocalCharacter);
	}
}

void AMobaHUD::DrawGroundLine(const FVector& Start, const FVector& End, const FLinearColor& Color, float Thickness)
{
	const FVector ScreenStart = Project(Start);
	const FVector ScreenEnd = Project(End);
	if (ScreenStart.Z > 0.0 && ScreenEnd.Z > 0.0)
	{
		DrawLine(ScreenStart.X, ScreenStart.Y, ScreenEnd.X, ScreenEnd.Y, Color, Thickness);
	}
}

void AMobaHUD::DrawGroundCircle(const FVector& Center, float Radius, const FLinearColor& Color, float Thickness)
{
	const int32 NumSegments = 48;
	FVector Previous = Center + FVector(Radius, 0.0, 0.0);
	for (int32 Index = 1; Index <= NumSegments; ++Index)
	{
		const float Angle = 2.0f * UE_PI * Index / NumSegments;
		const FVector Current = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * Radius;
		DrawGroundLine(Previous, Current, Color, Thickness);
		Previous = Current;
	}
}

void AMobaHUD::DrawTargetingIndicator(const AMobaCharacter* Character)
{
	const AMobaPlayerController* MobaController = Cast<AMobaPlayerController>(GetOwningPlayerController());
	EMobaAbilitySlot Slot;
	if (!MobaController || !MobaController->GetTargetingSlot(Slot))
	{
		return;
	}

	const UMobaGameplayAbility* Ability = Character->GetAbilityInSlot(Slot);
	FVector CursorLocation;
	if (!Ability || !MobaController->GetCursorGroundLocation(CursorLocation))
	{
		return;
	}

	const FLinearColor RangeColor(0.3f, 0.8f, 1.0f, 0.8f);
	const FLinearColor AimColor(1.0f, 0.9f, 0.3f, 0.9f);

	FVector Origin = Character->GetActorLocation();
	Origin.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FVector Offset = CursorLocation - Origin;
	Offset.Z = 0.0;
	const float Range = Ability->GetCastRange();

	DrawGroundCircle(Origin, Range, RangeColor, 2.0f);

	if (Ability->GetTargetingMode() == EMobaTargetingMode::Direction)
	{
		const FVector Direction = Offset.GetSafeNormal();
		const FVector Side = FVector(-Direction.Y, Direction.X, 0.0) * Ability->GetIndicatorRadius();
		const FVector End = Origin + Direction * Range;
		DrawGroundLine(Origin + Side, End + Side, AimColor, 2.0f);
		DrawGroundLine(Origin - Side, End - Side, AimColor, 2.0f);
		DrawGroundLine(End + Side, End - Side, AimColor, 2.0f);
	}
	else if (Ability->GetTargetingMode() == EMobaTargetingMode::Location)
	{
		const FVector Target = Origin + Offset.GetClampedToMaxSize(Range);
		DrawGroundCircle(Target, FMath::Max(Ability->GetIndicatorRadius(), 40.0f), AimColor, 2.0f);
	}
}

void AMobaHUD::DrawStatusBar(const AMobaCharacter* Character)
{
	const UAbilitySystemComponent* AbilitySystem = Character->GetAbilitySystemComponent();
	if (!AbilitySystem || !Canvas)
	{
		return;
	}

	UFont* Font = GEngine->GetSmallFont();
	const float Health = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetHealthAttribute());
	const float MaxHealth = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
	const float Mana = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetManaAttribute());
	const float MaxMana = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxManaAttribute());

	const EMobaAbilitySlot Slots[] = { EMobaAbilitySlot::Q, EMobaAbilitySlot::W, EMobaAbilitySlot::E, EMobaAbilitySlot::R };
	const TCHAR* SlotKeys[] = { TEXT("Q"), TEXT("W"), TEXT("E"), TEXT("R") };
	const float Gap = 8.0f;
	const float TotalWidth = AbilityBoxSize * 4.0f + Gap * 3.0f;
	const float Left = (Canvas->ClipX - TotalWidth) * 0.5f;
	const float BarHeight = 16.0f;
	const float BoxTop = Canvas->ClipY - AbilityBoxSize - BarHeight * 2.0f - 28.0f;

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const UMobaGameplayAbility* Ability = Character->GetAbilityInSlot(Slots[Index]);
		const float BoxLeft = Left + (AbilityBoxSize + Gap) * Index;

		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), BoxLeft - 2.0f, BoxTop - 2.0f, AbilityBoxSize + 4.0f, AbilityBoxSize + 4.0f);
		if (!Ability)
		{
			continue;
		}

		const bool bEnoughMana = Mana >= Ability->GetManaCost();
		DrawRect(bEnoughMana ? FLinearColor(0.15f, 0.3f, 0.45f) : FLinearColor(0.12f, 0.12f, 0.3f), BoxLeft, BoxTop, AbilityBoxSize, AbilityBoxSize);

		float Remaining = 0.0f;
		float Duration = 0.0f;
		if (Character->GetCooldownRemaining(Slots[Index], Remaining, Duration) && Duration > 0.0f)
		{
			// 남은 대기 시간만큼 위에서부터 어둡게 덮는다.
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f), BoxLeft, BoxTop, AbilityBoxSize, AbilityBoxSize * FMath::Clamp(Remaining / Duration, 0.0f, 1.0f));
			DrawText(FString::Printf(TEXT("%.1f"), Remaining), FLinearColor::White, BoxLeft + AbilityBoxSize * 0.3f, BoxTop + AbilityBoxSize * 0.35f, Font, 1.4f);
		}

		DrawText(SlotKeys[Index], FLinearColor::White, BoxLeft + 4.0f, BoxTop + 2.0f, Font, 1.2f);
		DrawText(FString::FromInt(FMath::RoundToInt(Ability->GetManaCost())), FLinearColor(0.5f, 0.75f, 1.0f), BoxLeft + 4.0f, BoxTop + AbilityBoxSize - 16.0f, Font, 1.0f);
	}

	// 체력·마나 수치 바.
	const float BarsTop = BoxTop + AbilityBoxSize + 8.0f;
	const float HealthFraction = MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f;
	const float ManaFraction = MaxMana > 0.0f ? FMath::Clamp(Mana / MaxMana, 0.0f, 1.0f) : 0.0f;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), Left - 2.0f, BarsTop - 2.0f, TotalWidth + 4.0f, BarHeight * 2.0f + 6.0f);
	DrawRect(FLinearColor(0.15f, 0.7f, 0.2f), Left, BarsTop, TotalWidth * HealthFraction, BarHeight);
	DrawRect(FLinearColor(0.2f, 0.45f, 1.0f), Left, BarsTop + BarHeight + 2.0f, TotalWidth * ManaFraction, BarHeight);
	DrawText(FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::RoundToInt(MaxHealth)), FLinearColor::White, Left + 6.0f, BarsTop + 1.0f, Font, 1.0f);
	DrawText(FString::Printf(TEXT("%d / %d"), FMath::FloorToInt(Mana), FMath::RoundToInt(MaxMana)), FLinearColor::White, Left + 6.0f, BarsTop + BarHeight + 3.0f, Font, 1.0f);
}

void AMobaHUD::DrawHealthBar(const AMobaCharacter* Character)
{
	const UAbilitySystemComponent* AbilitySystem = Character->GetAbilitySystemComponent();
	if (!AbilitySystem || Character->IsDead())
	{
		return;
	}

	const float MaxHealth = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
	if (MaxHealth <= 0.0f)
	{
		return;
	}

	const FVector WorldLocation = Character->GetActorLocation() + FVector(0.0, 0.0, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + HealthBarHeightOffset);
	const FVector ScreenLocation = Project(WorldLocation);
	if (ScreenLocation.Z <= 0.0)
	{
		return;
	}

	const APawn* LocalPawn = GetOwningPawn();
	const AMobaCharacter* LocalCharacter = Cast<AMobaCharacter>(LocalPawn);

	// 내 챔피언은 초록, 아군은 파랑, 적은 빨강.
	FLinearColor FillColor = FLinearColor(0.85f, 0.1f, 0.1f);
	if (Character == LocalCharacter)
	{
		FillColor = FLinearColor(0.15f, 0.8f, 0.2f);
	}
	else if (LocalCharacter && LocalCharacter->GetTeam() == Character->GetTeam())
	{
		FillColor = FLinearColor(0.2f, 0.5f, 1.0f);
	}

	const float HealthFraction = FMath::Clamp(AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetHealthAttribute()) / MaxHealth, 0.0f, 1.0f);
	const float Left = ScreenLocation.X - HealthBarSize.X * 0.5f;
	const float Top = ScreenLocation.Y - HealthBarSize.Y;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), Left - 1.0f, Top - 1.0f, HealthBarSize.X + 2.0f, HealthBarSize.Y + 2.0f);
	DrawRect(FillColor, Left, Top, HealthBarSize.X * HealthFraction, HealthBarSize.Y);

	if (Character == LocalCharacter)
	{
		const float MaxMana = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxManaAttribute());
		if (MaxMana > 0.0f)
		{
			const float ManaFraction = FMath::Clamp(AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetManaAttribute()) / MaxMana, 0.0f, 1.0f);
			const float ManaHeight = 4.0f;
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), Left - 1.0f, Top + HealthBarSize.Y + 1.0f, HealthBarSize.X + 2.0f, ManaHeight + 1.0f);
			DrawRect(FLinearColor(0.2f, 0.45f, 1.0f), Left, Top + HealthBarSize.Y + 1.0f, HealthBarSize.X * ManaFraction, ManaHeight);
		}
	}
}

void AMobaHUD::DrawRespawnCountdown()
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	const AMobaPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<AMobaPlayerState>() : nullptr;
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!PlayerState || !GameState || !Canvas || PlayerState->GetRespawnServerTime() <= 0.0f)
	{
		return;
	}

	const float Remaining = PlayerState->GetRespawnServerTime() - GameState->GetServerWorldTimeSeconds();
	if (Remaining <= 0.0f)
	{
		return;
	}

	const FString Text = FString::Printf(TEXT("부활까지 %d초"), FMath::CeilToInt(Remaining));
	UFont* Font = GEngine->GetLargeFont();
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	GetTextSize(Text, TextWidth, TextHeight, Font, 2.0f);
	DrawText(Text, FLinearColor::White, (Canvas->ClipX - TextWidth) * 0.5f, Canvas->ClipY * 0.3f, Font, 2.0f);
}
