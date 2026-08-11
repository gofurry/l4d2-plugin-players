#if defined _l4d2_players_join_included
	#endinput
#endif
#define _l4d2_players_join_included

int g_LPJoinQueue[LP_JOIN_QUEUE_SIZE];
int g_LPJoinQueueCount;
bool g_LPCreatePending;
int g_LPPendingJoinSerial;
float g_LPCreateStartedAt;
int g_LPCreateFrames;
int g_LPBotSnapshot[MAXPLAYERS + 1];
int g_LPBotSnapshotCount;
int g_LPNextBotCharacter;

void LP_InitializeJoin()
{
	LP_CancelJoinQueue();
}

void LP_CancelJoinQueue()
{
	LP_CancelAllTakeovers();
	g_LPJoinQueueCount = 0;
	g_LPCreatePending = false;
	g_LPPendingJoinSerial = 0;
	g_LPCreateFrames = 0;
	g_LPBotSnapshotCount = 0;
	for (int client = 1; client <= MaxClients; client++)
	{
		g_LPBotReservation[client] = 0;
	}
}

bool LP_IsJoinQueued(int serial)
{
	if (g_LPCreatePending && g_LPPendingJoinSerial == serial)
	{
		return true;
	}
	for (int i = 0; i < g_LPJoinQueueCount; i++)
	{
		if (g_LPJoinQueue[i] == serial)
		{
			return true;
		}
	}
	return false;
}

void LP_EnqueueJoin(int client)
{
	int serial = GetClientSerial(client);
	if (LP_IsJoinQueued(serial))
	{
		LP_Print(client, "JoinPending");
		return;
	}
	if (g_LPJoinQueueCount >= LP_JOIN_QUEUE_SIZE)
	{
		LP_Print(client, "JoinFailed");
		return;
	}
	g_LPJoinQueue[g_LPJoinQueueCount++] = serial;
	LP_Print(client, "CreatingBot");
	LP_ProcessJoinQueue();
}

int LP_DequeueJoin()
{
	while (g_LPJoinQueueCount > 0)
	{
		int serial = g_LPJoinQueue[0];
		for (int i = 1; i < g_LPJoinQueueCount; i++)
		{
			g_LPJoinQueue[i - 1] = g_LPJoinQueue[i];
		}
		g_LPJoinQueueCount--;
		if (LP_IsHumanClient(GetClientFromSerial(serial)))
		{
			return serial;
		}
	}
	return 0;
}

void LP_SnapshotSurvivorBots()
{
	g_LPBotSnapshotCount = 0;
	for (int bot = 1; bot <= MaxClients; bot++)
	{
		if (LP_IsSurvivorBot(bot))
		{
			g_LPBotSnapshot[g_LPBotSnapshotCount++] = GetClientUserId(bot);
		}
	}
}

bool LP_BotWasInSnapshot(int bot)
{
	int userId = GetClientUserId(bot);
	for (int i = 0; i < g_LPBotSnapshotCount; i++)
	{
		if (g_LPBotSnapshot[i] == userId)
		{
			return true;
		}
	}
	return false;
}

int LP_FindNewSurvivorBot()
{
	for (int bot = 1; bot <= MaxClients; bot++)
	{
		if (LP_IsSurvivorBot(bot) && !LP_BotWasInSnapshot(bot) && g_LPBotReservation[bot] == 0)
		{
			return bot;
		}
	}
	return 0;
}

void LP_ProcessJoinQueue()
{
	if (g_LPCreatePending || LP_HasPendingTakeover() || g_LPMapEnding || !g_LPEnabled)
	{
		return;
	}

	int serial = LP_DequeueJoin();
	if (serial == 0)
	{
		return;
	}
	int client = GetClientFromSerial(serial);
	if (LP_IsActiveSurvivor(client))
	{
		RequestFrame(LP_FrameContinueJoinQueue);
		return;
	}

	int bot = LP_FindIdleBotForHuman(client);
	if (bot > 0)
	{
		if (!LP_ReturnFromIdleBot(client, bot))
		{
			g_LPBotReservation[bot] = 0;
			LP_Print(client, "TakeoverFailed");
			RequestFrame(LP_FrameContinueJoinQueue);
		}
		return;
	}

	bot = LP_FindAvailableSurvivorBot();
	if (bot > 0)
	{
		if (!LP_BindAndTakeOverBot(client, bot))
		{
			g_LPBotReservation[bot] = 0;
			LP_Print(client, "TakeoverFailed");
			RequestFrame(LP_FrameContinueJoinQueue);
		}
		return;
	}
	if (!LP_IsFreeSpectator(client))
	{
		LP_Print(client, "MustBeSpectator");
		RequestFrame(LP_FrameContinueJoinQueue);
		return;
	}
	if (!LP_CanCreateSurvivorBot(client))
	{
		RequestFrame(LP_FrameContinueJoinQueue);
		return;
	}

	g_LPCreatePending = true;
	g_LPPendingJoinSerial = serial;
	g_LPCreateStartedAt = GetGameTime();
	g_LPCreateFrames = 0;
	LP_SnapshotSurvivorBots();
	LP_EngineAddSurvivorBot(g_LPNextBotCharacter);
	g_LPNextBotCharacter = (g_LPNextBotCharacter + 1) % 4;
	RequestFrame(LP_FrameVerifyCreatedBot);
}

public void LP_FrameContinueJoinQueue(any data)
{
	LP_ProcessJoinQueue();
}

public void LP_FrameVerifyCreatedBot(any data)
{
	if (!g_LPCreatePending)
	{
		return;
	}

	int client = GetClientFromSerial(g_LPPendingJoinSerial);
	if (!LP_IsHumanClient(client))
	{
		g_LPCreatePending = false;
		g_LPPendingJoinSerial = 0;
		LP_ProcessJoinQueue();
		return;
	}

	int bot = LP_FindNewSurvivorBot();
	if (bot > 0)
	{
		g_LPBotReservation[bot] = g_LPPendingJoinSerial;
		g_LPCreatePending = false;
		g_LPPendingJoinSerial = 0;
		if (!LP_BindAndTakeOverBot(client, bot))
		{
			g_LPBotReservation[bot] = 0;
			LP_Print(client, "TakeoverFailed");
			LP_ProcessJoinQueue();
		}
		return;
	}

	g_LPCreateFrames++;
	if (g_LPCreateFrames >= LP_CREATE_MAX_FRAMES || GetGameTime() - g_LPCreateStartedAt >= LP_CREATE_TIMEOUT)
	{
		LP_LogError("Survivor bot creation timed out for %N.", client);
		LP_Print(client, "BotCreationFailed");
		g_LPCreatePending = false;
		g_LPPendingJoinSerial = 0;
		LP_ProcessJoinQueue();
		return;
	}
	RequestFrame(LP_FrameVerifyCreatedBot);
}

bool LP_JoinSurvivor(int client)
{
	if (!g_LPEnabled || !LP_IsHumanClient(client))
	{
		return false;
	}
	if (LP_IsActiveSurvivor(client))
	{
		LP_Print(client, "AlreadyPlaying");
		return false;
	}

	if (LP_IsTakeoverPendingForClient(client))
	{
		LP_Print(client, "JoinPending");
		return true;
	}

	int bot = LP_FindIdleBotForHuman(client);
	if (bot > 0)
	{
		if (LP_ReturnFromIdleBot(client, bot))
		{
			return true;
		}
		g_LPBotReservation[bot] = 0;
		LP_Print(client, "TakeoverFailed");
		return false;
	}

	bot = LP_FindAvailableSurvivorBot();
	if (bot > 0)
	{
		if (LP_BindAndTakeOverBot(client, bot))
		{
			return true;
		}
		g_LPBotReservation[bot] = 0;
		LP_Print(client, "TakeoverFailed");
		return false;
	}

	if (!LP_IsFreeSpectator(client))
	{
		LP_Print(client, "MustBeSpectator");
		return false;
	}
	if (!LP_CanCreateSurvivorBot(client))
	{
		return false;
	}
	LP_EnqueueJoin(client);
	return true;
}

void LP_JoinClientDisconnected(int client)
{
	LP_CancelTakeoverForClient(client);
	int serial = GetClientSerial(client);
	for (int i = 0; i < g_LPJoinQueueCount; )
	{
		if (g_LPJoinQueue[i] == serial)
		{
			for (int j = i + 1; j < g_LPJoinQueueCount; j++)
			{
				g_LPJoinQueue[j - 1] = g_LPJoinQueue[j];
			}
			g_LPJoinQueueCount--;
			continue;
		}
		i++;
	}
}

Action LP_CommandJoin(int client)
{
	if (LP_IsHumanClient(client))
	{
		LP_RecordActivity(client);
		if (!g_LPEnabled)
		{
			LP_Print(client, "PluginDisabled");
			return Plugin_Handled;
		}
		LP_JoinSurvivor(client);
	}
	return Plugin_Handled;
}
