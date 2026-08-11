#if defined _l4d2_players_population_included
	#endinput
#endif
#define _l4d2_players_population_included

Handle g_LPPopulationTimer;
bool g_LPPopulationInitializationWindow;
bool g_LPPopulationReconciledThisRound;
bool g_LPPopulationReady;
int g_LPPopulationAttempts;

void LP_InitializePopulation()
{
	HookEvent("round_start", LP_PopulationRoundStart, EventHookMode_PostNoCopy);
	HookEvent("round_end", LP_PopulationRoundEnd, EventHookMode_PostNoCopy);
	g_LPPopulationInitializationWindow = false;
	g_LPPopulationReconciledThisRound = false;
	g_LPPopulationReady = false;
	g_LPPopulationAttempts = 0;
}

void LP_CancelPopulationTimer()
{
	delete g_LPPopulationTimer;
	g_LPPopulationTimer = null;
}

bool LP_HasHumanClientInGame()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		if (LP_IsHumanClient(client))
		{
			return true;
		}
	}
	return false;
}

void LP_SchedulePopulationReconcile(float delay)
{
	if (!g_LPEnabled || g_LPMapEnding || !g_LPPopulationInitializationWindow
		|| g_LPPopulationReconciledThisRound || g_LPPopulationTimer != null
		|| !LP_HasHumanClientInGame())
	{
		return;
	}
	g_LPPopulationTimer = CreateTimer(delay, LP_TimerReconcilePopulation, _, TIMER_FLAG_NO_MAPCHANGE);
}

void LP_ResetPopulationInitialization()
{
	LP_CancelPopulationTimer();
	g_LPPopulationInitializationWindow = true;
	g_LPPopulationReconciledThisRound = false;
	g_LPPopulationReady = false;
	g_LPPopulationAttempts = 0;
}

void LP_PopulationMapStart()
{
	LP_ResetPopulationInitialization();
	LP_SchedulePopulationReconcile(LP_POPULATION_RECONCILE_DELAY);
}

void LP_PopulationMapEnd()
{
	LP_CancelPopulationTimer();
	g_LPPopulationInitializationWindow = false;
	g_LPPopulationReady = false;
}

void LP_PopulationClientPutInServer(int client)
{
	if (!LP_IsHumanClient(client) || g_LPPopulationReconciledThisRound)
	{
		return;
	}
	g_LPPopulationInitializationWindow = true;
	LP_SchedulePopulationReconcile(LP_POPULATION_RECONCILE_DELAY);
}

bool LP_IsPopulationReady()
{
	return g_LPPopulationReady && g_LPPopulationReconciledThisRound;
}

bool LP_IsPopulationBaselineSatisfied()
{
	return LP_GetSurvivorCount() >= g_LPMinSurvivors;
}

void LP_PopulationConfigurationChanged()
{
	if (!g_LPEnabled)
	{
		LP_CancelPopulationTimer();
		return;
	}
	if (g_LPPopulationInitializationWindow && !g_LPPopulationReconciledThisRound)
	{
		LP_SchedulePopulationReconcile(LP_POPULATION_RECONCILE_DELAY);
	}
}

public Action LP_TimerReconcilePopulation(Handle timer)
{
	g_LPPopulationTimer = null;
	if (!g_LPEnabled || g_LPMapEnding || !g_LPPopulationInitializationWindow
		|| g_LPPopulationReconciledThisRound || !LP_HasHumanClientInGame())
	{
		return Plugin_Stop;
	}

	g_LPPopulationAttempts++;
	int before = LP_GetSurvivorCount();
	while (LP_GetSurvivorCount() < g_LPMinSurvivors)
	{
		if (LP_CreateUnlimitedSurvivorBot("population") <= 0)
		{
			break;
		}
	}

	int after = LP_GetSurvivorCount();
	if (after >= g_LPMinSurvivors)
	{
		g_LPPopulationReady = true;
		g_LPPopulationReconciledThisRound = true;
		g_LPPopulationInitializationWindow = false;
		LP_Log("Population reconcile succeeded: before=%d after=%d baseline=%d attempt=%d.",
			before, after, g_LPMinSurvivors, g_LPPopulationAttempts);
		return Plugin_Stop;
	}

	if (g_LPPopulationAttempts < LP_POPULATION_MAX_ATTEMPTS)
	{
		LP_LogError("Population reconcile retry: before=%d after=%d baseline=%d attempt=%d/%d.",
			before, after, g_LPMinSurvivors, g_LPPopulationAttempts, LP_POPULATION_MAX_ATTEMPTS);
		LP_SchedulePopulationReconcile(LP_POPULATION_RETRY_DELAY);
		return Plugin_Stop;
	}

	g_LPPopulationReady = false;
	g_LPPopulationReconciledThisRound = true;
	g_LPPopulationInitializationWindow = false;
	LP_LogError("FATAL population reconcile failed: survivors=%d baseline=%d attempts=%d; Auto Join remains blocked.",
		after, g_LPMinSurvivors, g_LPPopulationAttempts);
	return Plugin_Stop;
}

public void LP_PopulationRoundStart(Event event, const char[] name, bool dontBroadcast)
{
	LP_ResetPopulationInitialization();
	LP_SchedulePopulationReconcile(LP_POPULATION_RECONCILE_DELAY);
}

public void LP_PopulationRoundEnd(Event event, const char[] name, bool dontBroadcast)
{
	LP_CancelPopulationTimer();
	g_LPPopulationInitializationWindow = false;
	g_LPPopulationReady = false;
}
