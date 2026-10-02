#include "UI/MobaHUD.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystem/MobaGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Game/MobaGameState.h"
#include "Player/MobaPlayerController.h"
#include "Player/MobaPlayerState.h"
#include "SocketSubsystem.h"
#include "Structures/MobaStructure.h"
#include "Units/MobaMinion.h"

void AMobaHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	DrawStructureBars();
	DrawMinionBars();
	DrawChampionBars();

	DrawMatchInfo();
	DrawRespawnCountdown();
	DrawWaitingForPlayers();

	if (const AMobaCharacter* LocalCharacter = Cast<AMobaCharacter>(GetOwningPawn()))
	{
		DrawTargetingIndicator(LocalCharacter);
		DrawStatusBar(LocalCharacter);
	}

	DrawMatchResult();
}

// ---------------------------------------------------------------------------------------------------------------------
// 공용 그리기

void AMobaHUD::DrawCenteredText(const FString& Text, const FLinearColor& Color, float CenterX, float Y, UFont* Font, float Scale)
{
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	GetTextSize(Text, TextWidth, TextHeight, Font, Scale);
	DrawText(Text, Color, CenterX - TextWidth * 0.5f, Y, Font, Scale);
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

FLinearColor AMobaHUD::GetRelationColor(EMobaTeam UnitTeam, bool bIsLocalUnit) const
{
	if (bIsLocalUnit)
	{
		return FLinearColor(0.15f, 0.8f, 0.2f);
	}

	const AMobaPlayerState* LocalState = GetOwningPlayerController() ? GetOwningPlayerController()->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (LocalState && LocalState->GetTeam() == UnitTeam)
	{
		return FLinearColor(0.2f, 0.5f, 1.0f);
	}
	return FLinearColor(0.85f, 0.1f, 0.1f);
}

void AMobaHUD::DrawUnitBar(const UAbilitySystemComponent* AbilitySystem, const FVector& WorldLocation, const FVector2D& Size, const FLinearColor& FillColor, bool bShowMana, const FString& Label)
{
	const float MaxHealth = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
	if (MaxHealth <= 0.0f)
	{
		return;
	}

	const FVector ScreenLocation = Project(WorldLocation);
	if (ScreenLocation.Z <= 0.0 || ScreenLocation.X < -Size.X || ScreenLocation.X > Canvas->ClipX + Size.X || ScreenLocation.Y < -50.0 || ScreenLocation.Y > Canvas->ClipY + 50.0)
	{
		return;
	}

	const float HealthFraction = FMath::Clamp(AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetHealthAttribute()) / MaxHealth, 0.0f, 1.0f);
	const float Left = ScreenLocation.X - Size.X * 0.5f;
	const float Top = ScreenLocation.Y - Size.Y;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), Left - 1.0f, Top - 1.0f, Size.X + 2.0f, Size.Y + 2.0f);
	DrawRect(FillColor, Left, Top, Size.X * HealthFraction, Size.Y);

	if (bShowMana)
	{
		const float MaxMana = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxManaAttribute());
		if (MaxMana > 0.0f)
		{
			const float ManaFraction = FMath::Clamp(AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetManaAttribute()) / MaxMana, 0.0f, 1.0f);
			const float ManaHeight = 4.0f;
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), Left - 1.0f, Top + Size.Y + 1.0f, Size.X + 2.0f, ManaHeight + 1.0f);
			DrawRect(FLinearColor(0.2f, 0.45f, 1.0f), Left, Top + Size.Y + 1.0f, Size.X * ManaFraction, ManaHeight);
		}
	}

	if (!Label.IsEmpty())
	{
		DrawText(Label, FLinearColor::White, Left - 18.0f, Top - 3.0f, GEngine->GetSmallFont(), 1.0f);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 머리 위 체력바

void AMobaHUD::DrawChampionBars()
{
	const AMobaCharacter* LocalCharacter = Cast<AMobaCharacter>(GetOwningPawn());

	for (TActorIterator<AMobaCharacter> It(GetWorld()); It; ++It)
	{
		const AMobaCharacter* Character = *It;
		const UAbilitySystemComponent* AbilitySystem = Character->GetAbilitySystemComponent();
		if (!AbilitySystem || Character->IsDead())
		{
			continue;
		}

		const AMobaPlayerState* PlayerState = Character->GetPlayerState<AMobaPlayerState>();
		const FString LevelLabel = PlayerState ? FString::FromInt(PlayerState->GetChampionLevel()) : FString();
		const FVector BarLocation = Character->GetActorLocation() + FVector(0.0, 0.0, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + HealthBarHeightOffset);
		const bool bIsLocal = Character == LocalCharacter;

		DrawUnitBar(AbilitySystem, BarLocation, ChampionBarSize, GetRelationColor(Character->GetTeam(), bIsLocal), bIsLocal, LevelLabel);
	}
}

void AMobaHUD::DrawMinionBars()
{
	for (TActorIterator<AMobaMinion> It(GetWorld()); It; ++It)
	{
		const AMobaMinion* Minion = *It;
		if (Minion->IsHidden())
		{
			continue;
		}

		const FVector BarLocation = Minion->GetActorLocation() + FVector(0.0, 0.0, Minion->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 30.0f);
		DrawUnitBar(Minion->GetAbilitySystemComponent(), BarLocation, MinionBarSize, GetRelationColor(Minion->GetTeam(), false), false, FString());
	}
}

void AMobaHUD::DrawStructureBars()
{
	for (TActorIterator<AMobaStructure> It(GetWorld()); It; ++It)
	{
		const AMobaStructure* Structure = *It;
		if (Structure->IsDestroyed())
		{
			continue;
		}

		// 아직 공격할 수 없는 건물은 회색으로 표시한다.
		const FLinearColor FillColor = Structure->IsInvulnerable() ? FLinearColor(0.45f, 0.45f, 0.45f) : GetRelationColor(Structure->GetTeam(), false);
		const FVector BarLocation = Structure->GetActorLocation() + FVector(0.0, 0.0, Structure->GetHealthBarHeight());
		DrawUnitBar(Structure->GetAbilitySystemComponent(), BarLocation, StructureBarSize, FillColor, false, FString());
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 경기 정보

void AMobaHUD::DrawMatchInfo()
{
	const AMobaGameState* GameState = GetWorld()->GetGameState<AMobaGameState>();
	const AMobaPlayerState* PlayerState = GetOwningPlayerController() ? GetOwningPlayerController()->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (!GameState || !PlayerState)
	{
		return;
	}

	const int32 TotalSeconds = FMath::FloorToInt(GameState->GetMatchTimeSeconds());
	const FString Text = FString::Printf(TEXT("%02d:%02d    K %d / D %d"), TotalSeconds / 60, TotalSeconds % 60, PlayerState->GetKills(), PlayerState->GetDeaths());
	DrawCenteredText(Text, FLinearColor::White, Canvas->ClipX * 0.5f, 12.0f, GEngine->GetSmallFont(), 1.3f);
}

void AMobaHUD::DrawRespawnCountdown()
{
	const AMobaPlayerState* PlayerState = GetOwningPlayerController() ? GetOwningPlayerController()->GetPlayerState<AMobaPlayerState>() : nullptr;
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!PlayerState || !GameState || PlayerState->GetRespawnServerTime() <= 0.0f)
	{
		return;
	}

	const float Remaining = PlayerState->GetRespawnServerTime() - GameState->GetServerWorldTimeSeconds();
	if (Remaining > 0.0f)
	{
		DrawCenteredText(FString::Printf(TEXT("부활까지 %d초"), FMath::CeilToInt(Remaining)), FLinearColor::White, Canvas->ClipX * 0.5f, Canvas->ClipY * 0.3f, GEngine->GetLargeFont(), 2.0f);
	}
}

void AMobaHUD::DrawWaitingForPlayers()
{
	const AMobaGameState* GameState = GetWorld()->GetGameState<AMobaGameState>();
	if (!GameState || GameState->HasMatchStarted())
	{
		return;
	}

	const float CenterX = Canvas->ClipX * 0.5f;
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f), 0.0f, Canvas->ClipY * 0.16f, Canvas->ClipX, Canvas->ClipY * 0.14f);
	DrawCenteredText(TEXT("상대를 기다리는 중..."), FLinearColor::White, CenterX, Canvas->ClipY * 0.18f, GEngine->GetLargeFont(), 2.0f);

	if (GetNetMode() != NM_ListenServer)
	{
		return;
	}

	if (!bLocalAddressResolved)
	{
		bLocalAddressResolved = true;
		if (ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
		{
			bool bCanBindAll = false;
			LocalAddress = SocketSubsystem->GetLocalHostAddr(*GLog, bCanBindAll)->ToString(false);
		}
	}

	// 랜 어댑터가 여러 개면 다른 주소가 잡힐 수 있으므로 참고용이다.
	const FString Hint = LocalAddress.IsEmpty()
		? FString(TEXT("상대에게 이 PC의 IP 주소를 알려 주세요 (UDP 7777)"))
		: FString::Printf(TEXT("상대에게 알려 줄 주소: %s (UDP 7777)"), *LocalAddress);
	DrawCenteredText(Hint, FLinearColor(1.0f, 0.85f, 0.4f), CenterX, Canvas->ClipY * 0.25f, GEngine->GetSmallFont(), 1.3f);
}

void AMobaHUD::DrawMatchResult()
{
	const AMobaGameState* GameState = GetWorld()->GetGameState<AMobaGameState>();
	const AMobaPlayerState* PlayerState = GetOwningPlayerController() ? GetOwningPlayerController()->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (!GameState || !PlayerState || !GameState->IsMatchOver())
	{
		return;
	}

	const bool bWon = GameState->GetWinningTeam() == PlayerState->GetTeam();
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f), 0.0f, Canvas->ClipY * 0.35f, Canvas->ClipX, Canvas->ClipY * 0.2f);
	DrawCenteredText(bWon ? TEXT("승리") : TEXT("패배"), bWon ? FLinearColor(0.3f, 0.7f, 1.0f) : FLinearColor(1.0f, 0.3f, 0.25f), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.38f, GEngine->GetLargeFont(), 4.0f);

	const FString Detail = GameState->WasEndedByForfeit() ? TEXT("상대가 게임을 떠났습니다.    Enter: 메뉴로 돌아가기") : TEXT("Enter: 메뉴로 돌아가기");
	DrawCenteredText(Detail, FLinearColor::White, Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f, GEngine->GetSmallFont(), 1.3f);
}

// ---------------------------------------------------------------------------------------------------------------------
// 조준 표시

void AMobaHUD::DrawTargetingIndicator(const AMobaCharacter* Character)
{
	const AMobaPlayerController* MobaController = Cast<AMobaPlayerController>(GetOwningPlayerController());
	if (!MobaController)
	{
		return;
	}

	// A키로 공격 대상을 고르는 중이면 기본 공격 사거리를 빨간 원으로 보여 준다.
	if (MobaController->IsAttackTargeting())
	{
		FVector Feet = Character->GetActorLocation();
		Feet.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		DrawGroundCircle(Feet, Character->GetCombatComponent()->GetAttackRange(), FLinearColor(1.0f, 0.25f, 0.2f, 0.9f), 2.0f);
		return;
	}

	EMobaAbilitySlot Slot;
	if (!MobaController->GetTargetingSlot(Slot))
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

// ---------------------------------------------------------------------------------------------------------------------
// 하단 상태 바

void AMobaHUD::DrawStatusBar(const AMobaCharacter* Character)
{
	const UAbilitySystemComponent* AbilitySystem = Character->GetAbilitySystemComponent();
	const AMobaPlayerState* PlayerState = Character->GetPlayerState<AMobaPlayerState>();
	if (!AbilitySystem || !PlayerState)
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

	// 레벨, 경험치, 골드.
	const float InfoTop = BoxTop - 44.0f;
	const float NeededExperience = AMobaPlayerState::GetExperienceForNextLevel(PlayerState->GetChampionLevel());
	const float ExperienceFraction = PlayerState->GetChampionLevel() >= AMobaPlayerState::MaxChampionLevel ? 1.0f : FMath::Clamp(PlayerState->GetExperience() / NeededExperience, 0.0f, 1.0f);

	DrawText(FString::Printf(TEXT("Lv %d"), PlayerState->GetChampionLevel()), FLinearColor::White, Left, InfoTop, Font, 1.2f);
	DrawText(FString::Printf(TEXT("%d G"), FMath::FloorToInt(PlayerState->GetGold())), FLinearColor(1.0f, 0.85f, 0.3f), Left + TotalWidth - 70.0f, InfoTop, Font, 1.2f);
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), Left - 1.0f, InfoTop + 20.0f, TotalWidth + 2.0f, 6.0f);
	DrawRect(FLinearColor(0.6f, 0.3f, 0.9f), Left, InfoTop + 21.0f, TotalWidth * ExperienceFraction, 4.0f);

	// QWER 스킬 칸.
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const UMobaGameplayAbility* Ability = Character->GetAbilityInSlot(Slots[Index]);
		const float BoxLeft = Left + (AbilityBoxSize + Gap) * Index;

		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), BoxLeft - 2.0f, BoxTop - 2.0f, AbilityBoxSize + 4.0f, AbilityBoxSize + 4.0f);
		if (!Ability)
		{
			continue;
		}

		const int32 AbilityLevel = Character->GetAbilityLevel(Slots[Index]);
		const bool bEnoughMana = Mana >= Ability->GetManaCost();

		// 아직 찍지 않은 스킬은 회색, 마나가 모자라면 어두운 파랑.
		FLinearColor BoxColor(0.15f, 0.3f, 0.45f);
		if (AbilityLevel <= 0)
		{
			BoxColor = FLinearColor(0.12f, 0.12f, 0.12f);
		}
		else if (!bEnoughMana)
		{
			BoxColor = FLinearColor(0.12f, 0.12f, 0.3f);
		}
		DrawRect(BoxColor, BoxLeft, BoxTop, AbilityBoxSize, AbilityBoxSize);

		float Remaining = 0.0f;
		float Duration = 0.0f;
		if (Character->GetCooldownRemaining(Slots[Index], Remaining, Duration) && Duration > 0.0f)
		{
			// 남은 대기 시간만큼 위에서부터 어둡게 덮는다.
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f), BoxLeft, BoxTop, AbilityBoxSize, AbilityBoxSize * FMath::Clamp(Remaining / Duration, 0.0f, 1.0f));
			DrawText(FString::Printf(TEXT("%.1f"), Remaining), FLinearColor::White, BoxLeft + AbilityBoxSize * 0.3f, BoxTop + AbilityBoxSize * 0.35f, Font, 1.4f);
		}

		DrawText(SlotKeys[Index], FLinearColor::White, BoxLeft + 4.0f, BoxTop + 2.0f, Font, 1.2f);
		DrawText(FString::Printf(TEXT("%d/%d"), AbilityLevel, Ability->GetMaxLevel()), FLinearColor(0.85f, 0.85f, 0.85f), BoxLeft + AbilityBoxSize - 26.0f, BoxTop + 2.0f, Font, 1.0f);
		DrawText(FString::FromInt(FMath::RoundToInt(Ability->GetManaCost())), FLinearColor(0.5f, 0.75f, 1.0f), BoxLeft + 4.0f, BoxTop + AbilityBoxSize - 16.0f, Font, 1.0f);

		// 스킬 포인트로 올릴 수 있으면 누를 키를 알려 준다.
		if (Character->CanLevelUpAbility(Slots[Index]))
		{
			DrawRect(FLinearColor(0.9f, 0.75f, 0.1f), BoxLeft, BoxTop - 20.0f, AbilityBoxSize, 16.0f);
			DrawCenteredText(FString::Printf(TEXT("Ctrl+%s"), SlotKeys[Index]), FLinearColor::Black, BoxLeft + AbilityBoxSize * 0.5f, BoxTop - 20.0f, Font, 1.0f);
		}
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
