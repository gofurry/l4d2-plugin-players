#if defined _l4d2_players_suicide_included
	#endinput
#endif
#define _l4d2_players_suicide_included

Action LP_CommandSuicide(int client)
{
	if (!LP_IsHumanClient(client))
	{
		return Plugin_Handled;
	}
	LP_RecordActivity(client);
	if (!g_LPEnabled)
	{
		LP_Print(client, "PluginDisabled");
		return Plugin_Handled;
	}
	if (!LP_IsActiveSurvivor(client))
	{
		LP_Print(client, "MustBeActiveSurvivor");
		return Plugin_Handled;
	}
	if (!IsPlayerAlive(client))
	{
		LP_Print(client, "MustBeAlive");
		return Plugin_Handled;
	}
	ForcePlayerSuicide(client);
	LP_Log("Client %N used suicide.", client);
	return Plugin_Handled;
}
