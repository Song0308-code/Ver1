#include "Game/MobaTypes.h"

FName MobaTeam::GetTeamTag(EMobaTeam Team)
{
	switch (Team)
	{
	case EMobaTeam::Blue: return FName(TEXT("Blue"));
	case EMobaTeam::Red: return FName(TEXT("Red"));
	default: return NAME_None;
	}
}

FLinearColor MobaTeam::GetTeamColor(EMobaTeam Team)
{
	switch (Team)
	{
	case EMobaTeam::Blue: return FLinearColor(0.05f, 0.25f, 1.0f);
	case EMobaTeam::Red: return FLinearColor(1.0f, 0.08f, 0.05f);
	case EMobaTeam::Neutral: return FLinearColor(0.95f, 0.6f, 0.05f);
	default: return FLinearColor::Gray;
	}
}
