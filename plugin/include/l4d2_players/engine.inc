#if defined _l4d2_players_engine_included
	#endinput
#endif
#define _l4d2_players_engine_included

GameData g_LPGameData;
Handle g_LPSDKGoAwayFromKeyboard;
Handle g_LPSDKSetHumanSpectator;
Handle g_LPSDKTakeOverBot;
Handle g_LPSDKAddSurvivorBot;
Address g_LPDirector = Address_Null;
bool g_LPEngineReady;

void LP_EngineFail(const char[] name)
{
	SetFailState("L4D2 Players failed to initialize required gamedata entry: %s", name);
}

void LP_InitializeEngine()
{
	g_LPGameData = new GameData("l4d2_players");
	if (g_LPGameData == null)
	{
		LP_EngineFail("l4d2_players.txt");
		return;
	}

	g_LPDirector = g_LPGameData.GetAddress("CDirector");
	if (g_LPDirector == Address_Null)
	{
		LP_EngineFail("CDirector");
		return;
	}

	StartPrepSDKCall(SDKCall_Player);
	if (!PrepSDKCall_SetFromConf(g_LPGameData, SDKConf_Signature, "CTerrorPlayer::GoAwayFromKeyboard"))
	{
		LP_EngineFail("CTerrorPlayer::GoAwayFromKeyboard");
		return;
	}
	PrepSDKCall_SetReturnInfo(SDKType_Bool, SDKPass_Plain);
	g_LPSDKGoAwayFromKeyboard = EndPrepSDKCall();
	if (g_LPSDKGoAwayFromKeyboard == null)
	{
		LP_EngineFail("CTerrorPlayer::GoAwayFromKeyboard handle");
		return;
	}

	StartPrepSDKCall(SDKCall_Player);
	if (!PrepSDKCall_SetFromConf(g_LPGameData, SDKConf_Signature, "SurvivorBot::SetHumanSpectator"))
	{
		LP_EngineFail("SurvivorBot::SetHumanSpectator");
		return;
	}
	PrepSDKCall_AddParameter(SDKType_CBasePlayer, SDKPass_Pointer);
	PrepSDKCall_SetReturnInfo(SDKType_Bool, SDKPass_Plain);
	g_LPSDKSetHumanSpectator = EndPrepSDKCall();
	if (g_LPSDKSetHumanSpectator == null)
	{
		LP_EngineFail("SurvivorBot::SetHumanSpectator handle");
		return;
	}

	StartPrepSDKCall(SDKCall_Player);
	if (!PrepSDKCall_SetFromConf(g_LPGameData, SDKConf_Signature, "CTerrorPlayer::TakeOverBot"))
	{
		LP_EngineFail("CTerrorPlayer::TakeOverBot");
		return;
	}
	PrepSDKCall_AddParameter(SDKType_Bool, SDKPass_Plain);
	PrepSDKCall_SetReturnInfo(SDKType_Bool, SDKPass_Plain);
	g_LPSDKTakeOverBot = EndPrepSDKCall();
	if (g_LPSDKTakeOverBot == null)
	{
		LP_EngineFail("CTerrorPlayer::TakeOverBot handle");
		return;
	}

	StartPrepSDKCall(SDKCall_Raw);
	if (!PrepSDKCall_SetFromConf(g_LPGameData, SDKConf_Signature, "CDirector::AddSurvivorBot"))
	{
		LP_EngineFail("CDirector::AddSurvivorBot");
		return;
	}
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
	PrepSDKCall_SetReturnInfo(SDKType_PlainOldData, SDKPass_Plain);
	g_LPSDKAddSurvivorBot = EndPrepSDKCall();
	if (g_LPSDKAddSurvivorBot == null)
	{
		LP_EngineFail("CDirector::AddSurvivorBot handle");
		return;
	}

	g_LPEngineReady = true;
	LP_Log("Gamedata initialized.");
}

void LP_ShutdownEngine()
{
	delete g_LPSDKGoAwayFromKeyboard;
	delete g_LPSDKSetHumanSpectator;
	delete g_LPSDKTakeOverBot;
	delete g_LPSDKAddSurvivorBot;
	delete g_LPGameData;
	g_LPEngineReady = false;
}

bool LP_EngineIsReady()
{
	return g_LPEngineReady;
}

bool LP_EngineGoAwayFromKeyboard(int client)
{
	return g_LPEngineReady && SDKCall(g_LPSDKGoAwayFromKeyboard, client);
}

bool LP_EngineSetHumanSpectator(int bot, int client)
{
	return g_LPEngineReady && SDKCall(g_LPSDKSetHumanSpectator, bot, client);
}

bool LP_EngineTakeOverBot(int client)
{
	return g_LPEngineReady && SDKCall(g_LPSDKTakeOverBot, client, true);
}

void LP_EngineAddSurvivorBot(int character)
{
	if (g_LPEngineReady)
	{
		SDKCall(g_LPSDKAddSurvivorBot, g_LPDirector, character);
	}
}

