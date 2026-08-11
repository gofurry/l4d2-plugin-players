#if defined _l4d2_players_midjoin_included
	#endinput
#endif
#define _l4d2_players_midjoin_included

#define LP_MIDJOIN_HULL_OFFSETS 16
#define LP_MIDJOIN_MAX_MAP_MELEE 32
#define LP_MIDJOIN_MELEE_NAME_LENGTH 64

static const char g_LPMidJoinPrimaryWeapons[][] =
{
	"weapon_pumpshotgun",
	"weapon_shotgun_chrome",
	"weapon_smg",
	"weapon_smg_silenced"
};

static const char g_LPMidJoinMedicines[][] =
{
	"weapon_pain_pills",
	"weapon_adrenaline"
};

static const char g_LPMidJoinMeleeFallbacks[][] =
{
	"fireaxe",
	"frying_pan",
	"machete",
	"baseball_bat",
	"crowbar",
	"cricket_bat",
	"tonfa",
	"katana",
	"electric_guitar",
	"golfclub",
	"knife",
	"pitchfork",
	"shovel"
};

static const float g_LPMidJoinPlacementOffsets[LP_MIDJOIN_HULL_OFFSETS][2] =
{
	{48.0, 0.0}, {-48.0, 0.0}, {0.0, 48.0}, {0.0, -48.0},
	{48.0, 48.0}, {-48.0, 48.0}, {48.0, -48.0}, {-48.0, -48.0},
	{72.0, 0.0}, {-72.0, 0.0}, {0.0, 72.0}, {0.0, -72.0},
	{64.0, 64.0}, {-64.0, 64.0}, {64.0, -64.0}, {-64.0, -64.0}
};

bool LP_IsMidJoinTeammateHealthy(int client)
{
	if (client < 1 || client > MaxClients || !IsClientInGame(client)
		|| GetClientTeam(client) != LP_TEAM_SURVIVOR || !IsPlayerAlive(client))
	{
		return false;
	}
	if (HasEntProp(client, Prop_Send, "m_isIncapacitated")
		&& GetEntProp(client, Prop_Send, "m_isIncapacitated") != 0)
	{
		return false;
	}
	if (HasEntProp(client, Prop_Send, "m_isHangingFromLedge")
		&& GetEntProp(client, Prop_Send, "m_isHangingFromLedge") != 0)
	{
		return false;
	}
	return true;
}

int LP_FindMidJoinPlacementTarget(int client)
{
	int candidates[MAXPLAYERS + 1];
	int count = 0;
	for (int teammate = 1; teammate <= MaxClients; teammate++)
	{
		if (teammate != client && LP_IsActiveSurvivor(teammate) && LP_IsMidJoinTeammateHealthy(teammate))
		{
			candidates[count++] = teammate;
		}
	}
	if (count > 0)
	{
		return candidates[GetRandomInt(0, count - 1)];
	}

	count = 0;
	for (int teammate = 1; teammate <= MaxClients; teammate++)
	{
		if (LP_IsSurvivorBot(teammate) && LP_IsMidJoinTeammateHealthy(teammate))
		{
			candidates[count++] = teammate;
		}
	}
	if (count > 0)
	{
		return candidates[GetRandomInt(0, count - 1)];
	}

	count = 0;
	for (int teammate = 1; teammate <= MaxClients; teammate++)
	{
		if (LP_IsSurvivorBot(teammate) && IsPlayerAlive(teammate))
		{
			candidates[count++] = teammate;
		}
	}
	return count > 0 ? candidates[GetRandomInt(0, count - 1)] : 0;
}

public bool LP_TraceIgnoreJoiningClient(int entity, int contentsMask, any client)
{
	return entity != client;
}

bool LP_FindSafeMidJoinOrigin(int client, int target, float safeOrigin[3])
{
	static const float hullMins[3] = {-16.0, -16.0, 0.0};
	static const float hullMaxs[3] = {16.0, 16.0, 72.0};
	float targetOrigin[3];
	GetClientAbsOrigin(target, targetOrigin);
	int firstOffset = GetRandomInt(0, LP_MIDJOIN_HULL_OFFSETS - 1);

	for (int attempt = 0; attempt < LP_MIDJOIN_HULL_OFFSETS; attempt++)
	{
		int offset = (firstOffset + attempt) % LP_MIDJOIN_HULL_OFFSETS;
		float traceStart[3];
		float traceEnd[3];
		traceStart[0] = targetOrigin[0] + g_LPMidJoinPlacementOffsets[offset][0];
		traceStart[1] = targetOrigin[1] + g_LPMidJoinPlacementOffsets[offset][1];
		traceStart[2] = targetOrigin[2] + 36.0;
		traceEnd[0] = traceStart[0];
		traceEnd[1] = traceStart[1];
		traceEnd[2] = targetOrigin[2] - 72.0;

		Handle groundTrace = TR_TraceRayFilterEx(traceStart, traceEnd, MASK_PLAYERSOLID_BRUSHONLY,
			RayType_EndPoint, LP_TraceIgnoreJoiningClient, client);
		bool foundGround = TR_DidHit(groundTrace) && !TR_StartSolid(groundTrace);
		float groundNormal[3];
		if (foundGround)
		{
			TR_GetEndPosition(safeOrigin, groundTrace);
			TR_GetPlaneNormal(groundTrace, groundNormal);
		}
		delete groundTrace;
		if (!foundGround || groundNormal[2] < 0.7 || FloatAbs(safeOrigin[2] - targetOrigin[2]) > 72.0)
		{
			continue;
		}

		safeOrigin[2] += 2.0;
		if (TR_PointOutsideWorld(safeOrigin))
		{
			continue;
		}

		Handle spaceTrace = TR_TraceHullFilterEx(safeOrigin, safeOrigin, hullMins, hullMaxs,
			MASK_PLAYERSOLID, LP_TraceIgnoreJoiningClient, client);
		bool blocked = TR_DidHit(spaceTrace) || TR_StartSolid(spaceTrace);
		delete spaceTrace;
		if (!blocked)
		{
			return true;
		}
	}
	return false;
}

bool LP_PlaceMidJoinSurvivor(int client)
{
	int target = LP_FindMidJoinPlacementTarget(client);
	if (target <= 0)
	{
		LP_LogError("Mid-join placement failed: client=%N stage=no_living_teammate.", client);
		return false;
	}

	float safeOrigin[3];
	if (!LP_FindSafeMidJoinOrigin(client, target, safeOrigin))
	{
		LP_Log("Mid-join placement: client=%N target=%N success=0 stage=no_safe_candidate; preserving engine origin.",
			client, target);
		return false;
	}

	TeleportEntity(client, safeOrigin, NULL_VECTOR, NULL_VECTOR);
	float actualOrigin[3];
	GetClientAbsOrigin(client, actualOrigin);
	bool success = GetVectorDistance(actualOrigin, safeOrigin) <= 16.0;
	LP_Log("Mid-join placement: client=%N target=%N success=%d.", client, target, success ? 1 : 0);
	if (!success)
	{
		LP_LogError("Mid-join placement failed: client=%N target=%N stage=teleport_verification.", client, target);
	}
	return success;
}

void LP_RemoveMidJoinDefaultEquipment(int client)
{
	for (int slot = 0; slot <= 4; slot++)
	{
		for (int guard = 0; guard < 8; guard++)
		{
			int weapon = GetPlayerWeaponSlot(client, slot);
			if (weapon <= MaxClients || !IsValidEntity(weapon))
			{
				break;
			}
			if (!RemovePlayerItem(client, weapon))
			{
				LP_LogError("Mid-join loadout cleanup failed: client=%N slot=%d entity=%d.", client, slot, weapon);
				break;
			}
			AcceptEntityInput(weapon, "Kill");
		}
	}
}

int LP_GiveRandomMidJoinItem(int client, const char[][] items, int itemCount,
	char[] selected, int selectedLength)
{
	int first = GetRandomInt(0, itemCount - 1);
	for (int attempt = 0; attempt < itemCount; attempt++)
	{
		int choice = (first + attempt) % itemCount;
		int entity = GivePlayerItem(client, items[choice]);
		if (entity > MaxClients && IsValidEntity(entity))
		{
			strcopy(selected, selectedLength, items[choice]);
			return entity;
		}
	}
	selected[0] = '\0';
	return -1;
}

bool LP_MeleeScriptAlreadyCollected(const char scripts[][LP_MIDJOIN_MELEE_NAME_LENGTH], int count,
	const char[] script)
{
	for (int i = 0; i < count; i++)
	{
		if (StrEqual(scripts[i], script, false))
		{
			return true;
		}
	}
	return false;
}

int LP_CollectMapMeleeScripts(char scripts[][LP_MIDJOIN_MELEE_NAME_LENGTH], int maxScripts)
{
	int count = 0;
	int entity = -1;
	while (count < maxScripts && (entity = FindEntityByClassname(entity, "weapon_melee")) != -1)
	{
		if (!HasEntProp(entity, Prop_Data, "m_strMapSetScriptName"))
		{
			continue;
		}
		char script[LP_MIDJOIN_MELEE_NAME_LENGTH];
		GetEntPropString(entity, Prop_Data, "m_strMapSetScriptName", script, sizeof(script));
		if (script[0] == '\0' || LP_MeleeScriptAlreadyCollected(scripts, count, script))
		{
			continue;
		}
		strcopy(scripts[count++], LP_MIDJOIN_MELEE_NAME_LENGTH, script);
	}
	return count;
}

bool LP_TryGiveMidJoinMelee(int client, const char[] script)
{
	int entity = CreateEntityByName("weapon_melee");
	if (entity == -1)
	{
		return false;
	}
	DispatchKeyValue(entity, "melee_script_name", script);
	if (!DispatchSpawn(entity) || !IsValidEntity(entity)
		|| !HasEntProp(entity, Prop_Data, "m_strMapSetScriptName"))
	{
		if (IsValidEntity(entity))
		{
			AcceptEntityInput(entity, "Kill");
		}
		return false;
	}

	char actualScript[LP_MIDJOIN_MELEE_NAME_LENGTH];
	GetEntPropString(entity, Prop_Data, "m_strMapSetScriptName", actualScript, sizeof(actualScript));
	if (!StrEqual(actualScript, script, false))
	{
		AcceptEntityInput(entity, "Kill");
		return false;
	}

	EquipPlayerWeapon(client, entity);
	if (GetPlayerWeaponSlot(client, 1) != entity)
	{
		if (IsValidEntity(entity))
		{
			AcceptEntityInput(entity, "Kill");
		}
		return false;
	}
	return true;
}

bool LP_GivePreferredMidJoinMelee(int client,
	const char mapScripts[][LP_MIDJOIN_MELEE_NAME_LENGTH], int mapScriptCount,
	char[] selected, int selectedLength)
{
	if (mapScriptCount > 0)
	{
		int first = GetRandomInt(0, mapScriptCount - 1);
		for (int attempt = 0; attempt < mapScriptCount; attempt++)
		{
			int choice = (first + attempt) % mapScriptCount;
			if (LP_TryGiveMidJoinMelee(client, mapScripts[choice]))
			{
				strcopy(selected, selectedLength, mapScripts[choice]);
				return true;
			}
		}
	}

	int firstFallback = GetRandomInt(0, sizeof(g_LPMidJoinMeleeFallbacks) - 1);
	for (int attempt = 0; attempt < sizeof(g_LPMidJoinMeleeFallbacks); attempt++)
	{
		int choice = (firstFallback + attempt) % sizeof(g_LPMidJoinMeleeFallbacks);
		if (LP_TryGiveMidJoinMelee(client, g_LPMidJoinMeleeFallbacks[choice]))
		{
			strcopy(selected, selectedLength, g_LPMidJoinMeleeFallbacks[choice]);
			return true;
		}
	}

	int pistol = GivePlayerItem(client, "weapon_pistol");
	if (pistol > MaxClients && IsValidEntity(pistol))
	{
		strcopy(selected, selectedLength, "weapon_pistol");
		LP_LogError("Mid-join melee creation failed for all candidates: client=%N fallback=weapon_pistol.", client);
		return true;
	}

	selected[0] = '\0';
	LP_LogError("Mid-join secondary creation failed: client=%N stage=melee_and_pistol_fallback.", client);
	return false;
}

bool LP_GiveMidJoinLoadout(int client)
{
	if (!LP_IsActiveSurvivor(client) || !IsPlayerAlive(client))
	{
		LP_LogError("Mid-join loadout skipped: client=%d stage=not_living_active_survivor.", client);
		return false;
	}

	char mapScripts[LP_MIDJOIN_MAX_MAP_MELEE][LP_MIDJOIN_MELEE_NAME_LENGTH];
	int mapScriptCount = LP_CollectMapMeleeScripts(mapScripts, LP_MIDJOIN_MAX_MAP_MELEE);
	LP_RemoveMidJoinDefaultEquipment(client);
	char primary[32];
	char melee[LP_MIDJOIN_MELEE_NAME_LENGTH];
	char medicine[32];
	int primaryEntity = LP_GiveRandomMidJoinItem(client, g_LPMidJoinPrimaryWeapons,
		sizeof(g_LPMidJoinPrimaryWeapons), primary, sizeof(primary));
	bool secondarySuccess = LP_GivePreferredMidJoinMelee(client, mapScripts, mapScriptCount,
		melee, sizeof(melee));
	int medicineEntity = LP_GiveRandomMidJoinItem(client, g_LPMidJoinMedicines,
		sizeof(g_LPMidJoinMedicines), medicine, sizeof(medicine));

	if (primaryEntity == -1)
	{
		LP_LogError("Mid-join loadout failed: client=%N stage=primary_creation.", client);
	}
	if (medicineEntity == -1)
	{
		LP_LogError("Mid-join loadout failed: client=%N stage=medicine_creation.", client);
	}
	LP_Log("Mid-join loadout applied: client=%N primary=%s melee=%s medicine=%s.",
		client, primary[0] == '\0' ? "<failed>" : primary,
		melee[0] == '\0' ? "<failed>" : melee,
		medicine[0] == '\0' ? "<failed>" : medicine);
	return primaryEntity != -1 && secondarySuccess && medicineEntity != -1;
}

void LP_ScheduleMidJoinSetup(int client)
{
	RequestFrame(LP_FrameApplyMidJoinSetup, GetClientSerial(client));
}

public void LP_FrameApplyMidJoinSetup(any serial)
{
	int client = GetClientFromSerial(serial);
	if (!g_LPEnabled || g_LPMapEnding || !LP_IsActiveSurvivor(client) || !IsPlayerAlive(client))
	{
		if (client > 0)
		{
			LP_LogError("Mid-join post-takeover setup skipped: client=%d stage=state_changed_before_frame.", client);
		}
		return;
	}
	if (g_LPMidJoinSpawnNearPlayer)
	{
		LP_PlaceMidJoinSurvivor(client);
	}
	if (g_LPMidJoinLoadout)
	{
		LP_GiveMidJoinLoadout(client);
	}
}
