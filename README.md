# FrontendUI — UE5 游戏前端 UI 框架

基于 **Unreal Engine 5.8 + CommonUI** 的游戏前端 UI 框架，采用 Lyra 式的分层架构：**C++ 搭骨架、蓝图做表现**，界面之间以 GameplayTag 寻址，互不产生硬引用。

> **项目性质**：在课程工程（Udemy《UE5 C++: Advanced Frontend UI Programming》）基础上重构并扩展的个人项目。标注【个人扩展】的部分为课程之外的自主改动。
>
> **完成范围**：UI 框架层（界面栈寻址、资源异步加载、输入适配）与设置分类注册层已完成；设置项内容层（分辨率、音量等具体设置项）尚未实现，详见「后续计划」。

---

## 技术栈

| 项 | 内容 |
|---|---|
| 引擎 | Unreal Engine 5.8 |
| 语言 | C++（框架层） + 蓝图（表现层） |
| UI / 输入 | CommonUI、CommonInput、UMG、Enhanced Input |
| 其他 | GameplayTags、AssetManager 异步加载、DeveloperSettings、DataTable |

模块依赖见 `Source/FrontendUI/FrontendUI.Build.cs`。

---

## 核心实现

### 1. 四层 WidgetStack

界面按语义分入四个分层界面栈，以 Tag 寻址与入栈：

| 层栈 | 用途 |
|---|---|
| `Frontend.WidgetStack.Frontend` | 脱离游戏的前端流程（按任意键 → 主菜单 → 设置） |
| `Frontend.WidgetStack.GameMenu` | 游戏内叠加菜单（暂停菜单） |
| `Frontend.WidgetStack.GameHud` | 常驻 HUD（不抢焦点） |
| `Frontend.WidgetStack.Modal` | 最上层模态（确认弹窗，必须响应才能继续） |

- 主界面用 `RegisterWidgetStack(Tag, Stack)` 把 `CommonActivatableWidgetStack` 以 Tag 为键注册进 `TMap`（`Widget_PrimaryLayout.cpp`）
- 系统层统一走 `FindWidgetStackByTag()` 取栈，**不持有任何具体容器指针**
- 收益：新增界面只需「注册一个 Tag + 加一条映射」，不必改动已有的界面代码

### 2. 配置驱动的界面资源异步加载

- `UFrontendDeveloperSettings`（继承 `UDeveloperSettings`）在 **Project Settings** 中维护 `TMap<FGameplayTag, TSoftClassPtr<UWidget_ActivatableBase>>` 映射表，数据落在 `Config/DefaultGame.ini`
- 界面资源路径**零硬编码**，运行时经 `UAssetManager` 的 `StreamableManager` **按需异步加载**
- 蓝图侧通过 `UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag()` 取值，映射缺失时 `ensureMsgf` 报错而不是崩溃

### 3. 自定义蓝图异步节点（双时机委托）

`UAsyncAction_PushSoftWidget`（继承 `UBlueprintAsyncActionBase`）把整条链路封装成一个蓝图节点：

```
加载软引用 → 实例化 → 入栈 → 设置 OwningPlayer → 焦点设置
```

节点暴露两个时机，分别对应两个必须分开的阶段：

| 时机 | 触发点 | 用途 |
|---|---|---|
| `OnCreatedBeforePush` | Widget 已创建、**尚未入栈** | 注入初始化数据（如弹窗标题与按钮配置）、`SetOwningPlayer` |
| `AfterPush` | Widget 已入栈 | 设置焦点、收尾 |

> **为什么必须分两个时机**：数据注入要在入栈前完成，否则玩家会看到「先弹空壳、再刷出内容」的闪烁；而焦点设置必须等 Widget 进入可视树之后才可靠。

### 4. 数据驱动的设置界面

- `UOptionsDataRegistry`：注册中心，统一管理设置分类
- `UListDataObject_Base` / `UListDataObject_Collection`：以**组合模式**描述设置项层级（分类 → 分组 → 具体设置项）
- `UWidget_OptionsScreen` 激活时**动态注册 Tab 并做幂等去重**（`GetTabButtonBaseByID()` 判空跳过，避免重复注册）

### 5. 通用确认弹窗

- `UConfirmScreenInfoObject` 以**静态工厂**产出三种按钮组合：`CreateOkScreen` / `CreateYesNoScreen` / `CreateOkCancelScreen`
- 弹窗用 `UDynamicEntryBox` **动态生成按钮**，重建前 `Reset()` 并清理旧 `OnClicked()` 委托，避免委托堆积
- 按按钮语义自动绑定 CommonUI 输入动作与焦点

### 6. 手柄 / 键鼠双输入适配

- `CommonInputPlatformSettings` 注册两套 `ControllerData`（手柄 / 键鼠）
- 按键映射收敛到 `DT_CommonInputKeyMapping` 数据表（含 `BackAction` / `ConfirmAction` / `ResetAction`）
- 底部操作提示用 `CommonBoundActionBar` + 自定义 `WBP_Button_BoundAction`，**随输入设备自动切换文案与图标**
- 扩展 `ActivatableWidget` / `CommonButtonBase` 基类，并用 `IWidgetCompilerLog::ValidateCompiledDefaults` 在**蓝图编译期**拦截必填配置缺失（`FrontendTabListWidgetBase.cpp`）

---

## 个人扩展

### 扩展 1：设置分类由 C++ 硬注册改为 DataTable 驱动

**改造前**：4 个设置分类（Gameplay / Audio / Video / Control）由 4 个 C++ 函数硬编码创建，新增分类必须改代码并重新编译。

**改造后**：

- 新增行结构 `FFrontendOptionsTabRow`（继承 `FTableRowBase`），字段为 `DisplayName` + `SortOrder`
- 分类数据收敛到数据表 `DT_OptionsTabs`，表资产在 DeveloperSettings 中以 `TSoftObjectPtr` 配置（`FrontendDeveloperSettings.h`）
- `UOptionsDataRegistry` 改为读表构建分类对象，加载结果做缓存

**三个关键设计判断**

| 判断 | 理由 |
|---|---|
| **用 RowName 作为分类 ID**，不额外加 ID 字段 | DataTable 行名天然唯一，避免冗余字段与「行名和 ID 不一致」的隐患 |
| **必须按 `SortOrder` 显式排序** | `GetRowMap()` 的遍历走的是 `TMap` 底层 `TSet` 的**稀疏数组槽位**（`0,1,2,…` 跳过空槽），哈希桶只参与查找、不参与遍历。只插入不删除时顺序恰好等于插入顺序；但**删除后再插入会复用刚释放的槽位** —— 删中间一行再加一行，新行会出现在**中间**。即顺序是「资产保存 + 增删历史」的产物，**没有任何接口承诺它**，被 CSV 重导入或删行操作改掉后 UI 顺序会静默变化。用显式字段把它变成可 diff 的数据契约 |
| **入场校验行结构** | 该函数用 `GetRowMap()` + `reinterpret_cast` 按固定偏移裸读行内存（省掉 `FindRow` 的哈希查找），代价是放弃了引擎的类型校验，因此入口显式比对 `GetRowStruct()`，不一致时 `ensureMsgf` 报出 actual/expected 并放弃构表。注意用**相等**而非引擎 `FindRowInternal` 所用的 `IsChildOf`：`FindRow<T>` 按 `StaticStruct` 逐字段访问、允许子类，而裸读只认**这一种内存布局**，子类若有额外字段偏移即错位 |

> **约定**：`SortOrder` 必须唯一。重复时 Tab 顺序由 `TArray::Sort`（不稳定排序）的内部交换决定，没有业务含义、也不应被依赖 —— 与行结构一样，重复的 `SortOrder` 属于配置错误。

**验证方式**：在表中新增一行 `GraphicsTabCollection` 后重新运行，设置界面自动多出该分类，**全程未修改 C++ 代码、未重新编译**。

### 扩展 2：修复自研逻辑缺陷，并统一 UI 层失败处理

#### 缺陷 1：排序比较函数违反严格弱序

`BuildTabCollectionsFromTable()` 最初的实现是「先取全部 RowName，再按 `SortOrder` 排序」，比较函数在内部调用 `FindRow()`，且任一行数据缺失时**两个方向都返回 `false`**：

```cpp
if (!RowA || !RowB) { return false; }   // Comp(A,B) 与 Comp(B,A) 可能同时为假
return RowA->SortOrder < RowB->SortOrder;
```

| 问题 | 后果 |
|---|---|
| 比较函数内部调用 `FindRow()` | 每次比较最多 2 次哈希查找 + 字符串比较，n 个分类共 O(n log n) 次查表 |
| 比较函数不满足严格弱序 | `TArray::Sort` 的行为未定义，**玩家侧表现为 Tab 顺序错乱** |

**修复方式**：改为「单次遍历收集排序键 → 对纯值排序」。遍历时一次性取出 `RowName` / `DisplayName` / `SortOrder` 存入临时结构体，比较函数只比较 `int32`，成为满足严格弱序的纯函数；`DisplayName` 顺带取出，消除了原先排序后再 `FindRow()` 取显示名的重复查表。

**效果**：查表次数由 O(n log n) 降至 O(n)，且排序结果不再依赖未定义行为。

#### 缺陷 2：`BindWidget` 指针缺少空指针保护

`UWidget_OptionsScreen` 中 `TabListWidget_OptionsTabs` 为 `meta = (BindWidget)` 绑定，蓝图漏绑时该指针为空，原实现在两个入口处直接解引用（`NativeOnInitialized()` 直接注册委托、`NativeOnActivated()` 直接遍历分类并注册 Tab），漏绑即崩溃。

**修复方式**（与项目内其他失败处理保持一致）：

- `NativeOnInitialized()`：用 `ensureMsgf` 报一次配置错误（编辑器下即时报出，Shipping 下不致命），仅在 `IsValid()` 通过后才注册回调
- `NativeOnActivated()`：入口处判空并输出 `UE_LOG` 错误日志后提前返回
- `GetOrCreateDataRegistry()`：补上 `ensureMsgf`，并给 `NewObject` 传入 `this` 作为 Outer，把注册中心的生命周期挂到 Widget 上

**定位方式**：两处缺陷均通过**逐文件与课程参考实现 diff** 定位 —— 把本工程 `Source/` 与课程仓库的同名文件逐个对比，区分「跟随课程的部分」与「自己改动的部分」，再对自研代码单独 review。

---

## 如何运行

### 环境要求
- Unreal Engine **5.8**
- Visual Studio 2022 或 Rider

### 步骤

```
1. 克隆仓库
   git clone https://github.com/nas1lkzu/FrontendUI.git

2. 右键 FrontendUI.uproject → Generate Visual Studio project files

3. 用 VS / Rider 打开 FrontendUI.sln 并编译

4. 双击 FrontendUI.uproject 打开编辑器

5. 打开关卡 Content/Maps/FrontendTestMap，点击 PIE 运行
```

### 操作流程

```
启动 → 按任意键 → StartMenu（新游戏 / 继续）
                    → MainMenu（开始 / 设置 / 退出）
                       → 设置界面（可切换 Tab）
                       → 确认弹窗（由「新游戏」触发，演示按钮回调链路）
```

> 鼠标与手柄均可操作；切换输入设备时底部提示图标会自动变化。
>
> ⚠️ `StartMenu` 的「新游戏」演示的是确认弹窗的**按钮回调链路**，确认后**不会加载游戏关卡**（见「后续计划 · 演示范围」）。

---

## 目录结构

```
FrontendUI/
├── Config/
│   └── DefaultGame.ini          # 界面映射表、Options Tab 表、CommonInput 配置
├── Content/
│   ├── Assets/                  # 美术素材（课程配套，约 425MB，与 UI 实现无关）
│   ├── Maps/                    # FrontendTestMap（实际使用的关卡）
│   └── UI/
│       ├── CommonInputData/     # 输入数据表与 ControllerData
│       ├── Styles/              # CommonUI 按钮/文字样式
│       └── Widgets/             # 界面蓝图（Options/ 含 DT_OptionsTabs）
└── Source/FrontendUI/
    ├── Private/
    │   ├── AsyncActions/        # 蓝图异步节点
    │   ├── Subsystems/          # UI 子系统
    │   └── Widgets/             # 界面实现
    └── Public/
        ├── Data/                # DataTable 行结构（FFrontendOptionsTabRow）
        ├── FrontendSettings/    # DeveloperSettings
        └── Widgets/             # 界面与组件基类
```

> 仓库工作区约 426 MB，其中 `Content/Assets/` 占 425 MB（课程配套美术素材），`Content/UI/` 仅 0.9 MB 才是本项目实际产出。编译产物（`Binaries/`、`Intermediate/`、`Saved/`、`DerivedDataCache/`、`.vs/`）已由 `.gitignore` 排除。
>
> 注：`Content/FrontendTestMap.umap` 是编辑器早期生成的重名文件，实际使用的关卡为 `Content/Maps/FrontendTestMap.umap`。

---

## 后续计划

| 优先级 | 改进点 | 方案 |
|---|---|---|
| 高 | **实现设置项内容层** | `OnOptionsTabSelected` 目前只打印日志。下一步按 `UListDataObject_Base` 的组合树为每个分类构建具体设置项。分类注册层已数据驱动，内容层沿用同一思路收敛到数据表 |
| 中 | **统一失败处理策略** | 可恢复的配置校验降为 `ensureMsgf` + 兜底。全工程仍有 6 处硬断言：`FrontendUISubsystem.cpp` 4 处（含 `CastChecked<UWidget_ConfirmScreen>`）、`AsyncAction_PushSoftWidget.cpp` 1 处 `checkf`、`Widget_ConfirmScreen.cpp` 1 处 `check`。判断标准是「调用方传参错误」可降级、「类型契约被破坏」保留 —— 因此 `CastChecked` 那处**保持硬断言**，其余 5 处逐步降级 |
| 中 | **界面栈防重入** | 连续点击两次同一按钮会入栈两个界面。可在推入前按 Tag + 界面类判重，或对触发按钮做一次短时禁用 |
| 中 | **接入设置持久化** | 接 `UGameUserSettings` 或 SaveGame，让设置项改动在重启后保留 |
| 低 | **接入本地化** | UI 文案目前是硬编码英文 `FText`，迁到 String Table |
| 低 | **实现 Reset 操作** | `OnResetBoundActionTriggered` 目前只打印日志，补上恢复默认值的逻辑 |

**已完成**：

- `Widget_OptionsScreen.cpp` 中 3 处硬失败已降级 —— `BindWidget` 指针改用 `ensureMsgf` + `IsValid()` 保护、`NativeOnActivated` 入口判空后提前返回、`GetOrCreateDataRegistry()` 的 `checkf` 降为 `ensureMsgf` 并补上 `NewObject` 的 Outer
- `BuildTabCollectionsFromTable()` 补上行结构校验（`ensureMsgf` 报出 actual/expected 双方），并把判空移到 `reinterpret_cast` **之前**、改判 `RowPair.Value` —— 前者检查的是「这行有无数据」，而不是「转换是否失败」（`reinterpret_cast` 不会失败）
- 修正 `FrontendTabListWidgetBase.cpp` 中 `ValidateCompiledDefaults` 的拼写错误（`varible` → `variable`）

### 演示范围

`StartMenu` 的「新游戏」演示的是确认弹窗的按钮回调链路，确认后不会加载游戏关卡 —— 这是刻意的演示范围，不是缺陷：`Content/Assets/LevelVisuals/` 提供的是角色与场景素材，本身不含可游玩关卡，接入游戏流程需要另做关卡与 GameMode，超出本项目的 UI 框架范围。
