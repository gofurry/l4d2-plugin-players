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
int g_LPIdleExpectedBotUserId[MAXPLAYERS + 1];
int g_LPIdleOriginalCharacter[MAXPLAYERS + 1];
char g_LPIdleOriginalModel[MAXPLAYERS + 1][PLATFORM_MAX_PATH];
bool g_LPIdleRollbackPending[MAXPLAYERS + 1];

bool LP_IsAutomaticIdleVerificationPending(int client)
{
	return client >= 1 && client <= MaxClients && g_LPIdleVerifyPending[client]
		&& !g_LPIdleRollbackPending[client]
		&& g_LPIdleVerifyReason[client] == LP_IDLE_REASON_AUTOMATIC;
}

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
	if (reason == LP_IDLE_REASON_AUTOMATIC)
	{
		LP_StopAutoIdleHud(client);
	}

	if (g_LPIdleVerifyPending[client])
	{
		return true;
	}
	int originalCharacter;
	char originalModel[PLATFORM_MAX_PATH];
	if (!LP_CaptureSurvivorIdentity(client, originalCharacter, originalModel, sizeof(originalModel)))
	{
		LP_LogError("Idle transaction rejected: client=%N stage=identity_capture.", client);
		if (notify) LP_Print(client, "IdleFailed");
		return false;
	}

	g_LPIdleVerifyPending[client] = true;
	g_LPIdleVerifySerial[client] = GetClientSerial(client);
	g_LPIdleVerifyFrames[client] = 0;
	g_LPIdleVerifyStartedAt[client] = GetGameTime();
	g_LPIdleVerifyReason[client] = reason;
	g_LPIdleExpectedBotUserId[client] = 0;
	g_LPIdleOriginalCharacter[client] = originalCharacter;
	strcopy(g_LPIdleOriginalModel[client], PLATFORM_MAX_PATH, originalModel);
	g_LPIdleRollbackPending[client] = false;
	// The immediate result is diagnostic only. Linux can finish the native Idle
	// transition over multiple frames.
	g_LPIdleImmediate[client] = LP_EngineGoAwayFromKeyboard(client);

	RequestFrame(LP_FrameVerifyIdle, g_LPIdleVerifySerial[client]);
	LP_Log("Idle transaction started: reason=%s client=%N serial=%d character=%d model=%s sdk_immediate=%d.",
		reason == LP_IDLE_REASON_AUTOMATIC ? "automatic" : "manual", client,
		g_LPIdleVerifySerial[client], originalCharacter, originalModel,
		g_LPIdleImmediate[client] ? 1 : 0);
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
	g_LPIdleExpectedBotUserId[client] = 0;
	g_LPIdleOriginalCharacter[client] = -1;
	g_LPIdleOriginalModel[client][0] = '\0';
	g_LPIdleRollbackPending[client] = false;
}

void LP_IdleObserveReplacementBot(int human, int bot, int character, const char[] model)
{
	if (human < 1 || human > MaxClients || !g_LPIdleVerifyPending[human]
		|| g_LPIdleRollbackPending[human]
		|| g_LPIdleVerifySerial[human] != GetClientSerial(human))
	{
		return;
	}

	g_LPIdleExpectedBotUserId[human] = GetClientUserId(bot);
	bool identityMatches = character == g_LPIdleOriginalCharacter[human]
		&& StrEqual(model, g_LPIdleOriginalModel[human], false)
		&& LP_VerifySurvivorIdentity(bot, g_LPIdleOriginalCharacter[human]);
	LP_Log("Idle replacement observed: human=%N bot=%d bot_userid=%d character=%d identity_match=%d.",
		human, bot, g_LPIdleExpectedBotUserId[human], g_LPIdleOriginalCharacter[human], identityMatches ? 1 : 0);
}

bool LP_IsIdleTransactionSatisfied(int client, int &bot)
{
	bot = GetClientOfUserId(g_LPIdleExpectedBotUserId[client]);
	if (!LP_IsHumanClient(client) || GetClientTeam(client) != LP_TEAM_SPECTATOR
		|| !LP_IsSurvivorBot(bot) || !IsPlayerAlive(bot)
		|| LP_GetBotHumanSpectatorUserId(bot) != GetClientUserId(client))
	{
		return false;
	}
	return LP_VerifySurvivorIdentity(bot, g_LPIdleOriginalCharacter[client]);
}

void LP_FailIdleRollbackFatal(int client, int bot, const char[] stage)
{
	int team = LP_IsHumanClient(client) ? GetClientTeam(client) : -1;
	LP_LogError("FATAL idle rollback failed: stage=%s client=%d team=%d bot=%d bot_userid=%d character=%d model=%s.",
		stage, client, team, bot, g_LPIdleExpectedBotUserId[client],
		g_LPIdleOriginalCharacter[client], g_LPIdleOriginalModel[client]);
	LP_StopAutoIdleHud(client);
	LP_ResetIdleVerification(client);
	if (LP_IsHumanClient(client))
	{
		LP_Print(client, "IdleFailed");
	}
}

bool LP_BeginIdleRollback(int client)
{
	int bot = GetClientOfUserId(g_LPIdleExpectedBotUserId[client]);
	if (!LP_IsSurvivorBot(bot))
	{
		bot = LP_FindIdleBotForHuman(client);
		if (bot > 0)
		{
			g_LPIdleExpectedBotUserId[client] = GetClientUserId(bot);
		}
	}

	if (LP_IsActiveSurvivor(client))
	{
		bool restored = LP_ApplyTransferredSurvivorIdentity(client,
			g_LPIdleOriginalCharacter[client], g_LPIdleOriginalModel[client]);
		if (!restored)
		{
			LP_FailIdleRollbackFatal(client, bot, "active_identity_restore");
			return false;
		}
		LP_Log("Idle rollback completed without takeover: client=%N identity_restored=%d.", client, restored ? 1 : 0);
		LP_ResetIdleVerification(client);
		LP_Print(client, "IdleFailed");
		return restored;
	}
	if (!LP_IsSurvivorBot(bot))
	{
		LP_FailIdleRollbackFatal(client, bot, "replacement_bot_unavailable");
		return false;
	}
	if (!LP_ApplyTransferredSurvivorIdentity(bot,
		g_LPIdleOriginalCharacter[client], g_LPIdleOriginalModel[client]))
	{
		LP_FailIdleRollbackFatal(client, bot, "replacement_identity_restore");
		return false;
	}

	g_LPIdleRollbackPending[client] = true;
	bool started = false;
	bool returnIdlePath = LP_FindIdleHumanForBot(bot) == client;
	if (returnIdlePath && GetClientTeam(client) != LP_TEAM_SPECTATOR)
	{
		ChangeClientTeam(client, LP_TEAM_SPECTATOR);
	}
	if (returnIdlePath && LP_IsEngineIdle(client) && LP_FindIdleBotForHuman(client) == bot)
	{
		started = LP_ReturnFromIdleBot(client, bot, true);
	}
	else if (LP_FindIdleHumanForBot(bot) == 0)
	{
		started = LP_BindAndTakeOverBot(client, bot, false, true);
	}
	if (!started)
	{
		LP_FailIdleRollbackFatal(client, bot, "takeover_start");
		return false;
	}

	LP_Log("Idle rollback takeover started: client=%N bot=%d path=%s.", client, bot,
		returnIdlePath ? "return_idle" : "bind_replacement");
	return true;
}

void LP_IdleRollbackFinished(int client, bool success, int bot)
{
	if (client < 1 || client > MaxClients || !g_LPIdleVerifyPending[client]
		|| !g_LPIdleRollbackPending[client])
	{
		return;
	}

	if (!success || !LP_IsActiveSurvivor(client)
		|| !LP_ApplyTransferredSurvivorIdentity(client,
			g_LPIdleOriginalCharacter[client], g_LPIdleOriginalModel[client]))
	{
		LP_FailIdleRollbackFatal(client, bot, success ? "final_identity_or_active_state" : "takeover_verify");
		return;
	}

	LP_Log("Idle rollback succeeded: client=%N bot=%d character=%d model=%s.",
		client, bot, g_LPIdleOriginalCharacter[client], g_LPIdleOriginalModel[client]);
	LP_ResetIdleVerification(client);
	LP_Print(client, "IdleFailed");
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
		LP_StopAutoIdleHud(stateClient);
		LP_ResetIdleVerification(stateClient);
		return;
	}
	int expectedBot;
	if (!g_LPIdleRollbackPending[stateClient] && LP_IsIdleTransactionSatisfied(stateClient, expectedBot))
	{
		LPIdleReason reason = g_LPIdleVerifyReason[stateClient];
		int originalCharacter = g_LPIdleOriginalCharacter[stateClient];
		char originalModel[PLATFORM_MAX_PATH];
		strcopy(originalModel, sizeof(originalModel), g_LPIdleOriginalModel[stateClient]);
		g_LPPlayers[stateClient].idleSince = GetGameTime();
		g_LPPlayers[stateClient].wasIdle = true;
		LP_StopAutoIdleHud(stateClient);
		LP_ResetIdleVerification(stateClient);
		LP_HumanTeamWipeObserveClient(stateClient);
		LP_Print(stateClient, "NowIdle");
		LP_Log("Idle transaction committed: reason=%s client=%N bot=%d character=%d model=%s.",
			reason == LP_IDLE_REASON_AUTOMATIC ? "automatic" : "manual",
			stateClient, expectedBot, originalCharacter, originalModel);
		return;
	}
	if (g_LPIdleRollbackPending[stateClient])
	{
		return;
	}

	g_LPIdleVerifyFrames[stateClient]++;
	if (g_LPIdleVerifyFrames[stateClient] >= LP_STATE_VERIFY_MAX_FRAMES
		|| GetGameTime() - g_LPIdleVerifyStartedAt[stateClient] >= LP_STATE_VERIFY_TIMEOUT)
	{
		int bot = LP_FindIdleBotForHuman(stateClient);
		LPIdleReason reason = g_LPIdleVerifyReason[stateClient];
		bool sdkImmediate = g_LPIdleImmediate[stateClient];
		LP_StopAutoIdleHud(stateClient);
		LP_LogStateFailure(reason == LP_IDLE_REASON_AUTOMATIC ? "idle_automatic_verify_timeout" : "idle_manual_verify_timeout",
			stateClient, bot, LP_GetBotHumanSpectatorUserId(bot), sdkImmediate ? 1 : 0);
		LP_LogError("Idle transaction timed out; starting rollback: reason=%s client=%N expected_bot_userid=%d.",
			reason == LP_IDLE_REASON_AUTOMATIC ? "automatic" : "manual",
			stateClient, g_LPIdleExpectedBotUserId[stateClient]);
		LP_BeginIdleRollback(stateClient);
		return;
	}
	RequestFrame(LP_FrameVerifyIdle, serial);
}

public Action LP_EventPlayerDeathPre(Event event, const char[] name, bool dontBroadcast)
{
	int bot = GetClientOfUserId(event.GetInt("userid"));
	if (LP_IsHumanClient(bot))
	{
		LP_StopAutoIdleHud(bot);
		LP_ClearIdleKickHint(bot);
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
