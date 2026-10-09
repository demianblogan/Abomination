# Game Design

> **Draft.** The story, the episodes and the first level as they are planned
> now. Names, places and details change when the game needs it. How things
> look is in [ART_DIRECTION.md](ART_DIRECTION.md); when they are built is in
> [ROADMAP.md](ROADMAP.md).

## Story

### Before the game

The hero was a bad man: a criminal who killed and kidnapped innocent people.
Then he met a woman he liked so much that he tried to look like someone else
for her. They had a child and married, but the marriage was a torment: she
learned about his crimes, they quarreled all the time, and the crying baby
made it worse.

Sick of that life, he left both the crime and his family and went into the
army for a year. The army taught him to use weapons, but it was one more
torment: he was bullied there and had no power at all. Away from home he
understood how much he missed his wife and child, and that in the terrible
life he had built himself, his family was the only way out. He wrote letters
to his wife; she came to see him a few times, and they quarreled even there.

The game begins when he comes home from the army. He reaches the door of his
house, but before he can knock he blacks out, and wakes in another, terrible
world. He stays there for the whole game.

### What it really is

The world is a **purgatory inside his own mind**; he does not understand it
until the end. There he does what he was taught: he kills monsters. The
monsters and the places are allusions to memories of his life.

The four episodes are allusions to real places of his past where he committed
crimes and which are tied to his wife.

### The letters of his wife

Letters from his wife are hidden in the levels. In them she writes about the
terrible things he did, why she still sees a good part of his soul and why she
wants to bring it back. The one who finds them all understands what kind of
man he was and what kind of man he has to become. The last letter lies just
before the final boss of episode 4.

### Two endings

| Ending | When | What happens |
|--------|------|--------------|
| **Bad** | Not every letter was found | After the last monster dies, the hero wakes on the very first level again: his purgatory is an endless hell until he finds every letter. Single levels cannot be replayed to collect missed letters: the whole game is played again from the start |
| **Good** | Every letter was found | The final boss does not attack and walks away. The hero is back at the door of his home; he knocks, his wife opens with the child in her arms, and he says one word: *"Forgive me."* The game ends |

## Episodes

| Episode | Place | Mood |
|---------|-------|------|
| **E1 · City of the Dead** | A gothic town of old times behind a fence, struck by the plague: dirty ruined wooden houses and stone buildings; rot, corpses, blood and puddles in the streets | The color of rot, mold, vomit and plague: the color grading leans to green |
| E2–E4 | Planned: each one an allusion to another place of his past | See the draft palettes in [ART_DIRECTION.md](ART_DIRECTION.md) |

### Level 1-1 · Outskirts of the Church

1. The hero wakes in a back alley between houses, without a weapon.
2. About 20 seconds through a narrow lane, and he sees the first weapon: the
   **axe**, the only melee weapon of the whole game.
3. When he picks it up, the first enemy hears him: a **dog** runs out from
   around the corner.
4. About 10 more seconds, and he jumps down into a long wide street where
   dogs roam everywhere. He fights his way through to the church.
5. Inside he meets the second enemy: **villagers** who look like zombies —
   women, men, even children — with scythes, sickles and clubs.
6. After the main hall is cleared, he sees the door to the cellar. Reaching
   it ends the level.

## How the levels are built

Twenty different levels are months of work for one person, so the game grows
from **one complete level first** (a vertical slice): level 1-1 is made as
close to its concept art as possible, and every version makes it bigger.

| Version | Level 1-1 |
|---------|-----------|
| 0.4 | The nave of the flooded chapel |
| 0.5 | The player starts outside: a small courtyard and the entrance to the chapel, so the sun and the sky are seen outside and the fire inside; props (benches, chains, lanterns, torches); a better chapel |
| 0.6 | Closer to the description above: more houses, the alley and the street |

- **Concept art** comes from ChatGPT: the mood of every part of the level
  (the alley, the street, the church outside and inside).
- **The plan of the level** is a top-down scheme in meters, drawn from the
  timings above (seconds of running turned into distances), with the places
  of the enemies, the weapons and the lines of sight. The level is built from
  it in TrenchBroom.

### Light sources

There were no electric lights then, so every light of the world is fire or
the sky:

| Source | Where | Light |
|--------|-------|-------|
| Wall torch | streets, corridors, the chapel | warm, flickers strongly |
| Brazier | squares, the entrance of the church | bright, low, with sparks |
| Campfire | streets, courtyards | a large radius, smoke |
| Candles | the altar, niches, crypts | weak and calm |
| Chandelier with candles | the hall of the church | from above |
| Oil lantern | walls and posts in the streets | even, yellow |
| Fireplace, hearth | inside houses | through the opening |
| Burning wreckage | ruined houses | red, smoldering |

Fire is drawn as an animated sprite (frames of a flame in one texture) with
sparks and smoke as particles, and lights the world with a light that
flickers in time with it. The **flashlight** is a game convention, like the
shotgun: a light held a little away from the eyes that follows the view with
a delay, so its shadows are seen and move.

## Open questions

- The bad ending is harsh: a letter missed in episode 2 is found out only
  hours later. The number of letters found could be shown at the end of every
  level, like the secrets of Quake.
- Enemy children may affect the age rating and the rules of some stores.
- The enemies of the later levels: a half-zombie skeleton and a bloated flesh
  monster with a chain are in concept.
