#if defined _l4d2_players_hud_included
	#endinput
#endif
#define _l4d2_players_hud_included

void LP_Print(int client, const char[] phrase)
{
	if (LP_IsHumanClient(client))
	{
		PrintToChat(client, "%s %T", LP_PREFIX, phrase, client);
	}
}

void LP_ShowAutoIdleHint(int client, int seconds)
{
	if (g_LPAfkHud)
	{
		PrintHintText(client, "%T", "AutoIdleWarning", client, seconds);
	}
}

void LP_ShowIdleKickHint(int client, int seconds)
{
	if (g_LPAfkHud)
	{
		PrintHintText(client, "%T", "IdleKickWarning", client, seconds);
	}
}

void LP_ShowSpectatorKickHint(int client, int seconds)
{
	if (g_LPAfkHud)
	{
		PrintHintText(client, "%T", "SpectatorKickWarning", client, seconds);
	}
}
