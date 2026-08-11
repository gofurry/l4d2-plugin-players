#if defined _l4d2_players_runtime_included
	#endinput
#endif
#define _l4d2_players_runtime_included

enum struct LPPlayerRuntime
{
	bool connected;
	bool cookiesLoaded;
	bool bhopEnabled;
	float connectedAt;
	float lastActivityTime;
	float idleSince;
	float lastEyeAngles[3];
	bool hasEyeAngles;
	int lastButtons;
	int lastWeapon;
	bool autoIdleHudVisible;
	bool idleKickWarning;
	bool wasIdle;
}

LPPlayerRuntime g_LPPlayers[MAXPLAYERS + 1];

void LP_ResetClientRuntime(int client)
{
	g_LPPlayers[client].connected = false;
	g_LPPlayers[client].cookiesLoaded = false;
	g_LPPlayers[client].bhopEnabled = g_LPBhopDefault;
	g_LPPlayers[client].connectedAt = 0.0;
	g_LPPlayers[client].lastActivityTime = 0.0;
	g_LPPlayers[client].idleSince = 0.0;
	g_LPPlayers[client].lastEyeAngles[0] = 0.0;
	g_LPPlayers[client].lastEyeAngles[1] = 0.0;
	g_LPPlayers[client].lastEyeAngles[2] = 0.0;
	g_LPPlayers[client].hasEyeAngles = false;
	g_LPPlayers[client].lastButtons = 0;
	g_LPPlayers[client].lastWeapon = 0;
	g_LPPlayers[client].autoIdleHudVisible = false;
	g_LPPlayers[client].idleKickWarning = false;
	g_LPPlayers[client].wasIdle = false;
}

void LP_ResetRuntime()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		LP_ResetClientRuntime(client);
	}
}

void LP_ResetMapRuntime()
{
	float now = GetGameTime();
	for (int client = 1; client <= MaxClients; client++)
	{
		if (!g_LPPlayers[client].connected)
		{
			continue;
		}
		g_LPPlayers[client].lastActivityTime = now;
		g_LPPlayers[client].idleSince = 0.0;
		g_LPPlayers[client].autoIdleHudVisible = false;
		g_LPPlayers[client].idleKickWarning = false;
		g_LPPlayers[client].wasIdle = false;
		g_LPPlayers[client].hasEyeAngles = false;
	}
}

void LP_RuntimeClientConnected(int client)
{
	LP_ResetClientRuntime(client);
	g_LPPlayers[client].connected = true;
	float now = GetGameTime();
	g_LPPlayers[client].connectedAt = now;
	g_LPPlayers[client].lastActivityTime = now;
}

void LP_RuntimeClientPutInServer(int client)
{
	if (!g_LPPlayers[client].connected)
	{
		LP_RuntimeClientConnected(client);
	}
	if (!IsFakeClient(client) && AreClientCookiesCached(client))
	{
		LP_LoadBhopCookie(client);
	}
}

void LP_RuntimeClientDisconnected(int client)
{
	LP_ResetClientRuntime(client);
}

void LP_RecordActivity(int client)
{
	if (client < 1 || client > MaxClients || !g_LPPlayers[client].connected)
	{
		return;
	}
	g_LPPlayers[client].lastActivityTime = GetGameTime();
	LP_StopAutoIdleHud(client);
}
