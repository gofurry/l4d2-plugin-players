#if defined _l4d2_players_idle_included
	#endinput
#endif
#define _l4d2_players_idle_included

bool g_LPIdleVerifyPending[MAXPLAYERS + 1];
int g_LPIdleVerifySerial[MAXPLAYERS + 1];
int g_LPIdleVerifyFrames[MAXPLAYERS + 1];
float g_LPIdleVerifyStartedAt[MAXPLAYERS + 1];
bool g_LPIdleImmediate[MAXPLAYERS + 1];
LPIdleReason g_LPIdleVerifyReason[MAXPLAYERS + 1];

void LP_InitializeIdle()
{
	HookEvent("player_death", LP_EventPlayerDeathPre, EventHookMode_Pre);
}

bool LP_CanGoIdle(int client, bool notify)
{
	if (!LP_IsActiveSurvivor(client))
	{
		if (notify) LP_Print(client, "MustBeActiveSurvivor");
		return false;
	}
	if (!IsPlayerAlive(client))
	{
		if (notify) LP_Print(client, "MustBeAlive");
		return false;
	}
	if (LP_IsPinned(client))
	{
		if (notify) LP_Print(client, "CannotIdlePinned");
		return false;
	}
	if (GetClientCount(false) >= MaxClients)
	{
		if (notify) LP_Print(client, "NoClientSlotIdle");
		return false;
	}
	return true;
}

bool LP_GoIdle(int client, LPIdleReason reason, bool notify = true)
{
	if (!g_LPEnabled || LP_IsSpectatePendingForClient(client) || !LP_CanGoIdle(client, notify))
	{
		return false;
	}

	if (g_LPIdleVerifyPending[client])
	{
		return true;
	}

	g_LPIdleVerifyPending[client] = true;
	g_LPIdleVerifySerial[client] = GetClientSerial(client);
	g_LPIdleVerifyFrames[client] = 0;
	g_LPIdleVerifyStartedAt[client] = GetGameTime();
	g_LPIdleVerifyReason[client] = reason;
	// The immediate result is diagnostic only. Linux can finish the native Idle
	// transition over multiple frames.
	g_LPIdleImmediate[client] = LP_EngineGoAwayFromKeyboard(client);

	RequestFrame(LP_FrameVerifyIdle, g_LPIdleVerifySerial[client]);
	LP_Log("%s idle requested for %N.", reason == LP_IDLE_REASON_AUTOMATIC ? "Automatic" : "Manual", client);
	return true;
}

int LP_FindIdleVerificationClient(int serial)
{
	for (int client = 1; client <= MaxClients; client++)
	{
		if (g_LPIdleVerifyPending[client] && g_LPIdleVerifySerial[client] == serial)
		{
			return client;
		}
	}
	return 0;
}

void LP_ResetIdleVerification(int client)
{
	g_LPIdleVerifyPending[client] = false;
	g_LPIdleVerifySerial[client] = 0;
	g_LPIdleVerifyFrames[client] = 0;
	g_LPIdleVerifyStartedAt[client] = 0.0;
	g_LPIdleImmediate[client] = false;
	g_LPIdleVerifyReason[client] = LP_IDLE_REASON_ENGINE;
}

void LP_IdleClientDisconnected(int client)
{
	if (client >= 1 && client <= MaxClients)
	{
		LP_ResetIdleVerification(client);
	}
}

void LP_CancelAllIdleVerifications()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		LP_ResetIdleVerification(client);
	}
}

public void LP_FrameVerifyIdle(any serial)
{
	int stateClient = LP_FindIdleVerificationClient(serial);
	if (stateClient == 0)
	{
		return;
	}

	int currentClient = GetClientFromSerial(serial);
	if (currentClient != stateClient || !LP_IsHumanClient(stateClient))
	{
		LP_ResetIdleVerification(stateClient);
		return;
	}
	if (LP_IsEngineIdle(stateClient))
	{
		LPIdleReason reason = g_LPIdleVerifyReason[stateClient];
		g_LPPlayers[stateClient].idleSince = GetGameTime();
		g_LPPlayers[stateClient].wasIdle = true;
		LP_ClearAfkHint(stateClient);
		LP_ResetIdleVerification(stateClient);
		LP_HumanTeamWipeObserveClient(stateClient);
		LP_Print(stateClient, "NowIdle");
		LP_Log("Idle verified: reason=%s client=%N bot=%d.",
			reason == LP_IDLE_REASON_AUTOMATIC ? "automatic" : "manual",
			stateClient, LP_FindIdleBotForHuman(stateClient));
		return;
	}

	g_LPIdleVerifyFrames[stateClient]++;
	if (g_LPIdleVerifyFrames[stateClient] >= LP_STATE_VERIFY_MAX_FRAMES
		|| GetGameTime() - g_LPIdleVerifyStartedAt[stateClient] >= LP_STATE_VERIFY_TIMEOUT)
	{
		int bot = LP_FindIdleBotForHuman(stateClient);
		LPIdleReason reason = g_LPIdleVerifyReason[stateClient];
		bool sdkImmediate = g_LPIdleImmediate[stateClient];
		LP_LogStateFailure(reason == LP_IDLE_REASON_AUTOMATIC ? "idle_automatic_verify_timeout" : "idle_manual_verify_timeout",
			stateClient, bot, LP_GetBotHumanSpectatorUserId(bot), sdkImmediate ? 1 : 0);
		LP_ResetIdleVerification(stateClient);
		LP_Print(stateClient, "IdleFailed");
		return;
	}
	RequestFrame(LP_FrameVerifyIdle, serial);
}

public Action LP_EventPlayerDeathPre(Event event, const char[] name, bool dontBroadcast)
{
	int bot = GetClientOfUserId(event.GetInt("userid"));
	if (LP_IsHumanClient(bot))
	{
		LP_ClearAfkHint(bot);
		return Plugin_Continue;
	}
	if (!LP_IsSurvivorBot(bot))
	{
		return Plugin_Continue;
	}

	int human = LP_FindIdleHumanForBot(bot);
	if (!LP_IsHumanClient(human))
	{
		return Plugin_Continue;
	}
	LP_HumanTeamWipeRecordIdleBotDeath(human);

	if (!LP_ReturnFromIdleBot(human, bot))
	{
		LP_Print(human, "TakeoverFailed");
	}
	return Plugin_Continue;
}

Action LP_CommandAfk(int client)
{
	if (!LP_IsHumanClient(client))
	{
		return Plugin_Handled;
	}
	LP_RecordActivity(client);
	if (!g_LPEnabled)
	{
		LP_Print(client, "PluginDisabled");
		return Plugin_Handled;
	}
	LP_GoIdle(client, LP_IDLE_REASON_MANUAL, true);
	return Plugin_Handled;
}
