#include "UI/MobaHUD.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystem/MobaGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Game/MobaGameState.h"
#include "Game/MobaGameplayTags.h"
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

	DrawStructureBars();
	DrawMonsterBars();
	DrawMinionBars();
	DrawChampionBars();
	DrawWorldTimers();

	DrawMatchInfo();
	DrawRespawnCountdown();
	DrawWaitingForPlayers();

	if (const AMobaCharacter* LocalCharacter = Cast<AMobaCharacter>(GetOwningPawn()))
	{
		DrawTargetingIndicator(LocalCharacter);
		DrawStatusBar(LocalCharacter);
		DrawBuffsAndWard(LocalCharacter);
		DrawInventory(LocalCharacter);
		DrawRecall(LocalCharacter);
	}

	DrawPings();
	DrawMinimap();
	DrawAnnouncements();

	bShopDrawn = false;
	if (const AMobaCharacter* ShopCharacter = Cast<AMobaCharacter>(GetOwningPawn()))
	{
		DrawShop(ShopCharacter);
	}

	DrawScoreboard();

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
		if (!AbilitySystem || Character->IsDead() || !IsSeen(Character))
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
		if (Minion->IsHidden() || !IsSeen(Minion))
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
		DrawUnitBar(Structure->GetAbilitySystemComponent(), BarLocation, StructureBarSize, FillColor, false, FString());
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
	const AMobaPlayerState* LocalState = GetOwningPlayerController() ? GetOwningPlayerController()->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (!LocalState || LocalState->GetTeam() == EMobaTeam::None)
	{
		bMinimapDrawn = false;
		return;
	}

	MinimapOrigin = FVector2D(Canvas->ClipX - MinimapSize - MinimapMargin, Canvas->ClipY - MinimapSize - MinimapMargin);
	bMinimapDrawn = true;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f), MinimapOrigin.X - 2.0f, MinimapOrigin.Y - 2.0f, MinimapSize + 4.0f, MinimapSize + 4.0f);
	DrawRect(FLinearColor(0.06f, 0.1f, 0.07f, 0.9f), MinimapOrigin.X, MinimapOrigin.Y, MinimapSize, MinimapSize);

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
	const float Left = (Canvas->ClipX - TotalWidth) * 0.5f + TotalWidth + Gap * 2.0f + AbilityBoxSize * 0.7f + 14.0f;
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

	const float Left = 20.0f;
	const float Top = 110.0f;
	const float Width = 360.0f;
	const float RowHeight = 30.0f;
	const float HeaderHeight = 54.0f;
	const float Height = HeaderHeight + RowHeight * Items.Num() + 30.0f;

	ShopPanelRect = FBox2D(FVector2D(Left, Top), FVector2D(Left + Width, Top + Height));
	bShopDrawn = true;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f), Left, Top, Width, Height);
	DrawText(TEXT("상점  [P]"), FLinearColor::White, Left + 10.0f, Top + 8.0f, Font, 1.4f);
	DrawText(FString::Printf(TEXT("%d G"), FMath::FloorToInt(PlayerState->GetGold())), FLinearColor(1.0f, 0.85f, 0.3f), Left + Width - 80.0f, Top + 8.0f, Font, 1.4f);

	const bool bCanShop = Character->CanShop();
	DrawText(bCanShop ? TEXT("눌러서 구매. 가방의 아이템을 누르면 70%에 판매.") : TEXT("우물 안에 있거나 죽어 있을 때만 살 수 있습니다."),
		bCanShop ? FLinearColor(0.7f, 0.9f, 0.7f) : FLinearColor(1.0f, 0.55f, 0.4f), Left + 10.0f, Top + 32.0f, Font, 1.0f);

	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		const FMobaItemDefinition& Item = Items[Index];
		const float RowTop = Top + HeaderHeight + RowHeight * Index;
		ShopItemRects.Add(FBox2D(FVector2D(Left + 6.0f, RowTop), FVector2D(Left + Width - 6.0f, RowTop + RowHeight - 3.0f)));

		const bool bCanBuy = Character->CanBuyItem(Index);
		DrawRect(bCanBuy ? FLinearColor(0.12f, 0.2f, 0.14f) : FLinearColor(0.1f, 0.1f, 0.1f), Left + 6.0f, RowTop, Width - 12.0f, RowHeight - 3.0f);

		const FLinearColor TextColor = bCanBuy ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f);
		DrawText(Item.Name, TextColor, Left + 14.0f, RowTop + 6.0f, Font, 1.1f);
		DrawText(Item.Description, TextColor, Left + 120.0f, RowTop + 6.0f, Font, 1.0f);
		DrawText(FString::FromInt(Item.Cost), bCanBuy ? FLinearColor(1.0f, 0.85f, 0.3f) : FLinearColor(0.5f, 0.45f, 0.3f), Left + Width - 60.0f, RowTop + 6.0f, Font, 1.1f);
	}

	DrawText(FString::Printf(TEXT("가방 %d / %d"), PlayerState->GetItems().Num(), MobaItems::MaxInventorySlots), FLinearColor(0.8f, 0.8f, 0.8f), Left + 10.0f, Top + Height - 24.0f, Font, 1.0f);
}

bool AMobaHUD::IsOverPanel(const FVector2D& ScreenPosition) const
{
	return bShopDrawn && ShopPanelRect.IsInsideOrOn(ScreenPosition);
}

bool AMobaHUD::HandleClick(const FVector2D& ScreenPosition)
{
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
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!MobaController || !MobaController->IsScoreboardShown() || !GameState)
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
	const float Width = 760.0f;
	const float RowHeight = 30.0f;
	const float Height = 50.0f + RowHeight * Players.Num() + 12.0f;
	const float Left = (Canvas->ClipX - Width) * 0.5f;
	const float Top = Canvas->ClipY * 0.2f;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f), Left, Top, Width, Height);

	const float Columns[] = { 14.0f, 90.0f, 230.0f, 290.0f, 380.0f, 440.0f };
	const TCHAR* Headers[] = { TEXT("팀"), TEXT("챔피언"), TEXT("레벨"), TEXT("K / D"), TEXT("CS"), TEXT("아이템") };
	for (int32 Index = 0; Index < 6; ++Index)
	{
		DrawText(Headers[Index], FLinearColor(0.7f, 0.7f, 0.7f), Left + Columns[Index], Top + 12.0f, Font, 1.1f);
	}

	const AMobaPlayerState* LocalState = MobaController->GetPlayerState<AMobaPlayerState>();
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
		DrawText(FString::Printf(TEXT("%d / %d"), Player->GetKills(), Player->GetDeaths()), FLinearColor::White, Left + Columns[3], RowTop, Font, 1.2f);
		DrawText(FString::FromInt(Player->GetMinionKills()), FLinearColor::White, Left + Columns[4], RowTop, Font, 1.2f);
		DrawText(ItemNames, FLinearColor(1.0f, 0.9f, 0.6f), Left + Columns[5], RowTop + 2.0f, Font, 1.0f);
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
	const float Left = (Canvas->ClipX - TotalWidth) * 0.5f;
	const float BoxTop = Canvas->ClipY - AbilityBoxSize - 16.0f * 2.0f - 28.0f;

	// 스킬 칸 오른쪽에 와드 칸을 둔다.
	const float WardSize = AbilityBoxSize * 0.7f;
	const float WardLeft = Left + TotalWidth + Gap * 2.0f;
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
		DrawText(Text, Label.Color, Left - TextWidth - Gap * 2.0f, LabelTop, Font, 1.2f);
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
