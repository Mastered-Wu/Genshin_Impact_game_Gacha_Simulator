# 抽卡模拟器

这是一个本地运行的抽卡模拟器。项目使用 C++ 实现后端规则和本地 API，前端使用纯 HTML、CSS 和 JavaScript。所有名称和物品都是原创占位内容，不使用官方名称、美术或联网数据。

## 功能

- 角色活动祈愿：0.6% 5 星基础概率、73 抽后每抽增加 6% 出金率、90 抽硬保底、10 抽 4 星或以上保底、50/50 与限定保底。
- 武器活动祈愿：0.7% 5 星基础概率、80 抽硬保底、10 抽 4 星或以上保底、75/25、限定保底和命定值。
- 常驻祈愿：0.6% 5 星基础概率、90 抽硬保底、10 抽 4 星或以上保底。
- 打开网页后先输入 9 位数字 UID，校验通过后进入抽卡主界面。
- 单抽、十连、星石输入、限定池星轨之缘兑换、常驻池恒辉之缘兑换、星石补足确认、保底状态和武器定轨。统计和历史分别作为独立页面，通过抽卡主界面下方按钮进入。

## 构建

```powershell
.\scripts\build.ps1
```

## 测试

```powershell
.\scripts\test.ps1
```

## 运行

本项目是本地网页版工具，不建议开机自启或长期后台常驻。需要使用时启动服务，用完后停止即可。

最简单的方式是双击项目根目录的：

```text
launch-simulator.bat
```

它会用隐藏 PowerShell 启动本地服务并打开浏览器，启动窗口不会常驻。

启动服务：

```powershell
.\scripts\run.ps1
```

启动后在浏览器打开：

```text
http://127.0.0.1:18080
```

如果不想保留前台 PowerShell 窗口，也可以本次后台启动：

```powershell
.\scripts\start-background.ps1
```

用完后停止后台服务：

```powershell
.\scripts\stop-server.ps1
```

## API

- `GET /api/state`
- `POST /api/wish`，请求示例：`{ "bannerId": "character-event", "count": 10 }`
- `POST /api/resources`，请求示例：`{ "currency": 1600 }`
- `POST /api/exchange`，请求示例：`{ "bannerId": "character-event", "fates": 10 }`
- `POST /api/path`，请求示例：`{ "bannerId": "weapon-event", "itemId": "weapon-a" }`
- `POST /api/reset`，恢复 16000 星石、清空缘券、历史、保底和命定值。
- `POST /api/shutdown` / `POST /api/cancel-shutdown`，供页面关闭时自动停止本地服务，刷新页面时会取消关闭。

角色/武器限定池会优先消耗星轨之缘，常驻池会优先消耗恒辉之缘；不足时可用 `allowCurrencyTopUp: true` 确认按 160 星石一抽补足。错误响应包含稳定的 `code` 字段，例如 `unknown_banner`、`invalid_count`、`invalid_path_item`、`invalid_currency`、`invalid_exchange_count`、`need_currency_confirm`、`insufficient_currency`。

## 协作注意

不要提交 `build/`、可执行文件、日志或本地临时文件。修改规则、API 或运行方式时，同步更新 README、设计说明或实现计划。
