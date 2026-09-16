# 抽卡模拟器

这是一个本地运行的抽卡模拟器。项目使用 C++ 实现后端规则和本地 API，前端使用纯 HTML、CSS 和 JavaScript。所有名称和物品都是原创占位内容，不使用官方名称、美术或联网数据。

## 功能

- 角色活动祈愿：0.6% 5 星基础概率、90 抽硬保底、10 抽 4 星或以上保底、50/50 与限定保底。
- 武器活动祈愿：0.7% 5 星基础概率、80 抽硬保底、10 抽 4 星或以上保底、75/25、限定保底和命定值。
- 常驻祈愿：0.6% 5 星基础概率、90 抽硬保底、10 抽 4 星或以上保底。
- 单抽、十连、历史、统计、保底状态和武器定轨。

## 构建

```powershell
.\scripts\build.ps1
```

## 测试

```powershell
.\scripts\test.ps1
```

## 运行

```powershell
.\scripts\run.ps1
```

启动后打开：

```text
http://127.0.0.1:18080
```

## API

- `GET /api/state`
- `POST /api/wish`，请求示例：`{ "bannerId": "character-event", "count": 10 }`
- `POST /api/path`，请求示例：`{ "bannerId": "weapon-event", "itemId": "weapon-a" }`
- `POST /api/reset`

错误响应包含稳定的 `code` 字段，例如 `unknown_banner`、`invalid_count`、`invalid_path_item`。

## 协作注意

不要提交 `build/`、可执行文件、日志或本地临时文件。修改规则、API 或运行方式时，同步更新 README、设计说明或实现计划。
