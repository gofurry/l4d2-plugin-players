#if defined _l4d2_players_config_included
	#endinput
#endif
#define _l4d2_players_config_included

ConVar g_LPCvarEnabled;
ConVar g_LPCvarSurvivorLimit;
ConVar g_LPCvarMinSurvivors;
ConVar g_LPCvarAutoIdleSeconds;
ConVar g_LPCvarAutoIdleWarning;
ConVar g_LPCvarIdleKickSeconds;
ConVar g_LPCvarIdleKickWarning;
ConVar g_LPCvarAfkHud;
ConVar g_LPCvarAdminKickImmunity;
ConVar g_LPCvarAdminFlags;
ConVar g_LPCvarBhopEnabled;
ConVar g_LPCvarBhopDefault;
ConVar g_LPCvarCsmEnabled;
ConVar g_LPCvarAnnounceIdleKick;
ConVar g_LPCvarHumanTeamWipe;
ConVar g_LPCvarAutoJoin;
ConVar g_LPCvarMidJoinSpawnNearPlayer;
ConVar g_LPCvarMidJoinLoadout;

bool g_LPEnabled;
int g_LPSurvivorLimit;
int g_LPMinSurvivors;
float g_LPAutoIdleSeconds;
float g_LPAutoIdleWarningSeconds;
float g_LPIdleKickSeconds;
float g_LPIdleKickWarningSeconds;
bool g_LPAfkHud;
bool g_LPAdminKickImmunity;
int g_LPAdminFlagBits;
bool g_LPBhopEnabled;
bool g_LPBhopDefault;
bool g_LPCsmEnabled;
bool g_LPAnnounceIdleKick;
bool g_LPHumanTeamWipeEnabled;
bool g_LPAutoJoinEnabled;
bool g_LPMidJoinSpawnNearPlayer;
bool g_LPMidJoinLoadout;

void LP_CreateConfig()
{
	g_LPCvarEnabled = CreateConVar("sm_l4dp_enabled", "1", "Enable L4D2 Players.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarSurvivorLimit = CreateConVar("sm_l4dp_survivor_limit", "4", "Maximum survivor clients managed by Players.", FCVAR_NOTIFY, true, 1.0, true, 16.0);
	g_LPCvarMinSurvivors = CreateConVar("sm_l4dp_min_survivors", "4", "Baseline Survivor bodies reconciled once during round initialization.", FCVAR_NOTIFY, true, 1.0, true, 16.0);
	g_LPCvarAutoIdleSeconds = CreateConVar("sm_l4dp_auto_idle_seconds", "120", "Seconds of inactivity before automatic idle; 0 disables.", FCVAR_NOTIFY, true, 0.0);
	g_LPCvarAutoIdleWarning = CreateConVar("sm_l4dp_auto_idle_warning_seconds", "10", "Warning lead time for automatic idle.", FCVAR_NOTIFY, true, 0.0);
	g_LPCvarIdleKickSeconds = CreateConVar("sm_l4dp_idle_kick_seconds", "600", "Seconds in engine idle before kick; 0 disables.", FCVAR_NOTIFY, true, 0.0);
	g_LPCvarIdleKickWarning = CreateConVar("sm_l4dp_idle_kick_warning_seconds", "60", "Warning lead time for idle kick.", FCVAR_NOTIFY, true, 0.0);
	g_LPCvarAfkHud = CreateConVar("sm_l4dp_afk_hud", "1", "Show Auto Idle CenterText and Idle Kick hints.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarAdminKickImmunity = CreateConVar("sm_l4dp_afk_admin_kick_immunity", "1", "Allow matching admins to avoid AFK kicks.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarAdminFlags = CreateConVar("sm_l4dp_afk_admin_flags", "z", "Admin flags immune from AFK kicks.", FCVAR_NOTIFY);
	g_LPCvarBhopEnabled = CreateConVar("sm_l4dp_bhop_enabled", "1", "Allow personal automatic bunny hop.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarBhopDefault = CreateConVar("sm_l4dp_bhop_default", "0", "Default bunny hop preference for new clients.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarCsmEnabled = CreateConVar("sm_l4dp_csm_enabled", "1", "Allow survivor character selection.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarAnnounceIdleKick = CreateConVar("sm_l4dp_announce_idle_kick", "1", "Announce idle kicks to other players.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarHumanTeamWipe = CreateConVar("sm_l4dp_human_team_wipe", "1", "Kill remaining Survivor Bots after all participating humans have died.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarAutoJoin = CreateConVar("sm_l4dp_auto_join", "1", "Automatically attempt to join each new human client to Survivors once per connection.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarMidJoinSpawnNearPlayer = CreateConVar("sm_l4dp_midjoin_spawn_near_player", "1", "Place newly created 5+ Survivors near a living teammate after verified takeover.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarMidJoinLoadout = CreateConVar("sm_l4dp_midjoin_loadout", "1", "Give the Players mid-join T1 loadout only after a newly created 5+ Survivor takeover.", FCVAR_NOTIFY, true, 0.0, true, 1.0);

	g_LPCvarEnabled.AddChangeHook(LP_ConVarChanged);
	g_LPCvarSurvivorLimit.AddChangeHook(LP_ConVarChanged);
	g_LPCvarMinSurvivors.AddChangeHook(LP_ConVarChanged);
	g_LPCvarAutoIdleSeconds.AddChangeHook(LP_ConVarChanged);
	g_LPCvarAutoIdleWarning.AddChangeHook(LP_ConVarChanged);
	g_LPCvarIdleKickSeconds.AddChangeHook(LP_ConVarChanged);
	g_LPCvarIdleKickWarning.AddChangeHook(LP_ConVarChanged);
	g_LPCvarAfkHud.AddChangeHook(LP_ConVarChanged);
	g_LPCvarAdminKickImmunity.AddChangeHook(LP_ConVarChanged);
	g_LPCvarAdminFlags.AddChangeHook(LP_ConVarChanged);
	g_LPCvarBhopEnabled.AddChangeHook(LP_ConVarChanged);
	g_LPCvarBhopDefault.AddChangeHook(LP_ConVarChanged);
	g_LPCvarCsmEnabled.AddChangeHook(LP_ConVarChanged);
	g_LPCvarAnnounceIdleKick.AddChangeHook(LP_ConVarChanged);
	g_LPCvarHumanTeamWipe.AddChangeHook(LP_ConVarChanged);
	g_LPCvarAutoJoin.AddChangeHook(LP_ConVarChanged);
	g_LPCvarMidJoinSpawnNearPlayer.AddChangeHook(LP_ConVarChanged);
	g_LPCvarMidJoinLoadout.AddChangeHook(LP_ConVarChanged);
}

public void LP_ConVarChanged(ConVar convar, const char[] oldValue, const char[] newValue)
{
	LP_ApplyConfiguration();
	if (!g_LPEnabled)
	{
		LP_CancelJoinQueue();
		LP_CancelAllSpectateTransitions();
	}
}

float LP_ClampWarning(ConVar warningCvar, float warning, float timeout, const char[] name)
{
	if (timeout <= 0.0)
	{
		return 0.0;
	}
	if (warning > timeout)
	{
		LogError("[Players] %s exceeded its timeout and was clamped to %.0f seconds.", name, timeout);
		warningCvar.FloatValue = timeout;
		return timeout;
	}
	return warning;
}

void LP_ApplyConfiguration()
{
	g_LPEnabled = g_LPCvarEnabled.BoolValue;
	g_LPSurvivorLimit = g_LPCvarSurvivorLimit.IntValue;
	if (g_LPSurvivorLimit > LP_MAX_SURVIVORS)
	{
		g_LPSurvivorLimit = LP_MAX_SURVIVORS;
	}
	if (g_LPSurvivorLimit > MaxClients)
	{
		g_LPSurvivorLimit = MaxClients;
	}
	g_LPMinSurvivors = g_LPCvarMinSurvivors.IntValue;
	if (g_LPMinSurvivors > g_LPSurvivorLimit)
	{
		LP_LogError("sm_l4dp_min_survivors %d exceeded sm_l4dp_survivor_limit %d and was clamped.",
			g_LPMinSurvivors, g_LPSurvivorLimit);
		g_LPMinSurvivors = g_LPSurvivorLimit;
		if (g_LPCvarMinSurvivors.IntValue != g_LPMinSurvivors)
		{
			g_LPCvarMinSurvivors.IntValue = g_LPMinSurvivors;
		}
	}

	g_LPAutoIdleSeconds = g_LPCvarAutoIdleSeconds.FloatValue;
	g_LPAutoIdleWarningSeconds = LP_ClampWarning(g_LPCvarAutoIdleWarning, g_LPCvarAutoIdleWarning.FloatValue, g_LPAutoIdleSeconds, "sm_l4dp_auto_idle_warning_seconds");
	g_LPIdleKickSeconds = g_LPCvarIdleKickSeconds.FloatValue;
	g_LPIdleKickWarningSeconds = LP_ClampWarning(g_LPCvarIdleKickWarning, g_LPCvarIdleKickWarning.FloatValue, g_LPIdleKickSeconds, "sm_l4dp_idle_kick_warning_seconds");
	g_LPAfkHud = g_LPCvarAfkHud.BoolValue;
	g_LPAdminKickImmunity = g_LPCvarAdminKickImmunity.BoolValue;
	g_LPBhopEnabled = g_LPCvarBhopEnabled.BoolValue;
	g_LPBhopDefault = g_LPCvarBhopDefault.BoolValue;
	g_LPCsmEnabled = g_LPCvarCsmEnabled.BoolValue;
	g_LPAnnounceIdleKick = g_LPCvarAnnounceIdleKick.BoolValue;
	g_LPHumanTeamWipeEnabled = g_LPCvarHumanTeamWipe.BoolValue;
	g_LPAutoJoinEnabled = g_LPCvarAutoJoin.BoolValue;
	g_LPMidJoinSpawnNearPlayer = g_LPCvarMidJoinSpawnNearPlayer.BoolValue;
	g_LPMidJoinLoadout = g_LPCvarMidJoinLoadout.BoolValue;
	LP_PopulationConfigurationChanged();
	if (!g_LPEnabled || !g_LPAutoJoinEnabled)
	{
		LP_CancelAllAutoJoinTimers();
	}
	else
	{
		LP_AutoJoinMapStart();
	}
	if (!g_LPEnabled || !g_LPAfkHud)
	{
		LP_StopAllAutoIdleHuds();
		LP_ClearAllIdleKickHints();
	}

	char flags[32];
	g_LPCvarAdminFlags.GetString(flags, sizeof(flags));
	g_LPAdminFlagBits = ReadFlagString(flags);
}

void LP_CheckServerPolicy()
{
	ConVar cvar = FindConVar("director_afk_timeout");
	if (cvar != null && cvar.FloatValue < 3600.0)
	{
		LogError("[Players] Warning: director_afk_timeout may conflict with plugin-managed auto idle.");
	}

	cvar = FindConVar("sb_all_bot_game");
	if (cvar != null && !cvar.BoolValue)
	{
		LogError("[Players] Warning: sb_all_bot_game is disabled; all-idle games may stop.");
	}

	cvar = FindConVar("allow_all_bot_survivor_team");
	if (cvar != null && !cvar.BoolValue)
	{
		LogError("[Players] Warning: allow_all_bot_survivor_team is disabled; all-idle games may stop.");
	}
}
