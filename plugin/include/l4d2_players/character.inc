#if defined _l4d2_players_character_included
	#endinput
#endif
#define _l4d2_players_character_included

#define LP_L4D2_SURVIVOR_CHARACTER_COUNT 4

int g_LPNextCreatedSurvivorCharacter;

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

bool LP_IsValidSurvivorCharacter(int character)
{
	return character >= 0 && character < sizeof(g_LPCharacterModels);
}

void LP_GetSurvivorCharacterModel(int character, char[] model, int modelLength)
{
	if (!LP_IsValidSurvivorCharacter(character))
	{
		model[0] = '\0';
		return;
	}
	strcopy(model, modelLength, g_LPCharacterModels[character]);
}

bool LP_VerifySurvivorIdentity(int client, int character)
{
	if (client < 1 || client > MaxClients || !IsClientInGame(client)
		|| !LP_IsValidSurvivorCharacter(character)
		|| !HasEntProp(client, Prop_Send, "m_survivorCharacter")
		|| GetEntProp(client, Prop_Send, "m_survivorCharacter") != character)
	{
		return false;
	}

	char expectedModel[PLATFORM_MAX_PATH];
	char actualModel[PLATFORM_MAX_PATH];
	LP_GetSurvivorCharacterModel(character, expectedModel, sizeof(expectedModel));
	GetClientModel(client, actualModel, sizeof(actualModel));
	return StrEqual(actualModel, expectedModel, false);
}

bool LP_ApplySurvivorIdentity(int client, int character)
{
	if (client < 1 || client > MaxClients || !IsClientInGame(client)
		|| !LP_IsValidSurvivorCharacter(character)
		|| !HasEntProp(client, Prop_Send, "m_survivorCharacter"))
	{
		return false;
	}

	SetEntProp(client, Prop_Send, "m_survivorCharacter", character);
	SetEntityModel(client, g_LPCharacterModels[character]);
	return LP_VerifySurvivorIdentity(client, character);
}

int LP_GetNextCreatedSurvivorCharacter()
{
	int character = g_LPNextCreatedSurvivorCharacter;
	g_LPNextCreatedSurvivorCharacter = (g_LPNextCreatedSurvivorCharacter + 1)
		% LP_L4D2_SURVIVOR_CHARACTER_COUNT;
	return character;
}

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

	if (!LP_ApplySurvivorIdentity(client, character))
	{
		LP_LogError("Character change failed identity verification: client=%N character=%d.", client, character);
		LP_Print(client, "CharacterUnavailable");
		return 0;
	}
	LP_CancelIdentityVerification(client);
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
