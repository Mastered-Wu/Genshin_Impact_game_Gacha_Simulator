# Gacha Simulator Design

## Goal

Build a local gacha simulator that closely models Genshin Impact-style wish mechanics while using original placeholder names and assets. The first version must be runnable from the desktop project folder, keep backend logic in C++, keep the frontend in plain HTML/CSS/JavaScript, and preserve every milestone in Git.

## Scope

The first version includes:

- Character Event Wish simulation with 5-star pity, 4-star pity, 50/50 featured guarantee, and carry-over state.
- Weapon Event Wish simulation with 5-star pity, 4-star pity, 75/25 promotional guarantee, two featured 5-star weapons, and Epitomized Path-style fate points.
- Standard Wish simulation with independent pity and a mixed character/weapon pool.
- Single wish and ten-wish actions.
- Visible pity counters, guarantee state, fate points, wish history, and summary statistics.
- Configurable item pools stored as project files.
- Automated tests for backend probability-state behavior.

The first version does not include user accounts, payments, live game data scraping, official art, official logos, or any networked backend beyond a local development server.

## Project Structure

All files serving this simulator live under `C:\Users\27415\Desktop\抽卡模拟器`.

```text
抽卡模拟器/
  backend/
    include/
    src/
    tests/
    CMakeLists.txt
  frontend/
    index.html
    styles.css
    app.js
  configs/
    banners.json
  docs/
    superpowers/
      specs/
      plans/
  scripts/
  README.md
  .gitignore
```

## Backend Architecture

The C++ backend owns all wish rules and persistent session state. It exposes a small local HTTP API so the HTML frontend can call it from the browser.

Core units:

- `Item`: immutable item metadata, including id, display name, rarity, kind, and featured flags.
- `BannerConfig`: item pools and rule settings for one banner.
- `WishState`: pity counters, guarantee flags, fate points, selected epitomized path, wish history, and aggregate statistics.
- `RandomProvider`: injectable random source so tests can use deterministic rolls.
- `WishEngine`: pure rule engine that consumes `BannerConfig`, `WishState`, wish count, and random rolls, then returns results plus updated state.
- `HttpServer`: local API wrapper around the engine.

The engine must not depend on the frontend or HTTP layer. Tests should target `WishEngine` directly.

## Wish Rules

Character Event Wish:

- 5-star base rate is 0.6%.
- A 5-star is guaranteed by the 90th wish since the previous 5-star.
- 4-star or above is guaranteed by the 10th wish since the previous 4-star or above.
- When a 5-star is hit without featured guarantee, there is a 50% chance to receive the featured 5-star character.
- If the 5-star is not featured, the next 5-star on the character event wish is guaranteed featured.
- Character event pity and guarantee state are independent from standard and weapon wishes.

Weapon Event Wish:

- 5-star base rate is 0.7%.
- A 5-star weapon is guaranteed by the 80th wish since the previous 5-star.
- 4-star or above is guaranteed by the 10th wish since the previous 4-star or above.
- When a 5-star is hit without promotional guarantee, there is a 75% chance it is one of the two featured 5-star weapons.
- If the 5-star is not promotional, the next 5-star on the weapon event wish is guaranteed promotional.
- If an epitomized path is selected and a 5-star is not the selected weapon, gain 1 fate point.
- Once fate points reach 2, the next 5-star is guaranteed to be the selected weapon.
- Obtaining the selected weapon resets fate points to 0.
- Changing or clearing the selected path resets fate points to 0.

Standard Wish:

- 5-star base rate is 0.6%.
- A 5-star is guaranteed by the 90th wish since the previous 5-star.
- 4-star or above is guaranteed by the 10th wish since the previous 4-star or above.
- There is no featured guarantee in the first version.

Soft pity is not required in the first version. The hard-pity and guarantee rules must be correct first; soft pity can be added later as a configurable rule.

## API Design

The local server listens on `127.0.0.1` and serves both static frontend files and JSON endpoints.

- `GET /api/state`: returns banner states, selected banner, pity counters, guarantees, fate points, history, and stats.
- `POST /api/wish`: accepts `{ "bannerId": "character-event", "count": 1 }` or `{ "bannerId": "character-event", "count": 10 }`; returns wish results and updated state.
- `POST /api/path`: accepts `{ "bannerId": "weapon-event", "itemId": "weapon-a" }` or `{ "bannerId": "weapon-event", "itemId": null }`; updates weapon path and resets fate points when changed.
- `POST /api/reset`: resets simulator state for local testing.

Errors use JSON with an `error` string and a stable `code`, such as `unknown_banner`, `invalid_count`, or `invalid_path_item`.

## Frontend Design

The first screen is the usable simulator, not a landing page. It should include:

- A compact top bar with banner selector and reset action.
- A main wish panel showing banner name, featured items, pity counters, guarantee labels, and fate points when relevant.
- Primary controls for single wish and ten wishes.
- A result area that shows the latest draw group with rarity styling.
- A history panel with newest wishes first.
- A stats panel showing total wishes, 5-star count, 4-star count, featured wins, and current pity.

The frontend uses plain HTML/CSS/JavaScript and `fetch()` calls. It should remain responsive on desktop and mobile. UI labels can be Chinese because the project name and user context are Chinese.

## Data and Persistence

Banner definitions live in `configs/banners.json`. The first implementation may keep runtime state in memory and reset when the server restarts. Saving state to disk is optional for a later version.

The config file uses original placeholder names rather than official game item names. This avoids bundling proprietary names or assets while preserving the mechanics.

## Testing

Backend tests must cover:

- Character event hard pity at 90.
- Character event 50/50 loss followed by guaranteed featured 5-star.
- 4-star hard pity at 10.
- Weapon event hard pity at 80.
- Weapon event 75/25 promotional guarantee after non-promotional 5-star.
- Weapon path fate points reaching 2 and forcing the selected weapon.
- Fate point reset after selected weapon or path change.
- Standard wish hard pity at 90.

Frontend verification must cover:

- Server starts and serves the page.
- Single wish updates result, history, and pity display.
- Ten wishes returns ten rows.
- Weapon path selector appears only for weapon banner.

## Build and Run

Use CMake for the backend.

```powershell
cmake -S backend -B build
cmake --build build
.\build\gacha_server.exe
```

The server should print the local URL when it starts.

## Git Workflow

The repository starts on `main` with an empty initialization commit. Each implementation task should make a focused commit after passing its tests. Generated build outputs must stay out of Git.

## Open Decisions

- Use a small header-only HTTP library if available locally; otherwise implement a minimal local HTTP server sufficient for the API.
- Use deterministic random queues in tests rather than statistical tests.
- Keep soft pity out of version 1 unless the user explicitly asks for it before implementation begins.
