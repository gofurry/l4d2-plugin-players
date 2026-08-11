#if defined _l4d2_players_auto_join_included
	#endinput
#endif
#define _l4d2_players_auto_join_included

void LP_CancelAutoJoinTimer(int client)
{
	if (client < 1 || client > MaxClients)
	{
		return;
	}
	delete g_LPPlayers[client].autoJoinTimer;
	g_LPPlayers[client].autoJoinTimer = null;
}

void LP_CancelAllAutoJoinTimers()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		LP_CancelAutoJoinTimer(client);
	}
}

void LP_CompleteAutoJoinForConnection(int client)
{
	if (client < 1 || client > MaxClients)
	{
		return;
	}
	g_LPPlayers[client].autoJoinCompleted = true;
	LP_CancelAutoJoinTimer(client);
}

void LP_SuppressAutoJoinForConnection(int client)
{
	if (client < 1 || client > MaxClients)
	{
		return;
	}
	g_LPPlayers[client].autoJoinSuppressed = true;
	LP_CompleteAutoJoinForConnection(client);
}

void LP_ScheduleAutoJoin(int client, float delay)
{
	if (!g_LPEnabled || !g_LPAutoJoinEnabled || g_LPMapEnding
		|| client < 1 || client > MaxClients || !g_LPPlayers[client].autoJoinEligible
		|| g_LPPlayers[client].autoJoinCompleted || g_LPPlayers[client].autoJoinSuppressed
		|| g_LPPlayers[client].autoJoinTimer != null)
	{
		return;
	}

	g_LPPlayers[client].autoJoinTimer = CreateTimer(delay, LP_TimerAutoJoin,
		GetClientSerial(client), TIMER_FLAG_NO_MAPCHANGE);
}

void LP_AutoJoinClientPutInServer(int client)
{
	if (client < 1 || client > MaxClients || !IsClientInGame(client)
		|| IsFakeClient(client) || IsClientSourceTV(client))
	{
		return;
	}
	if (!g_LPPlayers[client].autoJoinEligible)
	{
		g_LPPlayers[client].autoJoinEligible = true;
		g_LPPlayers[client].autoJoinAttempts = 0;
	}
	if (LP_IsActiveSurvivor(client) || LP_IsEngineIdle(client))
	{
		LP_CompleteAutoJoinForConnection(client);
		return;
	}
	LP_ScheduleAutoJoin(client, LP_AUTO_JOIN_INITIAL_DELAY);
}

void LP_AutoJoinMapStart()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		if (LP_IsHumanClient(client))
		{
			LP_ScheduleAutoJoin(client, LP_AUTO_JOIN_INITIAL_DELAY);
		}
	}
}

void LP_AutoJoinMapEnd()
{
	LP_CancelAllAutoJoinTimers();
}

void LP_AutoJoinRetryOrFinish(int client, const char[] stage)
{
	if (g_LPPlayers[client].autoJoinAttempts < LP_AUTO_JOIN_MAX_ATTEMPTS
		&& !g_LPPlayers[client].autoJoinSuppressed)
	{
		LP_ScheduleAutoJoin(client, LP_AUTO_JOIN_RETRY_DELAY);
		return;
	}

	g_LPPlayers[client].autoJoinCompleted = true;
	LP_LogError("Auto join stopped after bounded retries: client=%N stage=%s attempts=%d.",
		client, stage, g_LPPlayers[client].autoJoinAttempts);
}

public Action LP_TimerAutoJoin(Handle timer, any serial)
{
	int client = GetClientFromSerial(serial);
	if (client <= 0 || client > MaxClients)
	{
		return Plugin_Stop;
	}
	g_LPPlayers[client].autoJoinTimer = null;

	if (!LP_IsHumanClient(client) || !g_LPEnabled || !g_LPAutoJoinEnabled
		|| g_LPPlayers[client].autoJoinCompleted || g_LPPlayers[client].autoJoinSuppressed)
	{
		return Plugin_Stop;
	}

	g_LPPlayers[client].autoJoinAttempts++;
	if (g_LPMapEnding)
	{
		LP_AutoJoinRetryOrFinish(client, "map_ending");
		return Plugin_Stop;
	}
	if (LP_IsActiveSurvivor(client) || LP_IsEngineIdle(client))
	{
		LP_CompleteAutoJoinForConnection(client);
		return Plugin_Stop;
	}
	if (!LP_IsPopulationReady())
	{
		LP_AutoJoinRetryOrFinish(client, "population_not_ready");
		return Plugin_Stop;
	}
	if (LP_IsSpectatePendingForClient(client) || LP_IsTakeoverPendingForClient(client)
		|| g_LPIdleVerifyPending[client] || LP_IsJoinQueued(serial))
	{
		LP_AutoJoinRetryOrFinish(client, "player_transition_pending");
		return Plugin_Stop;
	}
	if (!LP_IsFreeSpectator(client))
	{
		if (GetClientTeam(client) != LP_TEAM_SPECTATOR)
		{
			ChangeClientTeam(client, LP_TEAM_SPECTATOR);
		}
		LP_AutoJoinRetryOrFinish(client, "waiting_for_free_spectator");
		return Plugin_Stop;
	}

	LP_Log("Auto join requested for %N.", client);
	LP_JoinSurvivor(client);
	LP_CompleteAutoJoinForConnection(client);
	return Plugin_Stop;
}
