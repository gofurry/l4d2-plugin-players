#if defined _l4d2_players_commands_included
	#endinput
#endif
#define _l4d2_players_commands_included

void LP_RegisterCommands()
{
	RegConsoleCmd("sm_player", LP_CommandPlayerMenu);
	RegConsoleCmd("sm_players", LP_CommandPlayerMenu);
	RegConsoleCmd("sm_p", LP_CommandPlayerMenu);
	RegConsoleCmd("sm_afk", LP_CommandAfkPublic);
	RegConsoleCmd("sm_spec", LP_CommandSpectatePublic);
	RegConsoleCmd("sm_spectate", LP_CommandSpectatePublic);
	RegConsoleCmd("sm_join", LP_CommandJoinPublic);
	RegConsoleCmd("sm_zs", LP_CommandSuicidePublic);
	RegConsoleCmd("sm_bhop", LP_CommandBhopPublic);
	RegConsoleCmd("sm_csm", LP_CommandCharacterPublic);
	RegAdminCmd("sm_l4dp_addbot", LP_CommandAddBot, ADMFLAG_ROOT,
		"Admin diagnostic: create unlimited Survivor Bots through the Players engine path. Usage: sm_l4dp_addbot [count]");
}

public Action LP_CommandAddBot(int client, int args)
{
	if (!g_LPEnabled || g_LPMapEnding)
	{
		ReplyToCommand(client, "[Players] Diagnostic Bot creation is unavailable while the plugin is disabled or the map is ending.");
		return Plugin_Handled;
	}

	int requested = 1;
	if (args >= 1)
	{
		char value[16];
		GetCmdArg(1, value, sizeof(value));
		requested = StringToInt(value);
		if (requested < 1)
		{
			ReplyToCommand(client, "[Players] Usage: sm_l4dp_addbot [positive count]");
			return Plugin_Handled;
		}
	}

	int created = 0;
	for (int i = 0; i < requested; i++)
	{
		int bot = LP_CreateUnlimitedSurvivorBot("admin_diagnostic");
		if (bot <= 0)
		{
			break;
		}
		created++;
	}

	ReplyToCommand(client, "[Players] Admin diagnostic created %d/%d requested Survivor Bots; Survivors=%d, Players limit=%d.",
		created, requested, LP_GetSurvivorCount(), LP_GetConfiguredSurvivorCapacity());
	if (created < requested)
	{
		LP_LogError("Admin diagnostic stopped after %d/%d Bots; see the preceding creation failure.", created, requested);
	}
	return Plugin_Handled;
}

void LP_ShowPlayerMenu(int client)
{
	if (!LP_IsHumanClient(client))
	{
		return;
	}
	if (!g_LPEnabled)
	{
		LP_Print(client, "PluginDisabled");
		return;
	}

	Menu menu = new Menu(LP_PlayerMenuHandler);
	char text[96];
	Format(text, sizeof(text), "%T", "MenuTitle", client);
	menu.SetTitle(text);
	Format(text, sizeof(text), "%T", "MenuIdle", client);
	menu.AddItem("afk", text);
	Format(text, sizeof(text), "%T", "MenuSpectate", client);
	menu.AddItem("spectate", text);
	Format(text, sizeof(text), "%T", "MenuJoin", client);
	menu.AddItem("join", text);
	Format(text, sizeof(text), "%T", "MenuSuicide", client);
	menu.AddItem("suicide", text);
	Format(text, sizeof(text), "%T", g_LPPlayers[client].bhopEnabled ? "MenuBhopOn" : "MenuBhopOff", client);
	menu.AddItem("bhop", text, g_LPBhopEnabled ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
	Format(text, sizeof(text), "%T", "MenuCharacter", client);
	menu.AddItem("character", text, g_LPCsmEnabled ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
	menu.Display(client, MENU_TIME_FOREVER);
}

public int LP_PlayerMenuHandler(Menu menu, MenuAction action, int client, int item)
{
	if (action == MenuAction_End)
	{
		delete menu;
		return 0;
	}
	if (action != MenuAction_Select || !LP_IsHumanClient(client))
	{
		return 0;
	}

	char info[16];
	menu.GetItem(item, info, sizeof(info));
	if (StrEqual(info, "afk")) LP_CommandAfk(client);
	else if (StrEqual(info, "spectate")) LP_CommandSpectate(client);
	else if (StrEqual(info, "join")) LP_CommandJoin(client);
	else if (StrEqual(info, "suicide")) LP_CommandSuicide(client);
	else if (StrEqual(info, "bhop")) LP_CommandBhop(client);
	else if (StrEqual(info, "character")) LP_CommandCharacter(client);
	return 0;
}

public Action LP_CommandPlayerMenu(int client, int args)
{
	if (LP_IsHumanClient(client))
	{
		LP_RecordActivity(client);
		LP_ShowPlayerMenu(client);
	}
	return Plugin_Handled;
}

public Action LP_CommandAfkPublic(int client, int args) { return LP_CommandAfk(client); }
public Action LP_CommandSpectatePublic(int client, int args) { return LP_CommandSpectate(client); }
public Action LP_CommandJoinPublic(int client, int args) { return LP_CommandJoin(client); }
public Action LP_CommandSuicidePublic(int client, int args) { return LP_CommandSuicide(client); }
public Action LP_CommandBhopPublic(int client, int args) { return LP_CommandBhop(client); }
public Action LP_CommandCharacterPublic(int client, int args) { return LP_CommandCharacter(client); }
