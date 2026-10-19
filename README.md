# FrontendUI

基于 **Unreal Engine 5.8** 与 **CommonUI** 的游戏前端 UI 框架。框架层由 C++ 实现，界面表现交给蓝图：
前端界面按 GameplayTag 归入不同的界面栈，界面类以软引用传递、按需异步加载，设置界面的分类由数据表驱动。

**技术栈**：UE 5.8 · C++ · UMG / CommonUI / CommonInput · Enhanced Input · GameplayTags · DeveloperSettings · DataTable

## 特性

- **Tag 寻址的界面栈**：四个语义分层的 `CommonActivatableWidgetStack`（Frontend / Modal / GameMenu / GameHud），界面以 GameplayTag 入栈，界面之间不产生硬引用。
- **按需异步加载**：界面类以 `TSoftClassPtr` 传递，通过 `StreamableManager` 异步加载，只加载真正要显示的界面。
- **两个蓝图异步节点**：`Push Soft Widget To Widget Stack` 提供「实例创建后 / 入栈后」两段式输出引脚；`Show Confirmation Screen` 把确认弹窗做成可等待的蓝图节点。
- **通用确认弹窗**：Ok / YesNo / OkCancel 三种形态，按数据对象动态生成按钮，点击后回传按钮类型。
- **数据驱动的设置分类**：设置分类由 `DT_OptionsTabs` 配置，新增分类只需加一行数据，不需要修改 C++（当前实现到分类层）。
- **键鼠与手柄双输入**：CommonInput 提供两套输入数据，底部操作提示随当前输入设备切换文案与图标。

## 环境要求

- Unreal Engine **5.8**
- Visual Studio 2022 或 Rider（Windows）


## 操作流程

```
启动 → 按任意键 → 主菜单（START / OPTIONS / QUIT）
                   ├─ START   → 开始菜单（NEW GAME / CONTINUE）
                   │            └─ NEW GAME → 确认弹窗
                   ├─ OPTIONS → 设置界面（分类 Tab，可键鼠/手柄切换，返回键退出）
                   └─ QUIT    → 确认弹窗 → 退出游戏
```

鼠标与手柄均可操作；切换输入设备时，底部提示条的文案与图标会同步变化。
NEW GAME 确认后不加载游戏关卡——本项目的范围是前端 UI 框架本身。
关卡内有一台 Tag 为 `Default` 的 `CineCameraActor`，`AFrontendPlayerController::OnPossess` 会将其设为 ViewTarget。

## 项目结构

```
FrontendUI/
├─ Config/
│  ├─ DefaultEngine.ini          # 默认关卡、CommonGameViewportClient、焦点与 FontDPI 设置
│  ├─ DefaultGame.ini            # 界面映射表、设置数据表、CommonInput 双设备配置
│  └─ DefaultInput.ini           # Enhanced Input 相关配置
├─ Content/
│  ├─ UI/                        # 前端界面：UMG 蓝图、样式、输入数据、设置分类表
│  ├─ Maps/FrontendTestMap.umap  # 测试关卡
│  └─ Assets/                    # 素材（字体、图标、场景与角色）
└─ Source/FrontendUI/
   ├─ Private/
   │  ├─ Subsystems/             # UFrontendUISubsystem：异步加载调度、弹窗入栈
   │  ├─ AsyncActions/           # 两个蓝图异步节点
   │  ├─ Widgets/                # PrimaryLayout、ActivatableBase、ConfirmScreen、OptionsScreen
   │  │  ├─ Components/          # 按钮与 Tab 列表基类
   │  │  └─ Options/DataObjects/ # 设置项数据对象
   │  ├─ Controllers/            # AFrontendPlayerController
   │  └─ FrontendSettings/       # UFrontendDeveloperSettings
   └─ Public/                    # 对应头文件；Data/ 下为设置分类行结构
```

## 核心设计

### 界面栈：Tag 寻址

`UWidget_PrimaryLayout` 维护一张 `Tag → CommonActivatableWidgetStack` 的映射表，蓝图通过
`RegisterWidgetStack` 把界面栈注册进去，框架层只按 Tag 查找，不持有具体容器指针。
使用方只依赖 `Push Soft Widget To Widget Stack` 这一个入口，因此新增界面栈不需要改动调用方。

### 异步加载

界面类通过 `UFrontendDeveloperSettings` 中的「Tag → 软类」映射配置（落在 `DefaultGame.ini`，可在项目设置中编辑）。
推送界面时先发起异步加载，加载完成后再创建实例并入栈。异步节点提供两个输出引脚：

- `OnWidgetCreatedBeforePush`：实例已创建、尚未入栈，用于 `SetOwningPlayer` 与注入初始化数据；
- `AfterPush`：已入栈，用于设置焦点等依赖可视树的逻辑。

这样可以在界面显示前完成数据填充，避免出现「先弹空壳、再刷新内容」的闪烁。

### 通用确认弹窗

`UConfirmScreenInfoObject` 用数据描述一个弹窗（标题、正文、按钮列表），`PushConfirmScreenToModalStackAsync`
负责将其推入 Modal 界面栈。所有弹窗共用同一个界面蓝图，按钮由 `UDynamicEntryBox` 按数据生成，
业务侧只需处理按钮点击回传的枚举结果。

### 数据驱动的设置界面

设置分类定义在 `DT_OptionsTabs` 中，行结构为 `FFrontendOptionsTabRow`（显示名 + 排序序号）。
`UOptionsDataRegistry` 读取该表并构建分类对象，`UWidget_OptionsScreen` 在界面激活时按分类动态注册 Tab。
新增一个分类只需要在表里追加一行；构表过程会校验行结构是否匹配、排序序号是否唯一，配置有误时以断言提示，而不是静默失败。

### 输入适配

- C++ 侧不硬编码按键行名，返回键通过 `ICommonInputModule::GetSettings().GetDefaultBackAction()` 取得；
- 按键映射集中在 `DT_CommonInputKeyMapping`（返回、确认、上一个 / 下一个 Tab、重置）；
- 底部操作提示由 `CommonBoundActionBar` 与自定义按钮项绘制，随输入设备自动切换。

## 后续计划

### 界面框架

- **推送幂等化**：把 `Push(软类)` 改为按界面 Tag 的 `ShowScreen(Tag, Stack)`。语义从「往栈里加一个」变成「显示这个界面」：正在加载或已在栈内时折叠重复请求，重复点击不会再产生第二份实例。

### 设置系统

- **补完设置项内容层**：按「分类 → 分组 → 设置项」构建数据树（`UListDataObject_Base` 的父子结构已预留），点击 Tab 时切换到对应的设置列表。
- **接入具体设置项**：音量、分辨率、画质等级、按键重绑定等，改动即时生效。
- **持久化与重置**：通过 `UGameUserSettings` 或 SaveGame 保存，重启后恢复；Reset 动作恢复默认值。

### 游戏内容

- **接入 Gameplay 框架**：补齐 GameMode / PlayerState / Character 与关卡流程，让 NEW GAME 真正进入游戏。
- **复用已预留的界面栈**：`GameHud` 栈显示生命值与状态，`GameMenu` 栈做暂停菜单，同时验证这套框架在游戏内流程下同样适用。
- **按游戏状态切换输入**：前端、游戏内、菜单三种状态各用一套 `InputMappingContext`，避免输入互相串扰。
