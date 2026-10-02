#include "Vision/MobaBush.h"
#include "Components/BoxComponent.h"
#include "Vision/MobaVisionSubsystem.h"

AMobaBush::AMobaBush()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(50.0, 50.0, 50.0));
	Box->SetMobility(EComponentMobility::Static);

	// 위치 판정은 계산으로 하므로 충돌은 필요 없다. 커서와 이동을 막지 않아야 한다.
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->SetCanEverAffectNavigation(false);
	RootComponent = Box;
}

void AMobaBush::BeginPlay()
{
	Super::BeginPlay();

	if (UMobaVisionSubsystem* Vision = UMobaVisionSubsystem::Get(this))
	{
		Vision->RegisterBush(this);
	}
}

void AMobaBush::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMobaVisionSubsystem* Vision = UMobaVisionSubsystem::Get(this))
	{
		Vision->UnregisterBush(this);
	}

	Super::EndPlay(EndPlayReason);
}

bool AMobaBush::ContainsLocation(const FVector& WorldLocation) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
	return FMath::Abs(Local.X) <= 50.0 && FMath::Abs(Local.Y) <= 50.0;
}
