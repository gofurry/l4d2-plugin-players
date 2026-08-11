#if defined _l4d2_players_join_included
	#endinput
#endif
#define _l4d2_players_join_included

int g_LPJoinQueue[LP_JOIN_QUEUE_SIZE];
int g_LPJoinQueueCount;

void LP_InitializeJoin()
{
	LP_CancelJoinQueue();
}

void LP_CancelJoinQueue()
{
	LP_CancelAllTakeovers();
	g_LPJoinQueueCount = 0;
	for (int client = 1; client <= MaxClients; client++)
	{
		g_LPBotReservation[client] = 0;
	}
}

bool LP_IsJoinQueued(int serial)
{
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

void LP_ProcessJoinQueue()
{
	if (LP_HasPendingTakeover() || g_LPMapEnding || !g_LPEnabled)
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

	bot = LP_CreateUnlimitedSurvivorBot("join");
	if (bot <= 0)
	{
		LP_Print(client, "BotCreationFailed");
		RequestFrame(LP_FrameContinueJoinQueue);
		return;
	}

	g_LPBotReservation[bot] = serial;
	if (!LP_BindAndTakeOverBot(client, bot))
	{
		g_LPBotReservation[bot] = 0;
		LP_Print(client, "TakeoverFailed");
		RequestFrame(LP_FrameContinueJoinQueue);
	}
}

public void LP_FrameContinueJoinQueue(any data)
{
	LP_ProcessJoinQueue();
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
	if (LP_IsSpectatePendingForClient(client))
	{
		LP_Print(client, "SpectatePending");
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
		LP_StopAutoIdleHud(client);
		if (!g_LPEnabled)
		{
			LP_Print(client, "PluginDisabled");
			return Plugin_Handled;
		}
		LP_JoinSurvivor(client);
	}
	return Plugin_Handled;
}
