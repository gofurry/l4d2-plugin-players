#if defined _l4d2_players_state_included
	#endinput
#endif
#define _l4d2_players_state_included

bool LP_IsHumanClient(int client)
{
	return client >= 1 && client <= MaxClients && IsClientInGame(client) && !IsFakeClient(client);
}

bool LP_IsSurvivorBot(int client)
{
	return client >= 1 && client <= MaxClients && IsClientInGame(client) && IsFakeClient(client)
		&& GetClientTeam(client) == LP_TEAM_SURVIVOR;
}

bool LP_IsActiveSurvivor(int client)
{
	return LP_IsHumanClient(client) && GetClientTeam(client) == LP_TEAM_SURVIVOR;
}

int LP_FindIdleBotForHuman(int client)
{
	if (!LP_IsHumanClient(client))
	{
		return 0;
	}

	int userId = GetClientUserId(client);
	for (int bot = 1; bot <= MaxClients; bot++)
	{
		if (!LP_IsSurvivorBot(bot) || !HasEntProp(bot, Prop_Send, "m_humanSpectatorUserID"))
		{
			continue;
		}
		if (GetEntProp(bot, Prop_Send, "m_humanSpectatorUserID") == userId)
		{
			return bot;
		}
	}
	return 0;
}

int LP_FindIdleHumanForBot(int bot)
{
	if (!LP_IsSurvivorBot(bot) || !HasEntProp(bot, Prop_Send, "m_humanSpectatorUserID"))
	{
		return 0;
	}

	int userId = GetEntProp(bot, Prop_Send, "m_humanSpectatorUserID");
	if (userId <= 0)
	{
		return 0;
	}

	int client = GetClientOfUserId(userId);
	return LP_IsHumanClient(client) ? client : 0;
}

bool LP_IsEngineIdle(int client)
{
	return LP_IsHumanClient(client) && GetClientTeam(client) == LP_TEAM_SPECTATOR && LP_FindIdleBotForHuman(client) > 0;
}

bool LP_IsFreeSpectator(int client)
{
	return LP_IsHumanClient(client) && GetClientTeam(client) == LP_TEAM_SPECTATOR && LP_FindIdleBotForHuman(client) == 0;
}

LPPlayerState LP_GetPlayerState(int client)
{
	if (LP_IsActiveSurvivor(client))
	{
		return LP_PLAYER_STATE_SURVIVOR_ACTIVE;
	}
	if (LP_IsEngineIdle(client))
	{
		return LP_PLAYER_STATE_SURVIVOR_IDLE;
	}
	if (LP_IsFreeSpectator(client))
	{
		return LP_PLAYER_STATE_SPECTATOR_FREE;
	}
	return LP_PLAYER_STATE_NONE;
}

bool LP_IsPinned(int client)
{
	static const char props[][] =
	{
		"m_tongueOwner",
		"m_pounceAttacker",
		"m_jockeyAttacker",
		"m_carryAttacker",
		"m_pummelAttacker"
	};

	for (int i = 0; i < sizeof(props); i++)
	{
		if (HasEntProp(client, Prop_Send, props[i]) && GetEntPropEnt(client, Prop_Send, props[i]) > 0)
		{
			return true;
		}
	}
	return false;
}

int LP_GetHumanSurvivorCount()
{
	int count;
	for (int client = 1; client <= MaxClients; client++)
	{
		if (LP_IsActiveSurvivor(client))
		{
			count++;
		}
	}
	return count;
}

int LP_GetSurvivorCount()
{
	int count;
	for (int client = 1; client <= MaxClients; client++)
	{
		if (IsClientInGame(client) && GetClientTeam(client) == LP_TEAM_SURVIVOR)
		{
			count++;
		}
	}
	return count;
}

int LP_GetConfiguredSurvivorCapacity()
{
	return g_LPSurvivorLimit;
}

bool LP_HasAdminKickImmunity(int client)
{
	if (!g_LPAdminKickImmunity || !LP_IsHumanClient(client))
	{
		return false;
	}
	return (GetUserFlagBits(client) & g_LPAdminFlagBits) != 0;
}

