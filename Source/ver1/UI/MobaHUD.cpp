#include "UI/MobaHUD.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystem/MobaGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatLibrary.h"
#include "Combat/MobaCombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SplineComponent.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "TextureResource.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Game/MobaGameState.h"
#include "Game/MobaDisplaySettings.h"
#include "Game/MobaGameMode.h"
#include "Game/MobaGameplayTags.h"
#include "Character/MobaChampionData.h"
#include "Character/MobaPassive.h"
#include "AbilitySystem/Abilities/MobaSummonerSpells.h"
#include "Game/MobaItems.h"
#include "Map/MobaJungleCamp.h"
#include "Map/MobaLanePath.h"
#include "Player/MobaPlayerController.h"
#include "Player/MobaPlayerState.h"
#include "SocketSubsystem.h"
#include "Structures/MobaStructure.h"
#include "Units/MobaMinion.h"
#include "Units/MobaMonster.h"
#include "Vision/MobaVisionSubsystem.h"
#include "Vision/MobaWard.h"

void AMobaHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	LastCanvasSize = FVector2D(Canvas->ClipX, Canvas->ClipY);

	DrawFogOfWar();
	UpdateDamageNumbers();

	DrawStructureBars();
	DrawMonsterBars();
	DrawMinionBars();
	DrawChampionBars();
	DrawWorldTimers();
	DrawDamageNumbers();

	DrawMatchInfo();
	DrawRespawnCountdown();
	DrawWaitingForPlayers();

	// 경기가 끝나면 결과 화면만 보이도록 조작용 UI는 그리지 않는다.
	const AMobaGameState* MobaGameState = GetWorld()->GetGameState<AMobaGameState>();
	const bool bMatchOver = MobaGameState && MobaGameState->IsMatchOver();

	const AMobaCharacter* LocalCharacter = bMatchOver ? nullptr : Cast<AMobaCharacter>(GetOwningPawn());
	if (LocalCharacter)
	{
		DrawTargetingIndicator(LocalCharacter);
		DrawStatusBar(LocalCharacter);
		DrawBuffsAndWard(LocalCharacter);
		DrawInventory(LocalCharacter);
		DrawStatsPanel(LocalCharacter);
		DrawRecall(LocalCharacter);
	}

	if (!bMatchOver)
	{
		DrawMinimap();
	}
	else
	{
		bMinimapDrawn = false;
	}

	// 미니맵 위의 핑 표시가 미니맵 배경에 덮이지 않도록 미니맵 다음에 그린다.
	DrawPings();
	DrawAnnouncements();

	bShopDrawn = false;
	if (const AMobaCharacter* ShopCharacter = Cast<AMobaCharacter>(GetOwningPawn()))
	{
		DrawShop(ShopCharacter);
	}

	DrawScoreboard();

	// 툴팁은 미니맵·점수판보다 위에 보이도록 늦게 그린다.
	if (LocalCharacter)
	{
		DrawAbilityTooltip(LocalCharacter);
	}

	DrawGameMenu();

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

bool AMobaHUD::IsSeen(const AActor* Actor) const
{
	const UMobaVisionSubsystem* Vision = UMobaVisionSubsystem::Get(this);
	return !Vision || Vision->IsVisibleToLocalPlayer(Actor);
}

FLinearColor AMobaHUD::GetRelationColor(EMobaTeam UnitTeam, bool bIsLocalUnit) const
{
	if (bIsLocalUnit)
	{
		return FLinearColor(0.15f, 0.8f, 0.2f);
	}

	if (UnitTeam == EMobaTeam::Neutral)
	{
		return FLinearColor(0.95f, 0.6f, 0.05f);
	}

	const AMobaPlayerState* LocalState = GetOwningPlayerController() ? GetOwningPlayerController()->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (LocalState && LocalState->GetTeam() == UnitTeam)
	{
		return FLinearColor(0.2f, 0.5f, 1.0f);
	}
	return FLinearColor(0.85f, 0.1f, 0.1f);
}

void AMobaHUD::DrawUnitBar(const UAbilitySystemComponent* AbilitySystem, const FVector& WorldLocation, const FVector2D& Size, const FLinearColor& FillColor, bool bShowMana, const FString& Label, int32 Segments)
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

	// 보호막은 체력 뒤에 흰색으로 잇는다. 체력+보호막이 최대 체력을 넘으면 그 합을 막대 전체로 본다.
	const float Health = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetHealthAttribute());
	const float Shield = FMath::Max(AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetShieldAttribute()), 0.0f);
	const float BarTotal = FMath::Max(MaxHealth, Health + Shield);
	const float HealthFraction = FMath::Clamp(Health / BarTotal, 0.0f, 1.0f);
	const float ShieldFraction = FMath::Clamp(Shield / BarTotal, 0.0f, 1.0f - HealthFraction);
	const float Left = ScreenLocation.X - Size.X * 0.5f;
	const float Top = ScreenLocation.Y - Size.Y;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), Left - 1.0f, Top - 1.0f, Size.X + 2.0f, Size.Y + 2.0f);
	DrawRect(FillColor, Left, Top, Size.X * HealthFraction, Size.Y);
	if (ShieldFraction > 0.0f)
	{
		DrawRect(FLinearColor(0.92f, 0.92f, 0.92f), Left + Size.X * HealthFraction, Top, Size.X * ShieldFraction, Size.Y);
	}

	for (int32 Segment = 1; Segment < Segments; ++Segment)
	{
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f), Left + Size.X * Segment / Segments - 1.0f, Top, 2.0f, Size.Y);
	}

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
		if (!AbilitySystem || Character->IsDead() || !IsSeen(Character))
		{
			continue;
		}

		const AMobaPlayerState* PlayerState = Character->GetPlayerState<AMobaPlayerState>();
		const FString LevelLabel = PlayerState ? FString::FromInt(PlayerState->GetChampionLevel()) : FString();
		const FVector BarLocation = Character->GetActorLocation() + FVector(0.0, 0.0, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + HealthBarHeightOffset);
		const bool bIsLocal = Character == LocalCharacter;

		DrawUnitBar(AbilitySystem, BarLocation, ChampionBarSize, GetRelationColor(Character->GetTeam(), bIsLocal), bIsLocal, LevelLabel);

		// 걸린 군중 제어를 체력바 위에 적는다. 여러 개면 가장 강한 것 하나만.
		const TCHAR* Status = nullptr;
		if (AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Airborne))
		{
			Status = TEXT("공중에 뜸");
		}
		else if (AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Stunned))
		{
			Status = TEXT("기절");
		}
		else if (AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Rooted))
		{
			Status = TEXT("속박");
		}
		else if (AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Silenced))
		{
			Status = TEXT("침묵");
		}
		if (Status)
		{
			const FVector ScreenLocation = Project(BarLocation);
			if (ScreenLocation.Z > 0.0)
			{
				DrawCenteredText(Status, FLinearColor(1.0f, 0.85f, 0.3f), ScreenLocation.X, ScreenLocation.Y - ChampionBarSize.Y - 20.0f, GEngine->GetSmallFont(), 1.1f);
			}
		}
	}
}

void AMobaHUD::DrawMinionBars()
{
	for (TActorIterator<AMobaMinion> It(GetWorld()); It; ++It)
	{
		const AMobaMinion* Minion = *It;
		if (Minion->IsHidden() || MobaCombat::IsDead(Minion) || !IsSeen(Minion))
		{
			continue;
		}

		const FVector BarLocation = Minion->GetActorLocation() + FVector(0.0, 0.0, Minion->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 30.0f);
		DrawUnitBar(Minion->GetAbilitySystemComponent(), BarLocation, MinionBarSize, GetRelationColor(Minion->GetTeam(), false), false, FString());
	}
}

void AMobaHUD::DrawMonsterBars()
{
	for (TActorIterator<AMobaMonster> It(GetWorld()); It; ++It)
	{
		const AMobaMonster* Monster = *It;
		if (Monster->IsHidden() || !IsSeen(Monster))
		{
			continue;
		}

		const FVector BarLocation = Monster->GetActorLocation() + FVector(0.0, 0.0, Monster->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 35.0f);
		DrawUnitBar(Monster->GetAbilitySystemComponent(), BarLocation, FVector2D(80.0, 8.0), GetRelationColor(EMobaTeam::Neutral, false), false, FString());
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
		// 방패가 남은 외곽 포탑은 막대를 방패 수만큼 나눠 보여 준다.
		const AMobaTower* Tower = Cast<AMobaTower>(Structure);
		const int32 Segments = Tower && Tower->GetPlatesRemaining() > 0 ? AMobaTower::TotalPlates : 0;
		DrawUnitBar(Structure->GetAbilitySystemComponent(), BarLocation, StructureBarSize, FillColor, false, FString(), Segments);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 재생성 시간

void AMobaHUD::DrawWorldTimers()
{
	const auto DrawTimer = [this](const FVector& WorldLocation, float Seconds, const FLinearColor& Color)
	{
		const FVector Screen = Project(WorldLocation);
		if (Seconds <= 0.0f || Screen.Z <= 0.0 || Screen.X < 0.0 || Screen.X > Canvas->ClipX || Screen.Y < 0.0 || Screen.Y > Canvas->ClipY)
		{
			return;
		}

		const int32 Total = FMath::CeilToInt(Seconds);
		DrawCenteredText(FString::Printf(TEXT("%d:%02d"), Total / 60, Total % 60), Color, Screen.X, Screen.Y - 8.0f, GEngine->GetSmallFont(), 1.3f);
	};

	for (TActorIterator<AMobaJungleCamp> It(GetWorld()); It; ++It)
	{
		DrawTimer(It->GetActorLocation(), It->GetSecondsUntilSpawn(), FLinearColor(1.0f, 0.8f, 0.3f));
	}

	for (TActorIterator<AMobaInhibitor> It(GetWorld()); It; ++It)
	{
		DrawTimer(It->GetActorLocation() + FVector(0.0, 0.0, 100.0), It->GetSecondsUntilRespawn(), FLinearColor::White);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 미니맵

FVector2D AMobaHUD::WorldToMinimap(const FVector& WorldLocation) const
{
	const float U = FMath::Clamp(WorldLocation.Y / MapWorldSize, 0.0, 1.0);
	const float V = 1.0f - FMath::Clamp(WorldLocation.X / MapWorldSize, 0.0, 1.0);
	return MinimapOrigin + FVector2D(U, V) * MinimapSize;
}

bool AMobaHUD::GetMinimapWorldLocation(const FVector2D& ScreenPosition, FVector& OutWorldLocation) const
{
	const FVector2D Local = ScreenPosition - MinimapOrigin;
	if (!bMinimapDrawn || Local.X < 0.0 || Local.Y < 0.0 || Local.X > MinimapSize || Local.Y > MinimapSize)
	{
		return false;
	}

	OutWorldLocation = FVector((1.0 - Local.Y / MinimapSize) * MapWorldSize, Local.X / MinimapSize * MapWorldSize, 0.0);
	return true;
}

void AMobaHUD::DrawMinimapDot(const FVector& WorldLocation, float Size, const FLinearColor& Color)
{
	const FVector2D Center = WorldToMinimap(WorldLocation);
	DrawRect(Color, Center.X - Size * 0.5f, Center.Y - Size * 0.5f, Size, Size);
}

void AMobaHUD::DrawMinimap()
{
	// 로비 화면이 떠 있는 동안에는 그리지 않는다.
	const AMobaPlayerState* LocalState = GetOwningPlayerController() ? GetOwningPlayerController()->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (bLobbyDrawn || !LocalState || LocalState->GetTeam() == EMobaTeam::None)
	{
		bMinimapDrawn = false;
		return;
	}

	MinimapOrigin = FVector2D(Canvas->ClipX - MinimapSize - MinimapMargin, Canvas->ClipY - MinimapSize - MinimapMargin);
	bMinimapDrawn = true;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f), MinimapOrigin.X - 2.0f, MinimapOrigin.Y - 2.0f, MinimapSize + 4.0f, MinimapSize + 4.0f);
	DrawRect(FLinearColor(0.06f, 0.1f, 0.07f, 0.9f), MinimapOrigin.X, MinimapOrigin.Y, MinimapSize, MinimapSize);

	DrawMinimapFog();

	// 라인.
	for (TActorIterator<AMobaLanePath> It(GetWorld()); It; ++It)
	{
		const USplineComponent* Spline = It->GetSpline();
		const float Length = Spline->GetSplineLength();
		const int32 NumSegments = 24;
		FVector2D Previous = WorldToMinimap(Spline->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World));
		for (int32 Index = 1; Index <= NumSegments; ++Index)
		{
			const FVector2D Current = WorldToMinimap(Spline->GetLocationAtDistanceAlongSpline(Length * Index / NumSegments, ESplineCoordinateSpace::World));
			DrawLine(Previous.X, Previous.Y, Current.X, Current.Y, FLinearColor(0.35f, 0.33f, 0.25f), 3.0f);
			Previous = Current;
		}
	}

	// 살아 있는 정글 캠프.
	for (TActorIterator<AMobaJungleCamp> It(GetWorld()); It; ++It)
	{
		if (It->IsAlive())
		{
			DrawMinimapDot(It->GetActorLocation(), 4.0f, GetRelationColor(EMobaTeam::Neutral, false));
		}
	}

	for (TActorIterator<AMobaStructure> It(GetWorld()); It; ++It)
	{
		if (!It->IsDestroyed())
		{
			const float Size = It->GetStructureType() == EMobaStructureType::Nexus ? 9.0f : (It->GetStructureType() == EMobaStructureType::Inhibitor ? 6.0f : 5.0f);
			DrawMinimapDot(It->GetActorLocation(), Size, GetRelationColor(It->GetTeam(), false));
		}
	}

	for (TActorIterator<AMobaWard> It(GetWorld()); It; ++It)
	{
		if (It->GetTeam() == LocalState->GetTeam())
		{
			DrawMinimapDot(It->GetActorLocation(), 4.0f, FLinearColor(0.3f, 1.0f, 0.5f));
		}
	}

	for (TActorIterator<AMobaMinion> It(GetWorld()); It; ++It)
	{
		if (!It->IsHidden() && IsSeen(*It))
		{
			DrawMinimapDot(It->GetActorLocation(), 2.5f, GetRelationColor(It->GetTeam(), false));
		}
	}

	const AMobaCharacter* LocalCharacter = Cast<AMobaCharacter>(GetOwningPawn());
	for (TActorIterator<AMobaCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsDead() || !IsSeen(*It))
		{
			continue;
		}

		DrawMinimapDot(It->GetActorLocation(), 10.0f, FLinearColor::Black);
		DrawMinimapDot(It->GetActorLocation(), 8.0f, GetRelationColor(It->GetTeam(), *It == LocalCharacter));
	}

	// 지금 화면에 보이는 범위. 화면 네 귀퉁이를 지면에 내려 미니맵에 옮긴다.
	const FVector2D Corners[] = { FVector2D(0.0, 0.0), FVector2D(Canvas->ClipX, 0.0), FVector2D(Canvas->ClipX, Canvas->ClipY), FVector2D(0.0, Canvas->ClipY) };
	FVector2D MapCorners[4];
	for (int32 Index = 0; Index < 4; ++Index)
	{
		FVector WorldPosition;
		FVector WorldDirection;
		Deproject(Corners[Index].X, Corners[Index].Y, WorldPosition, WorldDirection);
		if (WorldDirection.Z >= -KINDA_SMALL_NUMBER)
		{
			return;
		}

		MapCorners[Index] = WorldToMinimap(WorldPosition - WorldDirection * (WorldPosition.Z / WorldDirection.Z));
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector2D& From = MapCorners[Index];
		const FVector2D& To = MapCorners[(Index + 1) % 4];
		DrawLine(From.X, From.Y, To.X, To.Y, FLinearColor::White, 1.0f);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 피해량 숫자와 툴팁

void AMobaHUD::UpdateDamageNumbers()
{
	const double Now = GetWorld()->GetTimeSeconds();
	const AActor* LocalPawn = GetOwningPawn();

	const auto Track = [this, Now, LocalPawn](AActor* Unit, const UAbilitySystemComponent* AbilitySystem, float HeightOffset)
	{
		if (!Unit || !AbilitySystem || !IsSeen(Unit))
		{
			return;
		}

		const float Health = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetHealthAttribute());
		float& Previous = LastHealth.FindOrAdd(Unit, Health);
		const float Lost = Previous - Health;
		Previous = Health;

		// 재생이나 회복으로 늘어난 것과 아주 작은 변화는 표시하지 않는다.
		if (Lost >= 1.0f)
		{
			const bool bMine = Unit == LocalPawn;
			DamageNumbers.Add({ Unit->GetActorLocation() + FVector(FMath::FRandRange(-30.0, 30.0), FMath::FRandRange(-30.0, 30.0), HeightOffset),
				FString::FromInt(FMath::RoundToInt(Lost)), bMine ? FLinearColor(1.0f, 0.3f, 0.25f) : FLinearColor(1.0f, 0.95f, 0.7f), Now });
		}
	};

	for (TActorIterator<AMobaCharacter> It(GetWorld()); It; ++It)
	{
		Track(*It, It->GetAbilitySystemComponent(), 140.0f);
	}
	for (TActorIterator<AMobaMonster> It(GetWorld()); It; ++It)
	{
		Track(*It, It->GetAbilitySystemComponent(), 120.0f);
	}
	for (TActorIterator<AMobaStructure> It(GetWorld()); It; ++It)
	{
		Track(*It, It->GetAbilitySystemComponent(), It->GetHealthBarHeight() + 40.0f);
	}

	// 사라진 유닛과 오래된 숫자를 정리한다.
	for (auto It = LastHealth.CreateIterator(); It; ++It)
	{
		if (!It->Key.IsValid())
		{
			It.RemoveCurrent();
		}
	}
	DamageNumbers.RemoveAll([Now](const FDamageNumber& Number) { return Now - Number.Time > 1.1; });
}

void AMobaHUD::DrawDamageNumbers()
{
	const double Now = GetWorld()->GetTimeSeconds();
	UFont* Font = GEngine->GetSmallFont();

	for (const FDamageNumber& Number : DamageNumbers)
	{
		const float Age = static_cast<float>(Now - Number.Time);
		const FVector Screen = Project(Number.Location + FVector(0.0, 0.0, Age * 120.0f));
		if (Screen.Z <= 0.0)
		{
			continue;
		}

		const float Alpha = FMath::Clamp(1.0f - Age / 1.1f, 0.0f, 1.0f);
		DrawCenteredText(Number.Text, FLinearColor(0.0f, 0.0f, 0.0f, Alpha), Screen.X + 1.5f, Screen.Y + 1.5f, Font, 1.5f);
		DrawCenteredText(Number.Text, Number.Color.CopyWithNewOpacity(Alpha), Screen.X, Screen.Y, Font, 1.5f);
	}
}

void AMobaHUD::DrawAbilityTooltip(const AMobaCharacter* Character)
{
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	const AMobaPlayerController* PointerController = Cast<AMobaPlayerController>(GetOwningPlayerController());
	if (!PointerController || !PointerController->GetPointerPosition(MouseX, MouseY))
	{
		return;
	}

	// 패시브 칸.
	const UMobaPassive* Passive = Character->GetPassiveDefaults();
	if (Passive && PassiveRect.bIsValid && PassiveRect.IsInsideOrOn(FVector2D(MouseX, MouseY)))
	{
		UFont* Font = GEngine->GetSmallFont();
		const FString Title = FString::Printf(TEXT("%s  (패시브)"), *Passive->GetDisplayName().ToString());
		const FString Body = Passive->GetDescription().ToString();
		float TitleWidth = 0.0f, BodyWidth = 0.0f, Height = 0.0f;
		GetTextSize(Title, TitleWidth, Height, Font, 1.1f);
		GetTextSize(Body, BodyWidth, Height, Font, 1.1f);
		const float Width = FMath::Max(TitleWidth, BodyWidth);
		const float Left = FMath::Clamp(PassiveRect.Min.X, 8.0f, Canvas->ClipX - Width - 24.0f);
		const float Top = PassiveRect.Min.Y - 82.0f;
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.88f), Left, Top, Width + 16.0f, 52.0f);
		DrawText(Title, FLinearColor(1.0f, 0.9f, 0.5f), Left + 8.0f, Top + 6.0f, Font, 1.1f);
		DrawText(Body, FLinearColor::White, Left + 8.0f, Top + 26.0f, Font, 1.1f);
		return;
	}

	const EMobaAbilitySlot Slots[] = { EMobaAbilitySlot::Q, EMobaAbilitySlot::W, EMobaAbilitySlot::E, EMobaAbilitySlot::R, EMobaAbilitySlot::D, EMobaAbilitySlot::F };
	for (int32 Index = 0; Index < 6; ++Index)
	{
		if (!AbilityRects[Index].bIsValid || !AbilityRects[Index].IsInsideOrOn(FVector2D(MouseX, MouseY)))
		{
			continue;
		}

		const UMobaGameplayAbility* Ability = Character->GetAbilityInSlot(Slots[Index]);
		if (!Ability)
		{
			return;
		}

		UFont* Font = GEngine->GetSmallFont();
		TArray<FString> Lines;
		if (Ability->IsSummonerSpell())
		{
			Lines.Add(FString::Printf(TEXT("%s  (소환사 주문)"), *Ability->GetDisplayName().ToString()));
		}
		else
		{
			Lines.Add(FString::Printf(TEXT("%s  (%d / %d)"), *Ability->GetDisplayName().ToString(), Character->GetAbilityLevel(Slots[Index]), Ability->GetMaxLevel()));
		}
		Lines.Add(Ability->GetDescription().ToString());
		FString Costs = Ability->IsSummonerSpell()
			? FString::Printf(TEXT("재사용 대기 %.0f초"), Ability->GetCooldownDuration())
			: FString::Printf(TEXT("마나 %d    재사용 대기 %.1f초"), FMath::RoundToInt(Ability->GetManaCost()), Ability->GetEffectiveCooldown(Character->GetAbilitySystemComponent()));
		if (Ability->GetCastRange() > 0.0f)
		{
			Costs += FString::Printf(TEXT("    사거리 %d"), FMath::RoundToInt(Ability->GetCastRange()));
		}
		Lines.Add(Costs);
		if (Character->GetAbilityLevel(Slots[Index]) <= 0)
		{
			Lines.Add(TEXT("아직 배우지 않았습니다. Ctrl + 키로 배웁니다."));
		}

		float Width = 0.0f;
		for (const FString& Line : Lines)
		{
			float LineWidth = 0.0f;
			float LineHeight = 0.0f;
			GetTextSize(Line, LineWidth, LineHeight, Font, 1.1f);
			Width = FMath::Max(Width, LineWidth);
		}

		const float LineSpacing = 20.0f;
		const float Height = LineSpacing * Lines.Num() + 12.0f;
		const float Left = FMath::Clamp(AbilityRects[Index].Min.X, 8.0f, Canvas->ClipX - Width - 24.0f);
		const float Top = AbilityRects[Index].Min.Y - Height - 30.0f;

		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.88f), Left, Top, Width + 16.0f, Height);
		for (int32 Line = 0; Line < Lines.Num(); ++Line)
		{
			const FLinearColor Color = Line == 0 ? FLinearColor(1.0f, 0.9f, 0.5f) : (Line == 2 ? FLinearColor(0.6f, 0.8f, 1.0f) : FLinearColor::White);
			DrawText(Lines[Line], Color, Left + 8.0f, Top + 6.0f + LineSpacing * Line, Font, 1.1f);
		}
		return;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 전장의 안개

void AMobaHUD::DrawFogOfWar()
{
	const UMobaVisionSubsystem* Vision = UMobaVisionSubsystem::Get(this);
	if (!Vision || !Vision->HasFog())
	{
		return;
	}

	// 화면 네 귀퉁이를 지면에 내려 화면에 보이는 격자 범위를 구한다.
	FBox2D ViewBounds(ForceInit);
	const FVector2D Corners[] = { FVector2D(0.0, 0.0), FVector2D(Canvas->ClipX, 0.0), FVector2D(Canvas->ClipX, Canvas->ClipY), FVector2D(0.0, Canvas->ClipY) };
	for (const FVector2D& Corner : Corners)
	{
		FVector WorldPosition;
		FVector WorldDirection;
		Deproject(Corner.X, Corner.Y, WorldPosition, WorldDirection);
		if (WorldDirection.Z >= -KINDA_SMALL_NUMBER)
		{
			return;
		}
		const FVector Ground = WorldPosition - WorldDirection * (WorldPosition.Z / WorldDirection.Z);
		ViewBounds += FVector2D(Ground.X, Ground.Y);
	}

	const int32 Cells = UMobaVisionSubsystem::FogCells;
	const float CellSize = UMobaVisionSubsystem::FogWorldSize / Cells;
	const int32 MinX = FMath::Clamp(FMath::FloorToInt(ViewBounds.Min.X / CellSize) - 1, 0, Cells - 1);
	const int32 MaxX = FMath::Clamp(FMath::CeilToInt(ViewBounds.Max.X / CellSize) + 1, 0, Cells - 1);
	const int32 MinY = FMath::Clamp(FMath::FloorToInt(ViewBounds.Min.Y / CellSize) - 1, 0, Cells - 1);
	const int32 MaxY = FMath::Clamp(FMath::CeilToInt(ViewBounds.Max.Y / CellSize) + 1, 0, Cells - 1);

	// 꼭짓점마다 투명도를 달리 줘서 안개 가장자리가 부드럽게 이어지게 한다.
	TArray<FCanvasUVTri> Triangles;
	const auto MakeVertex = [this, Vision, CellSize](int32 X, int32 Y, FVector2D& OutScreen, FLinearColor& OutColor)
	{
		const FVector Screen = Project(FVector(X * CellSize, Y * CellSize, 0.0));
		OutScreen = FVector2D(Screen.X, Screen.Y);
		OutColor = FLinearColor(0.0f, 0.0f, 0.02f, Vision->IsFogVertexVisible(X, Y) ? 0.0f : FogOpacity);
		return Screen.Z > 0.0;
	};

	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			if (Vision->IsFogVertexVisible(X, Y) && Vision->IsFogVertexVisible(X + 1, Y) && Vision->IsFogVertexVisible(X, Y + 1) && Vision->IsFogVertexVisible(X + 1, Y + 1))
			{
				continue;
			}

			FVector2D P00, P10, P01, P11;
			FLinearColor C00, C10, C01, C11;
			if (!MakeVertex(X, Y, P00, C00) || !MakeVertex(X + 1, Y, P10, C10) || !MakeVertex(X, Y + 1, P01, C01) || !MakeVertex(X + 1, Y + 1, P11, C11))
			{
				continue;
			}

			FCanvasUVTri& First = Triangles.AddDefaulted_GetRef();
			First.V0_Pos = P00; First.V0_Color = C00;
			First.V1_Pos = P10; First.V1_Color = C10;
			First.V2_Pos = P11; First.V2_Color = C11;

			FCanvasUVTri& Second = Triangles.AddDefaulted_GetRef();
			Second.V0_Pos = P00; Second.V0_Color = C00;
			Second.V1_Pos = P11; Second.V1_Color = C11;
			Second.V2_Pos = P01; Second.V2_Color = C01;
		}
	}

	if (Triangles.Num() > 0)
	{
		FCanvasTriangleItem Item(Triangles, GWhiteTexture);
		Item.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Item);
	}
}

void AMobaHUD::DrawMinimapFog()
{
	const UMobaVisionSubsystem* Vision = UMobaVisionSubsystem::Get(this);
	if (!Vision || !Vision->HasFog())
	{
		return;
	}

	// 미니맵은 작으므로 격자를 두 칸씩 묶어 그린다.
	const int32 Step = 2;
	const int32 Cells = UMobaVisionSubsystem::FogCells;
	const float CellSize = UMobaVisionSubsystem::FogWorldSize / Cells;

	TArray<FCanvasUVTri> Triangles;
	const auto MakeVertex = [this, Vision, CellSize](int32 X, int32 Y, FVector2D& OutScreen, FLinearColor& OutColor)
	{
		OutScreen = WorldToMinimap(FVector(X * CellSize, Y * CellSize, 0.0));
		OutColor = FLinearColor(0.0f, 0.0f, 0.0f, Vision->IsFogVertexVisible(X, Y) ? 0.0f : 0.5f);
	};

	for (int32 Y = 0; Y < Cells; Y += Step)
	{
		for (int32 X = 0; X < Cells; X += Step)
		{
			const int32 X1 = FMath::Min(X + Step, Cells);
			const int32 Y1 = FMath::Min(Y + Step, Cells);
			if (Vision->IsFogVertexVisible(X, Y) && Vision->IsFogVertexVisible(X1, Y) && Vision->IsFogVertexVisible(X, Y1) && Vision->IsFogVertexVisible(X1, Y1))
			{
				continue;
			}

			FVector2D P00, P10, P01, P11;
			FLinearColor C00, C10, C01, C11;
			MakeVertex(X, Y, P00, C00);
			MakeVertex(X1, Y, P10, C10);
			MakeVertex(X, Y1, P01, C01);
			MakeVertex(X1, Y1, P11, C11);

			FCanvasUVTri& First = Triangles.AddDefaulted_GetRef();
			First.V0_Pos = P00; First.V0_Color = C00;
			First.V1_Pos = P10; First.V1_Color = C10;
			First.V2_Pos = P11; First.V2_Color = C11;

			FCanvasUVTri& Second = Triangles.AddDefaulted_GetRef();
			Second.V0_Pos = P00; Second.V0_Color = C00;
			Second.V1_Pos = P11; Second.V1_Color = C11;
			Second.V2_Pos = P01; Second.V2_Color = C01;
		}
	}

	if (Triangles.Num() > 0)
	{
		FCanvasTriangleItem Item(Triangles, GWhiteTexture);
		Item.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Item);
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
	const FString Text = FString::Printf(TEXT("블루 %d : %d 레드    %02d:%02d    K %d / D %d / CS %d"),
		GameState->GetTeamKills(EMobaTeam::Blue), GameState->GetTeamKills(EMobaTeam::Red),
		TotalSeconds / 60, TotalSeconds % 60, PlayerState->GetKills(), PlayerState->GetDeaths(), PlayerState->GetMinionKills());
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
	bLobbyDrawn = false;

	const AMobaGameState* GameState = GetWorld()->GetGameState<AMobaGameState>();
	if (!GameState || GameState->HasMatchStarted())
	{
		return;
	}

	if (GameState->UsesLobby())
	{
		DrawLobby();
		return;
	}

	const float CenterX = Canvas->ClipX * 0.5f;
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f), 0.0f, Canvas->ClipY * 0.16f, Canvas->ClipX, Canvas->ClipY * 0.14f);
	DrawCenteredText(TEXT("상대를 기다리는 중..."), FLinearColor::White, CenterX, Canvas->ClipY * 0.18f, GEngine->GetLargeFont(), 2.0f);

	if (GetNetMode() == NM_ListenServer)
	{
		DrawCenteredText(GetHostAddressHint(), FLinearColor(1.0f, 0.85f, 0.4f), CenterX, Canvas->ClipY * 0.25f, GEngine->GetSmallFont(), 1.3f);
	}
}

FString AMobaHUD::GetHostAddressHint()
{
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
	return LocalAddress.IsEmpty()
		? FString(TEXT("상대에게 이 PC의 IP 주소를 알려 주세요 (UDP 7777)"))
		: FString::Printf(TEXT("상대에게 알려 줄 주소: %s (UDP 7777)"), *LocalAddress);
}

namespace
{
	/** R 칸 오른쪽에 놓는 D·F 칸 두 개가 차지하는 폭. 와드 칸과 가방은 그만큼 오른쪽으로 밀린다. */
	float GetSummonerAreaWidth(float AbilityBoxSize)
	{
		return (AbilityBoxSize * 0.7f + 6.0f) * 2.0f;
	}
}

float AMobaHUD::GetAbilityBarLeft() const
{
	const float Gap = 8.0f;
	const float TotalWidth = AbilityBoxSize * 4.0f + Gap * 3.0f;
	const float Centered = (Canvas->ClipX - TotalWidth) * 0.5f;

	// 오른쪽에 붙는 D·F 칸, 와드 칸, 가방(3칸)이 미니맵에 닿으면 묶음 전체를 왼쪽으로 민다. 왼쪽 패시브 칸 자리는 남긴다.
	const float RightExtent = TotalWidth + Gap * 2.0f + GetSummonerAreaWidth(AbilityBoxSize) + AbilityBoxSize * 0.7f + 14.0f + 78.0f * 3.0f;
	const float MinimapLeft = Canvas->ClipX - MinimapSize - MinimapMargin;
	return FMath::Max(FMath::Min(Centered, MinimapLeft - 12.0f - RightExtent), 70.0f);
}

void AMobaHUD::DrawLobby()
{
	const AMobaGameState* GameState = GetWorld()->GetGameState<AMobaGameState>();
	const AMobaPlayerState* LocalState = GetOwningPlayerController() ? GetOwningPlayerController()->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (!GameState || !LocalState)
	{
		return;
	}

	bLobbyDrawn = true;
	LobbyChampionRects.Reset();
	LobbySpellRects.Reset();

	UFont* Font = GEngine->GetSmallFont();
	const float CenterX = Canvas->ClipX * 0.5f;
	const float Width = 640.0f;
	const float Left = CenterX - Width * 0.5f;
	// 낮은 창(에디터 PIE 등)에서는 여백과 버튼 높이를 줄여 준비 버튼이 화면 안에 들어오게 한다.
	const bool bCompact = Canvas->ClipY < 720.0f;
	float Top = bCompact ? 16.0f : Canvas->ClipY * 0.12f;

	DrawRect(FLinearColor(0.01f, 0.015f, 0.03f, 0.92f), 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
	DrawCenteredText(TEXT("챔피언 선택"), FLinearColor(0.85f, 0.9f, 1.0f), CenterX, Top, GEngine->GetLargeFont(), 2.0f);
	Top += bCompact ? 44.0f : 54.0f;

	const int32 Joined = GameState->PlayerArray.Num();
	DrawCenteredText(FString::Printf(TEXT("플레이어 %d / %d"), Joined, GameState->GetRequiredPlayers()), FLinearColor(0.75f, 0.75f, 0.8f), CenterX, Top, Font, 1.3f);
	Top += bCompact ? 22.0f : 26.0f;

	if (GetNetMode() == NM_ListenServer && Joined < GameState->GetRequiredPlayers())
	{
		DrawCenteredText(GetHostAddressHint(), FLinearColor(1.0f, 0.85f, 0.4f), CenterX, Top, Font, 1.2f);
	}
	Top += bCompact ? 26.0f : 34.0f;

	// 참가자 목록.
	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		const AMobaPlayerState* Player = Cast<AMobaPlayerState>(PlayerState);
		if (!Player)
		{
			continue;
		}

		const bool bIsLocal = Player == LocalState;
		// 셋 이상이면 줄을 낮춰 화면에 다 들어가게 한다.
		const float RowHeight = GameState->PlayerArray.Num() > 2 ? 28.0f : 36.0f;
		const float TextTop = Top + (RowHeight - 18.0f) * 0.5f;
		DrawRect(FLinearColor(0.05f, 0.06f, 0.09f, 1.0f), Left, Top, Width, RowHeight);
		DrawRect(MobaTeam::GetTeamColor(Player->GetTeam()), Left, Top, 6.0f, RowHeight);
		DrawText(FString::Printf(TEXT("%s 팀%s"), *MobaTeam::GetTeamDisplayName(Player->GetTeam()), bIsLocal ? TEXT(" (나)") : TEXT("")), FLinearColor::White, Left + 18.0f, TextTop, Font, 1.2f);
		DrawText(Player->GetChampionDisplayName(), FLinearColor(1.0f, 0.9f, 0.6f), Left + 220.0f, TextTop, Font, 1.2f);
		DrawText(FString::Printf(TEXT("%s / %s"), *MobaSummoner::GetDisplayName(Player->GetSummonerSpell(0)).ToString(), *MobaSummoner::GetDisplayName(Player->GetSummonerSpell(1)).ToString()), FLinearColor(0.8f, 0.7f, 1.0f), Left + 350.0f, TextTop, Font, 1.1f);
		DrawText(Player->IsReady() ? TEXT("준비 완료") : TEXT("고르는 중"), Player->IsReady() ? FLinearColor(0.4f, 1.0f, 0.5f) : FLinearColor(0.7f, 0.7f, 0.7f), Left + Width - 120.0f, TextTop, Font, 1.2f);
		Top += RowHeight + 6.0f;
	}
	Top += 18.0f;

	// 내 챔피언 고르기. 준비를 누르면 바꿀 수 없다.
	DrawText(TEXT("내 챔피언"), FLinearColor(0.7f, 0.75f, 0.85f), Left, Top, Font, 1.1f);
	Top += bCompact ? 20.0f : 24.0f;

	const float ChampionButtonHeight = bCompact ? 40.0f : 48.0f;

	const AMobaGameMode* ModeDefaults = Cast<AMobaGameMode>(GameState->GetDefaultGameMode());
	const TArray<FSoftObjectPath> Roster = ModeDefaults ? ModeDefaults->GetChampionRoster() : TArray<FSoftObjectPath>();
	const float ButtonGap = 10.0f;
	const float ButtonWidth = Roster.Num() > 0 ? (Width - ButtonGap * (Roster.Num() - 1)) / Roster.Num() : Width;
	const UMobaChampionData* Selected = LocalState->GetChampion();

	for (int32 Index = 0; Index < Roster.Num(); ++Index)
	{
		const UMobaChampionData* Champion = Cast<UMobaChampionData>(Roster[Index].TryLoad());
		const FString Name = Champion && !Champion->DisplayName.IsEmpty() ? Champion->DisplayName.ToString() : Roster[Index].GetAssetName();
		const float ButtonLeft = Left + (ButtonWidth + ButtonGap) * Index;
		const bool bSelected = Champion && Champion == Selected;

		DrawRect(bSelected ? FLinearColor(0.2f, 0.35f, 0.6f) : FLinearColor(0.12f, 0.13f, 0.17f), ButtonLeft, Top, ButtonWidth, ChampionButtonHeight);
		DrawCenteredText(Name, LocalState->IsReady() && !bSelected ? FLinearColor(0.45f, 0.45f, 0.45f) : FLinearColor::White, ButtonLeft + ButtonWidth * 0.5f, Top + ChampionButtonHeight * 0.5f - 10.0f, Font, 1.4f);
		LobbyChampionRects.Add({ Roster[Index].GetAssetName(), FBox2D(FVector2D(ButtonLeft, Top), FVector2D(ButtonLeft + ButtonWidth, Top + ChampionButtonHeight)) });
	}
	Top += ChampionButtonHeight + 14.0f;

	// 소환사 주문 D·F 고르기. 다른 칸에 있는 주문을 고르면 두 칸이 바뀐다.
	DrawText(TEXT("소환사 주문"), FLinearColor(0.7f, 0.75f, 0.85f), Left, Top, Font, 1.1f);
	Top += bCompact ? 20.0f : 22.0f;

	const float SpellButtonHeight = bCompact ? 26.0f : 30.0f;
	const int32 SpellCount = static_cast<int32>(EMobaSummonerSpell::Count);
	const float KeyWidth = 34.0f;
	const float SpellGap = 6.0f;
	const float SpellWidth = (Width - KeyWidth - SpellGap * (SpellCount - 1)) / SpellCount;
	const TCHAR* SpellKeys[] = { TEXT("D"), TEXT("F") };
	for (int32 SlotIndex = 0; SlotIndex < 2; ++SlotIndex)
	{
		DrawText(SpellKeys[SlotIndex], FLinearColor::White, Left + 8.0f, Top + SpellButtonHeight * 0.5f - 9.0f, Font, 1.3f);
		for (int32 SpellIndex = 0; SpellIndex < SpellCount; ++SpellIndex)
		{
			const EMobaSummonerSpell Spell = static_cast<EMobaSummonerSpell>(SpellIndex);
			const float ButtonLeft = Left + KeyWidth + (SpellWidth + SpellGap) * SpellIndex;
			const bool bSelected = LocalState->GetSummonerSpell(SlotIndex) == Spell;

			DrawRect(bSelected ? FLinearColor(0.4f, 0.28f, 0.55f) : FLinearColor(0.12f, 0.13f, 0.17f), ButtonLeft, Top, SpellWidth, SpellButtonHeight);
			DrawCenteredText(MobaSummoner::GetDisplayName(Spell).ToString(), LocalState->IsReady() && !bSelected ? FLinearColor(0.45f, 0.45f, 0.45f) : FLinearColor::White, ButtonLeft + SpellWidth * 0.5f, Top + SpellButtonHeight * 0.5f - 8.0f, Font, 1.1f);
			LobbySpellRects.Add({ SlotIndex, Spell, FBox2D(FVector2D(ButtonLeft, Top), FVector2D(ButtonLeft + SpellWidth, Top + SpellButtonHeight)) });
		}
		Top += SpellButtonHeight + 4.0f;
	}
	Top += 10.0f;

	// 준비 버튼과 카운트다운.
	const float ReadyWidth = 260.0f;
	const float ReadyLeft = CenterX - ReadyWidth * 0.5f;
	const float ReadyHeight = bCompact ? 40.0f : 52.0f;
	LobbyReadyRect = FBox2D(FVector2D(ReadyLeft, Top), FVector2D(ReadyLeft + ReadyWidth, Top + ReadyHeight));
	DrawRect(LocalState->IsReady() ? FLinearColor(0.25f, 0.25f, 0.28f) : FLinearColor(0.15f, 0.55f, 0.25f), ReadyLeft, Top, ReadyWidth, ReadyHeight);
	DrawCenteredText(LocalState->IsReady() ? TEXT("준비 취소") : TEXT("준비"), FLinearColor::White, CenterX, Top + ReadyHeight * 0.5f - 12.0f, Font, 1.6f);
	Top += ReadyHeight + (bCompact ? 8.0f : 24.0f);

	const float Countdown = GameState->GetCountdownRemaining();
	if (Countdown > 0.0f)
	{
		DrawCenteredText(FString::Printf(TEXT("%d초 후 시작"), FMath::CeilToInt(Countdown)), FLinearColor(1.0f, 0.85f, 0.3f), CenterX, Top, GEngine->GetLargeFont(), 1.8f);
	}
	else if (Joined < GameState->GetRequiredPlayers())
	{
		DrawCenteredText(TEXT("상대를 기다리는 중..."), FLinearColor(0.8f, 0.8f, 0.8f), CenterX, Top, Font, 1.4f);
	}
	else
	{
		DrawCenteredText(TEXT("모두 준비하면 시작합니다"), FLinearColor(0.8f, 0.8f, 0.8f), CenterX, Top, Font, 1.4f);
	}
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
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f), 0.0f, Canvas->ClipY * 0.1f, Canvas->ClipX, Canvas->ClipY * 0.2f);
	DrawCenteredText(bWon ? TEXT("승리") : TEXT("패배"), bWon ? FLinearColor(0.3f, 0.7f, 1.0f) : FLinearColor(1.0f, 0.3f, 0.25f), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.12f, GEngine->GetLargeFont(), 4.0f);

	const int32 TotalSeconds = FMath::FloorToInt(GameState->GetMatchTimeSeconds());
	const FString Detail = FString::Printf(TEXT("경기 시간 %02d:%02d%s    Enter: 메뉴로 돌아가기"), TotalSeconds / 60, TotalSeconds % 60, GameState->WasEndedByForfeit() ? TEXT("    상대가 게임을 떠났습니다.") : (GameState->WasEndedBySurrender() ? (bWon ? TEXT("    상대가 항복했습니다.") : TEXT("    항복했습니다.")) : TEXT("")));
	DrawCenteredText(Detail, FLinearColor::White, Canvas->ClipX * 0.5f, Canvas->ClipY * 0.24f, GEngine->GetSmallFont(), 1.3f);

	DrawPlayerTable(Canvas->ClipY * 0.34f);
}

// ---------------------------------------------------------------------------------------------------------------------
// 알림, 핑, 귀환

void AMobaHUD::DrawAnnouncements()
{
	const AMobaGameState* GameState = GetWorld()->GetGameState<AMobaGameState>();
	if (!GameState)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	const double Lifetime = 7.0;
	UFont* Font = GEngine->GetSmallFont();
	float Top = 44.0f;

	for (const FMobaAnnouncement& Announcement : GameState->GetAnnouncements())
	{
		if (Now - Announcement.Time > Lifetime)
		{
			continue;
		}

		float TextWidth = 0.0f;
		float TextHeight = 0.0f;
		GetTextSize(Announcement.Message, TextWidth, TextHeight, Font, 1.2f);

		const float Left = Canvas->ClipX - TextWidth - 20.0f;
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), Left - 6.0f, Top - 2.0f, TextWidth + 12.0f, TextHeight + 4.0f);
		DrawText(Announcement.Message, GetRelationColor(Announcement.Team, false), Left, Top, Font, 1.2f);
		Top += TextHeight + 8.0f;
	}
}

void AMobaHUD::DrawPings()
{
	const AMobaPlayerController* MobaController = Cast<AMobaPlayerController>(GetOwningPlayerController());
	if (!MobaController)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	const double Lifetime = 4.0;
	const FLinearColor Color(1.0f, 0.9f, 0.2f);

	for (const FMobaPing& Ping : MobaController->GetPings())
	{
		const double Age = Now - Ping.Time;
		if (Age > Lifetime)
		{
			continue;
		}

		// 퍼져 나가는 원을 반복해서 그린다.
		const float Pulse = FMath::Frac(static_cast<float>(Age));
		DrawGroundCircle(Ping.Location, 60.0f, Color, 3.0f);
		DrawGroundCircle(Ping.Location, 60.0f + 180.0f * Pulse, Color.CopyWithNewOpacity(1.0f - Pulse), 2.0f);

		if (bMinimapDrawn)
		{
			DrawMinimapDot(Ping.Location, 6.0f + 6.0f * Pulse, Color);
		}
	}
}

void AMobaHUD::DrawRecall(const AMobaCharacter* Character)
{
	const float Remaining = Character->GetRecallRemaining();
	if (!Character->IsRecalling() || Remaining <= 0.0f)
	{
		return;
	}

	const float Width = 260.0f;
	const float Height = 18.0f;
	const float Left = (Canvas->ClipX - Width) * 0.5f;
	const float Top = Canvas->ClipY * 0.68f;
	const float Fraction = 1.0f - FMath::Clamp(Remaining / Character->GetRecallDuration(), 0.0f, 1.0f);

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), Left - 2.0f, Top - 2.0f, Width + 4.0f, Height + 4.0f);
	DrawRect(FLinearColor(0.3f, 0.6f, 1.0f), Left, Top, Width * Fraction, Height);
	DrawCenteredText(FString::Printf(TEXT("귀환 중 %.1f"), Remaining), FLinearColor::White, Canvas->ClipX * 0.5f, Top + 1.0f, GEngine->GetSmallFont(), 1.1f);
}

// ---------------------------------------------------------------------------------------------------------------------
// 가방과 상점

void AMobaHUD::DrawStatsPanel(const AMobaCharacter* Character)
{
	const UAbilitySystemComponent* AbilitySystem = Character->GetAbilitySystemComponent();
	if (!AbilitySystem)
	{
		return;
	}

	const auto Value = [AbilitySystem](const FGameplayAttribute& Attribute)
	{
		return AbilitySystem->GetNumericAttribute(Attribute);
	};

	struct FStatLine
	{
		const TCHAR* Label;
		FString Text;
	};
	const FStatLine Lines[] = {
		{ TEXT("공격력"), FString::FromInt(FMath::RoundToInt(Value(UMobaAttributeSet::GetAttackDamageAttribute()))) },
		{ TEXT("주문력"), FString::FromInt(FMath::RoundToInt(Value(UMobaAttributeSet::GetAbilityPowerAttribute()))) },
		{ TEXT("방어력"), FString::FromInt(FMath::RoundToInt(Value(UMobaAttributeSet::GetArmorAttribute()))) },
		{ TEXT("마법 저항"), FString::FromInt(FMath::RoundToInt(Value(UMobaAttributeSet::GetMagicResistAttribute()))) },
		{ TEXT("공격 속도"), FString::Printf(TEXT("%.2f"), Value(UMobaAttributeSet::GetAttackSpeedAttribute())) },
		{ TEXT("이동 속도"), FString::FromInt(FMath::RoundToInt(Value(UMobaAttributeSet::GetMoveSpeedAttribute()))) },
		{ TEXT("스킬 가속"), FString::FromInt(FMath::RoundToInt(Value(UMobaAttributeSet::GetAbilityHasteAttribute()))) },
		{ TEXT("흡혈"), FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value(UMobaAttributeSet::GetLifeStealAttribute()) * 100.0f)) },
	};

	// 두 열, 네 줄. 왼쪽 아래 구석에 둔다.
	UFont* Font = GEngine->GetSmallFont();
	const float ColumnWidth = 104.0f;
	const float LineHeight = 17.0f;
	const float Width = ColumnWidth * 2.0f + 12.0f;
	const float Height = LineHeight * 4.0f + 10.0f;
	const float Left = 10.0f;
	float Top = Canvas->ClipY - Height - 10.0f;

	// 좁은 화면에서 스킬 바(왼쪽 패시브 칸 포함)와 겹치면 스킬 바 묶음보다 위로 올린다.
	if (Left + Width > GetAbilityBarLeft() - 60.0f)
	{
		Top = Canvas->ClipY - AbilityBoxSize - 16.0f * 2.0f - 28.0f - 44.0f - Height - 30.0f;
	}

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f), Left, Top, Width, Height);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Lines); ++Index)
	{
		const float X = Left + 6.0f + ColumnWidth * (Index / 4);
		const float Y = Top + 5.0f + LineHeight * (Index % 4);
		DrawText(Lines[Index].Label, FLinearColor(0.7f, 0.72f, 0.78f), X, Y, Font, 0.95f);
		DrawText(Lines[Index].Text, FLinearColor::White, X + 62.0f, Y, Font, 0.95f);
	}
}

void AMobaHUD::DrawInventory(const AMobaCharacter* Character)
{
	InventoryRects.Reset();

	const AMobaPlayerState* PlayerState = Character->GetPlayerState<AMobaPlayerState>();
	if (!PlayerState)
	{
		return;
	}

	UFont* Font = GEngine->GetSmallFont();
	const float Gap = 8.0f;
	const float TotalWidth = AbilityBoxSize * 4.0f + Gap * 3.0f;
	const float BoxTop = Canvas->ClipY - AbilityBoxSize - 16.0f * 2.0f - 28.0f;

	// 와드 칸 오른쪽에 3칸씩 두 줄로 놓는다.
	const float SlotWidth = 74.0f;
	const float SlotHeight = 27.0f;
	const float Left = GetAbilityBarLeft() + TotalWidth + Gap * 2.0f + GetSummonerAreaWidth(AbilityBoxSize) + AbilityBoxSize * 0.7f + 14.0f;
	const TArray<uint8>& Items = PlayerState->GetItems();

	for (int32 Index = 0; Index < MobaItems::MaxInventorySlots; ++Index)
	{
		const float SlotLeft = Left + (SlotWidth + 4.0f) * (Index % 3);
		const float SlotTop = BoxTop + (SlotHeight + 4.0f) * (Index / 3);
		InventoryRects.Add(FBox2D(FVector2D(SlotLeft, SlotTop), FVector2D(SlotLeft + SlotWidth, SlotTop + SlotHeight)));

		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), SlotLeft - 1.0f, SlotTop - 1.0f, SlotWidth + 2.0f, SlotHeight + 2.0f);

		const FMobaItemDefinition* Item = Items.IsValidIndex(Index) ? MobaItems::GetItem(Items[Index]) : nullptr;
		DrawRect(Item ? FLinearColor(0.25f, 0.2f, 0.1f) : FLinearColor(0.08f, 0.08f, 0.08f), SlotLeft, SlotTop, SlotWidth, SlotHeight);
		if (Item)
		{
			DrawText(Item->Name, FLinearColor(1.0f, 0.9f, 0.6f), SlotLeft + 4.0f, SlotTop + 6.0f, Font, 1.0f);
		}
	}
}

void AMobaHUD::DrawShop(const AMobaCharacter* Character)
{
	ShopItemRects.Reset();

	const AMobaPlayerController* MobaController = Cast<AMobaPlayerController>(GetOwningPlayerController());
	const AMobaPlayerState* PlayerState = Character->GetPlayerState<AMobaPlayerState>();
	if (!MobaController || !MobaController->IsShopOpen() || !PlayerState)
	{
		return;
	}

	UFont* Font = GEngine->GetSmallFont();
	const TArray<FMobaItemDefinition>& Items = MobaItems::GetItems();

	// 왼쪽 열은 기본 아이템, 오른쪽 열은 재료가 있는 완성 아이템. ShopItemRects는 아이템 번호 순서를 유지한다.
	const float Left = 20.0f;
	const float Top = 70.0f;
	const float BasicWidth = 330.0f;
	const float AdvancedWidth = 420.0f;
	const float Width = BasicWidth + AdvancedWidth + 18.0f;
	const float BasicRowHeight = 30.0f;
	const float AdvancedRowHeight = 40.0f;
	const float HeaderHeight = 74.0f;

	int32 BasicCount = 0;
	int32 AdvancedCount = 0;
	for (const FMobaItemDefinition& Item : Items)
	{
		(Item.IsBasic() ? BasicCount : AdvancedCount) += 1;
	}
	const float Height = HeaderHeight + FMath::Max(BasicRowHeight * BasicCount, AdvancedRowHeight * AdvancedCount) + 30.0f;

	ShopPanelRect = FBox2D(FVector2D(Left, Top), FVector2D(Left + Width, Top + Height));
	bShopDrawn = true;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f), Left, Top, Width, Height);
	DrawText(TEXT("상점  [P]"), FLinearColor::White, Left + 10.0f, Top + 8.0f, Font, 1.4f);
	DrawText(FString::Printf(TEXT("%d G"), FMath::FloorToInt(PlayerState->GetGold())), FLinearColor(1.0f, 0.85f, 0.3f), Left + Width - 90.0f, Top + 8.0f, Font, 1.4f);

	const bool bCanShop = Character->CanShop();
	DrawText(bCanShop ? TEXT("눌러서 구매. 가진 재료만큼 싸집니다. 가방의 아이템을 누르면 70%에 판매.") : TEXT("우물 안에 있거나 죽어 있을 때만 살 수 있습니다."),
		bCanShop ? FLinearColor(0.7f, 0.9f, 0.7f) : FLinearColor(1.0f, 0.55f, 0.4f), Left + 10.0f, Top + 32.0f, Font, 1.0f);
	DrawText(TEXT("기본"), FLinearColor(0.75f, 0.8f, 0.9f), Left + 10.0f, Top + 52.0f, Font, 1.1f);
	DrawText(TEXT("완성 (재료 포함 가격)"), FLinearColor(0.75f, 0.8f, 0.9f), Left + BasicWidth + 18.0f, Top + 52.0f, Font, 1.1f);

	int32 BasicRow = 0;
	int32 AdvancedRow = 0;
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		const FMobaItemDefinition& Item = Items[Index];
		const bool bBasic = Item.IsBasic();
		const float ColumnLeft = bBasic ? Left + 6.0f : Left + BasicWidth + 12.0f;
		const float ColumnWidth = bBasic ? BasicWidth - 6.0f : AdvancedWidth;
		const float RowHeight = bBasic ? BasicRowHeight : AdvancedRowHeight;
		const float RowTop = Top + HeaderHeight + RowHeight * (bBasic ? BasicRow++ : AdvancedRow++);
		ShopItemRects.Add(FBox2D(FVector2D(ColumnLeft, RowTop), FVector2D(ColumnLeft + ColumnWidth, RowTop + RowHeight - 3.0f)));

		const bool bCanBuy = Character->CanBuyItem(Index);
		const int32 Cost = Character->GetItemPurchaseCost(Index);
		DrawRect(bCanBuy ? FLinearColor(0.12f, 0.2f, 0.14f) : FLinearColor(0.1f, 0.1f, 0.1f), ColumnLeft, RowTop, ColumnWidth, RowHeight - 3.0f);

		const FLinearColor TextColor = bCanBuy ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f);
		const FLinearColor CostColor = bCanBuy ? FLinearColor(1.0f, 0.85f, 0.3f) : FLinearColor(0.5f, 0.45f, 0.3f);
		DrawText(Item.Name, TextColor, ColumnLeft + 8.0f, RowTop + 6.0f, Font, 1.1f);
		DrawText(Item.Description, TextColor, ColumnLeft + (bBasic ? 100.0f : 130.0f), RowTop + 6.0f, Font, 1.0f);

		// 재료가 있어 싸졌으면 원래 가격을 작게 함께 적는다.
		DrawText(FString::FromInt(Cost), CostColor, ColumnLeft + ColumnWidth - 52.0f, RowTop + 6.0f, Font, 1.1f);
		if (!bBasic)
		{
			const FString Recipe = FString::Printf(TEXT("재료: %s"), *MobaItems::DescribeComponents(Index));
			DrawText(Recipe, FLinearColor(0.6f, 0.65f, 0.75f), ColumnLeft + 8.0f, RowTop + 22.0f, Font, 0.9f);
			if (Cost != Item.Cost)
			{
				DrawText(FString::Printf(TEXT("(%d)"), Item.Cost), FLinearColor(0.55f, 0.55f, 0.55f), ColumnLeft + ColumnWidth - 52.0f, RowTop + 22.0f, Font, 0.9f);
			}
		}
	}

	DrawText(FString::Printf(TEXT("가방 %d / %d"), PlayerState->GetItems().Num(), MobaItems::MaxInventorySlots), FLinearColor(0.8f, 0.8f, 0.8f), Left + 10.0f, Top + Height - 24.0f, Font, 1.0f);
}

bool AMobaHUD::IsOverPanel(const FVector2D& ScreenPosition) const
{
	return bLobbyDrawn || bGameMenuDrawn || (bShopDrawn && ShopPanelRect.IsInsideOrOn(ScreenPosition));
}

void AMobaHUD::DrawGameMenu()
{
	bGameMenuDrawn = false;

	const AMobaPlayerController* MobaController = Cast<AMobaPlayerController>(GetOwningPlayerController());
	const AMobaGameState* GameState = GetWorld()->GetGameState<AMobaGameState>();
	const AMobaPlayerState* LocalState = MobaController ? MobaController->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (!MobaController || !MobaController->IsGameMenuOpen() || !GameState || !LocalState || bLobbyDrawn)
	{
		return;
	}

	bGameMenuDrawn = true;
	UFont* Font = GEngine->GetSmallFont();
	const float Width = 320.0f;
	const float ButtonHeight = 46.0f;
	const float Left = (Canvas->ClipX - Width) * 0.5f;
	float Top = Canvas->ClipY * 0.28f;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f), 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
	DrawRect(FLinearColor(0.03f, 0.04f, 0.07f, 0.95f), Left - 20.0f, Top - 20.0f, Width + 40.0f, ButtonHeight * 5.0f + 330.0f);
	DrawCenteredText(TEXT("게임 메뉴  [F10]"), FLinearColor::White, Canvas->ClipX * 0.5f, Top, Font, 1.4f);
	Top += 40.0f;

	// 항복은 경기가 진행 중이고 정해진 시간이 지난 뒤, 아직 동의하지 않았을 때만 누를 수 있다.
	const AMobaGameMode* ModeDefaults = Cast<AMobaGameMode>(GameState->GetDefaultGameMode());
	const float MinTime = ModeDefaults ? ModeDefaults->GetSurrenderMinTime() : 180.0f;
	const bool bTimeOk = GameState->GetMatchTimeSeconds() >= MinTime;
	bSurrenderAllowed = GameState->HasMatchStarted() && !GameState->IsMatchOver() && bTimeOk && !LocalState->HasVotedSurrender();

	FString SurrenderLabel = TEXT("항복");
	if (LocalState->HasVotedSurrender())
	{
		SurrenderLabel = TEXT("항복에 동의함");
	}
	else if (!bTimeOk)
	{
		const int32 Remaining = FMath::CeilToInt(MinTime - GameState->GetMatchTimeSeconds());
		SurrenderLabel = FString::Printf(TEXT("항복 (%d:%02d 후 가능)"), Remaining / 60, Remaining % 60);
	}

	const auto DrawButton = [this, Font, Left, Width, ButtonHeight](float ButtonTop, const FString& Label, bool bEnabled, const FLinearColor& Color) -> FBox2D
	{
		DrawRect(bEnabled ? Color : FLinearColor(0.12f, 0.12f, 0.12f), Left, ButtonTop, Width, ButtonHeight);
		DrawCenteredText(Label, bEnabled ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f), Left + Width * 0.5f, ButtonTop + 13.0f, Font, 1.3f);
		return FBox2D(FVector2D(Left, ButtonTop), FVector2D(Left + Width, ButtonTop + ButtonHeight));
	};

	GameMenuSurrenderRect = DrawButton(Top, SurrenderLabel, bSurrenderAllowed, FLinearColor(0.55f, 0.2f, 0.15f));
	Top += ButtonHeight + 12.0f;
	GameMenuLeaveRect = DrawButton(Top, GameState->IsMatchOver() ? TEXT("메뉴로 나가기") : TEXT("메뉴로 나가기 (패배 처리)"), true, FLinearColor(0.25f, 0.25f, 0.3f));
	Top += ButtonHeight + 12.0f;
	GameMenuResumeRect = DrawButton(Top, TEXT("계속하기"), true, FLinearColor(0.15f, 0.4f, 0.25f));
	Top += ButtonHeight + 24.0f;

	GameMenuWindowModeRect = DrawButton(Top, FString::Printf(TEXT("화면: %s"), *MobaDisplaySettings::DescribeWindowMode()), true, FLinearColor(0.18f, 0.2f, 0.28f));
	Top += ButtonHeight + 12.0f;
	GameMenuResolutionRect = DrawButton(Top, FString::Printf(TEXT("해상도: %s"), *MobaDisplaySettings::DescribeResolution()), true, FLinearColor(0.18f, 0.2f, 0.28f));
	Top += ButtonHeight + 20.0f;

	// 조작 안내.
	const TCHAR* Help[] = {
		TEXT("우클릭 이동·공격   A+좌클릭 공격 이동"),
		TEXT("QWER 스킬   Ctrl+QWER 스킬 배우기   D·F 소환사 주문   1 체력 물약"),
		TEXT("4 와드   B 귀환   P 상점   Tab 점수판"),
		TEXT("Y 카메라 고정   Space 내 챔피언 보기"),
		TEXT("Alt+좌클릭 핑   미니맵 좌클릭 카메라 이동"),
	};
	for (const TCHAR* Line : Help)
	{
		DrawText(Line, FLinearColor(0.75f, 0.78f, 0.85f), Left, Top, Font, 1.05f);
		Top += 19.0f;
	}
}

bool AMobaHUD::HandleClick(const FVector2D& ScreenPosition)
{
	// 게임 메뉴가 열려 있으면 메뉴 버튼만 받는다.
	if (bGameMenuDrawn)
	{
		if (AMobaPlayerController* MobaController = Cast<AMobaPlayerController>(GetOwningPlayerController()))
		{
			if (bSurrenderAllowed && GameMenuSurrenderRect.IsInsideOrOn(ScreenPosition))
			{
				MobaController->RequestSurrender();
				MobaController->CloseGameMenu();
			}
			else if (GameMenuLeaveRect.IsInsideOrOn(ScreenPosition))
			{
				MobaController->LeaveMatch();
			}
			else if (GameMenuResumeRect.IsInsideOrOn(ScreenPosition))
			{
				MobaController->CloseGameMenu();
			}
			else if (GameMenuWindowModeRect.IsInsideOrOn(ScreenPosition))
			{
				MobaDisplaySettings::ToggleFullscreen();
			}
			else if (GameMenuResolutionRect.IsInsideOrOn(ScreenPosition))
			{
				MobaDisplaySettings::CycleResolution();
			}
		}
		return true;
	}

	// 로비 화면은 화면 전체를 덮으므로 모든 클릭을 받는다.
	if (bLobbyDrawn)
	{
		AMobaPlayerController* MobaController = Cast<AMobaPlayerController>(GetOwningPlayerController());
		const AMobaPlayerState* LocalState = MobaController ? MobaController->GetPlayerState<AMobaPlayerState>() : nullptr;
		if (MobaController && LocalState)
		{
			if (LobbyReadyRect.IsInsideOrOn(ScreenPosition))
			{
				MobaController->RequestReady(!LocalState->IsReady());
			}
			else if (!LocalState->IsReady())
			{
				for (const TPair<FString, FBox2D>& Button : LobbyChampionRects)
				{
					if (Button.Value.IsInsideOrOn(ScreenPosition))
					{
						MobaController->RequestChampion(Button.Key);
						break;
					}
				}
				for (const FLobbySpellButton& Button : LobbySpellRects)
				{
					if (Button.Rect.IsInsideOrOn(ScreenPosition))
					{
						MobaController->RequestSummonerSpell(Button.Index, Button.Spell);
						break;
					}
				}
			}
		}
		return true;
	}

	AMobaCharacter* Character = Cast<AMobaCharacter>(GetOwningPawn());
	if (!bShopDrawn || !Character)
	{
		return false;
	}

	for (int32 Index = 0; Index < ShopItemRects.Num(); ++Index)
	{
		if (ShopItemRects[Index].IsInsideOrOn(ScreenPosition))
		{
			Character->BuyItem(Index);
			return true;
		}
	}

	for (int32 Index = 0; Index < InventoryRects.Num(); ++Index)
	{
		if (InventoryRects[Index].IsInsideOrOn(ScreenPosition))
		{
			Character->SellItem(Index);
			return true;
		}
	}

	return ShopPanelRect.IsInsideOrOn(ScreenPosition);
}

// ---------------------------------------------------------------------------------------------------------------------
// 점수판

void AMobaHUD::DrawScoreboard()
{
	const AMobaPlayerController* MobaController = Cast<AMobaPlayerController>(GetOwningPlayerController());
	const AMobaGameState* GameState = GetWorld()->GetGameState<AMobaGameState>();
	if (!MobaController || !MobaController->IsScoreboardShown() || !GameState || GameState->IsMatchOver())
	{
		return;
	}

	DrawPlayerTable(Canvas->ClipY * 0.2f);
}

void AMobaHUD::DrawPlayerTable(float Top)
{
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!GameState)
	{
		return;
	}

	TArray<const AMobaPlayerState*> Players;
	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (const AMobaPlayerState* MobaPlayerState = Cast<AMobaPlayerState>(PlayerState))
		{
			Players.Add(MobaPlayerState);
		}
	}
	Players.Sort([](const AMobaPlayerState& A, const AMobaPlayerState& B) { return static_cast<uint8>(A.GetTeam()) < static_cast<uint8>(B.GetTeam()); });

	UFont* Font = GEngine->GetSmallFont();

	// 창이 좁으면 표를 화면 폭에 맞춰 줄인다.
	const float FullWidth = 860.0f;
	const float Width = FMath::Min(FullWidth, Canvas->ClipX - 24.0f);
	const float ColumnScale = Width / FullWidth;
	const float RowHeight = 30.0f;
	const float Height = 50.0f + RowHeight * Players.Num() + 12.0f;
	const float Left = (Canvas->ClipX - Width) * 0.5f;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f), Left, Top, Width, Height);

	float Columns[] = { 14.0f, 90.0f, 225.0f, 285.0f, 405.0f, 465.0f, 565.0f };
	for (float& Column : Columns)
	{
		Column *= ColumnScale;
	}
	const TCHAR* Headers[] = { TEXT("팀"), TEXT("챔피언"), TEXT("레벨"), TEXT("K / D / A"), TEXT("CS"), TEXT("피해량"), TEXT("아이템") };
	for (int32 Index = 0; Index < 7; ++Index)
	{
		DrawText(Headers[Index], FLinearColor(0.7f, 0.7f, 0.7f), Left + Columns[Index], Top + 12.0f, Font, 1.1f);
	}

	const AMobaPlayerState* LocalState = GetOwningPlayerController() ? GetOwningPlayerController()->GetPlayerState<AMobaPlayerState>() : nullptr;
	for (int32 Row = 0; Row < Players.Num(); ++Row)
	{
		const AMobaPlayerState* Player = Players[Row];
		const float RowTop = Top + 46.0f + RowHeight * Row;
		const FLinearColor Color = GetRelationColor(Player->GetTeam(), Player == LocalState);

		FString ItemNames;
		for (const uint8 ItemIndex : Player->GetItems())
		{
			if (const FMobaItemDefinition* Item = MobaItems::GetItem(ItemIndex))
			{
				ItemNames += ItemNames.IsEmpty() ? FString(Item->Name) : FString::Printf(TEXT(", %s"), Item->Name);
			}
		}

		DrawText(MobaTeam::GetTeamDisplayName(Player->GetTeam()), Color, Left + Columns[0], RowTop, Font, 1.2f);
		DrawText(Player->GetChampionDisplayName(), Color, Left + Columns[1], RowTop, Font, 1.2f);
		DrawText(FString::FromInt(Player->GetChampionLevel()), FLinearColor::White, Left + Columns[2], RowTop, Font, 1.2f);
		DrawText(FString::Printf(TEXT("%d / %d / %d"), Player->GetKills(), Player->GetDeaths(), Player->GetAssists()), FLinearColor::White, Left + Columns[3], RowTop, Font, 1.2f);
		DrawText(FString::FromInt(Player->GetMinionKills()), FLinearColor::White, Left + Columns[4], RowTop, Font, 1.2f);
		DrawText(FString::FromInt(FMath::RoundToInt(Player->GetDamageToChampions())), FLinearColor::White, Left + Columns[5], RowTop, Font, 1.2f);
		DrawText(ItemNames, FLinearColor(1.0f, 0.9f, 0.6f), Left + Columns[6], RowTop + 2.0f, Font, 1.0f);
	}
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
// 버프와 와드

void AMobaHUD::DrawBuffsAndWard(const AMobaCharacter* Character)
{
	const UAbilitySystemComponent* AbilitySystem = Character->GetAbilitySystemComponent();
	if (!AbilitySystem)
	{
		return;
	}

	UFont* Font = GEngine->GetSmallFont();
	const float Gap = 8.0f;
	const float TotalWidth = AbilityBoxSize * 4.0f + Gap * 3.0f;
	const float Left = GetAbilityBarLeft();
	const float BoxTop = Canvas->ClipY - AbilityBoxSize - 16.0f * 2.0f - 28.0f;

	// 스킬 칸 오른쪽에 와드 칸을 둔다.
	const float WardSize = AbilityBoxSize * 0.7f;
	const float WardLeft = Left + TotalWidth + Gap * 2.0f + GetSummonerAreaWidth(AbilityBoxSize);
	const float WardTop = BoxTop + AbilityBoxSize - WardSize;
	const float WardCooldown = Character->GetWardCooldownRemaining();

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), WardLeft - 2.0f, WardTop - 2.0f, WardSize + 4.0f, WardSize + 4.0f);
	DrawRect(WardCooldown > 0.0f ? FLinearColor(0.1f, 0.15f, 0.1f) : FLinearColor(0.15f, 0.45f, 0.2f), WardLeft, WardTop, WardSize, WardSize);
	DrawText(TEXT("4"), FLinearColor::White, WardLeft + 4.0f, WardTop + 2.0f, Font, 1.2f);
	if (WardCooldown > 0.0f)
	{
		DrawCenteredText(FString::FromInt(FMath::CeilToInt(WardCooldown)), FLinearColor::White, WardLeft + WardSize * 0.5f, WardTop + WardSize * 0.4f, Font, 1.3f);
	}
	else
	{
		DrawCenteredText(TEXT("와드"), FLinearColor::White, WardLeft + WardSize * 0.5f, WardTop + WardSize * 0.5f, Font, 1.0f);
	}

	// 와드 칸 위에 물약 칸. 마시는 중이면 밝은 초록.
	if (const AMobaPlayerState* PotionOwner = Character->GetPlayerState<AMobaPlayerState>())
	{
		const float PotionTop = WardTop - WardSize - 6.0f;
		const int32 Potions = PotionOwner->GetPotions();
		const bool bDrinking = Character->GetPotionHealRemaining() > 0.0f;
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), WardLeft - 2.0f, PotionTop - 2.0f, WardSize + 4.0f, WardSize + 4.0f);
		DrawRect(bDrinking ? FLinearColor(0.3f, 0.75f, 0.35f) : (Potions > 0 ? FLinearColor(0.55f, 0.15f, 0.15f) : FLinearColor(0.15f, 0.1f, 0.1f)), WardLeft, PotionTop, WardSize, WardSize);
		DrawText(TEXT("1"), FLinearColor::White, WardLeft + 4.0f, PotionTop + 2.0f, Font, 1.2f);
		DrawCenteredText(FString::Printf(TEXT("물약 %d"), Potions), FLinearColor::White, WardLeft + WardSize * 0.5f, PotionTop + WardSize * 0.5f, Font, 0.9f);
	}

	// 가지고 있는 정글 버프를 스킬 칸 왼쪽에 적는다.
	struct FBuffLabel
	{
		FGameplayTag Tag;
		const TCHAR* Name;
		FLinearColor Color;
	};
	const FBuffLabel Labels[] = {
		{ MobaTags::Buff_Blue, TEXT("블루"), FLinearColor(0.35f, 0.6f, 1.0f) },
		{ MobaTags::Buff_Red, TEXT("레드"), FLinearColor(1.0f, 0.4f, 0.3f) },
		{ MobaTags::Buff_Baron, TEXT("바론"), FLinearColor(0.75f, 0.45f, 1.0f) },
		{ MobaTags::Buff_Dragon, TEXT("드래곤"), FLinearColor(1.0f, 0.75f, 0.3f) },
	};

	float LabelTop = BoxTop;
	for (const FBuffLabel& Label : Labels)
	{
		const int32 Count = AbilitySystem->GetTagCount(Label.Tag);
		if (Count <= 0)
		{
			continue;
		}

		const FString Text = Count > 1 ? FString::Printf(TEXT("%s x%d"), Label.Name, Count) : FString(Label.Name);
		float TextWidth = 0.0f;
		float TextHeight = 0.0f;
		GetTextSize(Text, TextWidth, TextHeight, Font, 1.2f);
		// 패시브 칸을 피해 그 왼쪽에 적는다.
		DrawText(Text, Label.Color, Left - TextWidth - Gap * 2.0f - 52.0f, LabelTop, Font, 1.2f);
		LabelTop += 18.0f;
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
	const float Left = GetAbilityBarLeft();
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

	// 스킬 칸 왼쪽의 패시브 칸. 중첩 수나 켜짐 여부를 보여 준다.
	PassiveRect = FBox2D(ForceInit);
	if (const UMobaPassive* Passive = Character->GetPassiveDefaults())
	{
		const float PassiveSize = 44.0f;
		const float PassiveLeft = Left - Gap - PassiveSize;
		const float PassiveTop = BoxTop + AbilityBoxSize - PassiveSize;
		const int32 Stacks = Character->GetPassiveStacks();
		const bool bActive = Passive->GetMaxStacks() > 0 ? Stacks > 0 : Stacks != 0;

		PassiveRect = FBox2D(FVector2D(PassiveLeft, PassiveTop), FVector2D(PassiveLeft + PassiveSize, PassiveTop + PassiveSize));
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), PassiveLeft - 2.0f, PassiveTop - 2.0f, PassiveSize + 4.0f, PassiveSize + 4.0f);
		DrawRect(bActive ? FLinearColor(0.55f, 0.4f, 0.1f) : FLinearColor(0.2f, 0.17f, 0.1f), PassiveLeft, PassiveTop, PassiveSize, PassiveSize);
		DrawCenteredText(TEXT("패시브"), FLinearColor(0.9f, 0.85f, 0.7f), PassiveLeft + PassiveSize * 0.5f, PassiveTop + 3.0f, Font, 0.8f);
		if (Passive->GetMaxStacks() > 0)
		{
			DrawCenteredText(FString::Printf(TEXT("%d/%d"), Stacks, Passive->GetMaxStacks()), FLinearColor::White, PassiveLeft + PassiveSize * 0.5f, PassiveTop + 20.0f, Font, 1.1f);
		}
		else if (bActive)
		{
			DrawCenteredText(TEXT("ON"), FLinearColor(1.0f, 0.9f, 0.4f), PassiveLeft + PassiveSize * 0.5f, PassiveTop + 20.0f, Font, 1.1f);
		}
	}

	// QWER 스킬 칸.
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const UMobaGameplayAbility* Ability = Character->GetAbilityInSlot(Slots[Index]);
		const float BoxLeft = Left + (AbilityBoxSize + Gap) * Index;
		AbilityRects[Index] = FBox2D(FVector2D(BoxLeft, BoxTop), FVector2D(BoxLeft + AbilityBoxSize, BoxTop + AbilityBoxSize));

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

	// R 칸 오른쪽의 소환사 주문 D·F 칸.
	const EMobaAbilitySlot SpellSlots[] = { EMobaAbilitySlot::D, EMobaAbilitySlot::F };
	const TCHAR* SpellKeys[] = { TEXT("D"), TEXT("F") };
	const float SpellSize = AbilityBoxSize * 0.7f;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const UMobaGameplayAbility* Spell = Character->GetAbilityInSlot(SpellSlots[Index]);
		const float SpellLeft = Left + TotalWidth + Gap * 2.0f + (SpellSize + 6.0f) * Index;
		const float SpellTop = BoxTop + AbilityBoxSize - SpellSize;
		AbilityRects[4 + Index] = FBox2D(FVector2D(SpellLeft, SpellTop), FVector2D(SpellLeft + SpellSize, SpellTop + SpellSize));

		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), SpellLeft - 2.0f, SpellTop - 2.0f, SpellSize + 4.0f, SpellSize + 4.0f);
		if (!Spell)
		{
			continue;
		}

		DrawRect(FLinearColor(0.35f, 0.25f, 0.45f), SpellLeft, SpellTop, SpellSize, SpellSize);
		float Remaining = 0.0f;
		float Duration = 0.0f;
		if (Character->GetCooldownRemaining(SpellSlots[Index], Remaining, Duration) && Duration > 0.0f)
		{
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f), SpellLeft, SpellTop, SpellSize, SpellSize * FMath::Clamp(Remaining / Duration, 0.0f, 1.0f));
			DrawCenteredText(FString::FromInt(FMath::CeilToInt(Remaining)), FLinearColor::White, SpellLeft + SpellSize * 0.5f, SpellTop + SpellSize * 0.4f, Font, 1.2f);
		}
		else
		{
			DrawCenteredText(Spell->GetDisplayName().ToString(), FLinearColor::White, SpellLeft + SpellSize * 0.5f, SpellTop + SpellSize * 0.5f, Font, 0.9f);
		}
		DrawText(SpellKeys[Index], FLinearColor::White, SpellLeft + 3.0f, SpellTop + 1.0f, Font, 1.1f);
	}

	// 체력·마나 수치 바.
	const float BarsTop = BoxTop + AbilityBoxSize + 8.0f;
	const float Shield = FMath::Max(AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetShieldAttribute()), 0.0f);
	const float BarTotal = FMath::Max(MaxHealth, Health + Shield);
	const float HealthFraction = BarTotal > 0.0f ? FMath::Clamp(Health / BarTotal, 0.0f, 1.0f) : 0.0f;
	const float ShieldFraction = BarTotal > 0.0f ? FMath::Clamp(Shield / BarTotal, 0.0f, 1.0f - HealthFraction) : 0.0f;
	const float ManaFraction = MaxMana > 0.0f ? FMath::Clamp(Mana / MaxMana, 0.0f, 1.0f) : 0.0f;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), Left - 2.0f, BarsTop - 2.0f, TotalWidth + 4.0f, BarHeight * 2.0f + 6.0f);
	DrawRect(FLinearColor(0.15f, 0.7f, 0.2f), Left, BarsTop, TotalWidth * HealthFraction, BarHeight);
	if (ShieldFraction > 0.0f)
	{
		DrawRect(FLinearColor(0.92f, 0.92f, 0.92f), Left + TotalWidth * HealthFraction, BarsTop, TotalWidth * ShieldFraction, BarHeight);
	}
	DrawRect(FLinearColor(0.2f, 0.45f, 1.0f), Left, BarsTop + BarHeight + 2.0f, TotalWidth * ManaFraction, BarHeight);
	const FString HealthText = Shield > 0.0f
		? FString::Printf(TEXT("%d / %d  (+%d)"), FMath::CeilToInt(Health), FMath::RoundToInt(MaxHealth), FMath::CeilToInt(Shield))
		: FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::RoundToInt(MaxHealth));
	DrawText(HealthText, FLinearColor::White, Left + 6.0f, BarsTop + 1.0f, Font, 1.0f);
	DrawText(FString::Printf(TEXT("%d / %d"), FMath::FloorToInt(Mana), FMath::RoundToInt(MaxMana)), FLinearColor::White, Left + 6.0f, BarsTop + BarHeight + 3.0f, Font, 1.0f);
}
