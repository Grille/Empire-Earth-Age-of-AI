# Diplomacy, personalities and the AI voice

## Diplomacy

The original AI has no diplomacy at all: declaring war on it changes nothing on its side, and it never makes
peace or alliances. With the mod:

- **War is answered:** whoever declares war on an AI gets war back, with a revenge attack and a taunt.
- **Peace and alliances:** an AI judges peace offers by its opinion of you, the strength of both sides, how
  recently you attacked it and its character. Tribute buys goodwill. AIs ally against a much stronger enemy,
  and a disloyal one may betray a weak ally. Nothing changes in scenarios or with locked teams.
- **Offers to you are messages:** an AI offering you an alliance (or peace inside your block) only says so; it
  stays at war until you set it to allied (2 minutes), and only then turns allied itself.
- **Stance lines** (`stance_lines`): only the AIs' stance changes towards you are shown, naming the AI ("Khan
  Temur is now your enemy"). The game's line for every change - also between AIs, without saying towards whom - is
  `stance_lines=all`; `none` shows none (the personalities still speak).
- **Alliance blocks:** everyone linked by alliances is one block, with peace inside it: two AIs allied with you
  (or with the same AI) don't fight each other. An AI at war with you that joins your block offers you peace.
  Whoever turns on a member breaks the block, and every other AI picks a side (`alliance_blocks`). Joining
  means peace with the whole block, so an AI refuses an alliance when it - or one of its allies - holds a grudge
  against one of yours (and says who stands in the way). Vassals have no say in their lord's alliances - they
  make peace as told, grumbling if they hold a grudge.
- **Capitulation:** when your army is in its base and the fight is lost, an AI offers peace and tribute
  ("set us allied"); accept and it pays and stays loyal. Proud personalities hold out longer. Like every offer to
  you, it has no time limit but no cease-fire either: the AI fights on until you accept, spending what it needs
  (you get what its stock still holds), and withdraws the offer once it holds its own again.
  A vassal has no diplomacy of its own: no offers, no betrayal, and whoever asks it for peace is sent to its lord.
  It still defends itself, and a lord who attacks his vassal sets it free.
- **Reputation:** breaking an alliance is seen by every AI - each thinks worse of the traitor, the more loyal it
  is (twice as much if the victim is its own ally). Attacking your own vassal is three times as bad, and right after
  accepting its capitulation (taking the tribute, then attacking) five times.
- **Victory over vassals:** vassals at war with each other would keep the game running for ever. Once every
  other player has capitulated to you, you are told so; play on as long as you like, and type `sieg` (or
  `victory`) in the chat to end the game as the winner. Resigning or exiting from the menu then counts as
  a victory too.
- **Ending the game yourself** (`auto_victory=0`): the game no longer ends on its own when one alliance is left;
  once nobody stands against you, a hint says so, and `sieg`/`victory`, Resign or Exit ends it as a victory.
- **Saved games:** the AIs remember across save and load - grudges, alliances, capitulations, pending offers
  (a small `.ageofai` file next to each save; `save_state`).
- **Surrender:** an AI that has lost all its citizens and can't train new ones resigns (with a last line),
  instead of leaving its last buildings for you to hunt down (`surrender`, not in scenarios). It gives up to
  whoever destroyed the most of it lately, and doesn't capitulate or take sides in a war once it is beyond saving.

## Personalities and texts

- **Personalities** (optional folder [`config/personalities`](../config/personalities)): every AI gets a name, a favoured
  colour on random maps, character traits (aggression, loyalty, pride, greed, forgiveness, caution, talk) and
  its own lines for situations of the game: greetings, attacks, being attacked, new ages, winning and losing,
  diplomacy. Six personalities are included, in English (other languages: at the release, a file each). A saved
  game finds its personalities again.
  A file is a personality when it says so (`register_personality()`). `_template.eeai` is an empty personality
  with every key explained: copy it to a new name to make your own.
- **The mod's own texts** are in its subfolder `mod/`: `computer.eeai` is no personality but the base of every
  one, without being named: neutral lines for what must not go unsaid (war, peace, offers waiting for your answer,
  capitulation, victory), the diplomacy's rules and a few shared values. A personality without a line for such a
  situation uses its line, and AIs without any personality speak only these lines. `prompt.eeai` holds everything
  the mod tells the language model (the AI voice: who the AI is, the rules, the game as it stands, the situations,
  the tools) - every personality's last layer, so a personality can say any of it its own way
  (`prompt = { ... }`). `system.eeai` holds the system lines it shows you (offers waiting for your answer, chat
  hints, victory) in English. Placeholders in braces are filled in. The mod has no texts of its own: these files
  belong to it like the DLL - without them it says nothing (no personalities, no AI voice, no diplomacy of its own;
  a message at the start tells so).
- **The format** of all these files is the scripts' language ([scripts.md](scripts.md)), without the
  commands: the keys the mod reads in records - `personality = { ... }`, `lines = { greeting = ... }` - and names
  of the file's own outside them (`weakling = them.strength * 2 < me.strength`), each `name = value`, where a
  value is an expression - a `"string"` with `{placeholders}` and `{expressions}` in it, a
  `["list", "of", "strings"]` (one at random), conditions
  (`allied ? "Welcome back."`), fallbacks (`a ?? b`) - over the game as it stands: what the AI thinks of you, how
  strong each side is, how long the war has lasted, whether you betrayed it before, what you paid it, its epoch.
  Everything - the format, the expressions, every value, every situation - is in
  [`config/personalities/README.md`](../config/personalities/README.md), which goes into the folder with the files. A mistake is
  logged with the file and line and only that key is skipped; `strict_parsing=1` in ageofai.ini shows every mistake
  at once (for whoever writes texts).
- **Editing them in VS Code:** [`extensions/eeaimod-texts`](../extensions/eeaimod-texts) highlights the `.eeai`
  files - records, keys, strings, placeholders, expressions; an unknown key, situation or epoch name shows as an
  error. `python scripts/build/pack_vscode.py`, then `code --install-extension local/work/eeaimod-texts.vsix`
  and reload the window. The grammar takes the names from the DLL's sources (`scripts/build/gen_vscode_grammar.py`).
- **Play style** (experimental, `ai_style=1`, off by default): a personality can also change how its AI plays
  (attack timing, army share, home guard). Off, every AI plays the same, tested way.

## The AI voice

- **AI voice** (optional, `[llm] llm_enabled=1`): a language model on your PC (LM Studio, Ollama, llama.cpp - any
  server with the OpenAI chat API) speaks for the AIs. Their lines come in their own words, knowing the game's
  state (who is at war with whom, who is stronger, what they think of you, how many soldiers, citizens and buildings each of you has destroyed of the other's - in the whole game and in the last 20 minutes), and they answer when you write to them
  in the chat - the game's chat filter decides who gets your line (everyone, allies, enemies or the players you
  chose); of those, the one you name answers ("Temur, ..." - a typo or the first letters will do), else the last one
  you heard, else the only one that got it. In single player a line nobody will answer gets a system line saying
  why (no AI named, the filter reaches none or not the one you named, the model can't be reached; with the AI voice
  off, once a game: the AIs don't read the chat). Words only: the model decides
  nothing, and an AI can't be talked into anything - it knows its real terms for peace (the tribute that would buy
  it, by the same rules that decide, or why no price would: its pride after your declaration, an ally's veto, a
  grudge too deep), so it can name its price and its words match what it will do. Tribute amounts and names must survive the rewording, else the fixed line is shown;
  without a server (or too slow, `timeout`) the fixed lines show as before. Only on your PC - in multiplayer each
  player's own model (or none). `character=` in a personality file adds a few words about it for the model.
  - **The model decides** (`decide=1`, single player, a model with tool calls): your peace offers are answered by
    the model, and in the chat it can make peace, declare war on you or send you a tribute - real deals ("pay me
    1000 and we have peace" holds if it says so). The rules only limit it: what isn't possible right now (its
    pride after your declaration, an ally's veto, being a vassal, war within 5 minutes of an alliance, more than a
    quarter of its stock as tribute) is never offered to it, and without an answer in time the rules decide. Its
    nature and the rules' verdict are its advice. Everything else (answering war, capitulations, alliances between
    the AIs, betrayal) stays with the rules.
  - **The AIs talk to each other** (`ai_talk=1`, single player): when one AI says something to another (a
    declaration of war, a betrayal, an alliance offer, a capitulation), they talk it over - a few messages, each
    pair with its own memory of what they told each other (and what one tells another, you don't learn - it may tell
    you something else). Words only, their decisions stay with the rules. One exchange at a time, at most one a minute
    (`ai_talk_gap`), your own conversations go to the model first; `ai_talk_show=1` lets you read along.
  - **The model decides between the AIs too** (`ai_decide=1`, single player): an AI's alliance offer or capitulation
    to another AI is answered by the other's model (accept or refuse - and haggle: "an alliance against Gunther?
    250 gold first"), and in their talks an AI can offer peace, declare war on an ally or send a tribute. The rules
    still limit what is possible (an ally's veto, a vassal, war only after 5 minutes of alliance) and decide when
    the model doesn't answer; answering war, betrayal and taking sides in a split alliance stay with the rules.
  - **Nobody is special** (`equal=1`): the model isn't told who is human - you are another leader like the AIs, and
    no AI is told it is played by the computer.

## Debugging

The AIs can also explain themselves in the chat (`chat=debug`, a debug aid): attacks, new ages, towers, walls,
siege, home guard.
