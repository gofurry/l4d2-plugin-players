#if defined _l4d2_players_afk_monitor_included
	#endinput
#endif
#define _l4d2_players_afk_monitor_included

Handle g_LPAfkTimer;

void LP_InitializeAfkMonitor()
{
	HookEvent("round_start", LP_EventRoundStart, EventHookMode_PostNoCopy);
	HookEvent("round_end", LP_EventRoundEnd, EventHookMode_PostNoCopy);
	HookEvent("player_spawn", LP_EventPlayerSpawn, EventHookMode_Post);
	LP_StartAfkMonitor();
}

public void LP_EventRoundStart(Event event, const char[] name, bool dontBroadcast)
{
	g_LPMapEnding = false;
	LP_ResetMapRuntime();
	LP_StartAfkMonitor();
}

public void LP_EventRoundEnd(Event event, const char[] name, bool dontBroadcast)
{
	g_LPMapEnding = true;
	LP_CancelJoinQueue();
}

public void LP_EventPlayerSpawn(Event event, const char[] name, bool dontBroadcast)
{
	int client = GetClientOfUserId(event.GetInt("userid"));
	if (LP_IsHumanClient(client))
	{
		LP_RecordActivity(client);
	}
}

void LP_StartAfkMonitor()
{
	if (g_LPAfkTimer == null)
	{
		g_LPAfkTimer = CreateTimer(1.0, LP_TimerAfkMonitor, _, TIMER_REPEAT);
	}
}

void LP_StopAfkMonitor()
{
	delete g_LPAfkTimer;
	g_LPAfkTimer = null;
}

float LP_AngleDelta(float first, float second)
{
	float delta = FloatAbs(first - second);
	return delta > 180.0 ? 360.0 - delta : delta;
}

void LP_DetectRunCmdActivity(int client, int buttons, const float vel[3], const float angles[3], int weapon)
{
	bool active = buttons != 0 || FloatAbs(vel[0]) > 0.01 || FloatAbs(vel[1]) > 0.01 || FloatAbs(vel[2]) > 0.01;
	if (weapon != 0 && weapon != g_LPPlayers[client].lastWeapon)
	{
		active = true;
	}
	if (g_LPPlayers[client].hasEyeAngles)
	{
		if (LP_AngleDelta(angles[0], g_LPPlayers[client].lastEyeAngles[0]) >= LP_EYE_ACTIVITY_THRESHOLD
			|| LP_AngleDelta(angles[1], g_LPPlayers[client].lastEyeAngles[1]) >= LP_EYE_ACTIVITY_THRESHOLD)
		{
			active = true;
		}
	}

	g_LPPlayers[client].lastEyeAngles[0] = angles[0];
	g_LPPlayers[client].lastEyeAngles[1] = angles[1];
	g_LPPlayers[client].lastEyeAngles[2] = angles[2];
	g_LPPlayers[client].hasEyeAngles = true;
	g_LPPlayers[client].lastButtons = buttons;
	if (weapon != 0)
	{
		g_LPPlayers[client].lastWeapon = weapon;
	}
	if (active)
	{
		LP_RecordActivity(client);
	}
}

void LP_UpdateActiveClient(int client, float now)
{
	if (g_LPPlayers[client].wasIdle || g_LPPlayers[client].wasFreeSpectator)
	{
		g_LPPlayers[client].lastActivityTime = now;
	}
	g_LPPlayers[client].wasIdle = false;
	g_LPPlayers[client].wasFreeSpectator = false;
	g_LPPlayers[client].idleSince = 0.0;
	g_LPPlayers[client].spectatorSince = 0.0;
	g_LPPlayers[client].idleKickWarning = false;
	g_LPPlayers[client].spectatorKickWarning = false;

	if (g_LPAutoIdleSeconds <= 0.0 || !IsPlayerAlive(client))
	{
		return;
	}

	float inactive = now - g_LPPlayers[client].lastActivityTime;
	float warningAt = g_LPAutoIdleSeconds - g_LPAutoIdleWarningSeconds;
	if (g_LPAutoIdleWarningSeconds > 0.0 && inactive >= warningAt && inactive < g_LPAutoIdleSeconds)
	{
		g_LPPlayers[client].autoIdleWarning = true;
		LP_ShowAutoIdleHint(client, RoundToCeil(g_LPAutoIdleSeconds - inactive));
	}
	if (inactive >= g_LPAutoIdleSeconds)
	{
		LP_GoIdle(client, LP_IDLE_REASON_AUTOMATIC, false);
	}
}

void LP_UpdateIdleClient(int client, float now)
{
	if (!g_LPPlayers[client].wasIdle || g_LPPlayers[client].idleSince <= 0.0)
	{
		g_LPPlayers[client].idleSince = now;
	}
	g_LPPlayers[client].wasIdle = true;
	g_LPPlayers[client].wasFreeSpectator = false;
	g_LPPlayers[client].spectatorSince = 0.0;
	g_LPPlayers[client].autoIdleWarning = false;
	g_LPPlayers[client].spectatorKickWarning = false;

	if (g_LPIdleKickSeconds <= 0.0 || LP_HasAdminKickImmunity(client))
	{
		return;
	}

	float elapsed = now - g_LPPlayers[client].idleSince;
	float remaining = g_LPIdleKickSeconds - elapsed;
	if (remaining <= 0.0)
	{
		if (g_LPAnnounceIdleKick)
		{
			PrintToChatAll("%s %T", LP_PREFIX, "IdleKickBroadcast", LANG_SERVER, client);
		}
		LP_Log("Idle kick for %N after %.0f seconds.", client, elapsed);
		KickClient(client, "Idle timeout");
		return;
	}
	if (g_LPIdleKickWarningSeconds > 0.0 && remaining <= g_LPIdleKickWarningSeconds)
	{
		g_LPPlayers[client].idleKickWarning = true;
		LP_ShowIdleKickHint(client, RoundToCeil(remaining));
	}
}

void LP_UpdateFreeSpectator(int client, float now)
{
	if (!g_LPPlayers[client].wasFreeSpectator || g_LPPlayers[client].spectatorSince <= 0.0)
	{
		g_LPPlayers[client].spectatorSince = now;
	}
	g_LPPlayers[client].wasFreeSpectator = true;
	g_LPPlayers[client].wasIdle = false;
	g_LPPlayers[client].idleSince = 0.0;
	g_LPPlayers[client].autoIdleWarning = false;
	g_LPPlayers[client].idleKickWarning = false;

	if (g_LPSpectatorKickSeconds <= 0.0 || LP_HasAdminKickImmunity(client))
	{
		return;
	}

	float elapsed = now - g_LPPlayers[client].spectatorSince;
	if (elapsed < g_LPSpectatorGraceSeconds)
	{
		return;
	}
	float timeoutElapsed = elapsed - g_LPSpectatorGraceSeconds;
	float remaining = g_LPSpectatorKickSeconds - timeoutElapsed;
	if (remaining <= 0.0)
	{
		LP_Log("Free spectator kick for %N after %.0f seconds.", client, timeoutElapsed);
		KickClient(client, "Spectator timeout");
		return;
	}
	float warningWindow = g_LPSpectatorKickSeconds < 60.0 ? g_LPSpectatorKickSeconds : 60.0;
	if (warningWindow > 0.0 && remaining <= warningWindow)
	{
		g_LPPlayers[client].spectatorKickWarning = true;
		LP_ShowSpectatorKickHint(client, RoundToCeil(remaining));
	}
}

public Action LP_TimerAfkMonitor(Handle timer)
{
	if (!g_LPEnabled || g_LPMapEnding)
	{
		return Plugin_Continue;
	}

	float now = GetGameTime();
	for (int client = 1; client <= MaxClients; client++)
	{
		if (!LP_IsHumanClient(client))
		{
			continue;
		}
		switch (LP_GetPlayerState(client))
		{
			case LP_PLAYER_STATE_SURVIVOR_ACTIVE: LP_UpdateActiveClient(client, now);
			case LP_PLAYER_STATE_SURVIVOR_IDLE: LP_UpdateIdleClient(client, now);
			case LP_PLAYER_STATE_SPECTATOR_FREE: LP_UpdateFreeSpectator(client, now);
		}
	}
	return Plugin_Continue;
}
