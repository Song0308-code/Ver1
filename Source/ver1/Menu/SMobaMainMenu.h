#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class AMobaMenuPlayerController;
class SEditableTextBox;
class STextBlock;

/** UMG 화면이 들어오기 전까지 쓰는 임시 메인 메뉴. */
class SMobaMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMobaMainMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AMobaMenuPlayerController>, OwnerController)
		SLATE_ARGUMENT(FText, InitialStatus)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	FReply OnHostClicked();
	FReply OnPracticeClicked();
	FReply OnJoinClicked();
	FReply OnQuitClicked();
	void OnAddressCommitted(const FText& Text, ETextCommit::Type CommitType);

	void TryJoin();
	void SetStatus(const FText& Text);

	TSharedRef<SWidget> MakeButton(const FText& Label, FReply (SMobaMainMenu::*Handler)());

	TWeakObjectPtr<AMobaMenuPlayerController> OwnerController;
	TSharedPtr<SEditableTextBox> AddressBox;
	TSharedPtr<STextBlock> StatusText;

	FSlateColorBrush BackgroundBrush = FSlateColorBrush(FLinearColor(0.015f, 0.02f, 0.035f, 1.0f));
};
