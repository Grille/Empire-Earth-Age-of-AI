// [llm]: the AI voice - a language model speaks (and may decide) for the computer players (voice/llm*.c)
[section(60, "Language model")]
class llm {
    enum memory_mode_choice { pair, player }
    enum fallback_choice { rules, pause }

    /// AI voice (optional): a language model on this PC speaks for the computer players - LM Studio, Ollama, llama.cpp or
    /// any server with the OpenAI chat API. Words only: what the AIs decide stays with the mod's rules. Start the server
    /// with a model loaded, then set enabled=1. Without an answer the AIs say their fixed lines.
    bool enabled = false;
    /// LM Studio: http://localhost:1234/v1/chat/completions   Ollama: http://localhost:11434/v1/chat/completions
    [length(256)] string url = "http://localhost:1234/v1/chat/completions";
    /// the model's name as the server lists it (LM Studio: empty = the loaded one; Ollama needs it, e.g. gemma3:12b)
    [length(96)] string model = "";
    /// the personalities' lines said in the AI's own words (1), and answers when you write to an AI in the chat (1) - name
    /// it ("Temur, ...") or answer the last one that spoke to you
    bool lines = true;
    bool chat = true;
    /// The model decides the AIs' diplomacy towards you itself (1; single player only): your peace offers, and in the
    /// chat it can make peace (accept yours or offer it), declare war (on an ally of 5+ minutes) or send you a tribute
    /// (at most a quarter of its stock) - the model must support tool calls. What isn't possible (fixed teams, its pride
    /// after your declaration, an ally's veto, a vassal - constrained) is never offered to it; no answer: fallback.
    /// 0 = words only, the rules decide everything
    /// with decide=1 the rules' own moves towards you - an alliance offer, a betrayal, a capitulation, a rebellion -
    /// are put to the model first: it makes them in its own words, or declines (you never learn of it); no answer: the
    /// rules go ahead
    bool decide = false;
    /// The AIs talk to each other (1; single player): a line one AI says to another (war, betrayal, an alliance offer, a
    /// capitulation) opens an exchange of up to ai_talk_turns messages (4; any number - an AI may end it sooner with [end]),
    /// each pair with its own memory. Words only. At most
    /// one exchange every ai_talk_gap seconds (10+); yours go first. ai_talk_show=1: you read along ("to <name>: ..."),
    /// 0: only in the log
    bool ai_talk = false;
    bool ai_talk_show = true;
    [range(0, 1000000)] int ai_talk_gap = 60;
    [range(0, 1000000)] int ai_talk_turns = 4;
    /// The model decides between the AIs too (1; single player, not with fixed teams): an alliance offer or a capitulation
    /// from one AI to another is answered by the other's model, and in their talks an AI may offer peace, declare war on an
    /// ally (5+ minutes) or send a tribute. The rules say what is possible and decide when the model doesn't answer
    bool ai_decide = false;
    /// What the model may do when it decides (decide, ai_decide): 1 = within the rules - no peace while its pride holds
    /// after a declaration of war or an ally vetoes it, war only on an ally of 5+ minutes and never on its lord, no
    /// diplomacy of its own as a vassal, a tribute every 3 minutes of at most a quarter of each resource. 0 = only the
    /// game limits it: peace and war whenever it likes (a vassal may rebel against its lord), a vassal answers peace
    /// offers itself, tributes of all it has - for capable models; expect surprises
    bool constrained = true;
    /// what each AI remembers of its talk with you, and each pair of AIs of theirs: the last <memory> chat lines, answers
    /// and events, in order (8; 0 = nothing; more = longer requests - small models get slower and lose track). Kept in
    /// saved games
    [range(0, 100000)] int memory = 8;
    /// pair = a memory for the talk with you and one for each pair of AIs (each knows only what was said there);
    /// player = one memory per AI: all it said and heard - with you and with every AI - and its events, in order (it
    /// knows what it told the others and may keep it or share it; longer requests - set memory higher, e.g. 64)
    memory_mode_choice memory_mode = memory_mode_choice.pair;
    /// the model isn't told who is human: you are "another leader in this game" like every AI (1), or the human player (0)
    bool equal = false;
    /// seconds a line may wait for the model (2..120), else the fixed line
    [range(0, 1000000)] int timeout = 20;
    /// When the model can't be reached or hasn't answered within timeout: rules = the computer players say their fixed
    /// lines and the rules decide; pause = the game pauses until it answers (tried again every 5 s; resume the game
    /// yourself to play on - the answers still come). Single player only
    fallback_choice fallback = fallback_choice.rules;
    /// answer length (tokens, 16..8000; 300 is plenty for one or two sentences - more only with think=1) and temperature
    /// (80% = 0.8, up to 200%)
    [range(0, 1000000)] int max_tokens = 300;
    [pct(200)] int temperature = 80;
    /// Reasoning models think before they answer: 0 = asked not to ("reasoning_effort":"none" - answers in seconds; a server
    /// that doesn't know the field gets requests without it), 1 = let them think (much slower; needs more max_tokens)
    bool think = false;
    /// extra JSON fields for the request (e.g. "top_p":0.9)
    [length(256)] string extra = "";
    /// for a server that wants a key (a local one doesn't)
    [length(128)] string api_key = "";
    /// prompts and answers into ageofai_log.csv (# llm:)
    bool log = true;
}
