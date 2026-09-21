// 全局前端状态：保存后端返回的数据、当前卡池、最新结果和 UI 状态。
const state = {
  banners: [],
  states: {},
  resources: { currency: 16000, eventFates: 0, standardFates: 0, wishCost: 160 },
  currentBannerId: "character-event",
  latestResults: [],
  loading: false,
  pendingTopUp: null,
  uid: "",
  activeView: "wish",
};

// 抽卡结果图片映射：后端只返回物品 id，前端按 id 选择本地占位图。
const rewardImageById = {
  "featured-hero": "/assets/images/featured-hero.png",
  "standard-hero-a": "/assets/images/standard-hero-a.png",
  "standard-hero-b": "/assets/images/standard-hero-b.png",
  "standard-hero-c": "/assets/images/standard-hero-c.png",
  "featured-four-a": "/assets/images/featured-four-a.png",
  "featured-four-b": "/assets/images/featured-four-b.png",
  "standard-four-hero": "/assets/images/standard-four-hero.png",
  "standard-four-weapon": "/assets/images/standard-four-weapon.png",
  "weapon-a": "/assets/images/weapon-a.png",
  "weapon-b": "/assets/images/weapon-b.png",
  "weapon-standard-a": "/assets/images/weapon-standard-a.png",
  "weapon-standard-b": "/assets/images/weapon-standard-b.png",
  "weapon-four-a": "/assets/images/weapon-four-a.png",
  "weapon-four-b": "/assets/images/weapon-four-b.png",
  "three-sword": "/assets/images/three-sword.png",
  "three-bow": "/assets/images/three-bow.png",
  "three-catalyst": "/assets/images/three-catalyst.png",
};

const $ = (id) => document.getElementById(id);

// 页面生命周期信号：关闭页面时请求后端退出，刷新页面时再取消退出。
function postLifecycleSignal(url) {
  if (navigator.sendBeacon) {
    navigator.sendBeacon(url, new Blob(["{}"], { type: "application/json" }));
    return;
  }
  fetch(url, {
    method: "POST",
    body: "{}",
    keepalive: true,
    headers: { "Content-Type": "application/json" },
  }).catch(() => {});
}

// API 基础请求封装：所有前端操作都通过本地后端读写状态。
async function requestJson(url, options = {}) {
  const response = await fetch(url, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  return response.json();
}

// 状态加载：从后端读取卡池、资源、保底、历史和统计。
async function loadState() {
  setLoading(true);
  try {
    const data = await requestJson("/api/state");
    if (data.code) {
      throw new Error(data.error || "状态加载失败");
    }
    applyServerState(data);
    render();
    return true;
  } catch (error) {
    showMessage("无法连接本地服务");
    return false;
  } finally {
    setLoading(false);
  }
}

// 登录页输入处理：只允许 9 位数字 UID。
function handleUidInput() {
  const input = $("uid-input");
  input.value = input.value.replace(/\D/g, "").slice(0, 9);
  const valid = isValidUid(input.value);
  $("enter-button").disabled = !valid;
  $("login-message").textContent = valid ? "可以进入模拟器" : "UID 必须为 9 位数字";
}

// 进入模拟器：校验 UID，并把本次会话初始化为默认状态。
async function enterSimulator() {
  const uid = $("uid-input").value.trim();
  if (!isValidUid(uid)) {
    $("login-message").textContent = "请输入 9 位数字 UID";
    return;
  }

  $("enter-button").disabled = true;
  $("login-message").textContent = "正在初始化模拟器...";
  state.uid = uid;
  state.currentBannerId = "character-event";
  state.latestResults = [];
  state.pendingTopUp = null;
  state.activeView = "wish";
  const initialized = await resetSimulator(false);
  if (!initialized) {
    $("login-message").textContent = "无法加载模拟器，请确认本地服务正在运行";
    $("enter-button").disabled = false;
    return;
  }

  $("uid-display").textContent = `UID ${uid}`;
  $("login-screen").classList.add("hidden");
  $("simulator-app").classList.remove("hidden");
  $("simulator-app").setAttribute("aria-hidden", "false");
  showMessage("已加载本次模拟状态");
}

function isValidUid(uid) {
  return /^\d{9}$/.test(uid);
}

// 抽卡操作：发起单抽/十连，并处理缘券不足、结果弹窗和状态刷新。
async function performWish(count, allowCurrencyTopUp = false) {
  setLoading(true);
  try {
    const data = await requestJson("/api/wish", {
      method: "POST",
      body: JSON.stringify({ bannerId: state.currentBannerId, count, allowCurrencyTopUp }),
    });
    if (data.code === "need_currency_confirm") {
      applyServerState(data);
      render();
      openTopUpModal(count, data.missingFates || 0, data.requiredCurrency || 0);
      return;
    }
    if (data.code) {
      showMessage(data.error || "操作失败");
      return;
    }
    state.latestResults = data.results || [];
    applyServerState(data);
    render();
    openResultsModal();
    showMessage(`${count === 1 ? "单抽" : "十连"}完成`);
  } catch (error) {
    showMessage("抽卡请求失败");
  } finally {
    setLoading(false);
  }
}

// 星石输入区：手动设置当前星石余额。
async function updateResources() {
  const rawValue = $("resource-input").value.trim();
  const currency = Number.parseInt(rawValue, 10);
  if (!Number.isFinite(currency) || currency < 0) {
    showMessage("请输入不小于 0 的资源数量");
    return;
  }

  setLoading(true);
  try {
    const data = await requestJson("/api/resources", {
      method: "POST",
      body: JSON.stringify({ currency }),
    });
    if (data.code) {
      showMessage(data.error || "资源设置失败");
      return;
    }
    applyServerState(data);
    render();
    showMessage("资源已更新");
  } catch (error) {
    showMessage("资源设置请求失败");
  } finally {
    setLoading(false);
  }
}

// 缘券兑换区：把星石兑换为当前卡池使用的缘券。
async function exchangeFates() {
  const fates = Number.parseInt($("exchange-range").value, 10);
  if (!Number.isFinite(fates) || fates < 1) {
    showMessage("请选择要兑换的数量");
    return;
  }

  setLoading(true);
  try {
    const data = await requestJson("/api/exchange", {
      method: "POST",
      body: JSON.stringify({ bannerId: state.currentBannerId, fates }),
    });
    if (data.code) {
      showMessage(data.error || "兑换失败");
      return;
    }
    applyServerState(data);
    closeExchangeModal();
    render();
    showMessage(`已兑换 ${fates} 个${fateNameForCurrentBanner()}`);
  } catch (error) {
    showMessage("兑换请求失败");
  } finally {
    setLoading(false);
  }
}

// 武器定轨区：仅武器活动祈愿使用，切换目标会清空命定值。
async function updatePath(itemId) {
  setLoading(true);
  try {
    const data = await requestJson("/api/path", {
      method: "POST",
      body: JSON.stringify({ bannerId: "weapon-event", itemId: itemId || null }),
    });
    if (data.code) {
      showMessage(data.error || "定轨失败");
      return;
    }
    applyServerState(data);
    render();
    showMessage(itemId ? "定轨已更新" : "已清除定轨");
  } catch (error) {
    showMessage("定轨请求失败");
  } finally {
    setLoading(false);
  }
}

// 重置操作：恢复默认资源、清空历史、保底和命定值。
async function resetSimulator(showResult = true) {
  setLoading(true);
  try {
    const data = await requestJson("/api/reset", {
      method: "POST",
      body: "{}",
    });
    if (data.code) {
      showMessage(data.error || "重置失败");
      return false;
    }
    state.currentBannerId = "character-event";
    state.latestResults = [];
    state.pendingTopUp = null;
    state.activeView = "wish";
    closeResultsModal();
    applyServerState(data);
    render();
    if (showResult) {
      showMessage("已恢复默认状态");
    }
    return true;
  } catch (error) {
    showMessage("重置失败");
    return false;
  } finally {
    setLoading(false);
  }
}

// 后端状态合并：把 API 返回的数据写回前端 state。
function applyServerState(data) {
  state.banners = data.banners || state.banners;
  state.states = data.states || state.states;
  state.resources = data.resources || state.resources;
  if (data.currentBannerId && !state.banners.some((banner) => banner.id === state.currentBannerId)) {
    state.currentBannerId = data.currentBannerId;
  }
}

// 加载态控制：禁用按钮/输入框，防止重复请求。
function setLoading(loading) {
  state.loading = loading;
  [
    "wish-one",
    "wish-ten",
    "reset-button",
    "path-select",
    "resource-input",
    "resource-button",
    "exchange-button",
    "exchange-confirm",
    "exchange-cancel",
    "topup-confirm",
    "topup-cancel",
  ].forEach((id) => {
    const element = $(id);
    if (element) {
      element.disabled = loading;
    }
  });
  if (!loading && state.resources) {
    renderResources();
  }
}

function showMessage(message) {
  $("message").textContent = message;
}

// 当前卡池读取：后续渲染函数统一从这里拿活动卡池。
function currentBanner() {
  return state.banners.find((banner) => banner.id === state.currentBannerId) || state.banners[0];
}

function currentBannerState() {
  return state.states[state.currentBannerId] || {};
}

// 总渲染入口：每次状态变化后刷新所有可见板块。
function render() {
  renderBannerOptions();
  renderCurrentBanner();
  renderResources();
  renderResults(state.latestResults);
  renderHistory();
  renderStats();
  renderActiveView();
}

// 页面切换：在抽卡页、统计页和历史页之间切换。
function showView(view) {
  state.activeView = view;
  renderActiveView();
  showMessage("");
}

function renderActiveView() {
  const views = {
    wish: $("wish-view"),
    stats: $("stats-view"),
    history: $("history-view"),
  };
  Object.entries(views).forEach(([name, element]) => {
    if (element) {
      element.classList.toggle("hidden", name !== state.activeView);
    }
  });
}

// 顶部资源区与抽卡可用次数：同步星石、缘券和按钮可用状态。
function renderResources() {
  const banner = currentBanner();
  const currency = state.resources.currency || 0;
  const eventFates = state.resources.eventFates || 0;
  const standardFates = state.resources.standardFates || 0;
  const wishCost = state.resources.wishCost || 160;
  const activeFates = isStandardBanner(banner) ? standardFates : eventFates;
  const affordableWishes = activeFates + Math.floor(currency / wishCost);
  const exchangeableFates = state.resources.exchangeableFates ?? Math.floor(currency / wishCost);
  $("resource-balance").textContent = currency;
  $("event-fate-balance").textContent = eventFates;
  $("standard-fate-balance").textContent = standardFates;
  $("affordable-wishes").textContent = affordableWishes;
  $("resource-input").value = currency;
  $("exchange-button").textContent = `兑换${fateNameForBanner(banner)}`;
  $("exchange-button").disabled = state.loading || exchangeableFates < 1;
  $("wish-one").disabled = state.loading || affordableWishes < 1;
  $("wish-ten").disabled = state.loading || affordableWishes < 10;
  updateExchangeRange();
}

// 卡池标签区：根据后端配置渲染角色、武器、常驻三个入口。
function renderBannerOptions() {
  const tabs = $("banner-tabs");
  tabs.innerHTML = "";
  state.banners.forEach((banner) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = `tab-button${banner.id === state.currentBannerId ? " active" : ""}`;
    button.innerHTML = `<span>${escapeHtml(banner.name)}</span><small>${poolTypeLabel(banner)}</small>`;
    button.addEventListener("click", () => {
      state.currentBannerId = banner.id;
      state.latestResults = [];
      render();
      showMessage("");
    });
    tabs.appendChild(button);
  });
}

// 当前卡池信息区：渲染卡池名、限定列表、保底和保底状态。
function renderCurrentBanner() {
  const banner = currentBanner();
  if (!banner) {
    return;
  }
  const bannerState = currentBannerState();
  const fiveStars = banner.items.filter((item) => item.rarity === 5 && (item.featured || item.promotional));
  $("current-banner-name").textContent = banner.name;
  $("pool-type-badge").textContent = poolTypeLabel(banner);
  $("pool-type-badge").className = `pool-type ${isStandardBanner(banner) ? "standard" : "limited"}`;
  $("banner-subtitle").textContent = banner.id === "weapon-event" ? "武器活动祈愿" : banner.id === "standard" ? "常驻祈愿" : "角色活动祈愿";
  $("featured-line").textContent = fiveStars.length > 0
    ? `限定：${fiveStars.map((item) => item.name).join("、")}`
    : "无第一版限定保底";
  $("pity5").textContent = `${bannerState.pity5 || 0} / ${banner.fiveStarHardPity}`;
  $("pity4").textContent = `${bannerState.pity4 || 0} / ${banner.fourStarHardPity}`;
  $("fate-points").textContent = `${bannerState.fatePoints || 0} / 2`;

  const guaranteeText = banner.id === "character-event" && bannerState.featuredGuarantee
    ? "下个 5 星限定"
    : banner.id === "weapon-event" && bannerState.promotionalGuarantee
      ? "下个 5 星限定"
      : "普通状态";
  $("guarantee-badge").textContent = guaranteeText;

  renderPathSelector(banner, bannerState);
}

// 武器定轨选择器：只在武器池显示。
function renderPathSelector(banner, bannerState) {
  const panel = $("path-panel");
  const select = $("path-select");
  const fateTile = $("fate-tile");
  const isWeapon = banner.id === "weapon-event";
  panel.classList.toggle("hidden", !isWeapon);
  fateTile.classList.toggle("hidden", !isWeapon);
  if (!isWeapon) {
    return;
  }

  select.innerHTML = "";
  const clear = document.createElement("option");
  clear.value = "";
  clear.textContent = "不定轨";
  select.appendChild(clear);

  banner.pathItems.forEach((item) => {
    const option = document.createElement("option");
    option.value = item.id;
    option.textContent = item.name;
    select.appendChild(option);
  });

  select.value = bannerState.selectedPathItemId || "";
}

// 抽卡结果弹窗内容：展示最近一次抽卡返回的结果组。
function renderResults(results) {
  const container = $("results");
  $("result-count").textContent = `${results.length} 条`;
  if (!results.length) {
    container.className = "results empty";
    container.textContent = "暂无结果";
    return;
  }
  container.className = "results";
  container.innerHTML = "";
  results.forEach((result) => {
    container.appendChild(resultCard(result));
  });
}

// 单个抽卡结果卡片：包含稀有度、图片、名称和保底标签。
function resultCard(result) {
  const card = document.createElement("article");
  card.className = `result-card rarity-${result.item.rarity}`;
  const notes = [];
  if (result.hitHardPity5) notes.push("5 星硬保底");
  if (result.hitHardPity4) notes.push("4 星保底");
  if (result.usedGuarantee) notes.push("保底生效");
  if (result.item.featured || result.item.promotional) notes.push("限定");
  const art = document.createElement("div");
  art.className = "result-art";
  art.setAttribute("role", "img");
  art.setAttribute("aria-label", `${result.item.name}图片`);
  const imagePath = rewardImageById[result.item.id];
  if (imagePath) {
    art.style.backgroundImage = `url("${imagePath}")`;
  }

  const copy = document.createElement("div");
  copy.className = "result-copy";
  copy.innerHTML = `
    <span class="stars">${"★".repeat(result.item.rarity)}</span>
    <strong>${escapeHtml(result.item.name)}</strong>
    <div class="tagline">${notes.length ? notes.join(" / ") : "常规获得"}</div>
  `;

  card.append(art, copy);
  return card;
}

// 历史页：展示当前卡池最近 80 条抽卡记录。
function renderHistory() {
  const bannerState = currentBannerState();
  const history = bannerState.history || [];
  const container = $("history");
  $("history-count").textContent = `${history.length} 条`;
  if (!history.length) {
    container.className = "history-list empty";
    container.textContent = "暂无历史";
    return;
  }
  container.className = "history-list";
  container.innerHTML = "";
  history.slice(0, 80).forEach((result) => {
    const row = document.createElement("article");
    row.className = `history-row rarity-${result.item.rarity}`;
    row.innerHTML = `
      <span class="stars">${result.item.rarity} 星</span>
      <strong>${escapeHtml(result.item.name)}</strong>
      <span class="history-meta">第 ${result.wishNumber} 抽</span>
    `;
    container.appendChild(row);
  });
}

// 统计页：展示当前卡池的总抽数、4/5 星和当前保底计数。
function renderStats() {
  const bannerState = currentBannerState();
  const stats = bannerState.stats || {};
  const entries = [
    ["总抽数", stats.total || 0],
    ["5 星", stats.fiveStars || 0],
    ["4 星", stats.fourStars || 0],
    ["限定 5 星", stats.featuredFiveStars || 0],
    ["5 星计数", bannerState.pity5 || 0],
    ["4 星计数", bannerState.pity4 || 0],
  ];
  $("stats").innerHTML = entries.map(([label, value]) => `
    <div class="stat"><span>${label}</span><strong>${value}</strong></div>
  `).join("");
}

// 文本转义：防止后端物品名中的特殊字符破坏 HTML。
function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}

// 兑换弹窗：打开、关闭和同步滑条数值。
function openExchangeModal() {
  updateExchangeRange();
  $("exchange-title").textContent = `兑换${fateNameForCurrentBanner()}`;
  $("exchange-modal").classList.remove("hidden");
}

function closeExchangeModal() {
  $("exchange-modal").classList.add("hidden");
}

function updateExchangeRange() {
  const currency = state.resources.currency || 0;
  const wishCost = state.resources.wishCost || 160;
  const max = Math.floor(currency / wishCost);
  const range = $("exchange-range");
  range.max = String(max);
  if (Number.parseInt(range.value, 10) > max) {
    range.value = String(max);
  }
  if (max > 0 && Number.parseInt(range.value, 10) < 1) {
    range.value = "1";
  }
  const fates = Number.parseInt(range.value, 10) || 0;
  $("exchange-count").textContent = `${fates} 个`;
  $("exchange-cost").textContent = `消耗 ${fates * wishCost} 星石`;
  $("exchange-confirm").disabled = state.loading || fates < 1;
  $("exchange-copy").textContent = max > 0
    ? `当前池子使用${fateNameForCurrentBanner()}，最多可兑换 ${max} 个。`
    : "星石不足 160，暂时无法兑换。";
}

// 星石补足弹窗：缘券不足时确认是否用星石补齐本次抽卡。
function openTopUpModal(count, missingFates, requiredCurrency) {
  state.pendingTopUp = { count };
  $("topup-title").textContent = `${fateNameForCurrentBanner()}不足`;
  $("topup-copy").textContent = `还差 ${missingFates} 个${fateNameForCurrentBanner()}，需要补充 ${requiredCurrency} 星石。`;
  $("topup-modal").classList.remove("hidden");
}

function closeTopUpModal() {
  state.pendingTopUp = null;
  $("topup-modal").classList.add("hidden");
}

// 结果弹窗：展示最近一次单抽或十连。
function openResultsModal() {
  $("results-modal").classList.remove("hidden");
}

function closeResultsModal() {
  $("results-modal").classList.add("hidden");
}

async function confirmTopUpWish() {
  const pending = state.pendingTopUp;
  closeTopUpModal();
  if (pending) {
    await performWish(pending.count, true);
  }
}

// 卡池辅助文案：根据卡池类型决定缘券名称和标签。
function isStandardBanner(banner) {
  return banner && banner.id === "standard";
}

function fateNameForBanner(banner) {
  return isStandardBanner(banner) ? "恒辉之缘" : "星轨之缘";
}

function fateNameForCurrentBanner() {
  return fateNameForBanner(currentBanner());
}

function poolTypeLabel(banner) {
  return isStandardBanner(banner) ? "常驻池" : "限定池";
}

// 键盘快捷键：D 单抽/返回/关闭，F 十连/确认。
function handleWishShortcut(event) {
  if (event.repeat || state.loading) {
    return;
  }

  const key = event.key.toLowerCase();
  const visibleModal = ["exchange-modal", "topup-modal", "results-modal"]
    .map((id) => $(id))
    .find((modal) => modal && !modal.classList.contains("hidden"));

  if (visibleModal) {
    event.preventDefault();
    if (key === "d") {
      if (visibleModal.id === "exchange-modal") {
        closeExchangeModal();
      } else if (visibleModal.id === "topup-modal") {
        closeTopUpModal();
      } else {
        closeResultsModal();
      }
    } else if (key === "f") {
      if (visibleModal.id === "exchange-modal") {
        $("exchange-confirm").click();
      } else if (visibleModal.id === "topup-modal") {
        $("topup-confirm").click();
      } else {
        $("results-close").click();
      }
    }
    return;
  }

  if (state.activeView !== "wish") {
    if (key === "d") {
      event.preventDefault();
      showView("wish");
    }
    return;
  }

  const target = event.target;
  const isEditableTarget = target instanceof HTMLInputElement
    || target instanceof HTMLTextAreaElement
    || target instanceof HTMLSelectElement
    || target.isContentEditable;
  if (isEditableTarget || !$("simulator-app") || $("simulator-app").classList.contains("hidden")) {
    return;
  }

  if (key === "d") {
    event.preventDefault();
    performWish(1);
  } else if (key === "f") {
    event.preventDefault();
    performWish(10);
  }
}

// 事件绑定区：把页面按钮、输入框、弹窗和生命周期事件接到对应函数。
$("wish-one").addEventListener("click", () => performWish(1));
$("wish-ten").addEventListener("click", () => performWish(10));
$("open-stats").addEventListener("click", () => showView("stats"));
$("open-history").addEventListener("click", () => showView("history"));
document.querySelectorAll(".back-button").forEach((button) => {
  button.addEventListener("click", () => showView(button.dataset.view || "wish"));
});
$("uid-input").addEventListener("input", handleUidInput);
$("uid-input").addEventListener("keydown", (event) => {
  if (event.key === "Enter" && isValidUid(event.currentTarget.value)) {
    enterSimulator();
  }
});
$("enter-button").addEventListener("click", enterSimulator);
$("reset-button").addEventListener("click", () => resetSimulator(true));
$("resource-button").addEventListener("click", updateResources);
$("exchange-button").addEventListener("click", openExchangeModal);
$("exchange-range").addEventListener("input", updateExchangeRange);
$("exchange-cancel").addEventListener("click", closeExchangeModal);
$("exchange-confirm").addEventListener("click", exchangeFates);
$("topup-cancel").addEventListener("click", closeTopUpModal);
$("topup-confirm").addEventListener("click", confirmTopUpWish);
$("results-close").addEventListener("click", closeResultsModal);
$("resource-input").addEventListener("keydown", (event) => {
  if (event.key === "Enter") {
    updateResources();
  }
});
$("path-select").addEventListener("change", (event) => updatePath(event.target.value));
window.addEventListener("pagehide", () => postLifecycleSignal("/api/shutdown"));
document.addEventListener("keydown", handleWishShortcut);

postLifecycleSignal("/api/cancel-shutdown");
handleUidInput();
