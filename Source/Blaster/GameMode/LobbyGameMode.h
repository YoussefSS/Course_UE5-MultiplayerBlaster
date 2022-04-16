// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:

	/* Called when a player joins the game
	The first place where it's safe to access the player that joined the game*/ 
	virtual void PostLogin(APlayerController* NewPlayer) override;

};
