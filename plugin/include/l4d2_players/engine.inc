#if defined _l4d2_players_engine_included
	#endinput
#endif
#define _l4d2_players_engine_included

GameData g_LPGameData;
Handle g_LPSDKGoAwayFromKeyboard;
Handle g_LPSDKSetHumanSpectator;
Handle g_LPSDKTakeOverBot;
Handle g_LPSDKNextBotCreateSurvivorBot;
Handle g_LPSDKRoundRespawn;
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

	Address createAddress = g_LPGameData.GetAddress("NextBotCreatePlayerBot<SurvivorBot>");
	if (createAddress == Address_Null)
	{
		LP_EngineFail("NextBotCreatePlayerBot<SurvivorBot> address");
		return;
	}
	int operatingSystem = g_LPGameData.GetOffset("OS");
	if (operatingSystem == 1)
	{
		Address relativeOffset = view_as<Address>(LoadFromAddress(createAddress + view_as<Address>(1), NumberType_Int32));
		createAddress += relativeOffset + view_as<Address>(5);
	}
	else if (operatingSystem != 2)
	{
		LP_EngineFail("OS offset");
		return;
	}

	StartPrepSDKCall(SDKCall_Static);
	if (!PrepSDKCall_SetAddress(createAddress))
	{
		LP_EngineFail("NextBotCreatePlayerBot<SurvivorBot>");
		return;
	}
	PrepSDKCall_AddParameter(SDKType_String, SDKPass_Pointer);
	PrepSDKCall_SetReturnInfo(SDKType_CBasePlayer, SDKPass_Pointer);
	g_LPSDKNextBotCreateSurvivorBot = EndPrepSDKCall();
	if (g_LPSDKNextBotCreateSurvivorBot == null)
	{
		LP_EngineFail("NextBotCreatePlayerBot<SurvivorBot> handle");
		return;
	}

	StartPrepSDKCall(SDKCall_Player);
	if (!PrepSDKCall_SetFromConf(g_LPGameData, SDKConf_Signature, "CTerrorPlayer::RoundRespawn"))
	{
		LP_EngineFail("CTerrorPlayer::RoundRespawn");
		return;
	}
	g_LPSDKRoundRespawn = EndPrepSDKCall();
	if (g_LPSDKRoundRespawn == null)
	{
		LP_EngineFail("CTerrorPlayer::RoundRespawn handle");
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
	delete g_LPSDKNextBotCreateSurvivorBot;
	delete g_LPSDKRoundRespawn;
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

int LP_EngineCreateSurvivorBot(const char[] name)
{
	return g_LPEngineReady ? SDKCall(g_LPSDKNextBotCreateSurvivorBot, name) : 0;
}

void LP_EngineRoundRespawn(int client)
{
	if (g_LPEngineReady)
	{
		SDKCall(g_LPSDKRoundRespawn, client);
	}
}
