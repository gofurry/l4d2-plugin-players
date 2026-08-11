#if defined _l4d2_players_character_included
	#endinput
#endif
#define _l4d2_players_character_included

static const char g_LPCharacterNames[][] =
{
	"Nick", "Rochelle", "Coach", "Ellis", "Bill", "Zoey", "Francis", "Louis"
};

static const char g_LPCharacterModels[][] =
{
	"models/survivors/survivor_gambler.mdl",
	"models/survivors/survivor_producer.mdl",
	"models/survivors/survivor_coach.mdl",
	"models/survivors/survivor_mechanic.mdl",
	"models/survivors/survivor_namvet.mdl",
	"models/survivors/survivor_teenangst.mdl",
	"models/survivors/survivor_biker.mdl",
	"models/survivors/survivor_manager.mdl"
};

void LP_PrecacheCharacterModels()
{
	for (int i = 0; i < sizeof(g_LPCharacterModels); i++)
	{
		PrecacheModel(g_LPCharacterModels[i], true);
	}
}

bool LP_CanChangeCharacter(int client, bool notify = true)
{
	if (!g_LPEnabled || !g_LPCsmEnabled)
	{
		if (notify) LP_Print(client, "CharacterUnavailable");
		return false;
	}
	if (g_LPMapEnding)
	{
		if (notify) LP_Print(client, "MapChanging");
		return false;
	}
	if (!LP_IsActiveSurvivor(client))
	{
		if (notify) LP_Print(client, "MustBeActiveSurvivor");
		return false;
	}
	if (!IsPlayerAlive(client))
	{
		if (notify) LP_Print(client, "MustBeAlive");
		return false;
	}
	return true;
}

void LP_ShowCharacterMenu(int client)
{
	if (!LP_CanChangeCharacter(client))
	{
		return;
	}

	Menu menu = new Menu(LP_CharacterMenuHandler);
	char title[64];
	Format(title, sizeof(title), "%T", "CharacterMenuTitle", client);
	menu.SetTitle(title);
	for (int i = 0; i < sizeof(g_LPCharacterNames); i++)
	{
		char info[8];
		IntToString(i, info, sizeof(info));
		menu.AddItem(info, g_LPCharacterNames[i]);
	}
	menu.ExitBackButton = true;
	menu.Display(client, MENU_TIME_FOREVER);
}

public int LP_CharacterMenuHandler(Menu menu, MenuAction action, int client, int item)
{
	if (action == MenuAction_End)
	{
		delete menu;
		return 0;
	}
	if (action == MenuAction_Cancel && item == MenuCancel_ExitBack && LP_IsHumanClient(client))
	{
		LP_ShowPlayerMenu(client);
		return 0;
	}
	if (action != MenuAction_Select || !LP_CanChangeCharacter(client))
	{
		return 0;
	}

	char info[8];
	menu.GetItem(item, info, sizeof(info));
	int character = StringToInt(info);
	if (character < 0 || character >= sizeof(g_LPCharacterModels))
	{
		return 0;
	}

	SetEntProp(client, Prop_Send, "m_survivorCharacter", character);
	SetEntityModel(client, g_LPCharacterModels[character]);
	LP_RecordActivity(client);
	LP_Print(client, "CharacterChanged");
	return 0;
}

Action LP_CommandCharacter(int client)
{
	if (LP_IsHumanClient(client))
	{
		LP_RecordActivity(client);
		LP_ShowCharacterMenu(client);
	}
	return Plugin_Handled;
}
