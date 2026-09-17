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

const $ = (id) => document.getElementById(id);

async function requestJson(url, options = {}) {
  const response = await fetch(url, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  return response.json();
}

async function loadState() {
  setLoading(true);
  try {
    const data = await requestJson("/api/state");
    applyServerState(data);
    render();
  } catch (error) {
    showMessage("无法连接本地服务");
  } finally {
    setLoading(false);
  }
}

function handleUidInput() {
  const input = $("uid-input");
  input.value = input.value.replace(/\D/g, "").slice(0, 9);
  const valid = isValidUid(input.value);
  $("enter-button").disabled = !valid;
  $("login-message").textContent = valid ? "可以进入模拟器" : "UID 必须为 9 位数字";
}

async function enterSimulator() {
  const uid = $("uid-input").value.trim();
  if (!isValidUid(uid)) {
    $("login-message").textContent = "请输入 9 位数字 UID";
    return;
  }

  state.uid = uid;
  $("uid-display").textContent = `UID ${uid}`;
  $("login-screen").classList.add("hidden");
  $("simulator-app").classList.remove("hidden");
  $("simulator-app").setAttribute("aria-hidden", "false");
  await loadState();
}

function isValidUid(uid) {
  return /^\d{9}$/.test(uid);
}

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
    showMessage(`${count === 1 ? "单抽" : "十连"}完成`);
  } catch (error) {
    showMessage("抽卡请求失败");
  } finally {
    setLoading(false);
  }
}

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

async function resetSimulator() {
  setLoading(true);
  try {
    const data = await requestJson("/api/reset", { method: "POST", body: "{}" });
    state.latestResults = [];
    applyServerState(data);
    render();
    showMessage("已重置");
  } catch (error) {
    showMessage("重置失败");
  } finally {
    setLoading(false);
  }
}

function applyServerState(data) {
  state.banners = data.banners || state.banners;
  state.states = data.states || state.states;
  state.resources = data.resources || state.resources;
  if (data.currentBannerId && !state.banners.some((banner) => banner.id === state.currentBannerId)) {
    state.currentBannerId = data.currentBannerId;
  }
}

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

function currentBanner() {
  return state.banners.find((banner) => banner.id === state.currentBannerId) || state.banners[0];
}

function currentBannerState() {
  return state.states[state.currentBannerId] || {};
}

function render() {
  renderBannerOptions();
  renderCurrentBanner();
  renderResources();
  renderResults(state.latestResults);
  renderHistory();
  renderStats();
  renderActiveView();
}

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

function resultCard(result) {
  const card = document.createElement("article");
  card.className = `result-card rarity-${result.item.rarity}`;
  const notes = [];
  if (result.hitHardPity5) notes.push("5 星硬保底");
  if (result.hitHardPity4) notes.push("4 星保底");
  if (result.usedGuarantee) notes.push("保底生效");
  if (result.item.featured || result.item.promotional) notes.push("限定");
  card.innerHTML = `
    <span class="stars">${"★".repeat(result.item.rarity)}</span>
    <strong>${escapeHtml(result.item.name)}</strong>
    <div class="tagline">${notes.length ? notes.join(" / ") : "常规获得"}</div>
  `;
  return card;
}

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

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}

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

async function confirmTopUpWish() {
  const pending = state.pendingTopUp;
  closeTopUpModal();
  if (pending) {
    await performWish(pending.count, true);
  }
}

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
$("reset-button").addEventListener("click", resetSimulator);
$("resource-button").addEventListener("click", updateResources);
$("exchange-button").addEventListener("click", openExchangeModal);
$("exchange-range").addEventListener("input", updateExchangeRange);
$("exchange-cancel").addEventListener("click", closeExchangeModal);
$("exchange-confirm").addEventListener("click", exchangeFates);
$("topup-cancel").addEventListener("click", closeTopUpModal);
$("topup-confirm").addEventListener("click", confirmTopUpWish);
$("resource-input").addEventListener("keydown", (event) => {
  if (event.key === "Enter") {
    updateResources();
  }
});
$("path-select").addEventListener("change", (event) => updatePath(event.target.value));

handleUidInput();
