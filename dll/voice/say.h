// WHAT THE AI SAYS AND ASKS ITS PERSONALITY: the one door from ai/ to voice/ (scripts/build/layers.py). The situations it
// has a line for, its debug chat, its personality's traits and decisions, diplomacy's system lines and the language
// model's turns - declared here, made in voice/ (persona.c, chat.c, texts.c, llm.c).

// a personality's traits, 0-100 (trait(pl, T_LOYALTY); 50 without a personality)
enum trait { T_AGGRESSION, T_LOYALTY, T_PRIDE, T_GREED, T_FORGIVENESS, T_CAUTION, T_TALK, T_N };

// the situations a personality has a line for (persona_say) - their keys, chance, cooldown and audience: persona.c k_lines
enum line_kind {
    L_GREETING, L_WAR_DECLARED, L_BETRAYED, L_PEACE_ACCEPT, L_PEACE_REFUSE, L_ALLIANCE_OFFER, L_OFFER_ACCEPTED,
    L_OFFER_WITHDRAWN, L_BETRAY, L_TRIBUTE_THANKS, L_TRIBUTE_SMALL, L_ATTACK, L_UNDER_ATTACK, L_BUILDING_LOST,
    L_EPOCH_UP, L_ENEMY_AHEAD, L_WINNING, L_LOSING, L_DESPERATE, L_ENEMY_DEFEATED, L_ALLY_DEFEATED, L_DEFEATED,
    L_WONDER_THREAT, L_WONDER_HELP, L_WONDER_NOHELP, L_HELP_LAND, L_HELP_AIR, L_HELP_NAVAL, L_HELP_NONE, L_SURRENDER,
    L_CAPITULATE, L_CAPITULATE_ACCEPTED, L_CAPITULATE_REFUSED, L_ATTACK_THREAT, L_ALL_SUBMITTED, L_OTHER_VICTORY,
    L_BLOCK_REFUSE, L_BLOCK_REFUSE_OWN, L_VASSAL_GRUMBLE, L_ASK_LORD, L_N
};

// what an AI wants, from its personality's texts (persona_decide) - the rules around them are diplo.c's
enum decide { D_PEACE_SCORE, D_SEEK_ALLY, D_ALLY_SCORE, D_N };

// the debug chat's kinds (ai_say), each with its cooldown (chat.c)
enum chat_kind { CHAT_SAY, CHAT_EPOCH, CHAT_ATTACK, CHAT_ROUTE, CHAT_TOWER, CHAT_WALL, CHAT_SIEGE, CHAT_COUNTER,
                 CHAT_GUARD, CHAT_STATUS, CHAT_KINDS };

internal int g_say_tribute[7];   // {tribute}, {tribute_total}: amounts by resource (stock index 2..6), set by the caller (diplo.c)
internal int g_texts_missing;   // the mod's files aren't there (mod/system.eeai, prompt.eeai, computer.eeai, units.eeai): it says nothing

internal PRINTF_LIKE(3, 4) int ai_say(ee_player *pl, enum chat_kind kind, const char *fmt, ...);   // a debug chat line (aimod.ini chat)
internal int chat_level(void);
internal void chat_tick(ee_player *pl, unsigned idx, uint32_t now);
internal void chat_stance_line(ee_player *pl, int stance);   // the game's own line on a change of stance
internal void system_line(const char *text, int ms);
internal int persona_say(ee_player *pl, enum line_kind k, ee_player *other, ee_player *third);   // a situation's line
internal void persona_attack(ee_player *pl, ee_player *target);   // the attack line
internal void persona_attack_launched(unsigned ai);
internal void persona_assign(ee_player *pl);
internal double persona_decide(ee_player *pl, int k, ee_player *them, ee_player *enemy);
internal const char *player_name(ee_player *pl);
internal int trait(ee_player *pl, enum trait t);
internal float capit_share(ee_player *pl);
internal float tribute_worth(ee_player *pl);
internal void tribute_text(const int *amount, char *buf, int cap);
internal void x_errors_show(void);   // strict parsing: the mistakes in the texts as system lines
// the system texts (mod/system.eeai): one filled, by its key
internal const char *sysf(const char *key, ee_player *pl, ee_player *other, ee_player *third, ...);
internal int tsub(char *out, int cap, const char *key, ee_player *pl, ee_player *other, ...);
internal const char *tnum(char *buf, int n);
// the language model ([llm]): it decides, answers, remembers
internal int llm_ai_deciding(void);
internal int llm_ai_offer(int from, int to, int kind, int third);
internal PRINTF_LIKE(2, 3) void llm_happened(int ai, const char *fmt, ...);
internal int llm_peace_answer(ee_player *pl, int ai, int j, uint32_t now);
internal void llm_player_chat(const char *text);
internal int llm_propose(int ai, int h, int kind, int third, uint32_t now);
internal int llm_unbound(void);
