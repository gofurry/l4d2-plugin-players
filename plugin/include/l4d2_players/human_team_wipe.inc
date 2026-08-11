#if defined _l4d2_players_human_team_wipe_included
	#endinput
#endif
#define _l4d2_players_human_team_wipe_included

#define LP_HUMAN_WIPE_CHECK_DELAY 0.25

int g_LPHumanParticipantUserId[MAXPLAYERS + 1];
bool g_LPHumanParticipantDied[MAXPLAYERS + 1];
bool g_LPHumanWipeRoundActive;
bool g_LPHumanWipeTriggered;
Handle g_LPHumanWipeCheckTimer;

void LP_InitializeHumanTeamWipe()
{
	HookEvent("round_start", LP_HumanTeamWipeRoundStart, EventHookMode_PostNoCopy);
	HookEvent("round_end", LP_HumanTeamWipeRoundEnd, EventHookMode_PostNoCopy);
	HookEvent("player_death", LP_HumanTeamWipePlayerDeath, EventHookMode_Post);
	HookEvent("player_spawn", LP_HumanTeamWipePlayerSpawn, EventHookMode_Post);
	LP_ResetHumanTeamWipeRound();

	if (g_LPLateLoad)
	{
		g_LPHumanWipeRoundActive = true;
		for (int client = 1; client <= MaxClients; client++)
		{
			LP_HumanTeamWipeObserveClient(client);
		}
	}
}

void LP_ResetHumanTeamWipeRound()
{
	LP_CancelHumanTeamWipeCheck();
	g_LPHumanWipeTriggered = false;
	for (int i = 0; i <= MaxClients; i++)
	{
		g_LPHumanParticipantUserId[i] = 0;
		g_LPHumanParticipantDied[i] = false;
	}
}

void LP_HumanTeamWipeMapStart()
{
	g_LPHumanWipeRoundActive = false;
	LP_ResetHumanTeamWipeRound();
}

void LP_HumanTeamWipeMapEnd()
{
	g_LPHumanWipeRoundActive = false;
	LP_CancelHumanTeamWipeCheck();
}

void LP_CancelHumanTeamWipeCheck()
{
	delete g_LPHumanWipeCheckTimer;
	g_LPHumanWipeCheckTimer = null;
}

int LP_FindHumanParticipant(int userId)
{
	if (userId <= 0)
	{
		return -1;
	}
	for (int i = 0; i < MAXPLAYERS + 1; i++)
	{
		if (g_LPHumanParticipantUserId[i] == userId)
		{
			return i;
		}
	}
	return -1;
}

int LP_AddHumanParticipant(int client)
{
	if (!LP_IsHumanClient(client))
	{
		return -1;
	}

	int userId = GetClientUserId(client);
	int existing = LP_FindHumanParticipant(userId);
	if (existing >= 0)
	{
		return existing;
	}
	for (int i = 0; i < MAXPLAYERS + 1; i++)
	{
		if (g_LPHumanParticipantUserId[i] == 0)
		{
			g_LPHumanParticipantUserId[i] = userId;
			g_LPHumanParticipantDied[i] = false;
			return i;
		}
	}
	return -1;
}

bool LP_IsEffectiveHumanSurvivorAlive(int client)
{
	if (!LP_IsHumanClient(client))
	{
		return false;
	}
	if (LP_IsActiveSurvivor(client))
	{
		return IsPlayerAlive(client);
	}
	if (LP_IsEngineIdle(client))
	{
		int bot = LP_FindIdleBotForHuman(client);
		return bot > 0 && IsPlayerAlive(bot);
	}
	return false;
}

void LP_HumanTeamWipeObserveClient(int client)
{
	if (!g_LPHumanWipeRoundActive || !LP_IsHumanClient(client))
	{
		return;
	}
	if (!LP_IsActiveSurvivor(client) && !LP_IsEngineIdle(client))
	{
		return;
	}

	int participant = LP_AddHumanParticipant(client);
	if (participant >= 0 && LP_IsEffectiveHumanSurvivorAlive(client))
	{
		g_LPHumanParticipantDied[participant] = false;
	}
}

void LP_HumanTeamWipeMarkDead(int client)
{
	if (!g_LPHumanWipeRoundActive || !LP_IsHumanClient(client))
	{
		return;
	}
	int participant = LP_AddHumanParticipant(client);
	if (participant >= 0)
	{
		g_LPHumanParticipantDied[participant] = true;
		LP_ScheduleHumanTeamWipeCheck();
	}
}

void LP_HumanTeamWipeRecordIdleBotDeath(int human)
{
	LP_HumanTeamWipeMarkDead(human);
}

void LP_HumanTeamWipeClientDisconnected(int client)
{
	if (client < 1 || client > MaxClients)
	{
		return;
	}
	int participant = LP_FindHumanParticipant(GetClientUserId(client));
	if (participant >= 0 && !g_LPHumanParticipantDied[participant])
	{
		g_LPHumanParticipantUserId[participant] = 0;
		g_LPHumanParticipantDied[participant] = false;
	}
}

void LP_HumanTeamWipeLeaveForFreeSpectator(int client)
{
	if (client < 1 || client > MaxClients)
	{
		return;
	}
	int participant = LP_FindHumanParticipant(GetClientUserId(client));
	if (participant >= 0 && !g_LPHumanParticipantDied[participant])
	{
		g_LPHumanParticipantUserId[participant] = 0;
		g_LPHumanParticipantDied[participant] = false;
	}
}

void LP_ScheduleHumanTeamWipeCheck()
{
	if (!g_LPEnabled || !g_LPHumanTeamWipeEnabled || !g_LPHumanWipeRoundActive
		|| g_LPHumanWipeTriggered || g_LPMapEnding || g_LPHumanWipeCheckTimer != null)
	{
		return;
	}
	g_LPHumanWipeCheckTimer = CreateTimer(LP_HUMAN_WIPE_CHECK_DELAY, LP_TimerCheckHumanTeamWipe, _, TIMER_FLAG_NO_MAPCHANGE);
}

bool LP_HasLivingUnboundSurvivorBot()
{
	for (int bot = 1; bot <= MaxClients; bot++)
	{
		if (LP_IsSurvivorBot(bot) && IsPlayerAlive(bot) && LP_FindIdleHumanForBot(bot) == 0)
		{
			return true;
		}
	}
	return false;
}

bool LP_ShouldTriggerHumanTeamWipe()
{
	// Re-scan current humans immediately before the decision so a recently
	// established Active/Idle state cannot be missed between monitor ticks.
	for (int client = 1; client <= MaxClients; client++)
	{
		LP_HumanTeamWipeObserveClient(client);
	}

	bool hasParticipant;
	for (int i = 0; i < MAXPLAYERS + 1; i++)
	{
		int userId = g_LPHumanParticipantUserId[i];
		if (userId <= 0)
		{
			continue;
		}
		hasParticipant = true;
		int client = GetClientOfUserId(userId);
		if (LP_IsEffectiveHumanSurvivorAlive(client))
		{
			return false;
		}
		if (!g_LPHumanParticipantDied[i])
		{
			return false;
		}
	}
	return hasParticipant && LP_HasLivingUnboundSurvivorBot();
}

public Action LP_TimerCheckHumanTeamWipe(Handle timer)
{
	g_LPHumanWipeCheckTimer = null;
	if (!g_LPEnabled || !g_LPHumanTeamWipeEnabled || !g_LPHumanWipeRoundActive
		|| g_LPHumanWipeTriggered || g_LPMapEnding || !LP_ShouldTriggerHumanTeamWipe())
	{
		return Plugin_Stop;
	}

	g_LPHumanWipeTriggered = true;
	LP_Log("Human team wipe: no participating human survivor remains alive.");
	for (int bot = 1; bot <= MaxClients; bot++)
	{
		if (LP_IsSurvivorBot(bot) && IsPlayerAlive(bot))
		{
			ForcePlayerSuicide(bot);
		}
	}
	return Plugin_Stop;
}

public void LP_HumanTeamWipeRoundStart(Event event, const char[] name, bool dontBroadcast)
{
	LP_ResetHumanTeamWipeRound();
	g_LPHumanWipeRoundActive = true;
}

public void LP_HumanTeamWipeRoundEnd(Event event, const char[] name, bool dontBroadcast)
{
	g_LPHumanWipeRoundActive = false;
	LP_CancelHumanTeamWipeCheck();
}

public void LP_HumanTeamWipePlayerDeath(Event event, const char[] name, bool dontBroadcast)
{
	int client = GetClientOfUserId(event.GetInt("userid"));
	if (LP_IsHumanClient(client) && GetClientTeam(client) == LP_TEAM_SURVIVOR)
	{
		LP_HumanTeamWipeMarkDead(client);
	}
}

public void LP_HumanTeamWipePlayerSpawn(Event event, const char[] name, bool dontBroadcast)
{
	int client = GetClientOfUserId(event.GetInt("userid"));
	LP_HumanTeamWipeObserveClient(client);
}
