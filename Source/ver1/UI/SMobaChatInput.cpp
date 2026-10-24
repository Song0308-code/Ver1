#include "UI/SMobaChatInput.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"

void SMobaChatInput::Construct(const FArguments& InArgs)
{
	OnSubmit = InArgs._OnSubmit;
	OnCancel = InArgs._OnCancel;

	ChildSlot
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Bottom)
	.Padding(FMargin(12.0f, 0.0f, 0.0f, InArgs._BottomPadding))
	[
		SNew(SBox)
		.WidthOverride(420.0f)
		[
			SAssignNew(TextBox, SEditableTextBox)
			.HintText(FText::FromString(TEXT("팀에게 보내기 (/all 로 시작하면 전체)  Enter 보내기 · Esc 닫기")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
			.ClearKeyboardFocusOnCommit(false)
			.RevertTextOnEscape(true)
			.OnTextCommitted(this, &SMobaChatInput::HandleCommitted)
		]
	];
}

TSharedPtr<SWidget> SMobaChatInput::GetFocusTarget() const
{
	return TextBox;
}

void SMobaChatInput::HandleCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		OnSubmit.ExecuteIfBound(Text.ToString());
	}
	else if (CommitType == ETextCommit::OnCleared)
	{
		OnCancel.ExecuteIfBound();
	}
}
