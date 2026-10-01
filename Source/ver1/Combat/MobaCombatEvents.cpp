#include "Combat/MobaCombatEvents.h"
#include "Engine/World.h"

UMobaCombatEvents* UMobaCombatEvents::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UMobaCombatEvents>() : nullptr;
}
