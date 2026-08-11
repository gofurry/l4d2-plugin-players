#if defined _l4d2_players_survivor_engine_included
	#endinput
#endif
#define _l4d2_players_survivor_engine_included

enum LPTakeoverPath
{
	LP_TAKEOVER_PATH_NONE = 0,
	LP_TAKEOVER_PATH_RETURN_IDLE,
	LP_TAKEOVER_PATH_BIND_FREE_BOT
};

int g_LPBotReservation[MAXPLAYERS + 1];
bool g_LPTakeoverPending[MAXPLAYERS + 1];
int g_LPTakeoverSerial[MAXPLAYERS + 1];
int g_LPTakeoverBot[MAXPLAYERS + 1];
int g_LPTakeoverInitialHumanSpec[MAXPLAYERS + 1];
int g_LPTakeoverFrames[MAXPLAYERS + 1];
float g_LPTakeoverStartedAt[MAXPLAYERS + 1];
bool g_LPTakeoverImmediate[MAXPLAYERS + 1];
bool g_LPBindImmediate[MAXPLAYERS + 1];
bool g_LPTakeoverNewlyCreatedBot[MAXPLAYERS + 1];
LPTakeoverPath g_LPTakeoverPath[MAXPLAYERS + 1];

int LP_GetBotHumanSpectatorUserId(int bot, int fallback = -1)
{
	if (bot >= 1 && bot <= MaxClients && IsClientInGame(bot)
		&& HasEntProp(bot, Prop_Send, "m_humanSpectatorUserID"))
	{
		return GetEntProp(bot, Prop_Send, "m_humanSpectatorUserID");
	}
	return fallback;
}

void LP_LogStateFailure(const char[] stage, int client, int bot, int botHumanSpec, int sdkImmediate, int bindImmediate = -1)
{
	int team = -1;
	if (client >= 1 && client <= MaxClients && IsClientInGame(client))
	{
		team = GetClientTeam(client);
	}

	int currentHumanSpec = LP_GetBotHumanSpectatorUserId(bot, botHumanSpec);
	LP_LogError("State failure: stage=%s client=%d team=%d bot=%d bot_m_humanSpectatorUserID=%d sdk_immediate=%d bind_immediate=%d.",
		stage, client, team, bot, currentHumanSpec, sdkImmediate, bindImmediate);
}

bool LP_BindHumanToBot(int client, int bot)
{
	if (!LP_IsHumanClient(client) || !LP_IsSurvivorBot(bot))
	{
		return false;
	}
	return LP_EngineSetHumanSpectator(bot, client);
}

void LP_ResetTakeoverState(int client)
{
	g_LPTakeoverPending[client] = false;
	g_LPTakeoverSerial[client] = 0;
	g_LPTakeoverBot[client] = 0;
	g_LPTakeoverInitialHumanSpec[client] = -1;
	g_LPTakeoverFrames[client] = 0;
	g_LPTakeoverStartedAt[client] = 0.0;
	g_LPTakeoverImmediate[client] = false;
	g_LPBindImmediate[client] = false;
	g_LPTakeoverNewlyCreatedBot[client] = false;
	g_LPTakeoverPath[client] = LP_TAKEOVER_PATH_NONE;
}

bool LP_HasPendingTakeover()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		if (g_LPTakeoverPending[client])
		{
			return true;
		}
	}
	return false;
}

bool LP_IsTakeoverPendingForClient(int client)
{
	return client >= 1 && client <= MaxClients && g_LPTakeoverPending[client];
}

int LP_FindPendingTakeoverClient(int serial)
{
	for (int client = 1; client <= MaxClients; client++)
	{
		if (g_LPTakeoverPending[client] && g_LPTakeoverSerial[client] == serial)
		{
			return client;
		}
	}
	return 0;
}

void LP_CancelTakeoverForClient(int client)
{
	if (client < 1 || client > MaxClients || !g_LPTakeoverPending[client])
	{
		return;
	}

	int bot = g_LPTakeoverBot[client];
	if (bot >= 1 && bot <= MaxClients)
	{
		g_LPBotReservation[bot] = 0;
	}
	LP_ResetTakeoverState(client);
}

void LP_CancelAllTakeovers()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		LP_CancelTakeoverForClient(client);
	}
}

bool LP_BeginTakeover(int client, int bot, LPTakeoverPath path, bool newlyCreatedBot = false)
{
	if (!LP_IsHumanClient(client) || !LP_IsSurvivorBot(bot) || LP_IsTakeoverPendingForClient(client))
	{
		LP_LogStateFailure("takeover_precondition", client, bot, -1, -1);
		return false;
	}
	if (path == LP_TAKEOVER_PATH_RETURN_IDLE
		&& (!LP_IsEngineIdle(client) || LP_FindIdleBotForHuman(client) != bot))
	{
		LP_LogStateFailure("return_from_idle_precondition", client, bot, LP_GetBotHumanSpectatorUserId(bot), -1);
		return false;
	}

	g_LPTakeoverPending[client] = true;
	g_LPTakeoverSerial[client] = GetClientSerial(client);
	g_LPTakeoverBot[client] = bot;
	g_LPTakeoverInitialHumanSpec[client] = LP_GetBotHumanSpectatorUserId(bot);
	g_LPTakeoverFrames[client] = 0;
	g_LPTakeoverStartedAt[client] = GetGameTime();
	g_LPTakeoverPath[client] = path;
	g_LPTakeoverNewlyCreatedBot[client] = path == LP_TAKEOVER_PATH_BIND_FREE_BOT && newlyCreatedBot;
	g_LPBindImmediate[client] = true;
	g_LPBotReservation[bot] = g_LPTakeoverSerial[client];

	if (path == LP_TAKEOVER_PATH_BIND_FREE_BOT)
	{
		ChangeClientTeam(client, LP_TEAM_UNASSIGNED);
		g_LPBindImmediate[client] = LP_BindHumanToBot(client, bot);
	}

	// The engine can complete this state transition asynchronously on Linux.
	// The immediate return is diagnostic only; final success is Active Survivor state.
	g_LPTakeoverImmediate[client] = LP_EngineTakeOverBot(client);
	RequestFrame(LP_FrameVerifyTakeover, g_LPTakeoverSerial[client]);
	return true;
}

bool LP_ReturnFromIdleBot(int client, int bot)
{
	return LP_BeginTakeover(client, bot, LP_TAKEOVER_PATH_RETURN_IDLE);
}

bool LP_BindAndTakeOverBot(int client, int bot, bool newlyCreatedBot = false)
{
	return LP_BeginTakeover(client, bot, LP_TAKEOVER_PATH_BIND_FREE_BOT, newlyCreatedBot);
}

void LP_FinishTakeover(int client, bool success)
{
	int bot = g_LPTakeoverBot[client];
	int initialHumanSpec = g_LPTakeoverInitialHumanSpec[client];
	bool sdkImmediate = g_LPTakeoverImmediate[client];
	bool bindImmediate = g_LPBindImmediate[client];
	bool newlyCreatedBot = g_LPTakeoverNewlyCreatedBot[client];
	LPTakeoverPath path = g_LPTakeoverPath[client];

	if (bot >= 1 && bot <= MaxClients)
	{
		g_LPBotReservation[bot] = 0;
	}
	LP_ResetTakeoverState(client);

	if (success)
	{
		LP_RecordActivity(client);
		LP_StopAutoIdleHud(client);
		LP_ClearIdleKickHint(client);
		LP_HumanTeamWipeObserveClient(client);
		LP_Print(client, "JoinedGame");
		LP_Log("Takeover verified: path=%s bot_source=%s client=%N bot=%d.",
			path == LP_TAKEOVER_PATH_RETURN_IDLE ? "return_idle" : "bind_free_bot",
			path == LP_TAKEOVER_PATH_RETURN_IDLE ? "owned_idle_bot" : (newlyCreatedBot ? "newly_created_bot" : "existing_bot"),
			client, bot);
		if (newlyCreatedBot)
		{
			LP_ScheduleMidJoinSetup(client);
		}
	}
	else
	{
		LP_LogStateFailure(path == LP_TAKEOVER_PATH_RETURN_IDLE ? "return_from_idle_verify_timeout" : "bind_takeover_verify_timeout",
			client, bot, initialHumanSpec, sdkImmediate ? 1 : 0, bindImmediate ? 1 : 0);
		LP_Print(client, "TakeoverFailed");
		if (path == LP_TAKEOVER_PATH_BIND_FREE_BOT && LP_IsHumanClient(client) && !LP_IsActiveSurvivor(client))
		{
			ChangeClientTeam(client, LP_TEAM_SPECTATOR);
		}
	}

	LP_ProcessJoinQueue();
}

public void LP_FrameVerifyTakeover(any serial)
{
	int stateClient = LP_FindPendingTakeoverClient(serial);
	if (stateClient == 0)
	{
		return;
	}

	int currentClient = GetClientFromSerial(serial);
	if (currentClient != stateClient || !LP_IsHumanClient(stateClient))
	{
		LP_CancelTakeoverForClient(stateClient);
		LP_ProcessJoinQueue();
		return;
	}
	if (LP_IsActiveSurvivor(stateClient))
	{
		LP_FinishTakeover(stateClient, true);
		return;
	}

	g_LPTakeoverFrames[stateClient]++;
	if (g_LPTakeoverFrames[stateClient] >= LP_STATE_VERIFY_MAX_FRAMES
		|| GetGameTime() - g_LPTakeoverStartedAt[stateClient] >= LP_STATE_VERIFY_TIMEOUT)
	{
		LP_FinishTakeover(stateClient, false);
		return;
	}
	RequestFrame(LP_FrameVerifyTakeover, serial);
}

int LP_FindAvailableSurvivorBot()
{
	for (int bot = 1; bot <= MaxClients; bot++)
	{
		if (!LP_IsSurvivorBot(bot) || !IsPlayerAlive(bot) || g_LPBotReservation[bot] != 0)
		{
			continue;
		}
		if (LP_FindIdleHumanForBot(bot) == 0)
		{
			return bot;
		}
	}
	return 0;
}

bool LP_InitializeCreatedSurvivorIdentity(int bot, int character)
{
	if (!LP_IsSurvivorBot(bot))
	{
		LP_LogError("Created Survivor identity initialization failed: bot=%d character=%d stage=not_survivor_bot.",
			bot, character);
		return false;
	}
	if (!LP_ApplySurvivorIdentity(bot, character))
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

int LP_CreateUnlimitedSurvivorBot(const char[] context)
{
	if (!g_LPEnabled || g_LPMapEnding)
	{
		LP_LogError("Unlimited Survivor Bot creation rejected: context=%s plugin_enabled=%d map_ending=%d.",
			context, g_LPEnabled ? 1 : 0, g_LPMapEnding ? 1 : 0);
		return 0;
	}
	if (!LP_EngineIsReady())
	{
		LP_LogError("Unlimited Survivor Bot creation failed: context=%s required SDKCalls are unavailable.", context);
		return 0;
	}

	int survivorCount = LP_GetSurvivorCount();
	int survivorCapacity = LP_GetConfiguredSurvivorCapacity();
	if (survivorCount >= survivorCapacity)
	{
		LP_LogError("Unlimited Survivor Bot creation rejected: context=%s survivor_count=%d Players_limit=%d.",
			context, survivorCount, survivorCapacity);
		return 0;
	}
	if (GetClientCount(false) >= MaxClients)
	{
		LP_LogError("Unlimited Survivor Bot creation failed: context=%s no client slot (clients=%d MaxClients=%d).",
			context, GetClientCount(false), MaxClients);
		return 0;
	}

	int bot = LP_EngineCreateSurvivorBot("Players Survivor Bot");
	if (bot < 1 || bot > MaxClients || !IsValidEntity(bot))
	{
		LP_LogError("Unlimited Survivor Bot creation failed: context=%s NextBot returned invalid client index %d.", context, bot);
		return 0;
	}
	if (!IsClientInGame(bot) || !IsFakeClient(bot))
	{
		LP_LogError("Unlimited Survivor Bot creation failed: context=%s client=%d in_game=%d fake=%d.",
			context, bot, IsClientInGame(bot) ? 1 : 0, IsClientInGame(bot) && IsFakeClient(bot) ? 1 : 0);
		return 0;
	}

	ChangeClientTeam(bot, LP_TEAM_SURVIVOR);
	if (GetClientTeam(bot) != LP_TEAM_SURVIVOR)
	{
		LP_LogError("Unlimited Survivor Bot creation failed: context=%s bot=%d team transition produced team=%d.",
			context, bot, GetClientTeam(bot));
		return 0;
	}

	int character = LP_GetNextCreatedSurvivorCharacter();
	if (!LP_InitializeCreatedSurvivorIdentity(bot, character))
	{
		return 0;
	}
	bool respawned = false;
	if (!IsPlayerAlive(bot))
	{
		LP_EngineRoundRespawn(bot);
		respawned = true;
	}
	if (!IsPlayerAlive(bot))
	{
		LP_LogError("Unlimited Survivor Bot creation failed: context=%s bot=%d RoundRespawn did not make the Bot alive.",
			context, bot);
		return 0;
	}
	if (respawned && !LP_InitializeCreatedSurvivorIdentity(bot, character))
	{
		LP_LogError("Unlimited Survivor Bot creation failed: context=%s bot=%d identity was reset by RoundRespawn.",
			context, bot);
		return 0;
	}
	if (!LP_VerifySurvivorIdentity(bot, character))
	{
		LP_LogError("Unlimited Survivor Bot creation failed: context=%s bot=%d stage=final_identity_verification character=%d.",
			context, bot, character);
		return 0;
	}

	char model[PLATFORM_MAX_PATH];
	LP_GetSurvivorCharacterModel(character, model, sizeof(model));
	LP_Log("Created Survivor identity initialized: bot=%d character=%d model=%s.", bot, character, model);

	LP_Log("Unlimited Survivor Bot created: context=%s bot=%d survivors=%d/%d clients=%d/%d.",
		context, bot, LP_GetSurvivorCount(), survivorCapacity, GetClientCount(false), MaxClients);
	return bot;
}

bool LP_CanCreateSurvivorBot(int client)
{
	if (!LP_IsHumanClient(client))
	{
		return false;
	}
	if (!LP_EngineIsReady())
	{
		LP_LogError("Join creation precheck failed for client=%d: required SDKCalls are unavailable.", client);
		LP_Print(client, "JoinFailed");
		return false;
	}
	if (LP_GetSurvivorCount() >= LP_GetConfiguredSurvivorCapacity())
	{
		LP_Print(client, "SurvivorLimitReached");
		return false;
	}
	if (GetClientCount(false) >= MaxClients)
	{
		LP_LogError("Join creation precheck failed for client=%d: no client slot (clients=%d MaxClients=%d).",
			client, GetClientCount(false), MaxClients);
		LP_Print(client, "NoClientSlot");
		return false;
	}
	return true;
}
