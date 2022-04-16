// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num(); // GameState Has an array of players that have joined the game, more specifically, PlayerStates. PlayerArray has the PlayerStates
	if (NumberOfPlayers == 2)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			bUseSeamlessTravel = true; // To travel seamlessly

			// We are in the gamemode which only exists on the server, so there is no need to check if we are the server
			World->ServerTravel(FString("/Game/Maps/BlasterMap?listen")); // Since we are on the server, we can just pass a path to the level we want to travel to. We also specify it as a listen server
		}
	}

}