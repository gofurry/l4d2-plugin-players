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

void LP_StopAutoIdleHud(int client)
{
	if (client < 1 || client > MaxClients
		|| !g_LPPlayers[client].autoIdleHudVisible)
	{
		return;
	}

	g_LPPlayers[client].autoIdleHudVisible = false;
	if (IsClientInGame(client) && !IsFakeClient(client))
	{
		PrintCenterText(client, " ");
	}
}

void LP_StopAllAutoIdleHuds()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		LP_StopAutoIdleHud(client);
	}
}

void LP_ShowAutoIdleCenterText(int client, int seconds)
{
	if (!g_LPAfkHud || !LP_IsActiveSurvivor(client)
		|| LP_IsAutomaticIdleVerificationPending(client))
	{
		LP_StopAutoIdleHud(client);
		return;
	}

	g_LPPlayers[client].autoIdleHudVisible = true;
	PrintCenterText(client, "%T", "AutoIdleWarning", client, seconds);
}

void LP_ClearIdleKickHint(int client)
{
	if (client < 1 || client > MaxClients || !g_LPPlayers[client].idleKickWarning)
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
	g_LPPlayers[client].idleKickWarning = false;
}

void LP_ClearAllIdleKickHints()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		LP_ClearIdleKickHint(client);
	}
}

void LP_ShowIdleKickHint(int client, int seconds)
{
	if (!g_LPAfkHud || !LP_IsHumanClient(client))
	{
		LP_ClearIdleKickHint(client);
		return;
	}

	g_LPPlayers[client].idleKickWarning = true;
	PrintHintText(client, "%T", "IdleKickWarning", client, seconds);
}
