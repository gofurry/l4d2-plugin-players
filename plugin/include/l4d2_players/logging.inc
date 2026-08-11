#if defined _l4d2_players_logging_included
	#endinput
#endif
#define _l4d2_players_logging_included

void LP_Log(const char[] format, any ...)
{
	char message[256];
	VFormat(message, sizeof(message), format, 2);
	LogMessage("[Players] %s", message);
}

void LP_LogError(const char[] format, any ...)
{
	char message[256];
	VFormat(message, sizeof(message), format, 2);
	LogError("[Players] %s", message);
}

