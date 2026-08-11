#if defined _l4d2_players_idle_included
	#endinput
#endif
#define _l4d2_players_idle_included

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
	if (!g_LPEnabled || !LP_CanGoIdle(client, notify))
	{
		return false;
	}

	int serial = GetClientSerial(client);
	if (!LP_EngineGoAwayFromKeyboard(client))
	{
		if (notify) LP_Print(client, "IdleFailed");
		LP_LogError("Engine idle call failed for client %N.", client);
		return false;
	}

	RequestFrame(LP_FrameVerifyIdle, serial);
	LP_Log("%s idle requested for %N.", reason == LP_IDLE_REASON_AUTOMATIC ? "Automatic" : "Manual", client);
	return true;
}

public void LP_FrameVerifyIdle(any serial)
{
	int client = GetClientFromSerial(serial);
	if (!LP_IsHumanClient(client))
	{
		return;
	}
	if (LP_IsEngineIdle(client))
	{
		g_LPPlayers[client].idleSince = GetGameTime();
		g_LPPlayers[client].wasIdle = true;
		LP_Print(client, "NowIdle");
		return;
	}

	LP_Print(client, "IdleFailed");
	LP_LogError("Idle verification failed for client %N.", client);
}

public Action LP_EventPlayerDeathPre(Event event, const char[] name, bool dontBroadcast)
{
	int bot = GetClientOfUserId(event.GetInt("userid"));
	if (!LP_IsSurvivorBot(bot))
	{
		return Plugin_Continue;
	}

	int human = LP_FindIdleHumanForBot(bot);
	if (!LP_IsHumanClient(human))
	{
		return Plugin_Continue;
	}

	ChangeClientTeam(human, LP_TEAM_UNASSIGNED);
	bool bound = LP_EngineSetHumanSpectator(bot, human);
	bool taken = bound && LP_EngineTakeOverBot(human);
	if (!taken)
	{
		ChangeClientTeam(human, LP_TEAM_SPECTATOR);
		LP_LogError("Dead idle bot recovery failed for client %N and bot %d.", human, bot);
	}
	else
	{
		LP_Log("Recovered idle client %N from dying bot %d.", human, bot);
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
