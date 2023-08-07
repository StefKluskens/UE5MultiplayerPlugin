// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

void UMenu::MenuSetup(int32 numOfPublicConnections, FString matchType, FString lobbyPath)
{
	NumPublicConnections = numOfPublicConnections;
	MatchType = matchType;
	PathToLobby = FString::Printf(TEXT("%s?listen"), *lobbyPath);

	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	UWorld* pWorld = GetWorld();
	if (pWorld)
	{
		APlayerController* pController = pWorld->GetFirstPlayerController();
		if (pController)
		{
			//Only have input on the UI widget
			FInputModeUIOnly inputModeData;
			inputModeData.SetWidgetToFocus(TakeWidget());
			inputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			pController->SetInputMode(inputModeData);
			pController->SetShowMouseCursor(true);
		}
	}

	UGameInstance* pGame = GetGameInstance();
	if (pGame)
	{
		MultiplayerSessionsSubSystem = pGame->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (MultiplayerSessionsSubSystem)
	{
		MultiplayerSessionsSubSystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubSystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubSystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSessions);
		MultiplayerSessionsSubSystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubSystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
	}
}

bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}

	return true;
}

void UMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UWorld* pWorld = GetWorld();
		if (pWorld)
		{
			pWorld->ServerTravel(PathToLobby);
		}
	}
	else
	{
		HostButton->SetIsEnabled(true);
	}
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& sessionResults, bool bWasSuccessful)
{
	if (!MultiplayerSessionsSubSystem)
	{
		return;
	}

	for (auto result : sessionResults)
	{
		FString settingsValue;
		result.Session.SessionSettings.Get(FName("MatchType"), settingsValue);
		if (settingsValue == MatchType)
		{
			MultiplayerSessionsSubSystem->JoinsSession(result);
			return;
		}
	}

	JoinButton->SetIsEnabled(true);
}

void UMenu::OnJoinSessions(EOnJoinSessionCompleteResult::Type result)
{
	IOnlineSubsystem* pSubsystem = IOnlineSubsystem::Get();
	if (pSubsystem)
	{
		IOnlineSessionPtr pOnlineSessionInterface = pSubsystem->GetSessionInterface();
		if (pOnlineSessionInterface.IsValid())
		{
			FString address;
			if (pOnlineSessionInterface->GetResolvedConnectString(NAME_GameSession, address))
			{
				APlayerController* pController = GetGameInstance()->GetFirstLocalPlayerController();
				if (pController)
				{
					pController->ClientTravel(address, ETravelType::TRAVEL_Absolute);
				}
			}
		}
	}

	if (result != EOnJoinSessionCompleteResult::Success)
	{
		JoinButton->SetIsEnabled(true);
	}
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
	if (!GEngine)
	{
		return;
	}

	if (bWasSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.0f,
			FColor::Green,
			FString(TEXT("Session started"))
		);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.0f,
			FColor::Red,
			FString(TEXT("Session failed to start"))
		);
	}
}

void UMenu::HostButtonClicked()
{
	HostButton->SetIsEnabled(false);

	if (MultiplayerSessionsSubSystem)
	{
		MultiplayerSessionsSubSystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenu::JoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);

	if (MultiplayerSessionsSubSystem)
	{
		MultiplayerSessionsSubSystem->FindSessions(10000);
	}
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();

	UWorld* pWorld = GetWorld();
	if (pWorld)
	{
		APlayerController* pController = pWorld->GetFirstPlayerController();
		if (pController)
		{
			FInputModeGameOnly inputModeData;
			pController->SetInputMode(inputModeData);
			pController->SetShowMouseCursor(false);
		}
	}
}