# 抽卡模拟器实现计划

> **给自动化开发代理的说明：** 必须使用子技能：推荐 `superpowers:subagent-driven-development`，也可以使用 `superpowers:executing-plans`，按任务逐项实现本计划。步骤使用复选框（`- [ ]`）语法进行跟踪。

**目标：** 构建一个本地 C++ 加 HTML 的抽卡模拟器，使用原创占位内容来模拟《原神》风格的祈愿机制。

**架构：** C++ 后端负责所有卡池配置、状态、随机结果、祈愿规则和 JSON API 响应。前端使用纯 HTML/CSS/JavaScript，由本地后端提供静态文件，并通过 `fetch()` 调用 API。规则引擎与 HTTP 层隔离，以便使用确定性测试验证保底、限定保底和命定值。

**技术栈：** C++17、CMake、单仓库 header-only 风格代码、纯 HTML/CSS/JavaScript，以及适合 PowerShell 的构建命令。

**规格文档：** `docs/superpowers/specs/2026-08-31-gacha-simulator-design.md`

## 全局约束

- 所有模拟器文件必须位于 `C:\Users\27415\Desktop\抽卡模拟器`。
- 后端逻辑必须使用 C++。
- 前端必须使用纯 HTML/CSS/JavaScript。
- 用户后续要求网页第一屏改为 UID 登录界面；输入 9 位数字 UID 后进入抽卡主界面，统计和历史分别通过主界面下方按钮进入独立页面。
- 使用原创占位名称和资源，不使用官方游戏物品名、美术或 Logo。
- 角色活动祈愿硬保底为 90 抽。
- 角色活动祈愿 5 星基础概率为 0.6%。
- 角色活动祈愿第 73 抽后进入软保底，第 74 抽起每抽增加 6% 出金率。
- 角色活动祈愿在获得非限定 5 星后，进入限定保底。
- 武器活动祈愿硬保底为 80 抽。
- 武器活动祈愿 5 星基础概率为 0.7%。
- 武器活动祈愿在获得非限定 5 星后，进入限定保底。
- 武器活动祈愿命定值上限为 2，并在下一次 5 星时强制获得所选武器。
- 常驻祈愿硬保底为 90 抽。
- 每个卡池的 4 星或以上硬保底均为 10 抽。
- 武器活动祈愿和常驻祈愿软保底不属于版本 1 范围。
- 生成的构建产物必须排除在 Git 之外。

---

## 文件结构

- 创建 `.gitignore`：忽略 `build/`、CMake 生成文件、二进制文件、日志和本地临时文件。
- 创建 `README.md`：说明项目目的、构建、运行、测试和 Git 注意事项。
- 创建 `configs/banners.json`：原创占位卡池名称和物品池。
- 创建 `backend/CMakeLists.txt`：构建 `gacha_core_tests` 和 `gacha_server`。
- 创建 `backend/include/gacha/Models.h`：共享的物品、卡池、状态、结果和统计结构体。
- 创建 `backend/include/gacha/RandomProvider.h`：确定性随机和默认随机接口。
- 创建 `backend/include/gacha/WishEngine.h`：规则引擎公开 API。
- 创建 `backend/src/WishEngine.cpp`：规则引擎实现。
- 创建 `backend/tests/WishEngineTests.cpp`：确定性后端测试。
- 创建 `backend/include/gacha/BannerRepository.h`：内置卡池配置提供器。
- 创建 `backend/src/BannerRepository.cpp`：占位配置数据。
- 创建 `backend/include/gacha/Json.h`：API 输出的 JSON 序列化辅助函数。
- 创建 `backend/src/Json.cpp`：最小 JSON 转义和响应序列化。
- 创建 `backend/src/main.cpp`：本地 HTTP/静态文件服务器和 API 路由。
- 创建 `frontend/index.html`：模拟器页面骨架。
- 创建 `frontend/styles.css`：响应式模拟器样式。
- 创建 `frontend/app.js`：API 调用、渲染、卡池切换和祈愿操作。
- 创建 `scripts/build.ps1`：CMake 配置/构建辅助脚本。
- 创建 `scripts/test.ps1`：CMake 配置/构建/测试辅助脚本。
- 创建 `scripts/run.ps1`：构建并启动服务器的辅助脚本。

---

### 任务 1：后端祈愿引擎

**文件：**
- 创建：`backend/CMakeLists.txt`
- 创建：`backend/include/gacha/Models.h`
- 创建：`backend/include/gacha/RandomProvider.h`
- 创建：`backend/include/gacha/WishEngine.h`
- 创建：`backend/src/WishEngine.cpp`
- 创建：`backend/tests/WishEngineTests.cpp`

**接口：**
- 产出：`gacha::WishEngine::wish(const BannerConfig&, WishState&, int count, RandomProvider&) -> WishBatch`
- 产出：`gacha::WishState` 字段 `pity5`、`pity4`、`featuredGuarantee`、`promotionalGuarantee`、`fatePoints`、`selectedPathItemId`、`history`、`stats`
- 产出：`gacha::SequenceRandom`，供测试使用的确定性随机提供器。
- 消耗：不依赖之前任务的输出。

- [ ] **步骤 1：先编写角色和常驻硬保底的失败测试**

在 `backend/tests/WishEngineTests.cpp` 中添加测试：

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

- [ ] **步骤 2：运行测试，确认失败**

运行：

```powershell
cmake -S backend -B build
cmake --build build
.\build\gacha_core_tests.exe
```

预期：配置、构建或测试失败，因为引擎头文件和实现尚不存在。

- [ ] **步骤 3：实现核心模型、随机提供器和最小硬保底逻辑**

创建 `backend/include/gacha/Models.h`：

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

创建 `backend/include/gacha/RandomProvider.h`：

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

创建 `backend/include/gacha/WishEngine.h`：

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

创建 `backend/src/WishEngine.cpp`：包含选择辅助函数和循环逻辑。循环需要递增保底、应用硬保底、选择 5/4/3 星稀有度、在命中后重置保底、更新统计、追加历史，并返回复制后的状态。

创建 `backend/CMakeLists.txt`：

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

- [ ] **步骤 4：运行测试，确认通过**

运行：

```powershell
cmake -S backend -B build
cmake --build build
.\build\gacha_core_tests.exe
```

预期：输出 `WishEngineTests passed`。

- [ ] **步骤 5：先添加保底和武器定轨测试，确认红灯，再实现绿灯**

扩展 `WishEngineTests.cpp`，增加以下测试：

- 角色 50/50 歪掉后设置 `featuredGuarantee`，下一个 5 星返回限定物品。
- 第 10 抽 4 星硬保底返回 4 星或以上稀有度。
- 武器第 80 抽硬保底返回 5 星。
- 武器抽到非限定 5 星后设置 `promotionalGuarantee`，下一个 5 星为限定。
- 武器选择定轨后，获得非所选 5 星会增加命定值。
- 武器定轨命定值为 2 时，强制获得所选武器并重置命定值。
- 直接通过状态赋值更改 `selectedPathItemId` 后，下一抽时重置命定值的逻辑将在后续 API 任务添加 path 接口时覆盖；任务 1 只覆盖命定值已经为 2 时的引擎行为。

添加测试后、实现前，运行同样的构建和测试命令。预期：由于缺少保底和定轨逻辑，断言失败。

然后更新 `WishEngine.cpp`：

- 使用 50/50 随机结果选择角色限定物品。
- 使用 75/25 随机结果选择武器限定物品。
- 遵守 `featuredGuarantee` 和 `promotionalGuarantee`。
- 在普通武器限定选择前，优先处理 `fatePoints >= 2`。
- 在武器 5 星选择后递增或重置命定值。

- [ ] **步骤 6：运行最终后端核心测试**

运行：

```powershell
cmake -S backend -B build
cmake --build build
.\build\gacha_core_tests.exe
```

预期：输出 `WishEngineTests passed`。

- [ ] **步骤 7：提交**

```powershell
git add backend
git commit -m "feat: add gacha wish engine"
```

---

### 任务 2：配置、JSON 和本地 HTTP 服务器

**文件：**
- 创建：`configs/banners.json`
- 创建：`backend/include/gacha/BannerRepository.h`
- 创建：`backend/src/BannerRepository.cpp`
- 创建：`backend/include/gacha/Json.h`
- 创建：`backend/src/Json.cpp`
- 创建：`backend/src/main.cpp`
- 修改：`backend/CMakeLists.txt`
- 创建：`scripts/build.ps1`
- 创建：`scripts/test.ps1`
- 创建：`scripts/run.ps1`
- 创建：`.gitignore`
- 创建：`README.md`

**接口：**
- 消耗：`WishEngine::wish`、`BannerConfig`、`WishState`、`DefaultRandom`
- 产出：可执行文件 `gacha_server`
- 产出：接口 `GET /api/state`、`POST /api/wish`、`POST /api/resources`、`POST /api/exchange`、`POST /api/path`
- 产出：为 `/`、`/styles.css`、`/app.js` 提供静态文件服务

- [ ] **步骤 1：编写失败的 API 冒烟测试**

创建 `backend/tests/ApiSmokeTests.cpp`，链接服务器辅助逻辑但不打开 socket。测试默认应用状态的 JSON 输出包含三个卡池 id，并测试非法抽卡次数返回 `invalid_count`。

预期核心辅助接口：

```cpp
namespace gacha {
class AppController {
public:
    std::string stateJson() const;
    std::string wishJson(const std::string& bannerId, int count);
    std::string setPathJson(const std::string& bannerId, const std::string& itemId);
};
}
```

- [ ] **步骤 2：运行测试，确认失败**

运行：

```powershell
cmake -S backend -B build
cmake --build build
.\build\api_smoke_tests.exe
```

预期：构建失败，因为 `AppController` 和 JSON 辅助函数尚不存在。

- [ ] **步骤 3：实现卡池仓库和 JSON 辅助函数**

在内置配置中使用原创中文占位名：

- 角色卡池 id 为 `character-event`，限定 5 星为 `星辉旅人`。
- 武器卡池 id 为 `weapon-event`，限定 5 星武器为 `苍曜长弓` 和 `赤砂法杖`。
- 常驻卡池 id 为 `standard`，无任何限定物品。

同时创建 `configs/banners.json`，使用匹配的 id 和名称，作为便于人工查看的项目数据；即使第一版后端使用内置数据，也要保留该配置文件。

- [ ] **步骤 4：实现 AppController 和本地 HTTP 服务器**

`AppController` 拥有：

```cpp
std::map<std::string, BannerConfig> banners_;
std::map<std::string, WishState> states_;
DefaultRandom random_;
WishEngine engine_;
```

行为：

- `stateJson()` 返回所有卡池和当前所有状态。
- `wishJson()` 只接受抽卡次数 `1` 或 `10`；否则返回包含 `"code":"invalid_count"` 的 JSON。限定池优先消耗星轨之缘，常驻池优先消耗恒辉之缘，不足时可通过确认参数使用星石补足。
- `setPathJson()` 只接受武器卡池，以及限定 5 星武器中的物品 id；更改或清除定轨会重置命定值。

`main.cpp` 应在从仓库根目录运行时，或从可执行文件相对路径运行时，提供 `../frontend` 下的静态文件。启动时应打印 `http://127.0.0.1:18080`。

- [ ] **步骤 5：更新 CMake 和脚本**

添加可执行目标：

```cmake
add_executable(api_smoke_tests tests/ApiSmokeTests.cpp src/BannerRepository.cpp src/Json.cpp src/main.cpp)
target_compile_definitions(api_smoke_tests PRIVATE GACHA_TESTING=1)
target_link_libraries(api_smoke_tests PRIVATE gacha_core)

add_executable(gacha_server src/BannerRepository.cpp src/Json.cpp src/main.cpp)
target_link_libraries(gacha_server PRIVATE gacha_core)
```

确保定义 `GACHA_TESTING` 时排除 `main()`。

- [ ] **步骤 6：运行 API 测试**

运行：

```powershell
cmake -S backend -B build
cmake --build build
.\build\api_smoke_tests.exe
```

预期：API 冒烟测试通过。

- [ ] **步骤 7：提交**

```powershell
git add .gitignore README.md backend configs scripts
git commit -m "feat: add local gacha API server"
```

---

### 任务 3：前端模拟器界面

**文件：**
- 创建：`frontend/index.html`
- 创建：`frontend/styles.css`
- 创建：`frontend/app.js`
- 修改：`README.md`

**接口：**
- 消耗：`GET /api/state`
- 消耗：`POST /api/wish`
- 消耗：`POST /api/path`
- 消耗：`POST /api/exchange`
- 产出：可在浏览器中使用的中文模拟器。

- [ ] **步骤 1：创建由 API 驱动的前端文件**

构建 `index.html`，包含：

- 标题 `抽卡模拟器`。
- `character-event`、`weapon-event` 和 `standard` 的卡池标签页或选择器。
- 武器定轨选择区域，仅在当前卡池为 `weapon-event` 时显示。
- 保底和保底状态行。
- 资源输入区域，显示星石、星轨之缘、恒辉之缘和当前卡池总可抽次数，并支持按 160 星石兑换 1 个对应缘券。
- `单抽` 和 `十连` 按钮。
- 最新结果区域。
- 历史和统计面板。

构建 `app.js`，包含以下函数：

```javascript
async function loadState()
async function performWish(count)
async function updatePath(itemId)
function render()
function renderBannerOptions()
function renderCurrentBanner()
function renderResults(results)
function renderHistory()
function renderStats()
```

构建 `styles.css`，包含响应式布局、稳定的按钮尺寸、稀有度样式，并且不使用外部资源。

- [ ] **步骤 2：手动验证静态文件存在**

运行：

```powershell
Test-Path frontend/index.html
Test-Path frontend/styles.css
Test-Path frontend/app.js
```

预期：三条命令均输出 `True`。

- [ ] **步骤 3：提交**

```powershell
git add frontend README.md
git commit -m "feat: add simulator frontend"
```

---

### 任务 4：端到端验证和打磨

**文件：**
- 按需修改：`backend/src/main.cpp`
- 按需修改：`frontend/app.js`
- 按需修改：`frontend/styles.css`
- 按需修改：`README.md`

**接口：**
- 消耗：前面任务产出的服务器可执行文件和前端文件。
- 产出：已验证的本地运行说明和最终干净的 Git 状态。

- [ ] **步骤 1：运行完整后端测试套件**

运行：

```powershell
.\scripts\test.ps1
```

预期：输出 `WishEngineTests passed` 和 `ApiSmokeTests passed`。

- [ ] **步骤 2：启动本地服务器**

运行：

```powershell
.\scripts\run.ps1
```

预期：控制台打印 `http://127.0.0.1:18080`。

- [ ] **步骤 3：验证 API 接口**

在第二个 shell 中运行：

```powershell
Invoke-RestMethod http://127.0.0.1:18080/api/state
Invoke-RestMethod -Method Post -ContentType 'application/json' -Body '{"currency":1600}' http://127.0.0.1:18080/api/resources
Invoke-RestMethod -Method Post -ContentType 'application/json' -Body '{"bannerId":"character-event","fates":10}' http://127.0.0.1:18080/api/exchange
Invoke-RestMethod -Method Post -ContentType 'application/json' -Body '{"bannerId":"character-event","count":10}' http://127.0.0.1:18080/api/wish
Invoke-RestMethod -Method Post -ContentType 'application/json' -Body '{"bannerId":"weapon-event","itemId":"weapon-a"}' http://127.0.0.1:18080/api/path
```

预期：state JSON 返回卡池；resources JSON 返回资源状态；wish JSON 返回 10 条结果并扣除 1600 资源；path JSON 返回更新后的武器状态。如果最终占位 id 不同，则返回清晰错误。

- [ ] **步骤 4：在浏览器中验证前端**

打开 `http://127.0.0.1:18080` 并验证：

- 默认加载角色卡池。
- 输入资源数量后显示当前资源和可抽次数。
- 单抽会新增一条最新结果和一条历史记录。
- 十连会新增十条最新结果。
- 武器定轨选择器只在武器卡池显示。

- [ ] **步骤 5：只修复验证中发现的集成缺陷**

对于每个缺陷，如果该缺陷能通过后端测试观察到，应先添加或更新最小后端测试。随后运行失败测试、实现修复，再重新运行测试。

- [ ] **步骤 6：如果有文件变化，提交最终打磨**

```powershell
git status --short
git add backend frontend README.md scripts
git commit -m "fix: polish simulator integration"
```

如果 `git status --short` 为空，则跳过提交。

- [ ] **步骤 7：最终验证**

运行：

```powershell
.\scripts\test.ps1
git status --short --branch
git log --oneline --decorate -5
```

预期：测试通过，当前实现分支状态干净，最近提交展示各个里程碑。
