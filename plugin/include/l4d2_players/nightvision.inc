#if defined _l4d2_players_nightvision_included
	#endinput
#endif
#define _l4d2_players_nightvision_included

Cookie g_LPNightVisionLevelCookie;

int LP_ClampNightVisionLevel(int level)
{
	if (level < LP_NIGHTVISION_LEVEL_MIN)
	{
		return LP_NIGHTVISION_LEVEL_MIN;
	}
	if (level > LP_NIGHTVISION_LEVEL_MAX)
	{
		return LP_NIGHTVISION_LEVEL_MAX;
	}
	return level;
}

float LP_GetNightVisionDistance(int level)
{
	return float(LP_ClampNightVisionLevel(level)) * LP_NIGHTVISION_DISTANCE_STEP;
}

void LP_InitializeNightVision()
{
	g_LPNightVisionLevelCookie = RegClientCookie("l4d2_players_nightvision_level",
		"Personal night vision level (1-8); enabled state is never persisted", CookieAccess_Protected);
	HookEvent("player_death", LP_NightVisionPlayerDeath, EventHookMode_Post);
	HookEvent("player_spawn", LP_NightVisionPlayerSpawn, EventHookMode_Post);
	HookEvent("player_team", LP_NightVisionPlayerTeam, EventHookMode_Post);
	HookEvent("round_end", LP_NightVisionRoundEnd, EventHookMode_PostNoCopy);
}

void LP_LoadNightVisionCookie(int client)
{
	if (!LP_IsHumanClient(client) || !AreClientCookiesCached(client))
	{
		return;
	}

	char value[8];
	GetClientCookie(client, g_LPNightVisionLevelCookie, value, sizeof(value));
	if (value[0] == '\0')
	{
		g_LPPlayers[client].nightVisionLevel = LP_ClampNightVisionLevel(g_LPNightVisionDefaultLevel);
		g_LPPlayers[client].nightVisionLevelFromCookie = false;
	}
	else
	{
		g_LPPlayers[client].nightVisionLevel = LP_ClampNightVisionLevel(StringToInt(value));
		g_LPPlayers[client].nightVisionLevelFromCookie = true;
	}
	g_LPPlayers[client].nightVisionCookieLoaded = true;
}

void LP_SaveNightVisionLevelCookie(int client)
{
	if (!LP_IsHumanClient(client) || !AreClientCookiesCached(client))
	{
		return;
	}

	char value[8];
	IntToString(LP_ClampNightVisionLevel(g_LPPlayers[client].nightVisionLevel), value, sizeof(value));
	SetClientCookie(client, g_LPNightVisionLevelCookie, value);
	g_LPPlayers[client].nightVisionLevelFromCookie = true;
	g_LPPlayers[client].nightVisionCookieLoaded = true;
}

int LP_GetNightVisionEntity(int client)
{
	if (client < 1 || client > MaxClients)
	{
		return -1;
	}

	int entity = EntRefToEntIndex(g_LPPlayers[client].nightVisionEntityRef);
	if (entity <= MaxClients || !IsValidEntity(entity))
	{
		g_LPPlayers[client].nightVisionEntityRef = INVALID_ENT_REFERENCE;
		return -1;
	}

	char classname[32];
	GetEntityClassname(entity, classname, sizeof(classname));
	if (!StrEqual(classname, "light_dynamic"))
	{
		g_LPPlayers[client].nightVisionEntityRef = INVALID_ENT_REFERENCE;
		return -1;
	}
	return entity;
}

int LP_FindNightVisionOwner(int entity)
{
	int entityRef = EntIndexToEntRef(entity);
	for (int client = 1; client <= MaxClients; client++)
	{
		if (g_LPPlayers[client].nightVisionEntityRef == entityRef)
		{
			return client;
		}
	}
	return 0;
}

public Action LP_NightVisionSetTransmit(int entity, int client)
{
	int owner = LP_FindNightVisionOwner(entity);
	return owner == client && LP_IsHumanClient(owner) ? Plugin_Continue : Plugin_Handled;
}

void LP_DeleteNightVisionEntity(int client)
{
	if (client < 1 || client > MaxClients)
	{
		return;
	}

	int entity = LP_GetNightVisionEntity(client);
	g_LPPlayers[client].nightVisionEntityRef = INVALID_ENT_REFERENCE;
	if (entity > MaxClients && IsValidEntity(entity))
	{
		SDKUnhook(entity, SDKHook_SetTransmit, LP_NightVisionSetTransmit);
		AcceptEntityInput(entity, "Kill");
	}
}

bool LP_UpdateNightVisionDistance(int client, int entity)
{
	if (entity <= MaxClients || !IsValidEntity(entity))
	{
		return false;
	}
	g_LPPlayers[client].nightVisionLevel = LP_ClampNightVisionLevel(g_LPPlayers[client].nightVisionLevel);
	SetVariantFloat(LP_GetNightVisionDistance(g_LPPlayers[client].nightVisionLevel));
	return AcceptEntityInput(entity, "distance");
}

bool LP_CanProduceNightVisionLight(int client)
{
	return g_LPEnabled && g_LPNightVisionEnabled && !g_LPMapEnding
		&& LP_IsActiveSurvivor(client) && IsPlayerAlive(client);
}

bool LP_CreateNightVisionEntity(int client)
{
	if (!LP_CanProduceNightVisionLight(client))
	{
		return false;
	}
	int existing = LP_GetNightVisionEntity(client);
	if (existing > MaxClients)
	{
		return LP_UpdateNightVisionDistance(client, existing);
	}

	int entity = CreateEntityByName("light_dynamic");
	if (entity <= MaxClients || !IsValidEntity(entity))
	{
		LP_LogError("Night vision light creation failed: client=%N stage=create_entity.", client);
		return false;
	}

	char distance[16];
	FloatToString(LP_GetNightVisionDistance(g_LPPlayers[client].nightVisionLevel), distance, sizeof(distance));
	DispatchKeyValue(entity, "_light", "255 255 255 255");
	DispatchKeyValue(entity, "brightness", "1");
	DispatchKeyValue(entity, "style", "0");
	DispatchKeyValue(entity, "distance", distance);
	if (!DispatchSpawn(entity))
	{
		AcceptEntityInput(entity, "Kill");
		LP_LogError("Night vision light creation failed: client=%N stage=dispatch_spawn.", client);
		return false;
	}
	ActivateEntity(entity);

	float origin[3];
	GetClientEyePosition(client, origin);
	TeleportEntity(entity, origin, NULL_VECTOR, NULL_VECTOR);
	SetVariantString("!activator");
	if (!AcceptEntityInput(entity, "SetParent", client, entity))
	{
		AcceptEntityInput(entity, "Kill");
		LP_LogError("Night vision light creation failed: client=%N entity=%d stage=set_parent.", client, entity);
		return false;
	}

	g_LPPlayers[client].nightVisionEntityRef = EntIndexToEntRef(entity);
	SDKHook(entity, SDKHook_SetTransmit, LP_NightVisionSetTransmit);
	AcceptEntityInput(entity, "TurnOn");
	LP_Log("Night vision light created: client=%N entity=%d level=%d distance=%.0f color=255_255_255_255 brightness=%d.",
		client, entity, g_LPPlayers[client].nightVisionLevel,
		LP_GetNightVisionDistance(g_LPPlayers[client].nightVisionLevel), LP_NIGHTVISION_FIXED_BRIGHTNESS);
	return true;
}

bool LP_SyncNightVision(int client)
{
	if (client < 1 || client > MaxClients)
	{
		return false;
	}
	if (!g_LPPlayers[client].nightVisionEnabled || !LP_CanProduceNightVisionLight(client))
	{
		LP_DeleteNightVisionEntity(client);
		return true;
	}

	int entity = LP_GetNightVisionEntity(client);
	if (entity > MaxClients)
	{
		return LP_UpdateNightVisionDistance(client, entity);
	}
	return LP_CreateNightVisionEntity(client);
}

void LP_CloseNightVisionMenu(int client)
{
	if (client < 1 || client > MaxClients || !g_LPPlayers[client].nightVisionMenuOpen)
	{
		return;
	}

	g_LPPlayers[client].nightVisionMenuOpen = false;
	g_LPPlayers[client].nightVisionMenu = null;
	if (IsClientInGame(client) && GetClientMenu(client) == MenuSource_Normal)
	{
		CancelClientMenu(client);
	}
}

void LP_ShowNightVisionMenu(int client)
{
	if (!LP_IsHumanClient(client))
	{
		return;
	}
	if (!g_LPEnabled || !g_LPNightVisionEnabled)
	{
		LP_Print(client, "NightVisionUnavailable");
		return;
	}

	Menu menu = new Menu(LP_NightVisionMenuHandler);
	char text[96];
	Format(text, sizeof(text), "%T", "MenuNightVision", client);
	menu.SetTitle(text);
	Format(text, sizeof(text), "%T",
		g_LPPlayers[client].nightVisionEnabled ? "NightVisionStatusOn" : "NightVisionStatusOff",
		client, g_LPPlayers[client].nightVisionLevel);
	menu.AddItem("toggle", text);
	Format(text, sizeof(text), "%T", "NightVisionIncrease", client);
	menu.AddItem("increase", text);
	Format(text, sizeof(text), "%T", "NightVisionDecrease", client);
	menu.AddItem("decrease", text);

	g_LPPlayers[client].nightVisionMenuOpen = true;
	g_LPPlayers[client].nightVisionMenu = view_as<Handle>(menu);
	menu.Display(client, MENU_TIME_FOREVER);
}

void LP_PrintNightVisionLevel(int client)
{
	if (LP_IsHumanClient(client))
	{
		PrintToChat(client, "\x04%T\x01 %T", "ChatPrefix", client,
			"NightVisionCurrentLevel", client, g_LPPlayers[client].nightVisionLevel);
	}
}

void LP_ToggleNightVision(int client)
{
	if (g_LPPlayers[client].nightVisionEnabled)
	{
		g_LPPlayers[client].nightVisionEnabled = false;
		LP_DeleteNightVisionEntity(client);
		LP_Print(client, "NightVisionDisabled");
		return;
	}
	if (!LP_CanProduceNightVisionLight(client))
	{
		LP_Print(client, "NightVisionUnavailable");
		return;
	}

	g_LPPlayers[client].nightVisionEnabled = true;
	if (!LP_SyncNightVision(client))
	{
		g_LPPlayers[client].nightVisionEnabled = false;
		LP_DeleteNightVisionEntity(client);
		LP_Print(client, "NightVisionUnavailable");
		return;
	}
	LP_Print(client, "NightVisionEnabled");
}

void LP_AdjustNightVisionLevel(int client, int delta)
{
	g_LPPlayers[client].nightVisionLevel = LP_ClampNightVisionLevel(
		g_LPPlayers[client].nightVisionLevel + delta);
	LP_SaveNightVisionLevelCookie(client);
	if (g_LPPlayers[client].nightVisionEnabled)
	{
		LP_SyncNightVision(client);
	}
	LP_PrintNightVisionLevel(client);
}

public int LP_NightVisionMenuHandler(Menu menu, MenuAction action, int client, int item)
{
	Handle menuHandle = view_as<Handle>(menu);
	if (action == MenuAction_End)
	{
		for (int owner = 1; owner <= MaxClients; owner++)
		{
			if (g_LPPlayers[owner].nightVisionMenu == menuHandle)
			{
				g_LPPlayers[owner].nightVisionMenuOpen = false;
				g_LPPlayers[owner].nightVisionMenu = null;
				break;
			}
		}
		delete menu;
		return 0;
	}
	if (action == MenuAction_Cancel)
	{
		if (client >= 1 && client <= MaxClients && g_LPPlayers[client].nightVisionMenu == menuHandle)
		{
			g_LPPlayers[client].nightVisionMenuOpen = false;
			g_LPPlayers[client].nightVisionMenu = null;
		}
		return 0;
	}
	if (action != MenuAction_Select || !LP_IsHumanClient(client)
		|| g_LPPlayers[client].nightVisionMenu != menuHandle)
	{
		return 0;
	}

	g_LPPlayers[client].nightVisionMenuOpen = false;
	g_LPPlayers[client].nightVisionMenu = null;
	char info[16];
	menu.GetItem(item, info, sizeof(info));
	if (StrEqual(info, "toggle"))
	{
		LP_ToggleNightVision(client);
	}
	else if (StrEqual(info, "increase"))
	{
		LP_AdjustNightVisionLevel(client, 1);
	}
	else if (StrEqual(info, "decrease"))
	{
		LP_AdjustNightVisionLevel(client, -1);
	}
	LP_ShowNightVisionMenu(client);
	return 0;
}

Action LP_CommandNightVision(int client)
{
	if (LP_IsHumanClient(client))
	{
		LP_RecordActivity(client);
		LP_ShowNightVisionMenu(client);
	}
	return Plugin_Handled;
}

void LP_HandleNightVisionImpulse(int client, int impulse)
{
	if (impulse != 100)
	{
		g_LPPlayers[client].flashlightImpulseLatched = false;
		return;
	}
	if (g_LPPlayers[client].flashlightImpulseLatched)
	{
		return;
	}
	g_LPPlayers[client].flashlightImpulseLatched = true;

	float now = GetGameTime();
	if (now < g_LPPlayers[client].nightVisionInputCooldownUntil)
	{
		return;
	}
	float elapsed = now - g_LPPlayers[client].lastFlashlightPress;
	if (g_LPPlayers[client].lastFlashlightPress > 0.0
		&& elapsed <= LP_NIGHTVISION_DOUBLE_PRESS_WINDOW)
	{
		g_LPPlayers[client].lastFlashlightPress = 0.0;
		g_LPPlayers[client].nightVisionInputCooldownUntil = now + LP_NIGHTVISION_INPUT_COOLDOWN;
		LP_RecordActivity(client);
		if (g_LPPlayers[client].nightVisionMenuOpen)
		{
			LP_CloseNightVisionMenu(client);
		}
		else
		{
			LP_ShowNightVisionMenu(client);
		}
		return;
	}
	g_LPPlayers[client].lastFlashlightPress = now;
}

public void LP_NightVisionSyncFrame(any serial)
{
	int client = GetClientFromSerial(serial);
	if (LP_IsHumanClient(client))
	{
		LP_SyncNightVision(client);
	}
}

void LP_RequestNightVisionSync(int client)
{
	if (LP_IsHumanClient(client))
	{
		RequestFrame(LP_NightVisionSyncFrame, GetClientSerial(client));
	}
}

public void LP_NightVisionPlayerDeath(Event event, const char[] name, bool dontBroadcast)
{
	int client = GetClientOfUserId(event.GetInt("userid"));
	if (client >= 1 && client <= MaxClients)
	{
		LP_DeleteNightVisionEntity(client);
	}
}

public void LP_NightVisionPlayerSpawn(Event event, const char[] name, bool dontBroadcast)
{
	LP_RequestNightVisionSync(GetClientOfUserId(event.GetInt("userid")));
}

public void LP_NightVisionPlayerTeam(Event event, const char[] name, bool dontBroadcast)
{
	LP_RequestNightVisionSync(GetClientOfUserId(event.GetInt("userid")));
}

void LP_DeleteAllNightVisionEntities(bool closeMenus)
{
	for (int client = 1; client <= MaxClients; client++)
	{
		LP_DeleteNightVisionEntity(client);
		if (closeMenus)
		{
			LP_CloseNightVisionMenu(client);
		}
	}
}

public void LP_NightVisionRoundEnd(Event event, const char[] name, bool dontBroadcast)
{
	LP_DeleteAllNightVisionEntities(true);
}

void LP_NightVisionMapStart()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		g_LPPlayers[client].nightVisionEntityRef = INVALID_ENT_REFERENCE;
		g_LPPlayers[client].nightVisionMenuOpen = false;
		g_LPPlayers[client].nightVisionMenu = null;
		g_LPPlayers[client].lastFlashlightPress = 0.0;
		g_LPPlayers[client].nightVisionInputCooldownUntil = 0.0;
		g_LPPlayers[client].flashlightImpulseLatched = false;
	}
}

void LP_NightVisionMapEnd()
{
	LP_DeleteAllNightVisionEntities(true);
}

void LP_NightVisionPluginEnd()
{
	LP_DeleteAllNightVisionEntities(true);
}

void LP_NightVisionClientDisconnected(int client)
{
	LP_CloseNightVisionMenu(client);
	LP_DeleteNightVisionEntity(client);
	g_LPPlayers[client].nightVisionEnabled = false;
}

void LP_NightVisionConfigurationChanged()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		if (!LP_IsHumanClient(client))
		{
			continue;
		}
		if (!g_LPPlayers[client].nightVisionLevelFromCookie)
		{
			g_LPPlayers[client].nightVisionLevel = LP_ClampNightVisionLevel(g_LPNightVisionDefaultLevel);
		}
		if (!g_LPEnabled || !g_LPNightVisionEnabled)
		{
			LP_CloseNightVisionMenu(client);
			LP_DeleteNightVisionEntity(client);
		}
		else
		{
			LP_SyncNightVision(client);
		}
	}
}
