# Aeon SDK — Unreal plugin

Drop the `AeonSDK` folder into your project's `Plugins/`, enable it, and use the **Aeon SDK** subsystem
from Blueprint (`Get Game Instance Subsystem -> Aeon SDK Subsystem`) or C++
(`GetGameInstance()->GetSubsystem<UAeonSDKSubsystem>()`).

The plugin links a prebuilt `libaeon.so` for Linux. Build it on the Aeon dev box
(`sdk/libaeon && make`) and copy it to `ThirdParty/libaeon/Linux/x86_64/libaeon.so`. The console has
`libcurl.so.4`, which libaeon uses; no other runtime dependency.

In the editor and on Win64/Mac the module compiles with every call a no-op and `IsOnAeon()` false,
so your game plays unchanged while you develop.

## Nodes

| Node | Does |
|---|---|
| Is On Aeon | true on a console with a live session |
| Get Gamertag / Player Id / Language | who is playing |
| Unlock Achievement (Id) | idempotent; ids are defined per title in the developer portal |
| Achievement Progress (Id, Value, Max) | unlocks when Value >= Max |
| Submit Score (Category, Score, Metadata) | server keeps the best |
| Get Leaderboard (Category, Count, Friends Only, OnResult) | entries + your rank, on the game thread |
| Owns (Sku) | DLC / content-pack entitlement; always true off-Aeon |
| Purchase (Sku, OnResult) | the console shows the purchase UI; result when it's done |
| Set Presence (Status) | shown to friends |
| Send Event (Name, Payload) | telemetry |

Reports are queued, retried with backoff, and kept on disk across launches if the network is down.
Nothing blocks the game thread.

## A first integration (Cyber Siege)

1. Copy the plugin in, regenerate project files, build.
2. Where the first enemy dies: `Unlock Achievement("first_kill")`.
3. On match end: `Submit Score("score", FinalScore)`.
4. On level load: `Set Presence("Fighting through <level name>")`.
5. Package for Linux, publish with `aeon-pack`, play it on the console: the unlock appears in the shell's
   Achievements panel and the score on the leaderboard.
