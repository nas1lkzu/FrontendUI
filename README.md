# FrontendUI — UE5 游戏前端 UI 框架

基于 **Unreal Engine 5.8 + CommonUI** 实现的一套游戏前端 UI 框架，采用 Lyra 式的 UI 分层架构。

> **项目性质**：这是一个**学习实践项目**，架构思路参考外网英文教程（Udemy: *Unreal Engine 5 C++: Advanced Frontend UI Programming*）实现。
> 除下方「个人扩展」一节列出的内容外，其余功能均为跟随教程完成。这里如实说明，以便读者了解各部分来源。

---

## 技术栈

| 项目 | 内容 |
|---|---|
| 引擎 | Unreal Engine 5.8 |
| 语言 | C++（框架层） + 蓝图（表现层） |
| UI 框架 | CommonUI、CommonInput、UMG |
| 输入 | Enhanced Input |
| 其他 | GameplayTags、AssetManager（异步加载）、DeveloperSettings |

依赖插件：`CommonUI`（见 `FrontendUI.uproject`）。`CommonInput` 为 CommonUI 的配套模块，通过 `CommonInputPlatformSettings` 配置

---

## 功能概览

### 1. GameplayTag 驱动的四层 WidgetStack

界面按语义分入四个分层界面栈，界面之间以 Tag 寻址，**互不产生硬引用**：

| 层栈 | 用途 |
|---|---|
| `Frontend.WidgetStack.Frontend` | 脱离游戏的前端流程（按任意键 → 主菜单 → 设置） |
| `Frontend.WidgetStack.GameMenu` | 游戏内叠加菜单（暂停菜单） |
| `Frontend.WidgetStack.GameHud` | 常驻 HUD（不抢焦点） |
| `Frontend.WidgetStack.Modal` | 最上层模态（确认弹窗，必须响应才能继续） |

**关键实现**
- `UWidget_PrimaryLayout`：主界面通过蓝图节点 `RegisterWidgetStack(Tag, Stack)` 把 `CommonActivatableWidgetStack` 以 Tag 为键注册进 `TMap`
- `UFrontendUISubsystem`：系统层统一用 `FindWidgetStackByTag()` 取栈，**不持有任何具体容器指针**

### 2. 配置驱动的界面资源异步加载

- `UFrontendDeveloperSettings`（继承 `UDeveloperSettings`）在 **Project Settings** 中维护 `TMap<FGameplayTag, TSoftClassPtr<UWidget_ActivatableBase>>` 映射表，数据落在 `Config/DefaultGame.ini`
- 界面资源路径**零硬编码**，运行时通过 `UAssetManager` 的 `StreamableManager` **按需异步加载**
- 蓝图侧通过 `UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag()` 取用，并用 `ensureMsgf` 做运行时校验

### 3. 自定义蓝图异步节点（双时机委托）

`UAsyncAction_PushSoftWidget` 继承 `UBlueprintAsyncActionBase`，把整条链路封装成一个蓝图节点：

```
加载软引用 → 实例化 → 入栈 → 设置 OwningPlayer → 焦点设置
```

**双时机设计**（本项目的核心设计点之一）：

| 时机 | 触发点 | 用途 |
|---|---|---|
| `OnCreatedBeforePush` | Widget 已创建、**尚未入栈** | 注入初始化数据（如弹窗标题与按钮配置）、`SetOwningPlayer` |
| `AfterPush` | Widget 已入栈 | 设置焦点、收尾 |

> **为什么分两个时机**：若等入栈后再注入数据，玩家会看到「先空一帧、再刷出内容」的闪烁；而焦点设置需要 Widget 已进入可视树才可靠。

### 4. 数据驱动的设置界面

- `UOptionsDataRegistry`：注册中心，统一管理设置分类
- `UListDataObject_Base` / `UListDataObject_Collection`：以**组合模式**描述设置项的层级结构（分类 → 分组 → 具体设置项）
- `UWidget_OptionsScreen` 激活时**动态注册 Tab 并做幂等去重**（`GetTabButtonBaseByID()` 判空跳过）

### 5. 通用确认弹窗组件

- `UConfirmScreenInfoObject` 以**静态工厂**产出三种按钮组合：`CreateOkScreen` / `CreateYesNoScreen` / `CreateOkCancelScreen`
- 弹窗用 `UDynamicEntryBox` **动态生成按钮**，重建前 `Reset()` 并清理旧 `OnClicked()` 委托，避免委托堆积
- 按按钮语义自动绑定 CommonUI 输入动作与焦点

### 6. 手柄 / 键鼠双输入适配

- `CommonInputPlatformSettings` 注册两套 `ControllerData`（手柄 / 键鼠）
- 按键映射收敛到 `DT_CommonInputKeyMapping` 数据表（含 `BackAction` / `ConfirmAction` / `ResetAction`）
- 底部操作提示用 `CommonBoundActionBar` + 自定义 `WBP_Button_BoundAction`，**随输入设备自动切换文案与图标**

---

## 个人扩展

> 以下内容为**在教程之外自主完成**的部分。

### 将设置分类由 C++ 硬注册重构为 DataTable 驱动

**改造前**：4 个设置分类（Gameplay / Audio / Video / Control）以 4 个 C++ 函数硬编码创建，新增分类需要修改代码并重新编译。

**改造后**：

- 新增行结构体 `FFrontendOptionsTabRow`（继承 `FTableRowBase`），字段为 `DisplayName` + `SortOrder`
- 分类数据收敛到数据表 `DT_OptionsTabs`，表资产在 Project Settings 中配置
- `UOptionsDataRegistry` 改为读表构建分类

**两个关键设计判断**

| 判断 | 理由 |
|---|---|
| **用 RowName 作为分类 ID**，不额外加 ID 字段 | DataTable 行名天然唯一，避免冗余字段与「行名和 ID 不一致」的隐患 |
| **必须按 `SortOrder` 显式排序** | DataTable 底层是 `TMap`，**遍历顺序不保证稳定**；不排序会导致每次运行的 Tab 顺序可能不同 |

**验证结果**：在表中新增一行 `GraphicsTabCollection` 后重新运行，设置界面自动多出该分类，**全程未修改任何 C++ 代码、未重新编译**。

**实现细节：为什么用 `GetRowMap()` + `reinterpret_cast`**

构表时不调用 `FindRow()` 逐行查找，而是直接遍历 `GetRowMap()` 并按已知行结构解读 `uint8*`。原因是 `FindRow()` 每次都要做一次 `FName → 行` 的哈希查找，而此处已持有 `GetRowMap()`，逐行 `FindRow()` 等于把同一次遍历做了两遍。

> **代价**：放弃了行结构的类型校验。前提是 `DT_OptionsTabs` 的 Row Structure 必须为 `FFrontendOptionsTabRow`，否则数据会错位。目前依靠在编辑器中确认该配置。

### 健壮性修复

在持续迭代中发现并修复了两类缺陷。

#### 1. 排序比较函数违反严格弱序（自研逻辑缺陷）

`BuildTabCollectionsFromTable()` 最初的实现是「先取全部 RowName，再按 `SortOrder` 排序」，比较函数如下：

```cpp
RowNames.Sort([InTable](const FName& A, const FName& B)
{
    const FFrontendOptionsTabRow* RowA = InTable->FindRow<FFrontendOptionsTabRow>(A, TEXT("OptionsTabSort"), false);
    const FFrontendOptionsTabRow* RowB = InTable->FindRow<FFrontendOptionsTabRow>(B, TEXT("OptionsTabSort"), false);
    if (!RowA || !RowB) { return false; }
    return RowA->SortOrder < RowB->SortOrder;
});
```

存在两个问题：

| 问题 | 后果 |
|---|---|
| 比较函数内部调用 `FindRow()` | 每次比较最多 2 次哈希查找 + 字符串比较，n 个分类共 O(n log n) 次查表 |
| 任一行数据缺失时**两个方向都返回 `false`** | `Comp(A,B)` 与 `Comp(B,A)` 可能同时为假，**违反 strict weak ordering**，`TArray::Sort` 的行为未定义 |

**修复方式**：改为「单次遍历收集排序键 → 对纯值排序」。遍历时一次性取出 `RowName` / `DisplayName` / `SortOrder` 存入临时结构体，比较函数只比较 `int32`，成为满足严格弱序的纯函数；`DisplayName` 顺带取出，消除了原先排序后再 `FindRow()` 取显示名的重复查表。

**效果**：查表次数由 O(n log n) 降至 O(n)，且排序结果不再依赖未定义行为。

#### 2. `BindWidget` 指针缺少空指针保护

`UWidget_OptionsScreen` 中 `TabListWidget_OptionsTabs` 为 `meta = (BindWidget)` 绑定，蓝图漏绑时该指针为空。原实现在两个入口处直接解引用：

| 入口 | 风险 |
|---|---|
| `NativeOnInitialized()` | 直接调用 `OnTabSelected.AddUniqueDynamic(...)`，漏绑即崩溃 |
| `NativeOnActivated()` | 直接遍历 `GetRegisteredOptionsTabCollections()` 并调用 `RequestRegisterTab()` |

**修复方式**（与项目内其他失败处理保持一致）：

- `NativeOnInitialized()`：用 `ensureMsgf` 报一次配置错误（编辑器下即时报出，Shipping 下不致命），仅在 `IsValid()` 通过后才注册回调
- `NativeOnActivated()`：入口处判空并输出 `UE_LOG` 错误日志后提前返回
- `GetOrCreateDataRegistry()`：`checkf` 降级为 `ensureMsgf`；`NewObject` 补上 `this` 作为 Outer，把注册中心的生命周期挂到 Widget 上

#### 附：问题定位方式

两处缺陷均通过**逐文件对比课程参考实现**定位：将本工程 `Source/` 与课程仓库 `vinceright3/FrontendUIProgramming` 的同名文件逐个 diff，区分「跟随教程的部分」与「自己改动的部分」，再对自研代码单独 review。

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
                       → 确认弹窗（由「新游戏」触发，用于演示按钮回调链路）
```

> 鼠标与手柄均可操作；切换输入设备时底部提示图标会自动变化。
>
> ⚠️ **注意**：`StartMenu` 的「新游戏」目前只演示确认弹窗的**按钮回调链路**，确认后**不会真正加载游戏关卡**（详见「已知不足」）。

---

## 关于仓库体积

本仓库**工作区文件总计约 426 MB**（GitHub 上显示为约 338 MB 的压缩后大小），其中绝大部分是**课程配套的美术素材**：

| 目录 | 体积 | 说明 |
|---|---|---|
| `Content/Assets/` | **425 MB** | 美术素材（课程素材包 + Epic 官方素材） |
| `Content/Assets/LevelVisuals/Characters/` | 387 MB | 角色模型与贴图（Manny / Quinn） |
| `Content/UI/` | 0.9 MB | **本项目实际产出**：UI 蓝图、样式、输入配置 |

**这些素材与本项目的 UI 实现无关**，保留它们是为了让 `FrontendTestMap` 的场景完整可运行。

UI 相关的全部代码与资产位于：

```
Source/                    全部 C++ 代码
Content/UI/                UI 蓝图、CommonUI 样式、输入配置
Content/Maps/              测试关卡
Config/                    项目配置（含 Widget 映射表、输入设置）
```

编译产物（`Binaries/`、`Intermediate/`、`Saved/`、`DerivedDataCache/`、`.vs/`）均已通过 `.gitignore` 排除 —— 这几项在本机原本合计约 **9 GB**。

---

## 已知不足

- **设置项尚未打通**：目前完成的是框架与分类动态注册，`UWidget_OptionsScreen::OnOptionsTabSelected` 中仅打印日志，各分类下的具体设置项（分辨率、音量等）待实现
- **`OptionsDataRegistry` 自身尚未做行结构校验**：构表时用 `GetRowMap()` + `reinterpret_cast` 换取查找性能，代价是放弃了类型检查；更稳妥的做法是在 `LoadOptionsTabTable()` 中比对 `GetRowStruct()` 与 `FFrontendOptionsTabRow::StaticStruct()`，不一致时报警并放弃构表 —— **此项待补**
- **`FrontendUISubsystem` 中仍有若干 `check` / `CastChecked`**：异步推送界面链路上还有 4 处硬断言未降级（`FrontendUISubsystem.cpp`），失败处理策略尚未全项目统一
- **Reset 操作未实现**：`OnResetBoundActionTriggered` 目前仅打印日志，尚未接入真实重置逻辑
- **无本地化**：UI 文案目前为硬编码英文 `FText`，正式项目应走 String Table
- **无设置持久化**：尚未接入 `UGameUserSettings` 或 SaveGame
- **界面栈无防重入**：连续点击两次同一按钮会入栈两个界面
- **「新游戏」未接入游戏流程**：确认弹窗确认后不会加载游戏关卡（`Content/Assets/LevelVisuals/` 的素材本身不含可游玩关卡）

---

## 目录结构

```
FrontendUI/
├── Config/
│   ├── DefaultEngine.ini
│   ├── DefaultGame.ini          # Widget 映射表、Options Tab 表、CommonInput 配置
│   └── DefaultInput.ini
├── Content/
│   ├── Assets/                  # 美术素材（课程配套，约 425MB）
│   ├── Maps/                    # FrontendTestMap（实际使用的关卡）
│   └── UI/
│       ├── CommonInputData/     # 输入数据表与 ControllerData
│       ├── Styles/              # CommonUI 按钮/文字样式
│       └── Widgets/             # 界面蓝图
│           └── Options/         # 设置界面 + DT_OptionsTabs
└── Source/FrontendUI/
    ├── Private/
    │   ├── AsyncActions/        # 蓝图异步节点
    │   ├── Controllers/         # PlayerController
    │   ├── Subsystems/          # UI 子系统
    │   └── Widgets/             # 界面实现
    └── Public/
        ├── Data/                # DataTable 行结构（FFrontendOptionsTabRow）
        ├── FrontendSettings/    # DeveloperSettings
        ├── FrontendTypes/       # 枚举等类型
        └── Widgets/             # 界面与组件基类
```

> 注：`Content/FrontendTestMap.umap` 是编辑器早期生成的重名文件，实际使用的关卡为 `Content/Maps/FrontendTestMap.umap`。
