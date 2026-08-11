#if defined _l4d2_players_config_included
	#endinput
#endif
#define _l4d2_players_config_included

ConVar g_LPCvarEnabled;
ConVar g_LPCvarSurvivorLimit;
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

bool g_LPEnabled;
int g_LPSurvivorLimit;
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

void LP_CreateConfig()
{
	g_LPCvarEnabled = CreateConVar("sm_l4dp_enabled", "1", "Enable L4D2 Players.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarSurvivorLimit = CreateConVar("sm_l4dp_survivor_limit", "4", "Maximum survivor clients managed by Players.", FCVAR_NOTIFY, true, 1.0, true, 16.0);
	g_LPCvarAutoIdleSeconds = CreateConVar("sm_l4dp_auto_idle_seconds", "120", "Seconds of inactivity before automatic idle; 0 disables.", FCVAR_NOTIFY, true, 0.0);
	g_LPCvarAutoIdleWarning = CreateConVar("sm_l4dp_auto_idle_warning_seconds", "10", "Warning lead time for automatic idle.", FCVAR_NOTIFY, true, 0.0);
	g_LPCvarIdleKickSeconds = CreateConVar("sm_l4dp_idle_kick_seconds", "600", "Seconds in engine idle before kick; 0 disables.", FCVAR_NOTIFY, true, 0.0);
	g_LPCvarIdleKickWarning = CreateConVar("sm_l4dp_idle_kick_warning_seconds", "60", "Warning lead time for idle kick.", FCVAR_NOTIFY, true, 0.0);
	g_LPCvarAfkHud = CreateConVar("sm_l4dp_afk_hud", "1", "Show AFK and kick hints.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarAdminKickImmunity = CreateConVar("sm_l4dp_afk_admin_kick_immunity", "1", "Allow matching admins to avoid AFK kicks.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarAdminFlags = CreateConVar("sm_l4dp_afk_admin_flags", "z", "Admin flags immune from AFK kicks.", FCVAR_NOTIFY);
	g_LPCvarBhopEnabled = CreateConVar("sm_l4dp_bhop_enabled", "1", "Allow personal automatic bunny hop.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarBhopDefault = CreateConVar("sm_l4dp_bhop_default", "0", "Default bunny hop preference for new clients.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarCsmEnabled = CreateConVar("sm_l4dp_csm_enabled", "1", "Allow survivor character selection.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarAnnounceIdleKick = CreateConVar("sm_l4dp_announce_idle_kick", "1", "Announce idle kicks to other players.", FCVAR_NOTIFY, true, 0.0, true, 1.0);
	g_LPCvarHumanTeamWipe = CreateConVar("sm_l4dp_human_team_wipe", "1", "Kill remaining Survivor Bots after all participating humans have died.", FCVAR_NOTIFY, true, 0.0, true, 1.0);

	g_LPCvarEnabled.AddChangeHook(LP_ConVarChanged);
	g_LPCvarSurvivorLimit.AddChangeHook(LP_ConVarChanged);
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
	if (!g_LPEnabled || !g_LPAfkHud)
	{
		LP_ClearAllAfkHints();
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
