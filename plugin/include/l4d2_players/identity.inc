#if defined _l4d2_players_identity_included
	#endinput
#endif
#define _l4d2_players_identity_included

enum LPIdentityTransferDirection
{
	LP_IDENTITY_TRANSFER_NONE = 0,
	LP_IDENTITY_PLAYER_TO_BOT,
	LP_IDENTITY_BOT_TO_PLAYER
};

LPIdentityTransferDirection g_LPIdentityTransferDirection[MAXPLAYERS + 1];
int g_LPIdentityTransferHumanSerial[MAXPLAYERS + 1];
int g_LPIdentityTransferBotUserId[MAXPLAYERS + 1];
int g_LPIdentityTransferCharacter[MAXPLAYERS + 1];
char g_LPIdentityTransferModel[MAXPLAYERS + 1][PLATFORM_MAX_PATH];

bool g_LPIdentityVerifyPending[MAXPLAYERS + 1];
int g_LPIdentityVerifyUserId[MAXPLAYERS + 1];
int g_LPIdentityVerifyCharacter[MAXPLAYERS + 1];
char g_LPIdentityVerifyModel[MAXPLAYERS + 1][PLATFORM_MAX_PATH];
LPIdentityTransferDirection g_LPIdentityVerifyDirection[MAXPLAYERS + 1];

void LP_InitializeIdentityLifecycle()
{
	HookEvent("player_bot_replace", LP_IdentityPlayerBotReplacePre, EventHookMode_Pre);
	HookEvent("player_bot_replace", LP_IdentityPlayerBotReplacePost, EventHookMode_Post);
	HookEvent("bot_player_replace", LP_IdentityBotPlayerReplacePre, EventHookMode_Pre);
	HookEvent("bot_player_replace", LP_IdentityBotPlayerReplacePost, EventHookMode_Post);
	LP_ResetAllIdentityLifecycleState();
}

bool LP_CaptureSurvivorIdentity(int client, int &character, char[] model, int modelLength)
{
	if (client < 1 || client > MaxClients || !IsClientInGame(client)
		|| !HasEntProp(client, Prop_Send, "m_survivorCharacter"))
	{
		return false;
	}

	character = GetEntProp(client, Prop_Send, "m_survivorCharacter");
	if (!LP_IsValidSurvivorCharacter(character))
	{
		return false;
	}

	char actualModel[PLATFORM_MAX_PATH];
	char expectedModel[PLATFORM_MAX_PATH];
	GetClientModel(client, actualModel, sizeof(actualModel));
	LP_GetSurvivorCharacterModel(character, expectedModel, sizeof(expectedModel));
	if (!StrEqual(actualModel, expectedModel, false))
	{
		LP_LogError("Identity capture repaired non-Survivor or mismatched model: client=%d character=%d actual_model=%s canonical_model=%s.",
			client, character, actualModel, expectedModel);
	}
	strcopy(model, modelLength, expectedModel);
	return true;
}

bool LP_ApplyTransferredSurvivorIdentity(int client, int character, const char[] expectedModel)
{
	if (!LP_ApplySurvivorIdentity(client, character))
	{
		return false;
	}
	char actualModel[PLATFORM_MAX_PATH];
	GetClientModel(client, actualModel, sizeof(actualModel));
	return StrEqual(actualModel, expectedModel, false)
		&& LP_VerifySurvivorIdentity(client, character);
}

bool LP_InitializeCreatedSurvivorIdentity(int bot, int character)
{
	if (!LP_IsSurvivorBot(bot))
	{
		LP_LogError("Created Survivor identity initialization failed: bot=%d character=%d stage=not_survivor_bot.",
			bot, character);
		return false;
	}

	char model[PLATFORM_MAX_PATH];
	LP_GetSurvivorCharacterModel(character, model, sizeof(model));
	if (!LP_ApplyTransferredSurvivorIdentity(bot, character, model))
	{
		char actualModel[PLATFORM_MAX_PATH];
		GetClientModel(bot, actualModel, sizeof(actualModel));
		int actualCharacter = HasEntProp(bot, Prop_Send, "m_survivorCharacter")
			? GetEntProp(bot, Prop_Send, "m_survivorCharacter") : -1;
		LP_LogError("Created Survivor identity initialization failed: bot=%d character=%d stage=apply_or_verify actual_character=%d actual_model=%s.",
			bot, character, actualCharacter, actualModel);
		return false;
	}
	return true;
}

void LP_ResetIdentityTransfer(int human)
{
	if (human < 1 || human > MaxClients)
	{
		return;
	}
	g_LPIdentityTransferDirection[human] = LP_IDENTITY_TRANSFER_NONE;
	g_LPIdentityTransferHumanSerial[human] = 0;
	g_LPIdentityTransferBotUserId[human] = 0;
	g_LPIdentityTransferCharacter[human] = -1;
	g_LPIdentityTransferModel[human][0] = '\0';
}

void LP_CancelIdentityVerification(int client)
{
	if (client < 1 || client > MaxClients)
	{
		return;
	}
	g_LPIdentityVerifyPending[client] = false;
	g_LPIdentityVerifyUserId[client] = 0;
	g_LPIdentityVerifyCharacter[client] = -1;
	g_LPIdentityVerifyModel[client][0] = '\0';
	g_LPIdentityVerifyDirection[client] = LP_IDENTITY_TRANSFER_NONE;
}

void LP_ResetAllIdentityLifecycleState()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		LP_ResetIdentityTransfer(client);
		LP_CancelIdentityVerification(client);
	}
}

void LP_IdentityClientDisconnected(int client)
{
	LP_ResetIdentityTransfer(client);
	LP_CancelIdentityVerification(client);
}

void LP_ScheduleIdentityVerification(int target, int character, const char[] model,
	LPIdentityTransferDirection direction)
{
	if (target < 1 || target > MaxClients || !IsClientInGame(target))
	{
		return;
	}
	g_LPIdentityVerifyPending[target] = true;
	g_LPIdentityVerifyUserId[target] = GetClientUserId(target);
	g_LPIdentityVerifyCharacter[target] = character;
	strcopy(g_LPIdentityVerifyModel[target], PLATFORM_MAX_PATH, model);
	g_LPIdentityVerifyDirection[target] = direction;
	RequestFrame(LP_FrameVerifyTransferredIdentity, GetClientUserId(target));
}

public void LP_FrameVerifyTransferredIdentity(any userId)
{
	int stateTarget;
	for (int client = 1; client <= MaxClients; client++)
	{
		if (g_LPIdentityVerifyPending[client] && g_LPIdentityVerifyUserId[client] == userId)
		{
			stateTarget = client;
			break;
		}
	}
	if (stateTarget == 0)
	{
		return;
	}
	int target = GetClientOfUserId(userId);
	if (target != stateTarget || !IsClientInGame(stateTarget))
	{
		LP_CancelIdentityVerification(stateTarget);
		return;
	}

	int character = g_LPIdentityVerifyCharacter[target];
	LPIdentityTransferDirection direction = g_LPIdentityVerifyDirection[target];
	char model[PLATFORM_MAX_PATH];
	strcopy(model, sizeof(model), g_LPIdentityVerifyModel[target]);
	bool repaired = false;
	bool success = LP_VerifySurvivorIdentity(target, character);
	if (!success)
	{
		repaired = true;
		success = LP_ApplyTransferredSurvivorIdentity(target, character, model);
	}
	LP_CancelIdentityVerification(target);
	if (success)
	{
		LP_Log("Identity verification succeeded: direction=%s target=%N character=%d model=%s repaired=%d.",
			direction == LP_IDENTITY_PLAYER_TO_BOT ? "player_to_bot" : "bot_to_player",
			target, character, model, repaired ? 1 : 0);
	}
	else
	{
		LP_LogError("FATAL identity verification failed: direction=%d target=%d character=%d model=%s.",
			direction, target, character, model);
	}
}

void LP_BeginIdentityTransfer(Event event, LPIdentityTransferDirection direction)
{
	int human = GetClientOfUserId(event.GetInt("player"));
	int bot = GetClientOfUserId(event.GetInt("bot"));
	if (!LP_IsHumanClient(human) || bot < 1 || bot > MaxClients || !IsClientInGame(bot))
	{
		return;
	}

	int source = direction == LP_IDENTITY_PLAYER_TO_BOT ? human : bot;
	int character;
	char model[PLATFORM_MAX_PATH];
	if (!LP_CaptureSurvivorIdentity(source, character, model, sizeof(model)))
	{
		LP_LogError("FATAL identity transfer capture failed: direction=%d human=%d bot=%d.",
			direction, human, bot);
		return;
	}

	g_LPIdentityTransferDirection[human] = direction;
	g_LPIdentityTransferHumanSerial[human] = GetClientSerial(human);
	g_LPIdentityTransferBotUserId[human] = GetClientUserId(bot);
	g_LPIdentityTransferCharacter[human] = character;
	strcopy(g_LPIdentityTransferModel[human], PLATFORM_MAX_PATH, model);
}

bool LP_FinishIdentityTransfer(Event event, LPIdentityTransferDirection direction)
{
	int human = GetClientOfUserId(event.GetInt("player"));
	int botUserId = event.GetInt("bot");
	int bot = GetClientOfUserId(botUserId);
	bool liveBotRequired = direction == LP_IDENTITY_PLAYER_TO_BOT;
	if (human < 1 || human > MaxClients || (liveBotRequired && (bot < 1 || bot > MaxClients))
		|| g_LPIdentityTransferDirection[human] != direction
		|| g_LPIdentityTransferHumanSerial[human] != GetClientSerial(human)
		|| g_LPIdentityTransferBotUserId[human] != botUserId)
	{
		LP_LogError("FATAL identity transfer event mismatch: direction=%d human=%d bot=%d bot_userid=%d.",
			direction, human, bot, botUserId);
		return false;
	}

	int character = g_LPIdentityTransferCharacter[human];
	char model[PLATFORM_MAX_PATH];
	strcopy(model, sizeof(model), g_LPIdentityTransferModel[human]);
	int target = direction == LP_IDENTITY_PLAYER_TO_BOT ? bot : human;
	bool success = LP_ApplyTransferredSurvivorIdentity(target, character, model);
	if (direction == LP_IDENTITY_PLAYER_TO_BOT)
	{
		LP_Log("Identity player->bot: human=%N bot=%d character=%d model=%s success=%d.",
			human, bot, character, model, success ? 1 : 0);
		LP_IdleObserveReplacementBot(human, bot, character, model);
	}
	else
	{
		LP_Log("Identity bot->player: bot=%d bot_userid=%d human=%N character=%d model=%s success=%d.",
			bot, botUserId, human, character, model, success ? 1 : 0);
	}
	LP_ScheduleIdentityVerification(target, character, model, direction);
	LP_ResetIdentityTransfer(human);
	return success;
}

public Action LP_IdentityPlayerBotReplacePre(Event event, const char[] name, bool dontBroadcast)
{
	LP_BeginIdentityTransfer(event, LP_IDENTITY_PLAYER_TO_BOT);
	return Plugin_Continue;
}

public void LP_IdentityPlayerBotReplacePost(Event event, const char[] name, bool dontBroadcast)
{
	LP_FinishIdentityTransfer(event, LP_IDENTITY_PLAYER_TO_BOT);
}

public Action LP_IdentityBotPlayerReplacePre(Event event, const char[] name, bool dontBroadcast)
{
	LP_BeginIdentityTransfer(event, LP_IDENTITY_BOT_TO_PLAYER);
	return Plugin_Continue;
}

public void LP_IdentityBotPlayerReplacePost(Event event, const char[] name, bool dontBroadcast)
{
	LP_FinishIdentityTransfer(event, LP_IDENTITY_BOT_TO_PLAYER);
}
