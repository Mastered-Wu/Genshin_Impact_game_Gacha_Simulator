const state = {
  banners: [],
  states: {},
  resources: { currency: 0, wishCost: 160, affordableWishes: 0 },
  currentBannerId: "character-event",
  latestResults: [],
  loading: false,
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

async function performWish(count) {
  setLoading(true);
  try {
    const data = await requestJson("/api/wish", {
      method: "POST",
      body: JSON.stringify({ bannerId: state.currentBannerId, count }),
    });
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
  ["wish-one", "wish-ten", "reset-button", "path-select", "resource-input", "resource-button"].forEach((id) => {
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
}

function renderResources() {
  const currency = state.resources.currency || 0;
  const wishCost = state.resources.wishCost || 160;
  const affordableWishes = state.resources.affordableWishes || Math.floor(currency / wishCost);
  $("resource-balance").textContent = currency;
  $("affordable-wishes").textContent = affordableWishes;
  $("resource-input").value = currency;
  $("wish-one").disabled = state.loading || affordableWishes < 1;
  $("wish-ten").disabled = state.loading || affordableWishes < 10;
}

function renderBannerOptions() {
  const tabs = $("banner-tabs");
  tabs.innerHTML = "";
  state.banners.forEach((banner) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = `tab-button${banner.id === state.currentBannerId ? " active" : ""}`;
    button.textContent = banner.name;
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

$("wish-one").addEventListener("click", () => performWish(1));
$("wish-ten").addEventListener("click", () => performWish(10));
$("reset-button").addEventListener("click", resetSimulator);
$("resource-button").addEventListener("click", updateResources);
$("resource-input").addEventListener("keydown", (event) => {
  if (event.key === "Enter") {
    updateResources();
  }
});
$("path-select").addEventListener("change", (event) => updatePath(event.target.value));

loadState();
