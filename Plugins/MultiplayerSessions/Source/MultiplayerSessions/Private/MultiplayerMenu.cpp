// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerMenu.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

void UMultiplayerMenu::MultiplayerMenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString ListenLobby)
{
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	LobbyMap = FString::Printf(TEXT("%s?listen"), *ListenLobby);
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	UWorld* World = GetWorld();
	if (World) {
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController) {
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance) {
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (MultiplayerSessionsSubsystem) {
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
	}
}

bool UMultiplayerMenu::Initialize()
{
	if (!Super::Initialize()) {
		return false;
	}

	if (HostButton) {
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}

	if (JoinButton) {
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}

	return true;
}

void UMultiplayerMenu::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	MenuTearDown();
	Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}

void UMultiplayerMenu::OnCreateSession(bool bWasSuccessful)
{
	if (bWasSuccessful) {

		// DebugMsg - Session Creation Success
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(
				-1,
				60.f,
				FColor::Yellow,
				FString(TEXT("Session Created Successfully"))
			);
		}

		UWorld* World = GetWorld();
		if (World) {
			World->ServerTravel(LobbyMap);
		}
	}
	else {

		// DebugMsg - Session Creation Fail
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(
				-1,
				60.f,
				FColor::Red,
				FString(TEXT("Failed to create session"))
			);
		}

		HostButton->SetIsEnabled(true);
	}
}

void UMultiplayerMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	if (!bWasSuccessful) {
		JoinButton->SetIsEnabled(true);
	}

	// Subsystem valid check
	if (MultiplayerSessionsSubsystem == nullptr) {
		JoinButton->SetIsEnabled(true);
		return;
	}

	// Get session search results
	for (auto Result : SessionResults) {
		FString Id = Result.GetSessionIdStr();
		FString SettingsValue;
		Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);

		if (SettingsValue == MatchType) {
			MultiplayerSessionsSubsystem->JoinSession(Result);
			if (GEngine) {
				GEngine->AddOnScreenDebugMessage(
					-1,
					60.f,
					FColor::Blue,
					FString::Printf(TEXT("Found Session: %s"), *Id)
				);
			}
			return;
		}
	}
}

void UMultiplayerMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::UnknownError) {
		JoinButton->SetIsEnabled(true);
	}

	// Get a copy of Session Interface from Subsystem
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem) {
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

		if (SessionInterface.IsValid()) {

			// Get resolved connect string
			FString Address;
			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);
			if (GEngine) {
				GEngine->AddOnScreenDebugMessage(
					-1,
					60.f,
					FColor::Blue,
					FString::Printf(TEXT("Target Address: %s"), *Address)
				);
			}

			// Call client travel
			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController) {
				PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
			}
		}
	}
}

void UMultiplayerMenu::OnDestroySession(bool bWasSuccessful)
{
}

void UMultiplayerMenu::OnStartSession(bool bWasSuccessful)
{

}

void UMultiplayerMenu::HostButtonClicked()
{
	HostButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem) {
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMultiplayerMenu::JoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(
			-1,
			60.f,
			FColor::Yellow,
			FString(TEXT("Join Button Clicked"))
		);
	}

	if (MultiplayerSessionsSubsystem) {
		MultiplayerSessionsSubsystem->FindSessions(10000);
	}
}

void UMultiplayerMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World) {
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController) {
			FInputModeGameOnly InputDataMode;
			PlayerController->SetInputMode(InputDataMode);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
