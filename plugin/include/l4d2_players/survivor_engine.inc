#if defined _l4d2_players_survivor_engine_included
	#endinput
#endif
#define _l4d2_players_survivor_engine_included

int g_LPBotReservation[MAXPLAYERS + 1];

bool LP_BindHumanToBot(int client, int bot)
{
	if (!LP_IsHumanClient(client) || !LP_IsSurvivorBot(bot))
	{
		return false;
	}
	return LP_EngineSetHumanSpectator(bot, client);
}

bool LP_TakeOverBot(int client, int bot)
{
	if (!LP_IsHumanClient(client) || !LP_IsSurvivorBot(bot))
	{
		return false;
	}

	ChangeClientTeam(client, LP_TEAM_UNASSIGNED);
	if (!LP_BindHumanToBot(client, bot))
	{
		ChangeClientTeam(client, LP_TEAM_SPECTATOR);
		return false;
	}
	if (!LP_EngineTakeOverBot(client))
	{
		ChangeClientTeam(client, LP_TEAM_SPECTATOR);
		return false;
	}

	g_LPBotReservation[bot] = 0;
	LP_RecordActivity(client);
	RequestFrame(LP_FrameVerifyTakeover, GetClientSerial(client));
	return true;
}

public void LP_FrameVerifyTakeover(any serial)
{
	int client = GetClientFromSerial(serial);
	if (!LP_IsHumanClient(client))
	{
		return;
	}
	if (!LP_IsActiveSurvivor(client))
	{
		LP_Print(client, "TakeoverFailed");
		LP_LogError("Takeover verification failed for client %N.", client);
	}
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

bool LP_CanCreateSurvivorBot(int client)
{
	if (!LP_IsHumanClient(client))
	{
		return false;
	}
	if (!LP_EngineIsReady())
	{
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
		LP_Print(client, "NoClientSlot");
		return false;
	}
	return true;
}
