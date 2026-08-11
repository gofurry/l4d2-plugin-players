#if defined _l4d2_players_bhop_included
	#endinput
#endif
#define _l4d2_players_bhop_included

Cookie g_LPBhopCookie;

void LP_InitializeBhop()
{
	g_LPBhopCookie = RegClientCookie("l4d2_players_bhop", "Personal automatic bunny hop preference", CookieAccess_Protected);
}

void LP_LoadBhopCookie(int client)
{
	if (!LP_IsHumanClient(client) || !AreClientCookiesCached(client))
	{
		return;
	}

	char value[8];
	GetClientCookie(client, g_LPBhopCookie, value, sizeof(value));
	if (value[0] == '\0')
	{
		g_LPPlayers[client].bhopEnabled = g_LPBhopDefault;
	}
	else
	{
		g_LPPlayers[client].bhopEnabled = StringToInt(value) != 0;
	}
	g_LPPlayers[client].cookiesLoaded = true;
}

void LP_SaveBhopCookie(int client)
{
	if (!LP_IsHumanClient(client) || !AreClientCookiesCached(client))
	{
		return;
	}
	SetClientCookie(client, g_LPBhopCookie, g_LPPlayers[client].bhopEnabled ? "1" : "0");
}

void LP_ApplyBhop(int client, int &buttons)
{
	if (!g_LPEnabled || !g_LPBhopEnabled || !g_LPPlayers[client].bhopEnabled)
	{
		return;
	}
	if (!LP_IsActiveSurvivor(client) || !IsPlayerAlive(client) || (buttons & IN_JUMP) == 0)
	{
		return;
	}
	if ((GetEntityFlags(client) & FL_ONGROUND) == 0)
	{
		buttons &= ~IN_JUMP;
	}
}

Action LP_CommandBhop(int client)
{
	if (!LP_IsHumanClient(client))
	{
		return Plugin_Handled;
	}
	LP_RecordActivity(client);
	if (!g_LPEnabled || !g_LPBhopEnabled)
	{
		LP_Print(client, "BhopUnavailable");
		return Plugin_Handled;
	}

	g_LPPlayers[client].bhopEnabled = !g_LPPlayers[client].bhopEnabled;
	LP_SaveBhopCookie(client);
	LP_Print(client, g_LPPlayers[client].bhopEnabled ? "BhopEnabled" : "BhopDisabled");
	return Plugin_Handled;
}

