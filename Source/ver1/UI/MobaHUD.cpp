#include "UI/MobaHUD.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Player/MobaPlayerState.h"

void AMobaHUD::DrawHUD()
{
	Super::DrawHUD();

	for (TActorIterator<AMobaCharacter> It(GetWorld()); It; ++It)
	{
		DrawHealthBar(*It);
	}

	DrawRespawnCountdown();
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
