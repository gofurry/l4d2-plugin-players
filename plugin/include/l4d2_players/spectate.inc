#if defined _l4d2_players_spectate_included
	#endinput
#endif
#define _l4d2_players_spectate_included

bool g_LPSpectatePending[MAXPLAYERS + 1];
int g_LPSpectateSerial[MAXPLAYERS + 1];
int g_LPSpectateBotUserId[MAXPLAYERS + 1];
int g_LPSpectateFrames[MAXPLAYERS + 1];
int g_LPSpectateStableFrames[MAXPLAYERS + 1];
float g_LPSpectateStartedAt[MAXPLAYERS + 1];
bool g_LPSpectateUnbindImmediate[MAXPLAYERS + 1];

void LP_InitializeSpectate()
{
	LP_CancelAllSpectateTransitions();
}

void LP_ResetSpectateTransition(int client)
{
	g_LPSpectatePending[client] = false;
	g_LPSpectateSerial[client] = 0;
	g_LPSpectateBotUserId[client] = 0;
	g_LPSpectateFrames[client] = 0;
	g_LPSpectateStableFrames[client] = 0;
	g_LPSpectateStartedAt[client] = 0.0;
	g_LPSpectateUnbindImmediate[client] = false;
}

bool LP_IsSpectatePendingForClient(int client)
{
	return client >= 1 && client <= MaxClients && g_LPSpectatePending[client];
}

int LP_FindSpectateTransitionClient(int serial)
{
	for (int client = 1; client <= MaxClients; client++)
	{
		if (g_LPSpectatePending[client] && g_LPSpectateSerial[client] == serial)
		{
			return client;
		}
	}
	return 0;
}

bool LP_ClearHumanSpectatorBinding(int client, int bot)
{
	if (!LP_IsHumanClient(client) || !LP_IsSurvivorBot(bot)
		|| !HasEntProp(bot, Prop_Send, "m_humanSpectatorUserID"))
	{
		return false;
	}

	int humanUserId = GetClientUserId(client);
	int boundUserId = GetEntProp(bot, Prop_Send, "m_humanSpectatorUserID");
	if (boundUserId == 0)
	{
		return true;
	}
	if (boundUserId != humanUserId)
	{
		return false;
	}

	SetEntProp(bot, Prop_Send, "m_humanSpectatorUserID", 0);
	return GetEntProp(bot, Prop_Send, "m_humanSpectatorUserID") == 0;
}

void LP_CancelSpectateTransition(int client)
{
	if (client >= 1 && client <= MaxClients)
	{
		LP_ResetSpectateTransition(client);
	}
}

void LP_CancelAllSpectateTransitions()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		LP_ResetSpectateTransition(client);
	}
}

void LP_SpectateClientDisconnected(int client)
{
	LP_CancelSpectateTransition(client);
}

bool LP_BeginFreeSpectate(int client)
{
	if (!g_LPEnabled || !LP_IsHumanClient(client) || g_LPMapEnding)
	{
		return false;
	}
	if (LP_IsFreeSpectator(client))
	{
		LP_Print(client, "AlreadySpectating");
		return true;
	}
	if (LP_IsSpectatePendingForClient(client) || LP_IsTakeoverPendingForClient(client)
		|| g_LPIdleVerifyPending[client] || LP_IsJoinQueued(GetClientSerial(client)))
	{
		LP_Print(client, "SpectatePending");
		return false;
	}

	int bot = LP_FindIdleBotForHuman(client);
	g_LPSpectatePending[client] = true;
	g_LPSpectateSerial[client] = GetClientSerial(client);
	g_LPSpectateBotUserId[client] = bot > 0 ? GetClientUserId(bot) : 0;
	g_LPSpectateFrames[client] = 0;
	g_LPSpectateStableFrames[client] = 0;
	g_LPSpectateStartedAt[client] = GetGameTime();
	g_LPSpectateUnbindImmediate[client] = bot == 0 || LP_ClearHumanSpectatorBinding(client, bot);

	if (GetClientTeam(client) != LP_TEAM_SPECTATOR)
	{
		ChangeClientTeam(client, LP_TEAM_SPECTATOR);
	}
	RequestFrame(LP_FrameVerifyFreeSpectate, g_LPSpectateSerial[client]);
	return true;
}

void LP_FinishFreeSpectate(int client, bool success)
{
	int originalBotUserId = g_LPSpectateBotUserId[client];
	bool unbindImmediate = g_LPSpectateUnbindImmediate[client];
	int originalBot = GetClientOfUserId(originalBotUserId);
	int currentBot = LP_FindIdleBotForHuman(client);
	LP_ResetSpectateTransition(client);

	if (success)
	{
		g_LPPlayers[client].lastActivityTime = GetGameTime();
		g_LPPlayers[client].idleSince = 0.0;
		g_LPPlayers[client].wasIdle = false;
		LP_StopAutoIdleHud(client);
		LP_ClearIdleKickHint(client);
		LP_HumanTeamWipeLeaveForFreeSpectator(client);
		LP_Print(client, "NowSpectating");
		LP_Log("Free spectator verified: client=%N released_bot=%d.", client, originalBot);
		return;
	}

	LP_LogStateFailure("free_spectator_verify_timeout", client,
		currentBot > 0 ? currentBot : originalBot,
		LP_GetBotHumanSpectatorUserId(currentBot > 0 ? currentBot : originalBot),
		unbindImmediate ? 1 : 0);
	LP_Print(client, "SpectateFailed");
}

public void LP_FrameVerifyFreeSpectate(any serial)
{
	int stateClient = LP_FindSpectateTransitionClient(serial);
	if (stateClient == 0)
	{
		return;
	}

	int currentClient = GetClientFromSerial(serial);
	if (currentClient != stateClient || !LP_IsHumanClient(stateClient))
	{
		LP_ResetSpectateTransition(stateClient);
		return;
	}

	int boundBot = LP_FindIdleBotForHuman(stateClient);
	if (boundBot > 0)
	{
		LP_ClearHumanSpectatorBinding(stateClient, boundBot);
		g_LPSpectateStableFrames[stateClient] = 0;
	}

	int originalBotUserId = g_LPSpectateBotUserId[stateClient];
	int originalBot = GetClientOfUserId(originalBotUserId);
	bool originalBotPreserved = originalBotUserId == 0 || LP_IsSurvivorBot(originalBot);
	bool freeSpectatorState = GetClientTeam(stateClient) == LP_TEAM_SPECTATOR
		&& LP_FindIdleBotForHuman(stateClient) == 0 && originalBotPreserved;
	if (freeSpectatorState && boundBot == 0)
	{
		g_LPSpectateStableFrames[stateClient]++;
		if (g_LPSpectateStableFrames[stateClient] >= LP_SPECTATE_STABLE_FRAMES)
		{
			LP_FinishFreeSpectate(stateClient, true);
			return;
		}
	}
	else
	{
		g_LPSpectateStableFrames[stateClient] = 0;
	}

	g_LPSpectateFrames[stateClient]++;
	if (g_LPSpectateFrames[stateClient] >= LP_STATE_VERIFY_MAX_FRAMES
		|| GetGameTime() - g_LPSpectateStartedAt[stateClient] >= LP_STATE_VERIFY_TIMEOUT)
	{
		LP_FinishFreeSpectate(stateClient, false);
		return;
	}
	RequestFrame(LP_FrameVerifyFreeSpectate, serial);
}

Action LP_CommandSpectate(int client)
{
	if (!LP_IsHumanClient(client))
	{
		return Plugin_Handled;
	}
	LP_SuppressAutoJoinForConnection(client);
	LP_RecordActivity(client);
	LP_StopAutoIdleHud(client);
	if (!g_LPEnabled)
	{
		LP_Print(client, "PluginDisabled");
		return Plugin_Handled;
	}
	LP_BeginFreeSpectate(client);
	return Plugin_Handled;
}
