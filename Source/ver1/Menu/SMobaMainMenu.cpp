#include "Menu/SMobaMainMenu.h"
#include "Menu/MobaMenuPlayerController.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SMobaMainMenu::Construct(const FArguments& InArgs)
{
	OwnerController = InArgs._OwnerController;

	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 44);
	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Regular", 13);
	const FSlateFontInfo InputFont = FCoreStyle::GetDefaultFontStyle("Regular", 16);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(&BackgroundBrush)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(420.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 36.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("MOBA 프로토타입")))
					.Font(TitleFont)
					.ColorAndOpacity(FLinearColor(0.85f, 0.9f, 1.0f))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					MakeButton(FText::FromString(TEXT("방 만들기")), &SMobaMainMenu::OnHostClicked)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 28.0f)
				[
					MakeButton(FText::FromString(TEXT("혼자 연습")), &SMobaMainMenu::OnPracticeClicked)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("호스트 IP 주소")))
					.Font(LabelFont)
					.ColorAndOpacity(FLinearColor(0.7f, 0.75f, 0.85f))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SAssignNew(AddressBox, SEditableTextBox)
					.Text(FText::FromString(TEXT("127.0.0.1")))
					.Font(InputFont)
					.OnTextCommitted(this, &SMobaMainMenu::OnAddressCommitted)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 28.0f)
				[
					MakeButton(FText::FromString(TEXT("접속")), &SMobaMainMenu::OnJoinClicked)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 20.0f)
				[
					MakeButton(FText::FromString(TEXT("종료")), &SMobaMainMenu::OnQuitClicked)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SAssignNew(StatusText, STextBlock)
					.Text(InArgs._InitialStatus)
					.Font(LabelFont)
					.ColorAndOpacity(FLinearColor(1.0f, 0.75f, 0.35f))
					.AutoWrapText(true)
					.Justification(ETextJustify::Center)
				]
			]
		]
	];
}

TSharedRef<SWidget> SMobaMainMenu::MakeButton(const FText& Label, FReply (SMobaMainMenu::*Handler)())
{
	return SNew(SButton)
		.HAlign(HAlign_Center)
		.ContentPadding(FMargin(0.0f, 10.0f))
		.OnClicked(this, Handler)
		[
			SNew(STextBlock)
			.Text(Label)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
		];
}

FReply SMobaMainMenu::OnHostClicked()
{
	if (AMobaMenuPlayerController* Controller = OwnerController.Get())
	{
		SetStatus(FText::FromString(TEXT("방을 여는 중...")));
		Controller->HostGame();
	}
	return FReply::Handled();
}

FReply SMobaMainMenu::OnPracticeClicked()
{
	if (AMobaMenuPlayerController* Controller = OwnerController.Get())
	{
		Controller->StartPractice();
	}
	return FReply::Handled();
}

FReply SMobaMainMenu::OnJoinClicked()
{
	TryJoin();
	return FReply::Handled();
}

FReply SMobaMainMenu::OnQuitClicked()
{
	if (AMobaMenuPlayerController* Controller = OwnerController.Get())
	{
		Controller->QuitGame();
	}
	return FReply::Handled();
}

void SMobaMainMenu::OnAddressCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		TryJoin();
	}
}

void SMobaMainMenu::TryJoin()
{
	AMobaMenuPlayerController* Controller = OwnerController.Get();
	if (!Controller || !AddressBox.IsValid())
	{
		return;
	}

	const FString Address = AddressBox->GetText().ToString();
	if (Controller->JoinGame(Address))
	{
		SetStatus(FText::FromString(FString::Printf(TEXT("%s 에 접속하는 중..."), *Address.TrimStartAndEnd())));
	}
	else
	{
		SetStatus(FText::FromString(TEXT("IP 주소를 입력하세요. 예) 192.168.0.10")));
	}
}

void SMobaMainMenu::SetStatus(const FText& Text)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(Text);
	}
}
