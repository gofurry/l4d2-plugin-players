#if defined _l4d2_players_hud_included
	#endinput
#endif
#define _l4d2_players_hud_included

void LP_Print(int client, const char[] phrase)
{
	if (LP_IsHumanClient(client))
	{
		PrintToChat(client, "\x04%T\x01 %T", "ChatPrefix", client, phrase, client);
	}
}

void LP_BroadcastIdleKick(int subject)
{
	for (int client = 1; client <= MaxClients; client++)
	{
		if (LP_IsHumanClient(client))
		{
			PrintToChat(client, "\x04%T\x01 %T", "ChatPrefix", client,
				"IdleKickBroadcast", client, subject);
		}
	}
}

void LP_ClearAfkHint(int client)
{
	if (client < 1 || client > MaxClients
		|| g_LPPlayers[client].playersHint == LP_PLAYERS_HINT_NONE)
	{
		return;
	}

	if (IsClientInGame(client) && !IsFakeClient(client))
	{
		Handle message = StartMessageOne("HintText", client, USERMSG_RELIABLE);
		if (message != null)
		{
			BfWriteString(message, "");
			EndMessage();
		}
	}
	switch (g_LPPlayers[client].playersHint)
	{
		case LP_PLAYERS_HINT_AUTO_IDLE: g_LPPlayers[client].autoIdleWarning = false;
		case LP_PLAYERS_HINT_IDLE_KICK: g_LPPlayers[client].idleKickWarning = false;
	}
	g_LPPlayers[client].playersHint = LP_PLAYERS_HINT_NONE;
}

void LP_ClearAllAfkHints()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		LP_ClearAfkHint(client);
	}
}

void LP_ShowAfkHint(int client, LPPlayersHint hint, const char[] phrase, int seconds)
{
	if (!g_LPAfkHud || !LP_IsHumanClient(client))
	{
		LP_ClearAfkHint(client);
		return;
	}

	g_LPPlayers[client].playersHint = hint;
	PrintHintText(client, "%T", phrase, client, seconds);
}

void LP_ShowAutoIdleHint(int client, int seconds)
{
	LP_ShowAfkHint(client, LP_PLAYERS_HINT_AUTO_IDLE, "AutoIdleWarning", seconds);
}

void LP_ShowIdleKickHint(int client, int seconds)
{
	LP_ShowAfkHint(client, LP_PLAYERS_HINT_IDLE_KICK, "IdleKickWarning", seconds);
}
