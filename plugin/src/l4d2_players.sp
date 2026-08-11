#include <sourcemod>
#include <sdkhooks>
#include <sdktools>
#include <clientprefs>

#pragma semicolon 1
#pragma newdecls required

#include <l4d2_players/definitions>
#include <l4d2_players/config>
#include <l4d2_players/runtime>
#include <l4d2_players/logging>
#include <l4d2_players/engine>
#include <l4d2_players/state>
#include <l4d2_players/character>
#include <l4d2_players/hud>
#include <l4d2_players/midjoin>
#include <l4d2_players/survivor_engine>
#include <l4d2_players/idle>
#include <l4d2_players/join>
#include <l4d2_players/spectate>
#include <l4d2_players/human_team_wipe>
#include <l4d2_players/auto_join>
#include <l4d2_players/suicide>
#include <l4d2_players/bhop>
#include <l4d2_players/afk_monitor>
#include <l4d2_players/commands>

public Plugin myinfo =
{
	name = "L4D2 Players",
	author = "gofurry",
	description = "Self-contained player utilities and survivor session management for L4D2.",
	version = LP_VERSION,
	url = "https://github.com/gofurry/l4d2-plugin-players"
};

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int errorLength)
{
	if (GetEngineVersion() != Engine_Left4Dead2)
	{
		strcopy(error, errorLength, "L4D2 Players only supports Left 4 Dead 2.");
		return APLRes_SilentFailure;
	}

	g_LPLateLoad = late;
	return APLRes_Success;
}

public void OnPluginStart()
{
	LoadTranslations("l4d2_players.phrases");
	LP_CreateConfig();
	LP_InitializeEngine();
	LP_ResetRuntime();
	LP_InitializeIdle();
	LP_InitializeJoin();
	LP_InitializeSpectate();
	LP_InitializeHumanTeamWipe();
	LP_InitializeBhop();
	LP_InitializeAfkMonitor();
	LP_RegisterCommands();
	AutoExecConfig(true, "l4d2_players");

	if (g_LPLateLoad)
	{
		for (int client = 1; client <= MaxClients; client++)
		{
			if (IsClientConnected(client))
			{
				LP_RuntimeClientConnected(client);
			}
			if (IsClientInGame(client))
			{
				LP_RuntimeClientPutInServer(client);
			}
		}
	}

	LP_Log("Version %s loaded; self-contained engine layer initialized.", LP_VERSION);
}

public void OnConfigsExecuted()
{
	LP_ApplyConfiguration();
	LP_CheckServerPolicy();
	LP_Log("Configuration applied; survivor limit %d, active humans %d.", LP_GetConfiguredSurvivorCapacity(), LP_GetHumanSurvivorCount());
}

public void OnMapStart()
{
	g_LPMapEnding = false;
	LP_StopAllAutoIdleHuds();
	LP_ClearAllIdleKickHints();
	LP_HumanTeamWipeMapStart();
	LP_AutoJoinMapStart();
	LP_PrecacheCharacterModels();
	LP_ResetMapRuntime();
	LP_StartAfkMonitor();
}

public void OnMapEnd()
{
	g_LPMapEnding = true;
	LP_StopAfkMonitor();
	LP_CancelAllIdleVerifications();
	LP_CancelJoinQueue();
	LP_CancelAllSpectateTransitions();
	LP_AutoJoinMapEnd();
	LP_HumanTeamWipeMapEnd();
}

public void OnPluginEnd()
{
	LP_StopAfkMonitor();
	LP_CancelAllSpectateTransitions();
	LP_CancelAllAutoJoinTimers();
	LP_CancelHumanTeamWipeCheck();
	LP_ShutdownEngine();
}

public void OnClientConnected(int client)
{
	LP_RuntimeClientConnected(client);
}

public void OnClientPutInServer(int client)
{
	LP_RuntimeClientPutInServer(client);
	LP_AutoJoinClientPutInServer(client);
}

public void OnClientDisconnect(int client)
{
	LP_StopAutoIdleHud(client);
	LP_ClearIdleKickHint(client);
	LP_IdleClientDisconnected(client);
	LP_JoinClientDisconnected(client);
	LP_SpectateClientDisconnected(client);
	LP_CancelAutoJoinTimer(client);
	LP_HumanTeamWipeClientDisconnected(client);
	LP_RuntimeClientDisconnected(client);
}

public void OnClientCookiesCached(int client)
{
	LP_LoadBhopCookie(client);
}

public void OnClientSayCommand_Post(int client, const char[] command, const char[] sArgs)
{
	if (LP_IsHumanClient(client))
	{
		LP_RecordActivity(client);
	}
}

public Action OnPlayerRunCmd(int client, int &buttons, int &impulse, float vel[3], float angles[3], int &weapon,
	int &subtype, int &cmdnum, int &tickcount, int &seed, int mouse[2])
{
	if (!LP_IsHumanClient(client))
	{
		return Plugin_Continue;
	}

	LP_DetectRunCmdActivity(client, buttons, vel, angles, weapon);
	LP_ApplyBhop(client, buttons);
	return Plugin_Continue;
}
