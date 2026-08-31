# Gacha Simulator Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a local C++ plus HTML gacha simulator that models Genshin Impact-style wish mechanics with original placeholder content.

**Architecture:** The C++ backend owns all banner configuration, state, random rolls, wish rules, and JSON API responses. The frontend is plain HTML/CSS/JavaScript served by the local backend and uses `fetch()` to call the API. The rule engine is isolated from HTTP so deterministic tests can verify pity, guarantees, and fate points.

**Tech Stack:** C++17, CMake, header-only single-repository code, plain HTML/CSS/JavaScript, PowerShell-friendly build commands.

**Spec:** `docs/superpowers/specs/2026-08-31-gacha-simulator-design.md`

## Global Constraints

- All simulator files must live under `C:\Users\27415\Desktop\抽卡模拟器`.
- Backend logic must be C++.
- Frontend must be plain HTML/CSS/JavaScript.
- The first screen must be the usable simulator, not a landing page.
- Use original placeholder names and assets, not official game item names, art, or logos.
- Character Event Wish hard pity is 90 wishes.
- Character Event Wish 5-star base rate is 0.6%.
- Character Event Wish featured guarantee starts after a non-featured 5-star.
- Weapon Event Wish hard pity is 80 wishes.
- Weapon Event Wish 5-star base rate is 0.7%.
- Weapon Event Wish promotional guarantee starts after a non-promotional 5-star.
- Weapon Event Wish fate points max at 2 and force the selected weapon on the next 5-star.
- Standard Wish hard pity is 90 wishes.
- 4-star or above hard pity is 10 wishes for every banner.
- Soft pity is not part of version 1.
- Generated build outputs must stay out of Git.

---

## File Structure

- Create `.gitignore`: ignores `build/`, CMake generated files, binaries, logs, and local temporary files.
- Create `README.md`: project purpose, build, run, test, and Git notes.
- Create `configs/banners.json`: original placeholder banner names and item pools.
- Create `backend/CMakeLists.txt`: builds `gacha_core_tests` and `gacha_server`.
- Create `backend/include/gacha/Models.h`: shared item, banner, state, result, and stats structs.
- Create `backend/include/gacha/RandomProvider.h`: deterministic and default random interfaces.
- Create `backend/include/gacha/WishEngine.h`: public rule engine API.
- Create `backend/src/WishEngine.cpp`: rule engine implementation.
- Create `backend/tests/WishEngineTests.cpp`: deterministic backend tests.
- Create `backend/include/gacha/BannerRepository.h`: built-in banner config provider.
- Create `backend/src/BannerRepository.cpp`: placeholder config data.
- Create `backend/include/gacha/Json.h`: JSON serialization helpers for API output.
- Create `backend/src/Json.cpp`: minimal JSON escaping and response serialization.
- Create `backend/src/main.cpp`: local HTTP/static file server and API router.
- Create `frontend/index.html`: simulator shell.
- Create `frontend/styles.css`: responsive simulator styling.
- Create `frontend/app.js`: API calls, rendering, banner switching, wish actions.
- Create `scripts/build.ps1`: CMake configure/build helper.
- Create `scripts/test.ps1`: CMake configure/build/test helper.
- Create `scripts/run.ps1`: build and start server helper.

---

### Task 1: Backend Wish Engine

**Files:**
- Create: `backend/CMakeLists.txt`
- Create: `backend/include/gacha/Models.h`
- Create: `backend/include/gacha/RandomProvider.h`
- Create: `backend/include/gacha/WishEngine.h`
- Create: `backend/src/WishEngine.cpp`
- Create: `backend/tests/WishEngineTests.cpp`

**Interfaces:**
- Produces: `gacha::WishEngine::wish(const BannerConfig&, WishState&, int count, RandomProvider&) -> WishBatch`
- Produces: `gacha::WishState` fields `pity5`, `pity4`, `featuredGuarantee`, `promotionalGuarantee`, `fatePoints`, `selectedPathItemId`, `history`, `stats`
- Produces: `gacha::SequenceRandom`, a deterministic random provider for tests.
- Consumes: No earlier task output.

- [ ] **Step 1: Write failing tests for character and standard hard pity**

Add tests in `backend/tests/WishEngineTests.cpp`:

```cpp
#include "gacha/WishEngine.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace gacha;

static void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static BannerConfig makeCharacterBanner() {
    BannerConfig banner;
    banner.id = "character-event";
    banner.type = BannerType::CharacterEvent;
    banner.fiveStarBaseRate = 0.006;
    banner.fiveStarHardPity = 90;
    banner.fourStarBaseRate = 0.051;
    banner.fourStarHardPity = 10;
    banner.items = {
        {"featured-hero", "星辉旅人", 5, ItemKind::Character, true, false},
        {"standard-hero", "晴岚术士", 5, ItemKind::Character, false, false},
        {"featured-four", "巡夜弓手", 4, ItemKind::Character, true, false},
        {"standard-four", "铁影大剑", 4, ItemKind::Weapon, false, false},
        {"three-star", "训练长剑", 3, ItemKind::Weapon, false, false},
    };
    return banner;
}

static BannerConfig makeStandardBanner() {
    BannerConfig banner = makeCharacterBanner();
    banner.id = "standard";
    banner.type = BannerType::Standard;
    banner.items[0].featured = false;
    return banner;
}

static void character_hard_pity_forces_five_star_at_90() {
    WishEngine engine;
    WishState state;
    SequenceRandom random(std::vector<double>(200, 0.99));
    auto banner = makeCharacterBanner();

    auto batch = engine.wish(banner, state, 90, random);

    require(batch.results.size() == 90, "expected 90 results");
    require(batch.results.back().item.rarity == 5, "90th wish must be 5-star");
    require(state.pity5 == 0, "5-star pity resets after hard pity hit");
}

static void standard_hard_pity_forces_five_star_at_90() {
    WishEngine engine;
    WishState state;
    SequenceRandom random(std::vector<double>(200, 0.99));
    auto banner = makeStandardBanner();

    auto batch = engine.wish(banner, state, 90, random);

    require(batch.results.back().item.rarity == 5, "standard 90th wish must be 5-star");
    require(state.pity5 == 0, "standard 5-star pity resets");
}

int main() {
    try {
        character_hard_pity_forces_five_star_at_90();
        standard_hard_pity_forces_five_star_at_90();
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "WishEngineTests passed\n";
    return EXIT_SUCCESS;
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run:

```powershell
cmake -S backend -B build
cmake --build build
.\build\gacha_core_tests.exe
```

Expected: configure/build or test fails because engine headers and implementation do not exist.

- [ ] **Step 3: Implement core models, random provider, and minimal hard pity logic**

Create `backend/include/gacha/Models.h`:

```cpp
#pragma once

#include <string>
#include <vector>

namespace gacha {

enum class BannerType { CharacterEvent, WeaponEvent, Standard };
enum class ItemKind { Character, Weapon };

struct Item {
    std::string id;
    std::string name;
    int rarity = 3;
    ItemKind kind = ItemKind::Weapon;
    bool featured = false;
    bool promotional = false;
};

struct BannerConfig {
    std::string id;
    std::string name;
    BannerType type = BannerType::Standard;
    double fiveStarBaseRate = 0.006;
    int fiveStarHardPity = 90;
    double fourStarBaseRate = 0.051;
    int fourStarHardPity = 10;
    std::vector<Item> items;
};

struct WishResult {
    int wishNumber = 0;
    Item item;
    bool hitHardPity5 = false;
    bool hitHardPity4 = false;
    bool usedGuarantee = false;
    int fatePointsAfter = 0;
};

struct WishStats {
    int total = 0;
    int fiveStars = 0;
    int fourStars = 0;
    int featuredFiveStars = 0;
};

struct WishState {
    int pity5 = 0;
    int pity4 = 0;
    bool featuredGuarantee = false;
    bool promotionalGuarantee = false;
    int fatePoints = 0;
    std::string selectedPathItemId;
    std::vector<WishResult> history;
    WishStats stats;
};

struct WishBatch {
    std::vector<WishResult> results;
    WishState state;
};

}
```

Create `backend/include/gacha/RandomProvider.h`:

```cpp
#pragma once

#include <cstddef>
#include <random>
#include <stdexcept>
#include <vector>

namespace gacha {

class RandomProvider {
public:
    virtual ~RandomProvider() = default;
    virtual double nextDouble() = 0;
};

class DefaultRandom final : public RandomProvider {
public:
    double nextDouble() override {
        return distribution_(engine_);
    }

private:
    std::mt19937 engine_{std::random_device{}()};
    std::uniform_real_distribution<double> distribution_{0.0, 1.0};
};

class SequenceRandom final : public RandomProvider {
public:
    explicit SequenceRandom(std::vector<double> values) : values_(std::move(values)) {}

    double nextDouble() override {
        if (index_ >= values_.size()) {
            throw std::runtime_error("SequenceRandom exhausted");
        }
        return values_[index_++];
    }

private:
    std::vector<double> values_;
    std::size_t index_ = 0;
};

}
```

Create `backend/include/gacha/WishEngine.h`:

```cpp
#pragma once

#include "gacha/Models.h"
#include "gacha/RandomProvider.h"

namespace gacha {

class WishEngine {
public:
    WishBatch wish(const BannerConfig& banner, WishState& state, int count, RandomProvider& random) const;
};

}
```

Create `backend/src/WishEngine.cpp` with helper selection functions and a loop that increments pity, applies hard pity, selects rarity 5/4/3, resets pity after hits, updates stats, appends history, and returns the copied state.

Create `backend/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(GachaSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(gacha_core
    src/WishEngine.cpp
)
target_include_directories(gacha_core PUBLIC include)

add_executable(gacha_core_tests tests/WishEngineTests.cpp)
target_link_libraries(gacha_core_tests PRIVATE gacha_core)
```

- [ ] **Step 4: Run tests to verify they pass**

Run:

```powershell
cmake -S backend -B build
cmake --build build
.\build\gacha_core_tests.exe
```

Expected: `WishEngineTests passed`.

- [ ] **Step 5: Add guarantee and weapon path tests first, verify red, implement green**

Extend `WishEngineTests.cpp` with tests for:

- Character 50/50 loss sets `featuredGuarantee`, and the next 5-star returns the featured item.
- 4-star hard pity returns rarity 4 or above at the 10th wish.
- Weapon hard pity returns rarity 5 at wish 80.
- Weapon non-promotional 5-star sets `promotionalGuarantee`, and the next 5-star is promotional.
- Weapon selected path gains fate points for non-selected 5-stars.
- Weapon selected path with 2 fate points forces the selected weapon and resets fate points.
- Changing `selectedPathItemId` through direct state assignment before the next wish resets fate points only when later API task adds path endpoint; for Task 1 cover engine behavior when already at 2 fate points.

Run the same build and test command after adding tests and before implementing. Expected: failing assertions for missing guarantee/path logic.

Then update `WishEngine.cpp` to:

- Use 50/50 roll for character featured selection.
- Use 75/25 roll for weapon promotional selection.
- Respect `featuredGuarantee` and `promotionalGuarantee`.
- Respect `fatePoints >= 2` before ordinary weapon promotional selection.
- Increment or reset fate points after weapon 5-star selection.

- [ ] **Step 6: Run final backend core tests**

Run:

```powershell
cmake -S backend -B build
cmake --build build
.\build\gacha_core_tests.exe
```

Expected: `WishEngineTests passed`.

- [ ] **Step 7: Commit**

```powershell
git add backend
git commit -m "feat: add gacha wish engine"
```

---

### Task 2: Config, JSON, and Local HTTP Server

**Files:**
- Create: `configs/banners.json`
- Create: `backend/include/gacha/BannerRepository.h`
- Create: `backend/src/BannerRepository.cpp`
- Create: `backend/include/gacha/Json.h`
- Create: `backend/src/Json.cpp`
- Create: `backend/src/main.cpp`
- Modify: `backend/CMakeLists.txt`
- Create: `scripts/build.ps1`
- Create: `scripts/test.ps1`
- Create: `scripts/run.ps1`
- Create: `.gitignore`
- Create: `README.md`

**Interfaces:**
- Consumes: `WishEngine::wish`, `BannerConfig`, `WishState`, `DefaultRandom`
- Produces: executable `gacha_server`
- Produces: endpoints `GET /api/state`, `POST /api/wish`, `POST /api/path`, `POST /api/reset`
- Produces: static serving for `/`, `/styles.css`, `/app.js`

- [ ] **Step 1: Write failing API smoke test**

Create `backend/tests/ApiSmokeTests.cpp` that links server helpers without opening a socket. Test JSON output from a default app state includes all three banner ids and that invalid wish count returns `invalid_count`.

Expected core helper signatures:

```cpp
namespace gacha {
class AppController {
public:
    std::string stateJson() const;
    std::string wishJson(const std::string& bannerId, int count);
    std::string setPathJson(const std::string& bannerId, const std::string& itemId);
    std::string resetJson();
};
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run:

```powershell
cmake -S backend -B build
cmake --build build
.\build\api_smoke_tests.exe
```

Expected: build fails because `AppController` and JSON helpers do not exist.

- [ ] **Step 3: Implement banner repository and JSON helpers**

Use original Chinese placeholder names in built-in configs:

- Character banner id `character-event`, featured 5-star `星辉旅人`.
- Weapon banner id `weapon-event`, featured 5-star weapons `苍曜长弓` and `赤砂法杖`.
- Standard banner id `standard`, no featured items.

Also create `configs/banners.json` with matching ids and names for human-readable project data, even if the first backend uses built-in data.

- [ ] **Step 4: Implement AppController and local HTTP server**

`AppController` owns:

```cpp
std::map<std::string, BannerConfig> banners_;
std::map<std::string, WishState> states_;
DefaultRandom random_;
WishEngine engine_;
```

Behavior:

- `stateJson()` returns all banners and all current states.
- `wishJson()` accepts only count `1` or `10`; otherwise returns JSON containing `"code":"invalid_count"`.
- `setPathJson()` accepts only weapon banner and item ids among promotional 5-star weapons; changing or clearing path resets fate points.
- `resetJson()` resets all states to defaults.

`main.cpp` should serve static files from `../frontend` when run from the repository root or from paths relative to the executable when possible. It should print `http://127.0.0.1:18080` on startup.

- [ ] **Step 5: Update CMake and scripts**

Add executables:

```cmake
add_executable(api_smoke_tests tests/ApiSmokeTests.cpp src/BannerRepository.cpp src/Json.cpp src/main.cpp)
target_compile_definitions(api_smoke_tests PRIVATE GACHA_TESTING=1)
target_link_libraries(api_smoke_tests PRIVATE gacha_core)

add_executable(gacha_server src/BannerRepository.cpp src/Json.cpp src/main.cpp)
target_link_libraries(gacha_server PRIVATE gacha_core)
```

Ensure `main()` is excluded when `GACHA_TESTING` is defined.

- [ ] **Step 6: Run API tests**

Run:

```powershell
cmake -S backend -B build
cmake --build build
.\build\api_smoke_tests.exe
```

Expected: API smoke tests pass.

- [ ] **Step 7: Commit**

```powershell
git add .gitignore README.md backend configs scripts
git commit -m "feat: add local gacha API server"
```

---

### Task 3: Frontend Simulator UI

**Files:**
- Create: `frontend/index.html`
- Create: `frontend/styles.css`
- Create: `frontend/app.js`
- Modify: `README.md`

**Interfaces:**
- Consumes: `GET /api/state`
- Consumes: `POST /api/wish`
- Consumes: `POST /api/path`
- Consumes: `POST /api/reset`
- Produces: usable browser simulator in Chinese.

- [ ] **Step 1: Create frontend files with API-driven UI**

Build `index.html` with:

- Header title `抽卡模拟器`.
- Banner tabs/select for `character-event`, `weapon-event`, and `standard`.
- Weapon path selector area hidden unless current banner is `weapon-event`.
- Pity and guarantee status row.
- Buttons `单抽` and `十连`.
- Latest results region.
- History and stats panels.

Build `app.js` with functions:

```javascript
async function loadState()
async function performWish(count)
async function updatePath(itemId)
async function resetSimulator()
function render()
function renderBannerOptions()
function renderCurrentBanner()
function renderResults(results)
function renderHistory()
function renderStats()
```

Build `styles.css` with responsive layout, stable button sizes, rarity-specific styles, and no external assets.

- [ ] **Step 2: Manually verify static file presence**

Run:

```powershell
Test-Path frontend/index.html
Test-Path frontend/styles.css
Test-Path frontend/app.js
```

Expected: all three output `True`.

- [ ] **Step 3: Commit**

```powershell
git add frontend README.md
git commit -m "feat: add simulator frontend"
```

---

### Task 4: End-to-End Verification and Polish

**Files:**
- Modify as needed: `backend/src/main.cpp`
- Modify as needed: `frontend/app.js`
- Modify as needed: `frontend/styles.css`
- Modify as needed: `README.md`

**Interfaces:**
- Consumes: server executable and frontend files from earlier tasks.
- Produces: verified local run instructions and final clean Git state.

- [ ] **Step 1: Run full backend test suite**

Run:

```powershell
.\scripts\test.ps1
```

Expected: `WishEngineTests passed` and `ApiSmokeTests passed`.

- [ ] **Step 2: Start local server**

Run:

```powershell
.\scripts\run.ps1
```

Expected: console prints `http://127.0.0.1:18080`.

- [ ] **Step 3: Verify API endpoints**

In a second shell, run:

```powershell
Invoke-RestMethod http://127.0.0.1:18080/api/state
Invoke-RestMethod -Method Post -ContentType 'application/json' -Body '{"bannerId":"character-event","count":10}' http://127.0.0.1:18080/api/wish
Invoke-RestMethod -Method Post -ContentType 'application/json' -Body '{"bannerId":"weapon-event","itemId":"weapon-a"}' http://127.0.0.1:18080/api/path
```

Expected: state JSON returns banners; wish JSON returns 10 results; path JSON returns updated weapon state or a clear error if the id differs from the final placeholder id.

- [ ] **Step 4: Verify frontend in browser**

Open `http://127.0.0.1:18080` and verify:

- Character banner loads by default.
- Single wish adds one latest result and one history row.
- Ten wishes adds ten latest results.
- Weapon path selector appears only for the weapon banner.
- Reset clears counters and history.

- [ ] **Step 5: Fix only integration defects found by verification**

For each defect, first add or update the smallest backend test when the defect is backend-observable. Then run the failing test, implement the fix, and re-run the test.

- [ ] **Step 6: Commit final polish if files changed**

```powershell
git status --short
git add backend frontend README.md scripts
git commit -m "fix: polish simulator integration"
```

Skip the commit only if `git status --short` is empty.

- [ ] **Step 7: Final verification**

Run:

```powershell
.\scripts\test.ps1
git status --short --branch
git log --oneline --decorate -5
```

Expected: tests pass, status is clean on the implementation branch, and recent commits show each milestone.
