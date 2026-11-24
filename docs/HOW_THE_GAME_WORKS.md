# How CONFLICT works

The rules of the game as the code implements them, worked out while porting it. Numbers are the
game's own thresholds; "a roll of 1–10" means the game's `random(10)`, which gives 1 to 10.
Variable names refer to `src/game/globals.hpp`. Where the port fixes a bug of the original, the
fixed behaviour is described and the bug number from [ORIGINAL_BUGS.md](ORIGINAL_BUGS.md) given.

You are Israel's Prime Minister. Your neighbours are Egypt, Iran, Iraq, Jordan, Lebanon, Libya
and Syria; each runs on the same rules as you, driven by the computer.

## The month
Every turn is one month, starting in January 1997:

1. Defence funds arrive: each nation's **treasury** grows by its **defence share** (% of its
   **GNP**) every month.
2. Your month: the Foreign Office briefing, then the news, your directives to each nation
   (diplomacy, intelligence, other affairs), policy, the arms market and border command.
3. Your directives take effect: relations you negotiated and insurgency you supported or
   suppressed are set at the end of your month.
4. Covert operations (assassinations, coups) are carried out.
5. Each neighbour takes its turn (below).
6. Wars are fought, then air strikes are flown — the neighbours' and the ones you ordered.
7. Relations drift: one random pair of nations (two when the region is tense) changes.

Once a year come two special events: the **defence budget meeting** every January from the second
year on, and a **summit** in July if the region is tense.

## The numbers that matter

| Value | Range | What it does |
|---|---|---|
| **Instability** (`nation_status`) | 0 firm – 10 collapsed | At 10 a nation collapses: its army withdraws, its wars end, and it can be taken over. For Israel, 10 means you are voted out. |
| **Insurgency** | 0–10 | Pushes instability up. |
| **Prestige** | 0–10 | Holds instability down; unlocks better weapons; decides who leads whom in the region. Israel starts at 4. |
| **Relations** | 1 – 9 between every pair | 1 means war: nations at relation 1 fight every month. 9 is the closest alliance. |
| **Front line** | −5 … +5 per neighbour at war with Israel | + is Israel advancing; ±5 decides the war. |
| **Morale** | about 0–3 | Troops fight at reduced strength when instability exceeds morale; six months of war lower it. |
| **Nuclear programme** | 0–10 | 10 means nuclear weapons. |
| **US disapproval** | 0 and up | Israel only. 0 = excellent relations. Decides US aid and arms (below). |
| **Middle East calm** | starts at 150 | The sum of all relations, less the months of every war. The lower it is, the higher the "Middle East tension" the Foreign Office reports; under 121 it is "totally dire". |

### Instability
Every month each nation rolls 1–10; if the roll is **below insurgency + instability − prestige**,
its instability rises by one. So a nation with high prestige and no insurgency is stable, and one
whose insurgency and instability together exceed its prestige slides towards collapse.

For Israel the same roll is made, but the brigades policing the West Bank subtract from the
pressure: **policing = West Bank brigades × 3 (soft tactics) or × 6 (hard tactics)**. Hard
tactics hold the West Bank better but cost you with the US (below).

### What the neighbours do each month
- Each nation picks a **rival** (Israel by default; Iran and Iraq watch each other, Iraq may also
  pick Syria, Jordan may pick Syria).
- If not at war with Israel, a neighbour **matches Israel's forces on their common border**,
  moving units between home and the border. A neighbour whose rival is Israel and whose relation
  with Israel is below 3, with troops on the border, pushes the front 2 towards Israel and
  **breaks relations (relation 1): war**.
- It **courts or threatens its rival**: paying 2 to raise the rival's insurgency by one and
  setting their relation, which borders and relative prestige make worse.
- A stronger neighbour **pressures** a weaker one that it gets on well with (relation above 6):
  paying 100 to it, improving relations by one; at 9 the weaker one adopts the stronger one's
  rival, and the stronger one gains prestige.
- **Nuclear programme:** steps 1–6 cost 50 each (needs funds above 50 and prestige above 3);
  steps 7–9 cost 20 a month and succeed with a chance of 3 in 8. Reaching 10 makes the news.
- It **spends its treasury on forces**: on tanks and aircraft when threatened, on tanks only when
  at war, otherwise on a mix decided by chance.

### Relations drift
Each month one random pair of nations (two if Middle East calm is below 155) changes: relation 2
drops to **1 (war)**; 5 moves to 6 if they share a rival or one fights the other's rival, else to
4; 6 rises to 7. Nations already at instability 9 collapse when picked.

## War
Every pair at relation 1 fights each month: both sides mobilise, troops and tanks fight on their
common border, fighters, gunships, helicopters and SAMs take part, and the front moves.
Troop losses are refilled from **army reserves** while they last. A war is decided when one side
has no troops left on the border, collapses, or — in Israel's wars — the front reaches ±5. The
winner's instability drops to 0, its prestige rises by one and it takes the loser's border forces;
the loser collapses and is **controlled by the winner**. If Israel loses a war, the game is over.

A nation that wants peace with Israel when it is hard pressed **offers peace**; you can also
propose peace through diplomacy.

### Nuclear weapons
A nuclear power (programme at 10) uses its weapons when losing badly — at war over 6 months with
instability above 5 — or against a weaker enemy (lower prestige, no programme near completion)
after 2 months of war. Israel uses them only when you order a nuclear strike at border command.
A nuclear strike wipes out the troops on that border. If **both** sides have nuclear weapons, or
Middle East calm is below 135, a nuclear strike **ends the game for everyone**.

### Strikes
At border command you can order strikes on a neighbour: its air bases, military targets, civilian
targets or nuclear facilities (which set its programme back by 3, never below 0 — bug #20). Strikes
need bombers; how well one goes is decided by a roll, and a failed strike has no effect.
Successful strikes on military targets or nuclear facilities raise your prestige; all but the
strike on nuclear facilities raise US disapproval. The neighbours strike their rivals from the air
too, in order of prestige; damage to Israel is reported at border command.

## The United States
US disapproval rises with Israeli violence and falls with diplomacy:

| Raises it | | Lowers it | |
|---|---|---|---|
| Massing troops on a border | 5 | Signing the summit agreement | −15 |
| Provoking a neighbour (breaking off relations) | 15 | Accepting the land-for-peace offer | −25 |
| Successful strike on air bases / military / civilian targets | 10 / 10–12 / 30 | Cutting defence spending | −5 |
| Switching to hard tactics on the West Bank | 15 | | |
| A nuclear strike | 100 | | |

Strikes on nuclear facilities do not affect it.

Its effects:
- **US aid** at each January budget meeting: $3000M if disapproval is below 5, $2000M below 30,
  $1000M below 50, nothing from 50.
- **Arms**: from 50, the US, Britain and France refuse to sell (an official embargo; only the
  private dealer will trade). From 30, US deliveries take two months instead of one.
- The end-of-game summary describes relations with the US as excellent (0), good (1–4), poor
  (5–29) or sour (30 and up).

## The yearly events
**Defence budget meeting (January, from the second year):** US aid arrives; you may raise or cut
the share of GNP spent on defence by 2 points. Raising it at instability above 5 makes things
worse at home; cutting it calms the country (instability −1 if it is 1–5) and pleases the US.
Unless you signed last summit's agreement, you may also enlarge the army by 2 brigades.

**Summit (July, when Middle East calm is below 150):** a proposal to reduce tension. Signing it
lowers US disapproval by 15; it can be reversed the next year. If a neighbour bordering Israel
gets on well with you (relation above 3) and is stable, it may also offer to take control of a
border region to settle the Palestinian question (**land for peace**). Accepting costs territory
— the front with that neighbour moves to −2 and the West Bank brigades come home — but lowers US
disapproval by 25, raises both nations' prestige and ends Israel's insurgency. It is offered at
each summit until you accept it.

## The arms market
Four suppliers: the **USA** (weapons 1–7 of the list), **Britain** (8–11), **France** (12–14)
and a **private dealer** (15–19). A supplier offers a weapon only if:
- your **prestige** is at least the weapon's minimum,
- your **loyalty** to that supplier — the share of your past purchases made from it — is at least
  the weapon's minimum (with no purchases yet, everyone counts you as loyal),
- you can afford a unit, and your funds exceed 99 (USA, private dealer) or 49 (Britain, France),
- and, for the three governments, US disapproval is below 50.

Deliveries arrive the next month (Britain, France; the USA while US disapproval is below 30) or
after two months (the private dealer; the USA otherwise).

## Covert operations
- **Assassination** of a leader: succeeds more easily the more unstable the target is, and less
  each time you try (−5 per earlier attempt on that nation). Full success collapses the nation;
  partial success raises its instability by one; failure counts as violence. (The original also
  *lowers* US disapproval by 15 on a failure, which looks like a mistake in the original code;
  the port keeps it.)
- **Coup**: costs 50. Succeeds when the target's insurgency + instability exceeds 14 by more than
  its home troops divided by 200 — the nation collapses. Otherwise its insurgency is
  halved, or wiped out if the coup failed badly.

## How the game ends
- **Victory**: Syria, Egypt, Lebanon and Jordan have all collapsed.
- **Voted out**: Israel's instability reaches 10.
- **Overrun**: Israel loses a war.
- **Nuclear holocaust**: see *Nuclear weapons*.
- You can also resign (Shift-E).

The **leadership analysis** at the end scores your time in office:

    score = bonus − 8 × Israel's instability − US disapproval + 16 × Israel's prestige + 4 × months in office
            − 5 / 10 / 25 unless Lebanon / Jordan / Syria is under Israeli control
            − 20 if you lasted under 4 months
    bonus = 100 if all four border states fell, else 50 if Syria or Egypt fell

from "a complete joke" (below 30) upwards. The summary also counts your **acts of violence**
(provocations, strikes, failed assassinations, nuclear strikes): none, few, numerous or copious.
