# chinese.tcl:
# Chinese language support for Scid vs PC.
# Translated by Zhuge Prajna
# Released on Sep 25, 2025
# Updated on Sep 25, 2025
#

addLanguage Z Chinese 0 utf8

proc setLanguage_Z {} {

# File menu:
menuText Z File "文件(F)" 3
menuText Z FileNew "新建(N)" 3 {创建一个新的 Scid 数据库}
menuText Z FileOpen "打开(O)" 3 {打开一个已有的 Scid 数据库}
menuText Z FileClose "关闭(C)" 3 {关闭当前活动的 Scid 数据库}
menuText Z FileFinder "文件查找器(F)" 6 {打开文件查找器窗口}
menuText Z FileSavePgn "保存为 PGN(S)" 8 {将当前对局保存到文件}
menuText Z FileOpenBaseAsTree "以树状图打开数据库(T)" 10   {打开一个数据库并在树状图窗口中使用}
menuText Z FileOpenRecentBaseAsTree "以树状图打开最近数据库(O)" 12   {打开最近使用的数据库并在树状图窗口中使用}
menuText Z FileBookmarks "书签(B)" 3 {书签菜单}
menuText Z FileBookmarksAdd "添加书签(A)" 5 \
  {为当前数据库中的对局和局面添加书签}
menuText Z FileBookmarksFile "归档书签(F)" 5 \
  {为当前对局和局面创建归档书签}
menuText Z FileBookmarksEdit "编辑书签(E)" 5 \
  {编辑书签菜单}
menuText Z FileBookmarksList "文件夹显示为单列表(D)" 10 \
  {将书签文件夹显示为单一列表，而非子菜单}
menuText Z FileBookmarksSub "文件夹显示为子菜单(D)" 10 \
  {将书签文件夹显示为子菜单，而非单一列表}

menuText Z FileReadOnly "只读模式(R)" 5 \
  {将当前数据库设为只读，防止任何修改}
menuText Z FileSwitch "切换数据库(W)" 6 \
  {切换到另一个已打开的数据库}
menuText Z FileExit "退出(X)" 3 {退出 Scid}

# Edit menu:
menuText Z Edit "编辑(E)" 3
menuText Z EditAdd "添加变着(A)" 5 {在此着法处为对局添加一个变着}
menuText Z EditPasteVar "粘贴变着(P)" 5
menuText Z EditDelete "删除变着(D)" 5 {删除此着法的一个变着}
menuText Z EditDeleteComment "删除注释(D)" 5
menuText Z EditFirst "设为首个变着(F)" 7 \
  {将某个变着提升为列表中的第一个}
menuText Z EditMain "提升为主变(M)" 6 \
  {将某个变着提升为主线}
menuText Z EditTrial "试走变着(T)" 5 \
  {启动/停止试走模式，用于在棋盘上测试招法}
menuText Z EditStrip "清除(S)" 3 {从此对局中清除注释或变着}
menuText Z EditUndo "撤销(U)" 3 {撤销上一次对局修改}
menuText Z EditRedo "重做(R)" 3 {重做上一次对局修改}
menuText Z EditStripComments "注释(C)" 3 \
  {从此对局中清除所有注释和评注}
menuText Z EditStripVars "变着(V)" 3 {从此对局中清除所有变着}
menuText Z EditStripBegin "从开局清除着法(S)" 8 \
  {从对局开头清除着法}
menuText Z EditStripEnd "清除至终局着法(E)" 8 \
  {清除从当前位置到对局结束的着法}
menuText Z EditReset "清空剪辑库(E)" 6 \
  {将剪辑库重置为空}
menuText Z EditCopy "复制到剪辑库(O)" 7 \
  {将此对局复制到剪辑库数据库}
menuText Z EditPaste "从剪辑库粘贴(A)" 7 \
  {将当前剪辑库中的对局粘贴到此处}
menuText Z EditPastePGN "粘贴 PGN(P)" 7 \
  {将剪贴板文本解析为 PGN 并粘贴到此处}
menuText Z EditSetup "设置棋盘(B)" 5 \
  {为此对局设置起始局面}
menuText Z EditCopyBoard "复制 FEN(F)" 7 \
  {将当前棋盘以 FEN 格式复制到文本选择区（剪贴板）}
menuText Z EditCopyPGN "复制 PGN(C)" 7 \
  {将当前 PGN 复制到文本选择区（剪贴板）}
menuText Z EditPasteBoard "粘贴 FEN(F)" 7 \
  {从当前文本选择区（剪贴板）设置起始棋盘}

# Game menu:
menuText Z Game "对局(G)" 3
menuText Z GameNew "新建对局(N)" 5 {重置为一个空对局，丢弃所有更改}
menuText Z GameFirst "加载首局(F)" 5 {加载筛选结果中的第一局}
menuText Z GamePrev "加载上一局(P)" 6 {加载筛选结果中的上一局}
menuText Z GameReload "重新加载对局(L)" 7 {重新加载当前对局，丢弃所有修改}
menuText Z GameNext "加载下一局(X)" 6 {加载筛选结果中的下一局}
menuText Z GameLast "加载末局(T)" 5 {加载筛选结果中的最后一局}
menuText Z GameRandom "随机加载对局(D)" 7 {随机加载一局筛选结果中的对局}
menuText Z GameNumber "按编号加载对局(G)" 8 {通过输入编号加载对局}
menuText Z GameReplace "替换对局(R)" 5 {保存此对局，替换旧版本}
menuText Z GameAdd "添加对局(A)" 5 {将此对局作为新对局保存到数据库}
menuText Z GameInfo "设置对局信息(I)" 7
menuText Z GameBrowse "浏览对局(B)" 5
menuText Z GameList "列出所有对局(L)" 7
menuText Z GameDelete "删除对局(D)" 5
menuText Z GameDeepest "识别开局(I)" 5 {跳转到 ECO 开局书中记录的最深局面}
menuText Z GameGotoMove "跳转到指定着数(M)" 8 {跳转到当前对局中指定的着数}
menuText Z GameNovelty "查找新着(V)" 5 {查找此对局中首次出现的招法（新着）}

# Search Menu:
menuText Z Search "搜索(S)" 3
menuText Z SearchReset "重置筛选器(R)" 6 {重置筛选器，包含所有对局}
menuText Z SearchNegate "反转筛选器(N)" 6 {反转筛选器，仅包含被排除的对局}
menuText Z SearchEnd "筛选至终局(M)" 6 {所有筛选出的对局加载到终局局面}
menuText Z SearchCurrent "当前棋盘(C)" 5 {搜索当前棋盘局面}
menuText Z SearchHeader "常规(G)" 3 {按标题信息（棋手、赛事等）搜索}
menuText Z SearchMaterial "子力/局面模式(M)" 8 {搜索子力配置或棋盘模式}
menuText Z SearchMoves "着法(M)" 3 {}
menuText Z SearchUsing "加载搜索文件(L)" 7 {使用 SearchOptions 文件进行搜索}

# Windows menu:
menuText Z Windows "窗口(W)" 3
menuText Z WindowsGameinfo "对局信息(G)" 5 {显示/隐藏对局信息面板}
menuText Z WindowsComment "注释编辑器(D)" 6 {打开/关闭注释编辑器}
menuText Z WindowsGList "对局列表(G)" 5 {打开/关闭对局列表窗口}
menuText Z WindowsPGN "PGN 窗口(P)" 7 {打开/关闭 PGN（对局记谱）窗口}
menuText Z WindowsCross "交叉积分表(C)" 6 {打开/关闭赛事交叉积分表}
menuText Z WindowsPList "棋手查找器(A)" 6 {打开/关闭棋手查找器}
menuText Z WindowsTmt "赛事(U)" 3 {打开/关闭赛事查找器}
menuText Z WindowsSwitcher "数据库(D)" 4  {打开/关闭数据库切换器窗口}
menuText Z WindowsMaint "维护窗口(M)" 5 {打开/关闭维护窗口}
menuText Z WindowsECO "ECO 浏览器(E)" 8 {打开/关闭 ECO 浏览器窗口}
menuText Z WindowsStats "统计窗口(S)" 5 {打开/关闭筛选结果统计窗口}
menuText Z WindowsTree "树状图窗口(T)" 6 {打开/关闭树状图窗口}
menuText Z WindowsTB "残局库窗口(A)" 6 {打开/关闭残局库窗口}
menuText Z WindowsBook "开局库窗口(B)" 6 {打开/关闭开局库窗口}
menuText Z WindowsCorrChess "通讯赛窗口(O)" 6 {打开/关闭通讯赛窗口}

# Tools menu:
menuText Z Tools "工具(T)" 3
menuText Z ToolsAnalysis "分析引擎(A)" 5 {配置分析引擎}
menuText Z ToolsEmail "邮件管理器(E)" 6 {打开/关闭邮件对弈管理器窗口}
menuText Z ToolsFilterGraph "相对筛选图表(G)" 7 {打开/关闭相对数值的筛选图表窗口}
menuText Z ToolsAbsFilterGraph "绝对筛选图表(L)" 7 {打开/关闭绝对数值的筛选图表窗口}
menuText Z ToolsOpReport "开局报告(O)" 5 {为当前局面生成开局报告}
menuText Z ToolsTracker "棋子追踪器(T)"  6 {打开棋子追踪器窗口}
menuText Z ToolsTraining "训练(G)"  3 {训练工具（战术、开局等）}
menuText Z ToolsComp "电脑锦标赛(C)" 6 {国际象棋引擎锦标赛}
menuText Z ToolsTacticalGame "电脑对弈 - Phalanx(C)"  15 {进行带战术的对局}
menuText Z ToolsSeriousGame "电脑对弈 - UCI 引擎(U)"  14 {与 UCI 引擎对弈}
menuText Z ToolsTrainTactics "杀棋谜题（N 步杀）(M)"  11 {解答“N 步杀”谜题}
# ====== TODO To be translated ======
menuText Z ToolsTrainOpening "Opening Trainer" 0 {Rep training}
menuText Z ToolsTrainCalvar "变着计算训练(C)"  7 {Stoyko 训练}
menuText Z ToolsTrainFindBestMove "寻找最佳着法(F)"  7 {寻找最佳着法}
menuText Z ToolsTrainFics "互联网对弈（FICS）(I)"  12 {在 freechess.org 上对弈}
menuText Z ToolsBookTuning "开局库调优(B)" 6 {开局库调优}

menuText Z ToolsMaint "维护(M)" 3 {Scid 数据库维护工具}
menuText Z ToolsMaintWin "维护窗口(M)" 5 {打开/关闭 Scid 数据库维护窗口}
menuText Z ToolsMaintExtra "附加标签(E)" 5 {附加标签}
menuText Z ToolsMaintCompact "压缩数据库(C)" 6 {压缩数据库文件，移除已删除对局和未使用的名称}
menuText Z ToolsMaintClass "ECO 分类对局(O)" 9 {重新计算所有对局的 ECO 编码}
menuText Z ToolsMaintSort "排序数据库(S)" 6 {对数据库中所有对局进行排序}
menuText Z ToolsMaintDelete "删除重复对局(D)" 7 {查找重复对局并标记为删除}
menuText Z ToolsMaintTwin "重复对局检查器(T)" 8 {打开/更新重复对局检查器窗口}
menuText Z ToolsMaintNameEditor "名称编辑器(N)" 6 {打开/关闭名称编辑器窗口}
menuText Z ToolsMaintNamePlayer "校正棋手姓名(P)" 7 {使用拼写检查文件校正棋手姓名}
menuText Z ToolsMaintNameEvent "校正赛事名称(E)" 7 {使用拼写检查文件校正赛事名称}
menuText Z ToolsMaintNameSite "校正地点名称(S)" 7 {使用拼写检查文件校正地点名称}
menuText Z ToolsMaintNameRound "校正轮次名称(R)" 7 {使用拼写检查文件校正轮次名称}
menuText Z ToolsMaintFixBase "修复数据库(R)" 6 {尝试修复损坏的数据库}

menuText Z ToolsConnectHardware "电子棋盘/外部硬件(E)" 10 {连接外部硬件}
menuText Z ToolsConnectHardwareConfigure "配置(C)" 3 {配置外部硬件及连接}
menuText Z ToolsConnectHardwareNovagCitrineConnect "连接 Novag Citrine(O)" 17 {将 Novag Citrine 与 Scid 连接}
menuText Z ToolsConnectHardwareInputEngineConnect "连接输入引擎(N)" 7 {将输入引擎（如 DGT 棋盘）与 Scid 连接}

menuText Z ToolsPInfo "棋手信息(P)"  5 \
  {打开/更新棋手信息窗口}
menuText Z ToolsPlayerReport "棋手报告(Y)" 5 \
  {生成棋手报告}
menuText Z ToolsRating "棋手等级分(P)" 6 \
  {绘制当前对局棋手的等级分历史图表}
menuText Z ToolsScore "评分图表(S)" 5 {显示评分图表窗口}
menuText Z ToolsExpCurrent "导出当前对局(U)" 7 \
  {将当前对局写入文本文件}
menuText Z ToolsExpCurrentPGN "导出对局为 PGN(P)" 10 \
  {将当前对局写入 PGN 文件}
menuText Z ToolsExpCurrentHTML "导出对局为 HTML(H)" 11 \
  {将当前对局写入 HTML 文件}
menuText Z ToolsExpCurrentHTMLJS "导出对局为 HTML 与 JavaScript(H)" 24 {将当前对局写入 HTML 与 JavaScript 文件}  
menuText Z ToolsExpCurrentLaTeX "导出对局为 LaTeX(L)" 12 {使用 xSkak 将当前对局写入 LaTeX 文件}
menuText Z ToolsExpFilter "导出所有筛选对局(X)" 9 \
  {将所有筛选出的对局写入文本文件}
menuText Z ToolsExpFilterPGN "导出筛选结果为 PGN(P)" 12 \
  {将所有筛选出的对局写入 PGN 文件}
menuText Z ToolsExpFilterHTML "导出筛选结果为 HTML(H)" 13 \
  {将所有筛选出的对局写入 HTML 文件}
menuText Z ToolsExpFilterHTMLJS "导出筛选结果为 HTML 与 JavaScript(H)" 26 {将所有筛选出的对局写入 HTML 与 JavaScript 文件}  
menuText Z ToolsExpFilterLaTeX "导出筛选结果为 LaTeX(L)" 14  {将所有筛选出的对局写入 LaTeX 文件}
menuText Z ToolsExpFilterEPD "导出筛选结果为 EPD/FEN(E)" 16 {将所有筛选出的对局写入 EPD 文件}
menuText Z ToolsExpFilterGames "导出对局列表为文本(T)" 10 {打印格式化的对局列表。}
menuText Z ToolsImportOne "导入 PGN 文本(I)" 10 \
  {从 PGN 文本导入一局对局}
menuText Z ToolsImportFile "导入 PGN 文件(P)" 10 {从 PGN 文件导入对局}
menuText Z ToolsStartEngine1 "启动引擎 1(S)" 7  {启动引擎 1}
menuText Z ToolsStartEngine2 "启动引擎 2(S)" 7  {启动引擎 2}
menuText Z ToolsScreenshot "棋盘截图(B)" 5  {截取棋盘图像}

# Play menue
menuText Z Play "对弈(P)" 3 {对弈}

# --- Correspondence Chess
menuText Z CorrespondenceChess "通讯赛(C)" 4 {用于电子邮件和基于 Xfcc 的通讯赛功能}
menuText Z CCConfigure "配置(C)" 3 {配置外部工具和通用设置}
menuText Z CCConfigRelay "观赛(M)" 3 {配置要观看的对局}
menuText Z CCOpenDB "打开数据库(O)" 6 {打开默认的通讯赛数据库}
menuText Z CCRetrieve "获取对局(R)" 5 {通过外部（Xfcc）辅助程序获取对局}
menuText Z CCInbox "处理收件箱(I)" 6 {处理 Scid 收件箱中的所有文件}
menuText Z CCSend "发送着法(S)" 5 {通过电子邮件或外部（Xfcc）辅助程序发送你的着法}

menuText Z CCResign "认输(E)" 3 {认输}
menuText Z CCClaimDraw "提和（强制）(D)" 7 {发送着法并提和（非电子邮件方式）}
menuText Z CCOfferDraw "提和（提议）(F)" 7 {发送着法并提议和棋（非电子邮件方式）}
menuText Z CCAcceptDraw "接受和棋(A)" 5 {接受和棋提议（非电子邮件方式）}

menuText Z CCNewMailGame "新建邮件对局(W)" 7 {开始一局新的电子邮件对局}
menuText Z CCMailMove "邮件发送着法(M)" 7 {通过电子邮件将着法发送给对手}
menuText Z CCGamePage "对局页面(G)" 5 {通过网页浏览器打开对局页面}

menuText Z CCEditCopy "复制对局列表到剪贴板(C)" 11 {将对局以 CSV 列表形式复制到剪贴板}

# Options menu:
menuText Z Options "选项(O)" 3
menuText Z OptionsBoard "棋盘/棋子(C)" 6 {棋盘外观}
menuText Z OptionsColour "文字颜色(C)" 5 {默认文本组件颜色}
menuText Z OptionsBackColour "背景(B)" 3 {默认文本组件背景色}
menuText Z OptionsBackColourGlobal "全局(G)" 3 {全局背景颜色}
menuText Z OptionsMainLineColour "主线箭头(M)" 5 {主线箭头}
menuText Z OptionsVarLineColour "变着箭头(V)" 5 {变着箭头}
menuText Z OptionsEngineLineColour "引擎箭头(E)" 5 {引擎箭头}
menuText Z OptionsRowColour "行颜色(R)" 4 {默认树状图/开局库行颜色}
menuText Z OptionsHighlightColour "高亮颜色(H)" 5 {默认文本高亮颜色}
menuText Z OptionsSwitcherColour "切换器(S)" 4 {默认数据库切换器颜色}
menuText Z OptionsProgressColour "进度条(P)" 4 {默认进度条颜色}
menuText Z OptionsScoreColour "评分图表颜色(S)" 7 {}
menuText Z OptionsScoreBarColour "评分图表当前(S)" 7 {}
menuText Z OptionsNames "我的棋手名称(P)" 7 {编辑我的棋手名称}
menuText Z OptionsExport "导出设置(E)" 5 {更改文本导出选项}
menuText Z OptionsFonts "字体(F)" 3 {更改字体}
menuText Z OptionsFontsRegular "常规(R)" 3 {更改常规字体}
menuText Z OptionsFontsMenu "菜单(M)" 3 {更改菜单字体}
menuText Z OptionsFontsSmall "小号(S)" 3 {更改小号字体}
menuText Z OptionsFontsFixed "等宽(F)" 3 {更改等宽字体}
menuText Z OptionsEvalbar "评分条(S)" 4 {评分条选项}
menuText Z OptionsGInfo "对局信息(G)" 5 {对局信息选项}
menuText Z OptionsFics "FICS(F)" 5 {免费国际象棋互联网服务器}
menuText Z OptionsFicsAuto "自动升后(A)" 5
menuText Z OptionsFicsClk "保存时钟时间(S)" 7
menuText Z OptionsFicsColour "文字颜色(T)" 5
menuText Z OptionsFicsSize "棋盘大小(B)" 5
menuText Z OptionsFicsButtons "用户按钮(U)" 5
menuText Z OptionsFicsCommands "初始化命令(I)" 6
menuText Z OptionsFicsNoRes "不显示结果(N)" 6
menuText Z OptionsFicsNoReq "不显示请求(N)" 6
menuText Z OptionsFicsPremove "允许预走(A)" 5

menuText Z OptionsEvalMain "主棋盘(M)" 4
menuText Z OptionsEvalBrowser "对局浏览器(G)" 6
menuText Z OptionsEvalFics "FICS 棋盘(F)" 8
menuText Z OptionsEvalTime "分析时间(E)" 5 {快速分析探测时间}
menuText Z OptionsEvalScale "比例(S)" 3 {评分条比例}
menuText Z OptionsEvalTicks "显示刻度(S)" 5
menuText Z OptionsEvalLog  "对数评分(L)" 5

menuText Z OptionsLanguage "语言(L)" 3 {选择菜单语言}
menuText Z OptionsMovesTranslatePieces "翻译棋子符号(T)" 7 {翻译棋子首字母}
menuText Z OptionsMovesHighlightLastMove "高亮最后一步(H)" 7 {高亮最后一步}
menuText Z OptionsMovesHighlightLastMoveDisplay "显示(S)" 3 {显示最后一步高亮}
menuText Z OptionsMovesHighlightLastMoveWidth "宽度(W)" 3 {线条粗细}
menuText Z OptionsMovesHighlightLastMoveColor "选择颜色(S)" 5 {线条颜色}
menuText Z OptionsMovesColorSelected "高亮选中格(C)" 6 {着法颜色}
menuText Z OptionsMoves "着法输入(M)" 5 {着法输入选项}
menuText Z OptionsMovesAsk "替换着法前询问(A)" 8 \
  {覆盖已有着法前进行询问}
menuText Z OptionsMovesAnimate "动画时长(N)" 5 \
  {设置着法动画播放时长}
menuText Z OptionsMovesDelay "自动播放延迟(U)" 7 \
  {设置自动播放模式的时间延迟}
menuText Z OptionsMovesCoord "坐标式着法输入(C)" 8 \
  {接受坐标格式的着法输入（例如 "g1f3"）}
menuText Z OptionsMovesSuggest "显示推荐着法(S)" 7 {开启/关闭着法推荐}
menuText Z OptionsShowVarPopup "显示变着窗口(S)" 7 {开启/关闭变着窗口显示}  
menuText Z OptionsMovesSpace "着数后加空格(A)" 7 {在着数后添加空格}  
menuText Z OptionsMovesKey "键盘自动补全(K)" 7 \
  {开启/关闭键盘着法自动补全}
menuText Z OptionsMovesShowVarArrows "显示变着箭头(S)" 7 {开启/关闭显示变着箭头}
menuText Z OptionsNumbers "数字格式(N)" 5 {选择数字格式}
menuText Z OptionsStartup "启动(R)" 3 {选择启动时打开的窗口}
menuText Z OptionsTheme "主题(T)" 3 {Ttk 主题}
menuText Z OptionsWindows "窗口(W)" 3 {窗口选项}
menuText Z OptionsWindowsIconify "自动最小化(I)" 6 \
  {主窗口最小化时自动最小化所有窗口}
menuText Z OptionsWindowsRaise "自动置顶(R)" 5 \
  {当某些窗口（如进度条）被遮挡时自动置顶}
menuText Z OptionsSounds "声音(U)" 3 {配置着法播报声音}
menuText Z OptionsWindowsDock "停靠窗口(D)" 5 {停靠窗口（需重启生效）}
menuText Z OptionsWindowsSaveLayout "保存布局(S)" 5 {保存窗口布局}
menuText Z OptionsWindowsRestoreLayout "恢复布局(R)" 5 {恢复窗口布局}
menuText Z OptionsWindowsShowGameInfo "显示对局信息(S)" 8 {显示对局信息}
menuText Z OptionsWindowsAutoLoadLayout "启动时自动加载首个布局(A)" 12 {启动时自动加载首个布局}
menuText Z OptionsWindowsAutoResize "自动调整棋盘大小(A)" 9 {}
menuText Z OptionsWindowsFullScreen "全屏(F)" 3 {切换全屏模式}
menuText Z OptionsToolbar "工具栏(T)" 4 {配置主窗口工具栏}
menuText Z OptionsECO "加载 ECO 文件(O)" 10 {加载 ECO 分类文件}
menuText Z OptionsSpell "加载拼写检查文件(H)" 9 \
  {加载 Scid 拼写检查文件}
menuText Z OptionsTable "残局库目录(D)" 6 \
  {选择一个残局库文件；其所在目录下的所有残局库都将被使用}
menuText Z OptionsRecent "最近记录数量(R)" 7 {更改文件/对局历史菜单中显示的条目数量}
menuText Z OptionsBooksDir "开局库目录(B)" 6 {设置开局库目录}
menuText Z OptionsTacticsBasesDir "战术库目录(B)" 6 {设置战术（训练）数据库目录}
menuText Z OptionsInformant "配置 Informant 符号(C)" 16
menuText Z OptionsSave "保存选项(S)" 5 "将所有可设置选项保存到文件 $::optionsFile"
menuText Z OptionsSaveBases "保存数据库为默认(S)" 9 
menuText Z OptionsAutoSave "退出时自动保存选项(A)" 10 \
  {退出 Scid 时自动保存所有选项}

# Help menu:
menuText Z Help "帮助(H)" 3
menuText Z HelpContents "帮助(H)" 3 {显示帮助内容页面}
menuText Z HelpIndex "索引(I)" 3 {显示帮助索引页面}
menuText Z HelpGuide "快速指南(Q)" 5 {显示快速指南帮助页面}
menuText Z HelpHints "提示(I)" 3 {显示提示帮助页面}
menuText Z HelpContact "联系方式(O)" 5 {显示联系信息帮助页面}
menuText Z HelpTip "每日提示(T)" 5 {显示一条有用的 Scid 使用技巧}
menuText Z HelpStartup "启动窗口(S)" 5 {显示启动窗口}
menuText Z HelpAbout "关于(A)" 3 {关于 Scid vs. PC 的信息}

# Game info box popup menu:
menuText Z GInfoHideNext "隐藏下一步(H)" 6
menuText Z GInfoShow "轮走方(S)" 4
menuText Z GInfoCoords "切换坐标显示(T)" 7
menuText Z GInfoMaterial "切换子力显示(M)" 7
menuText Z GInfoFEN "FEN(F)" 4
menuText Z GInfoMarks "显示彩色格子和箭头(C)" 10
menuText Z GInfoWrap "自动换行长行(W)" 7
menuText Z GInfoFullComment "显示完整注释(C)" 7
menuText Z GInfoPhotos "显示棋手照片(P)" 7
menuText Z GInfoTBNothing "残局库：无(N)" 6
menuText Z GInfoTBResult "残局库：仅显示结果(R)" 10
menuText Z GInfoTBAll "残局库：显示结果与最佳着法(A)" 14
menuText Z GInfoDelete "（取消）删除本局(D)" 9
menuText Z GInfoMark "（取消）标记本局(M)" 9
menuText Z GInfoMenuBar "菜单栏(M)"  4
menuText Z GInfoToolBar "工具栏(T)"  4
menuText Z GInfoButtonBar "按钮栏(B)"  4
menuText Z GInfoStatusBar "状态栏(S)"  4
menuText Z GInfoEvalBar "评分条(S)" 4

translate Z FlipBoard {翻转棋盘}
translate Z RaiseWindows {置顶窗口}
translate Z AutoPlay {自动播放}
translate Z TrialMode {试走模式}
translate Z KeepOpen {启动后保持打开}

# General buttons:
translate Z Apply {应用}
translate Z Back {返回}
translate Z Browse {浏览}
translate Z Cancel {取消}
translate Z Continue {继续}
translate Z Clear {清除}
translate Z Close {关闭}
translate Z Contents {目录}
translate Z Defaults {默认}
translate Z Delete {删除}
translate Z Graph {图表}
translate Z Help {帮助}
translate Z Import {导入}
translate Z Index {索引}
translate Z LoadGame {载入}
translate Z BrowseGame {浏览}
translate Z MergeGame {合并}
translate Z MergeGames {合并对局}
# translate Z Ok {Ok}
translate Z Preview {预览}
translate Z Revert {还原}
translate Z Save {保存}
translate Z DontSave {不保存}
translate Z Search {搜索}
translate Z Stop {停止}
translate Z Store {存储}
translate Z Update {更新}
translate Z ChangeOrient {更改窗口方向}
translate Z ShowIcons {显示图标}
translate Z ConfirmCopy {确认复制}
translate Z None {无}
translate Z First {首局}
translate Z Current {当前}
translate Z Last {末局}
translate Z Font {字体}
translate Z Change {更改}
translate Z Random {随机}

# General messages:
translate Z game {对局}
translate Z games {对局}
translate Z move {着}
translate Z moves {着}
translate Z all {全部}
translate Z Yes {是}
translate Z No {否}
translate Z Both {双方}
translate Z King {王}
translate Z Queen {后}
translate Z Rook {车}
translate Z Bishop {象}
translate Z Knight {马}
translate Z Pawn {兵}
translate Z White {白方}
translate Z Black {黑方}
translate Z Player {棋手}
translate Z Rating {等级分}
translate Z RatingDiff {等级分差}
translate Z AverageRating {平均等级分}
translate Z Event {赛事}
translate Z Site {地点}
translate Z Country {国家}
translate Z IgnoreColors {忽略颜色}
translate Z MatchEnd {仅终局局面}
translate Z Date {日期}
translate Z EventDate {赛事日期}
translate Z Decade {年代}
translate Z Year {年}
translate Z Month {月}
translate Z Months {一月 二月 三月 四月 五月 六月 七月 八月 九月 十月 十一月 十二月}
translate Z Days {周日 周一 周二 周三 周四 周五 周六}
translate Z YearToToday {从年份至今}
translate Z Result {结果}
translate Z Round {轮次}
translate Z Length {长度}
translate Z ECOCode {ECO 编号}
translate Z ECO {ECO}
translate Z Deleted {已删除}
translate Z SearchResults {搜索结果}
translate Z OpeningTheDatabase {正在打开数据库}
translate Z Database {数据库}
translate Z Filter {筛选}
translate Z Reset {重置}
translate Z IgnoreCase {忽略大小写}
translate Z noGames {空}
translate Z allGames {全部}
translate Z empty {空}
translate Z clipbase {剪辑库}
translate Z score {评分}
translate Z Start {开始}
translate Z StartPos {起始局面}
translate Z Total {总计}
translate Z readonly {只读}
translate Z altered {已修改}
translate Z tagsDescript {额外标签（例如：Annotator "Anand"）}
translate Z prevTags {使用上一次}
translate Z value {值}

# Standard error messages:
translate Z ErrNotOpen {这不是一个已打开的数据库。}
translate Z ErrReadOnly {此数据库为只读，无法修改。}
translate Z ErrSearchInterrupted {搜索已中断}

# Game information:
translate Z twin {孪生局}
translate Z deleted {已删除}
translate Z comment {注释}
translate Z hidden {隐藏}
translate Z LastMove {着法}
translate Z NextMove {下一步}
translate Z GameStart {对局开始}
translate Z LineStart {分支开始}
translate Z GameEnd {对局结束}
translate Z LineEnd {分支结束}

# Player information:
translate Z PInfoAll {所有对局}
translate Z PInfoFilter {筛选对局}
translate Z PInfoAgainst {对阵}
translate Z PInfoMostWhite {执白最常见开局}
translate Z PInfoMostBlack {执黑最常见开局}
translate Z PInfoRating {等级分历史}
translate Z PInfoBio {简介}
translate Z PInfoEditRatings {编辑等级分}
translate Z PinfoEditName {编辑姓名}
translate Z PinfoLookupName {查询姓名}

# Tablebase information:
translate Z Draw {和棋}
translate Z stalemate {逼和}
translate Z checkmate {将死}
translate Z withAllMoves {包含所有着法}
translate Z withAllButOneMove {除一着外全部包含}
translate Z with {包含}
translate Z only {仅}
translate Z lose {输}
translate Z loses {输}
translate Z allOthersLose {其余均输}
translate Z matesIn {将在}
translate Z longest {最长}
translate Z WinningMoves {胜着}
translate Z DrawingMoves {和着}
translate Z LosingMoves {负着}
translate Z UnknownMoves {结果未知的着法}

# Tip of the day:
translate Z Tip {提示}
translate Z TipAtStartup {启动时显示提示}

# Tree window menus:
menuText Z TreeFile "树状图(T)" 4
menuText Z TreeFileFillWithBase "用筛选结果填充缓存(F)" 10 {用当前数据库中所有筛选后的对局填充缓存文件}
menuText Z TreeFileFillWithGame "用当前对局填充缓存(F)" 10 {用当前数据库中的当前对局填充缓存文件}
menuText Z TreeFileCacheInfo "缓存信息(C)" 5 {获取缓存使用情况信息}
menuText Z TreeFileSave "保存缓存文件(S)" 7 {保存树状图缓存（.stc）文件}
menuText Z TreeFileFill "填充缓存文件(F)" 7 \
  {用常见开局局面填充缓存文件}
menuText Z TreeFileBest "最佳对局(B)" 5 {显示最匹配的对局}
menuText Z TreeFileGraph "图表窗口(G)" 5 {显示此树状分支的图表}
menuText Z TreeFileCopy "复制树状文本到剪贴板(O)" 11 \
  {将树状统计数据复制到剪贴板}
menuText Z TreeFileClose "关闭树状图窗口(C)" 8 {关闭树状图窗口}
menuText Z TreeMask "掩码(M)" 3
menuText Z TreeMaskNew "新建(N)" 3 {新建掩码}
menuText Z TreeMaskOpen "打开(O)" 3 {打开掩码}
menuText Z TreeMaskOpenRecent "打开最近(O)" 5 {打开最近使用的掩码}
menuText Z TreeMaskSave "保存(S)" 3 {保存掩码}
menuText Z TreeMaskClose "关闭(C)" 3 {关闭掩码}
menuText Z TreeMaskFillWithLine "用路线填充(F)" 6 {用所有先前着法填充掩码}
menuText Z TreeMaskFillWithGame "用对局填充(F)" 6 {用对局填充掩码}
menuText Z TreeMaskFillWithBase "用筛选结果填充(F)" 8 {用筛选中的所有对局填充掩码}
menuText Z TreeMaskInfo "信息(I)" 3 {显示当前掩码的统计信息}
menuText Z TreeMaskDisplay "显示掩码(D)" 5 {以树状形式显示掩码数据}
menuText Z TreeMaskSearch "搜索掩码(S)" 5 {在当前掩码中搜索}
menuText Z TreeSort "排序(S)" 3
menuText Z TreeSortAlpha "按字母顺序(A)" 6
menuText Z TreeSortECO "按 ECO 编号(E)" 9
menuText Z TreeSortFreq "按频率(F)" 4
menuText Z TreeSortScore "按评分(S)" 4
menuText Z TreeOpt "选项(O)" 3
menuText Z TreeOptSlowmode "慢速模式(S)" 5 {更新时使用慢速模式（高精度）}
menuText Z TreeOptFastmode "快速模式(F)" 5 {更新时使用快速模式（不处理着法换序）}
menuText Z TreeOptFastAndSlowmode "快速+慢速模式(F)" 8 {先快速模式后慢速模式进行更新}
menuText Z TreeOptStartStop "自动刷新(A)" 5 {切换树状图窗口的自动刷新}
menuText Z TreeOptLock "锁定(L)" 3 {锁定/解锁树状图到当前数据库}
menuText Z TreeOptTraining "训练模式(T)" 5 {开启/关闭树状图训练模式}
menuText Z TreeOptShort "简略显示(S)" 5 {不显示等级分信息}
menuText Z TreeOptAutosave "自动保存缓存(A)" 7 {关闭树状图窗口时自动保存缓存文件}
menuText Z TreeOptAutomask "自动加载掩码(A)" 7 "打开树状图时自动加载最近使用的掩码。"
menuText Z TreeOptCacheSize "缓存大小(C)" 5 {设置缓存大小}
menuText Z TreeOptShowBar "显示进度条(S)" 6 "显示树状图进度条。"
menuText Z TreeOptShowFrame "显示按钮栏(S)" 6 "显示树状图按钮栏。"
menuText Z TreeOptSmallFont "使用自定义字体(U)" 8 "对局列表使用 font_Small 字体。"
menuText Z TreeOptSortBest "排序最佳对局(S)" 7 "按等级分对最佳对局排序。"
menuText Z TreeHelp "帮助(H)" 3
menuText Z TreeHelpTree "树状图帮助(T)" 6
menuText Z TreeHelpIndex "帮助索引(H)" 5
translate Z SaveCache {保存缓存}
translate Z Training {训练}
translate Z LockTree {锁定}
translate Z TreeLocked {已锁定}
translate Z TreeBest {最佳}
translate Z TreeBestGames {最佳对局}
translate Z TreeAdjust {调整筛选}
# Note: the next message is the tree window title row. After editing it,
# check the tree window to make sure it lines up with the actual columns.
translate Z TreeTitleRow      {    着法      频率        评分  和棋 平均分 表现分 平均年份 ECO}
translate Z TreeTitleRowShort {    着法      频率        评分  和棋}
translate Z TreeTotal: {总计：}
translate Z DoYouWantToSaveFirst {是否要先保存}
translate Z AddToMask {添加到掩码}
translate Z RemoveFromMask {从掩码中移除}
translate Z AddThisMoveToMask {将此着法加入掩码}
translate Z SearchMask {在掩码中搜索}
translate Z DisplayMask {掩码}
translate Z MaskShowMarkers {显示标记}
translate Z MaskShowToolTips {显示工具提示}
translate Z Nag {Nag 代码}
translate Z Marker {标记}
translate Z Include {包含}
translate Z Exclude {排除}
translate Z MainLine {主变}
translate Z Bookmark {书签}
translate Z NewLine {新分支}
translate Z ToBeVerified {待验证}
translate Z ToTrain {待训练}
translate Z Dubious {可疑}
translate Z ToRemove {待删除}
translate Z NoMarker {无标记}
translate Z ColorMarker {颜色}
translate Z WhiteMark {白色}
translate Z GreenMark {绿色}
translate Z YellowMark {黄色}
translate Z BlueMark {蓝色}
translate Z RedMark {红色}
translate Z CommentMove {注释着法}
translate Z CommentPosition {注释局面}
translate Z AddMoveToMaskFirst {请先将着法加入掩码}
translate Z OpenAMaskFileFirst {请先打开一个掩码文件}
translate Z positions {局面}
translate Z Moves {着法}

# Finder window:
menuText Z FinderFile "文件查找器(F)" 6
menuText Z FinderFileSubdirs "包含子目录(L)" 6
menuText Z FinderFileClose "关闭(C)" 3
menuText Z FinderSort "排序方式(S)" 5
menuText Z FinderSortType "类型(T)" 3
menuText Z FinderSortSize "对局数(G)" 4
menuText Z FinderSortMod "修改时间(M)" 5
menuText Z FinderSortName "名称(N)" 3
menuText Z FinderSortPath "路径(P)" 3
menuText Z FinderTypes "文件类型(T)" 5
menuText Z FinderTypesScid "Scid 数据库(S)" 9
menuText Z FinderTypesOld "旧版 Scid 数据库(O)" 12
menuText Z FinderTypesPGN "PGN 文件(P)" 7
menuText Z FinderTypesEPD "EPD 文件(E)" 7
menuText Z FinderHelp "帮助(H)" 3
menuText Z FinderHelpFinder "查找器帮助(F)" 6
menuText Z FinderHelpIndex "索引(I)" 3
translate Z FileFinder {文件查找器}
translate Z FinderDir {目录}
translate Z FinderDirs {目录}
translate Z FinderFiles {文件}
translate Z FinderUpDir {上级}
translate Z FinderCtxOpen {打开}
translate Z FinderCtxBackup {备份}
translate Z FinderCtxCopy {复制}
translate Z FinderCtxMove {移动}
translate Z FinderCtxDelete {删除}
translate Z FinderCtxRename {重命名}
translate Z FinderDirOpen {打开目录}

# Player finder:
menuText Z PListFile "棋手查找器(P)" 6
menuText Z PListFileUpdate "更新(U)" 3
menuText Z PListFileClose "关闭(C)" 3
menuText Z PListSort "排序(S)" 3
menuText Z PListSortName "姓名(N)" 3
menuText Z PListSortElo "等级分(E)" 4
menuText Z PListSortGames "对局数(G)" 4
menuText Z PListSortOldest "最早(O)" 3
menuText Z PListSortNewest "最新(W)" 3
menuText Z PListSortCountry "国家(C)" 3

# Tournament finder:
menuText Z TmtFile "赛事查找器(T)" 6
menuText Z TmtFileUpdate "更新(U)" 3
menuText Z TmtFileClose "关闭(C)" 3
menuText Z TmtSort "排序(S)" 3
menuText Z TmtSortDate "日期(D)" 3
menuText Z TmtSortPlayers "参赛人数(P)" 5
menuText Z TmtSortGames "对局数(G)" 4
menuText Z TmtSortElo "平均等级分(E)" 6
menuText Z TmtSortSite "地点(S)" 3
menuText Z TmtSortEvent "赛事(V)" 3
menuText Z TmtSortWinner "冠军(W)" 3
translate Z TmtLimit "列表大小"
translate Z TmtMeanElo "平均等级分"
translate Z TmtNone "未找到匹配的赛事。"

# Graph windows:
menuText Z GraphFile "文件(F)" 3
menuText Z GraphFileColor "保存为彩色 PostScript(C)" 17
menuText Z GraphFileGrey "保存为灰度 PostScript(G)" 17
menuText Z GraphFileClose "关闭(C)" 3
menuText Z GraphOptions "选项(O)" 3
menuText Z GraphOptionsWhite "反转白方(I)" 5
menuText Z GraphOptionsBlack "反转黑方(I)" 5
menuText Z GraphOptionsDots "显示点(S)" 4
menuText Z GraphOptionsSpelling "从拼写文件获取等级分(E)" 11
menuText Z GraphOptionsBar "高亮当前着法(H)" 7
menuText Z GraphOptionsBoth "双方(O)" 3
menuText Z GraphOptionsPInfo "棋手信息(P)" 5
translate Z GraphFilterTitle "每千局频率"
translate Z GraphAbsFilterTitle "对局频率"
translate Z ConfigureFilter "配置 X 轴"
translate Z FilterEstimate "估算"
translate Z TitleFilterGraph "Scid：筛选图表"

# Analysis window:
translate Z AddVariation {添加变着}
translate Z AddAllVariations {添加所有变着}
translate Z AddMove {添加着法}
translate Z Annotate {注释}
translate Z ShowAnalysisBoard {显示分析棋盘}
translate Z ShowInfo {显示引擎信息}
translate Z FinishGame {结束对局}
translate Z StopEngine {停止引擎}
translate Z StartEngine {启动引擎}
translate Z ExcludeMove {排除着法}
translate Z LockEngine {锁定引擎到当前局面}
translate Z AnalysisCommand {分析命令}
translate Z PreviousChoices {先前选项}
translate Z AnnotateTime {每着秒数}
translate Z AnnotateWhich {注释哪一方}
translate Z AnnotateAll {双方着法}
translate Z AnnotateAllMoves {所有着法}
translate Z AnnotateWhite {仅白方着法}
translate Z AnnotateBlack {仅黑方着法}
translate Z AnnotateNotBest {当着法非最优时}
translate Z AnnotateBlundersOnly {当着法为严重失误时}
translate Z BlundersNotBest {严重失误/非最优着}
translate Z AnnotateTitle {配置注释}
translate Z AnnotateMissedMates {错失/更短的杀棋}
translate Z AnnotateEnd {在对局结束时}
translate Z BlundersThreshold {阈值}
translate Z ScoreFormat {评分格式}
translate Z CutOff {截断}
translate Z LowPriority {低 CPU 优先级}
translate Z LogEngines {日志大小}
translate Z LogName {添加名称}
translate Z MaxPly {最大搜索深度}
translate Z ClickHereToSeeMoves {点击此处查看着法}
translate Z ConfigureInformant {配置国际象棋符号}
translate Z Informant!? {有趣着法}
translate Z Informant? {劣着}
translate Z Informant?? {严重失误}
translate Z Informant?! {可疑着法}
translate Z Informant+= {白方略有优势}
translate Z Informant+/- {白方有中等优势}
translate Z Informant+- {白方有决定性优势}
translate Z Informant++- {此局视为已胜}

# Book window
translate Z Book {开局库}
translate Z OtherBook {其他着法}
translate Z Alphabetical {按字母顺序}
translate Z TwoBooks {双开局库}

# Analysis Engine open dialog:
translate Z EngineList {分析引擎}
translate Z EngineKey {密钥}
translate Z EngineType {类型}
translate Z EngineName {名称}
translate Z EngineCmd {命令}
translate Z EngineArgs {参数}
translate Z EngineDir {目录}
translate Z EngineElo {等级分}
translate Z EngineTime {日期}
translate Z EngineNew {新建}
translate Z EngineEdit {编辑}
translate Z EngineRequired {粗体字段为必填项，其余为可选}

# Stats window menus:
menuText Z StatsFile "统计(S)" 3
menuText Z StatsFilePrint "打印到文件(P)" 6
menuText Z StatsFileClose "关闭窗口(C)" 5
menuText Z StatsOpt "选项(O)" 3

# PGN window menus:
menuText Z PgnFile "PGN(P)" 4
menuText Z PgnFileCopy "复制到剪贴板(C)" 7
menuText Z PgnFilePrint "另存为对局(S)" 6
menuText Z PgnFileClose "关闭(W)" 3
menuText Z PgnOpt "选项(O)" 3
menuText Z PgnOptColor "彩色显示(C)" 5
menuText Z PgnOptShort "简短标题(H)" 5
menuText Z PgnOptSymbols "符号注释(Y)" 5
menuText Z PgnOptIndentC "缩进注释(I)" 5
menuText Z PgnOptIndentV "缩进变着(V)" 5
menuText Z PgnOptColumn "列式布局(O)" 5
menuText Z PgnOptSpace "着法编号后加空格(U)" 9
menuText Z PgnOptStripMarks "隐藏方格/箭头代码(I)" 10
menuText Z PgnOptChess "棋子符号(P)" 5
menuText Z PgnOptScrollbar "滚动条(B)" 4
menuText Z PgnOptBoldMainLine "主变加粗(M)" 5
menuText Z PgnColor "颜色(C)" 3
menuText Z PgnColorHeader "标题(H)" 3
menuText Z PgnColorAnno "注释(A)" 3
menuText Z PgnColorComments "评论(C)" 3
menuText Z PgnColorVars "变着(V)" 3
menuText Z PgnColorBackground "背景(B)" 3
menuText Z PgnColorMain "主变(M)" 3
menuText Z PgnColorCurrent "当前着法(C)" 5
menuText Z PgnColorNextMove "下一手(N)" 4
menuText Z PgnHelp "帮助(H)" 3
menuText Z PgnHelpPgn "PGN 帮助(P)" 7
menuText Z PgnHelpIndex "索引(I)" 3
translate Z PgnWindowTitle {PGN：对局 %u}

# Crosstable window menus:
menuText Z CrosstabFile "交叉积分表(C)" 6
menuText Z CrosstabFileText "保存为文本(T)" 6
menuText Z CrosstabFileHtml "保存为 Html(H)" 9
menuText Z CrosstabFileLaTeX "保存为 LaTeX(L)" 10
menuText Z CrosstabFileClose "关闭(C)" 3
menuText Z CrosstabEdit "编辑(E)" 3
menuText Z CrosstabEditEvent "赛事(E)" 3
menuText Z CrosstabEditSite "地点(S)" 3
menuText Z CrosstabEditDate "日期(D)" 3
menuText Z CrosstabOpt "选项(O)" 3
menuText Z CrosstabOptColorPlain "纯文本(P)" 4
menuText Z CrosstabOptColorHyper "超文本(H)" 4
menuText Z CrosstabOptTieWin "按胜局数决胜(I)" 7
menuText Z CrosstabOptTieHead "按相互对局决胜(I)" 8
menuText Z CrosstabOptThreeWin "胜局计 3 分(P)" 8
menuText Z CrosstabOptAges "显示年龄（岁）(Y)" 8
menuText Z CrosstabOptNats "国籍(N)" 3
menuText Z CrosstabOptTallies "胜负和记录(W)" 6
menuText Z CrosstabOptRatings "等级分(R)" 4
menuText Z CrosstabOptTitles "称号(T)" 3
menuText Z CrosstabOptBreaks "决胜分(B)" 4
menuText Z CrosstabOptDeleted "包含已删除对局(D)" 8
menuText Z CrosstabOptColors "颜色信息（仅瑞士制）(C)" 11
menuText Z CrosstabOptColorRows "行着色(C)" 4
menuText Z CrosstabOptRowsColor "行颜色(R)" 4 {交叉积分表默认行颜色}
menuText Z CrosstabOptColumnNumbers "带编号列（仅循环赛）(M)" 11
menuText Z CrosstabOptGroup "分组积分(G)" 5
menuText Z CrosstabSort "排序依据(S)" 5
menuText Z CrosstabSortName "姓名(N)" 3
menuText Z CrosstabSortRating "等级分(R)" 4
menuText Z CrosstabSortScore "得分(S)" 3
menuText Z CrosstabSortCountry "国家(C)" 3
menuText Z CrosstabType "格式(F)" 3
menuText Z CrosstabTypeAll "循环赛(A)" 4
menuText Z CrosstabTypeSwiss "瑞士制(S)" 4
menuText Z CrosstabTypeKnockout "轮次(R)" 3
menuText Z CrosstabTypeAuto "自动(U)" 3
menuText Z CrosstabHelp "帮助(H)" 3
menuText Z CrosstabHelpCross "交叉积分表帮助(C)" 8
menuText Z CrosstabHelpIndex "帮助索引(H)" 5
translate Z SetFilter {设置筛选条件}
translate Z AddToFilter {添加到筛选条件}
translate Z Swiss {瑞士制}
translate Z Category {等级组}

# Opening report window menus:
menuText Z OprepFile "报告(R)" 3
menuText Z OprepFileText "打印为文本(T)" 6
menuText Z OprepFileHtml "打印为 HTML(H)" 9
menuText Z OprepFileLaTeX "打印为 LaTeX(L)" 10
menuText Z OprepFileOptions "选项(O)" 3
menuText Z OprepFileClose "关闭报告窗口(C)" 7
menuText Z OprepFavorites "收藏(A)" 3
menuText Z OprepFavoritesAdd "添加报告(A)" 5
menuText Z OprepFavoritesEdit "编辑报告收藏(E)" 7
menuText Z OprepFavoritesGenerate "生成报告(G)" 5
menuText Z OprepHelp "帮助(H)" 3
menuText Z OprepHelpReport "开局报告帮助(O)" 7
menuText Z OprepHelpIndex "帮助索引(H)" 5

# Header search:
translate Z HeaderSearch {通用搜索}
translate Z EndSideToMove {对局结束时行棋方}
translate Z GamesWithNoECO {无 ECO 编号的对局？}
translate Z GameLength {对局长度}
translate Z FindGamesWith {显示标记选项}
translate Z StdStart {非标准起始局面}
translate Z Promotions {升变}
translate Z UnderPromo {低级升变}
translate Z Comments {注释}
translate Z Variations {变着}
translate Z Annotations {评注}
translate Z DeleteFlag {删除标记}
translate Z WhiteOpFlag {白方开局}
translate Z BlackOpFlag {黑方开局}
translate Z MiddlegameFlag {中局}
translate Z EndgameFlag {残局}
translate Z NoveltyFlag {新着}
translate Z PawnFlag {兵结构}
translate Z TacticsFlag {战术}
translate Z QsideFlag {后翼作战}
translate Z KsideFlag {王翼作战}
translate Z BrilliancyFlag {妙着}
translate Z BlunderFlag {严重失误}
translate Z UserFlag {用户}
translate Z PgnContains {PGN 包含文本}
translate Z PreComment {赛前注释}
translate Z PostComment {赛后注释}

# Game list window:
translate Z GlistNumber {编号}
translate Z GlistWhite {白方}
translate Z GlistBlack {黑方}
translate Z GlistWElo {白方等级分}
translate Z GlistBElo {黑方等级分}
translate Z GlistEvent {赛事}
translate Z GlistSite {地点}
translate Z GlistRound {轮次}
translate Z GlistDate {日期}
translate Z GlistYear {年份}
translate Z GlistEventDate {赛事日期}
translate Z GlistResult {结果}
translate Z GlistLength {着数}
translate Z GlistCountry {国家}
translate Z GlistECO {ECO}
translate Z GlistOpening {着法}
translate Z GlistEndMaterial {终局子力}
translate Z GlistDeleted {已删除}
translate Z GlistFlags {标记}
translate Z GlistVariations {变着}
translate Z GlistComments {注释}
translate Z GlistAnnos {评注}
translate Z GlistStart {起始}
translate Z GlistGameNumber {对局编号}
translate Z GlistFindText {查找}
translate Z GlistMoveField {着法}
translate Z GlistEditField {配置}
translate Z GlistAddField {添加}
translate Z GlistDeleteField {删除}
translate Z GlistColor {颜色}
translate Z GlistSort {排序数据库}

# menu shown with right mouse button down on game list. 
translate Z GlistRemoveThisGameFromFilter  {移除}
translate Z GlistRemoveGameAndAboveFromFilter  {移除上方全部}
translate Z GlistRemoveGameAndBelowFromFilter  {移除下方全部}
translate Z GlistDeleteGame {删除此对局} 
translate Z GlistDeleteAllGames {删除筛选中的所有对局} 
translate Z GlistUndeleteAllGames {恢复筛选中的所有对局} 

translate Z GlistAlignL {左对齐}
translate Z GlistAlignR {右对齐}
translate Z GlistAlignC {居中对齐}

translate Z GlistMoveFieldUp {上移}
translate Z GlistMoveFieldDown {下移}
translate Z GlistMoveFieldFirst {移至最前}
translate Z GlistMoveFieldLast {移至最后}
translate Z GlistMoveFieldN {移至第 N 位}

# Maintenance window:
translate Z DatabaseName {数据库名称：}
translate Z TypeIcon {数据库类型}
translate Z NumOfGames {对局数：}
translate Z NumDeletedGames {已删除对局数：}
translate Z NumFilterGames {筛选中对局数：}
translate Z YearRange {年份范围：}
translate Z RatingRange {等级分范围：}
translate Z Description {描述}
translate Z Flag {标记}
translate Z CustomFlags {自定义标记}
translate Z DeleteCurrent {删除当前对局}
translate Z DeleteFilter {删除筛选中的对局}
translate Z DeleteAll {删除所有对局}
translate Z UndeleteCurrent {恢复当前对局}
translate Z UndeleteFilter {恢复筛选中的对局}
translate Z UndeleteAll {恢复所有对局}
translate Z DeleteTwins {删除重复对局}
translate Z MarkCurrent {标记当前对局}
translate Z MarkFilter {标记筛选中的对局}
translate Z MarkAll {标记所有对局}
translate Z UnmarkCurrent {取消标记当前对局}
translate Z UnmarkFilter {取消标记筛选中的对局}
translate Z UnmarkAll {取消标记所有对局}
translate Z Spellchecking {拼写检查}
translate Z MakeCorrections {执行修正}
translate Z Ambiguous {模糊项}
translate Z Surnames {姓氏}
translate Z Players {棋手}
translate Z Events {赛事}
translate Z Sites {地点}
translate Z Rounds {轮次}
translate Z DatabaseOps {数据库操作}
translate Z ReclassifyGames {ECO 分类对局}
translate Z CompactDatabase {压缩数据库}
translate Z SortDatabase {排序数据库}
translate Z AddEloRatings {添加等级分}
translate Z AutoloadGame {自动加载对局}
translate Z StripTags {额外标签}
translate Z StripTag {清除标签}
translate Z AddTag {添加标签}
translate Z CheckGames {检查对局}
translate Z Cleaner {清理器}
translate Z CleanerHelp {
清理器将对当前数据库执行下方所选的所有操作。

若已选择，ECO 分类和重复对局删除的当前设置将生效。
}
translate Z CleanerConfirm {
清理器维护一旦开始将无法中断。

此过程可能耗时较长。

您确定要运行所选的维护功能吗？
}
# Twinchecker
translate Z TwinCheckUndelete {（切换）}
translate Z TwinCheckprevPair {上一对}
translate Z TwinChecknextPair {下一对}
translate Z TwinChecker {重复对局检查器}
translate Z TwinCheckTournament {赛事中的对局数：}
translate Z TwinCheckNoTwin {无重复  }
translate Z TwinCheckNoTwinfound {未检测到此对局的重复项。\n要在此窗口中显示重复对局，您必须先使用“删除重复对局”功能。}
translate Z TwinCheckTag {共享标签…}
translate Z TwinCheckFound1 {Scid 找到 $result 个重复对局}
translate Z TwinCheckFound2 { 并已设置其删除标记}
translate Z TwinCheckNoDelete {此数据库中没有可删除的对局。}
# bug here... can't use \n\n
translate Z TwinCriteria1 {弱判定条件警告\n}
translate Z TwinCriteria2 {您已将“相同着法”设为“否”，这非常不妥。\n
是否仍要继续？}
translate Z TwinCriteria3 {您应至少在“相同地点”、“相同轮次”和“相同年份”三项中选择两项设为“是”。\n
是否仍要继续？}
translate Z TwinCriteriaConfirm {Scid：确认重复对局设置}
translate Z TwinChangeTag "更改以下对局标签：\n\n"
translate Z AllocRatingDescription "使用拼写（等级分）文件中的信息，为相关对局添加棋手等级分。"
translate Z RatingOverride "覆盖现有等级分？"
translate Z AddRatings "为以下内容添加等级分"
translate Z AddedRatings {Scid 在 $g 个对局中添加了 $r 个等级分。}

#Bookmark editor
translate Z NewSubmenu "添加菜单"

# Comment editor:
translate Z AnnotationSymbols  {评注}
translate Z Comment {注释}
translate Z InsertMark {标记}
translate Z InsertMarkHelp {
插入/移除标记：选择颜色、类型和格子。
插入/移除箭头：右键点击两个格子。
}

# Nag buttons in comment editor:
translate Z GoodMove {好着}
translate Z PoorMove {劣着}
translate Z ExcellentMove {妙着}
translate Z Blunder {大漏着}
translate Z InterestingMove {有趣着法}
translate Z DubiousMove {疑问着}
translate Z WhiteDecisiveAdvantage {白方有决定性优势}
translate Z BlackDecisiveAdvantage {黑方有决定性优势}
translate Z WhiteClearAdvantage {白方有明显优势}
translate Z BlackClearAdvantage {黑方有明显优势}
translate Z WhiteSlightAdvantage {白方有轻微优势}
translate Z BlackSlightAdvantage {黑方有轻微优势}
translate Z Equality {均势}
translate Z Unclear {局势不明}
translate Z Diagram {图示}

# Board search:
translate Z BoardSearch {局面搜索}
translate Z FilterOperation {筛选设置}
translate Z FilterAnd {限制筛选（与）}
translate Z FilterOr {添加到筛选（或）}
translate Z FilterIgnore {重置筛选}
translate Z SearchType {搜索类型}
translate Z SearchBoardExact {精确局面（所有棋子位置完全相同）}
translate Z SearchBoardPawns {兵结构（子力相同，所有兵位置相同）}
translate Z SearchBoardFiles {兵路（子力相同，所有兵位于相同线路）}
translate Z SearchBoardAny {任意（子力相同，兵与子位置任意）}
translate Z SearchInRefDatabase { 在数据库中搜索 }
translate Z LookInVars {在变着中查找}

translate Z CQLLabel {CQL 脚本}
translate Z CQLComments {允许注释}
translate Z CQLStrip {清除匹配注释}

# Material search:
translate Z MaterialSearch {子力搜索}
translate Z Material {子力}
translate Z Patterns {模式}
translate Z Zero {无}
translate Z Any {任意}
translate Z CurrentBoard {当前棋盘}
translate Z CommonEndings {常见残局}
translate Z CommonPatterns {常见局面模式}
translate Z MaterialDiff {子力差}
translate Z squares {格子}
translate Z SameColor {同色}
translate Z OppColor {异色}
translate Z Either {任一}
translate Z MoveNumberRange {回合数范围}
translate Z MatchForAtLeast {至少匹配}
translate Z HalfMoves {半回合}

# Common endings in material search:
translate Z EndingPawns {兵残局}
translate Z EndingRookVsPawns {车对兵（或数兵）}
translate Z EndingRookPawnVsRook {车加一兵对车}
translate Z EndingRookPawnsVsRook {车加兵（或数兵）对车}
translate Z EndingRooks {车对车残局}
translate Z EndingRooksPassedA {车对车残局含 a 路通路兵}
translate Z EndingRooksDouble {双车残局}
translate Z EndingBishops {象对象残局}
translate Z EndingBishopVsKnight {象对马残局}
translate Z EndingKnights {马对马残局}
translate Z EndingQueens {后对后残局}
translate Z EndingQueenPawnVsQueen {后加一兵对后}
translate Z BishopPairVsKnightPair {双象对双马中局}

# Common patterns in material search:
translate Z PatternWhiteIQP {白方孤兵（IQP）}
translate Z PatternWhiteIQPBreakE6 {白方孤兵：d4-d5 突破对抗 e6}
translate Z PatternWhiteIQPBreakC6 {白方孤兵：d4-d5 突破对抗 c6}
translate Z PatternBlackIQP {黑方孤兵（IQP）}
translate Z PatternWhiteBlackIQP {白方孤兵对黑方孤兵}
translate Z PatternCoupleC3D4 {白方 c3+d4 孤兵组合}
translate Z PatternHangingC5D5 {黑方 c5 和 d5 悬兵}
translate Z PatternMaroczy {马罗齐中心（c4 和 e4 兵）}
translate Z PatternRookSacC3 {c3 车弃子}
translate Z PatternKc1Kg8 {白方王车易位至后翼 vs. 黑方王翼易位（Kc1 对 Kg8）}
translate Z PatternKg1Kc8 {白方王翼易位 vs. 黑方后翼易位（Kg1 对 Kc8）}
translate Z PatternLightFian {浅色格堡垒象（白方 g2 象对黑方 b7 象）}
translate Z PatternDarkFian {深色格堡垒象（白方 b2 象对黑方 g7 象）}
translate Z PatternFourFian {四堡垒象（b2, g2, b7, g7 象）}

# Game saving:
translate Z Today {今天}
translate Z ClassifyGame {分类对局}

# Setup position:
translate Z EmptyBoard {清空棋盘}
translate Z InitialBoard {初始局面}
translate Z SideToMove {行棋方}
translate Z MoveNumber {回合数}
translate Z Castling {王车易位}
translate Z EnPassantFile {吃过路兵线路}
translate Z ClearFen {清除 FEN}
translate Z PasteFen {粘贴 FEN}

translate Z SaveAndContinue {保存并继续}
translate Z DiscardChangesAndContinue {放弃更改}
translate Z GoBack {返回}

# Replace move dialog:
translate Z ReplaceMove {替换着法}
translate Z AddNewVar {添加变着}
translate Z NewMainLine {设为新主线}
translate Z ReplaceMoveMessage {着法已存在。

请选择操作。
“替换着法”将丢弃此后所有着法。}

translate Z ReadOnlyDialog {是否将此数据库设为只读？

（您可以通过关闭并重新打开数据库来恢复为可写状态。）}

translate Z ExitDialog {确定要退出 Scid 吗？}
translate Z ClearGameDialog {本局已被修改。\n是否保存？}
translate Z ExitUnsaved {以下数据库包含未保存的对局。如果现在退出，这些更改将丢失。}
translate Z ChangeTextDialog {全局启用文字颜色需要重启软件。}
translate Z ChangeDockDialog {更改停靠设置需要重启软件。}
translate Z FontSelectDialog "这是示例文字\n双击上方以更改字体，然后点击“确定”\nAaBbCcDdEeFfGgHhIiJjKkLlMm 0123456789. +=-"

translate Z ExitNow {立即退出？}

# Import window:
translate Z PasteCurrentGame {粘贴当前对局}
translate Z ImportHelp1 {在上方框中输入或粘贴 PGN。}
translate Z ImportHelp2 {导入对局时的任何错误将显示在此处。}
translate Z OverwriteExistingMoves {是否覆盖已有着法？}

# ECO Browser:
translate Z ECOAllSections {所有 ECO 分类}
translate Z ECOSection {ECO 分类}
translate Z ECOSummary {摘要：}
translate Z ECOFrequency {子分类出现频率：}

# Opening Report:
translate Z OprepTitle {开局报告}
translate Z OprepReport {报告}
translate Z OprepGenerated {生成自}
translate Z OprepStatsHist {统计与历史}
translate Z OprepStats {统计}
translate Z OprepStatAll {报告包含的所有对局}
translate Z OprepStatBoth {双方均有等级分}
translate Z OprepStatSince {自}
translate Z OprepOldest {最早对局}
translate Z OprepNewest {最新对局}
translate Z OprepPopular {当前流行度}
translate Z OprepFreqAll {历年频率：   }
translate Z OprepFreq1   {最近 1 年至今： }
translate Z OprepFreq5   {最近 5 年至今： }
translate Z OprepFreq10  {最近10年至今： }
translate Z OprepEvery {每 %u 局出现一次}
translate Z OprepUp {较历年上升 %u%s}
translate Z OprepDown {较历年下降 %u%s}
translate Z OprepSame {与历年相比无变化}
translate Z OprepMostFrequent {最常出现的棋手}
translate Z OprepMostFrequentOpponents {最常遇到的对手}
translate Z OprepRatingsPerf {等级分与表现分}
translate Z OprepAvgPerf {平均等级分与表现分}
translate Z OprepWRating {白方等级分}
translate Z OprepBRating {黑方等级分}
translate Z OprepWPerf {白方表现分}
translate Z OprepBPerf {黑方表现分}
translate Z OprepHighRating {平均等级分最高的对局}
translate Z OprepTrends {结果趋势}
translate Z OprepResults {对局长度与结果频率}
translate Z OprepLength {对局长度}
translate Z OprepFrequency {频率}
translate Z OprepWWins {白胜： }
translate Z OprepBWins {黑胜： }
translate Z OprepDraws {和棋：      }
translate Z OprepWholeDB {整个数据库}
translate Z OprepShortest {最短胜局}
translate Z OprepMovesThemes {着法与局面主题}
translate Z OprepMoveOrders {到达报告局面的着法顺序}
translate Z OprepMoveOrdersOne \
  {仅有一种着法顺序到达此局面：}
translate Z OprepMoveOrdersAll \
  {共有 %u 种着法顺序到达此局面：}
translate Z OprepMoveOrdersMany \
  {共有 %u 种着法顺序到达此局面。前 %u 种如下：}
translate Z OprepMovesFrom {从报告局面出发的着法}
translate Z OprepMostFrequentEcoCodes {最常见的 ECO 编码}
translate Z OprepThemes {局面主题}
translate Z OprepThemeDescription {各对局前 %u 步中主题出现频率}
translate Z OprepThemeSameCastling {同侧易位}
translate Z OprepThemeOppCastling {异侧易位}
translate Z OprepThemeNoCastling {双方王均未易位}
translate Z OprepThemeKPawnStorm {王翼兵风暴}
translate Z OprepThemeQueenswap {后已兑掉}
translate Z OprepThemeWIQP {白方孤后兵}
translate Z OprepThemeBIQP {黑方孤后兵}
translate Z OprepThemeWP567 {白方兵位于第5/6/7横线}
translate Z OprepThemeBP234 {黑方兵位于第2/3/4横线}
translate Z OprepThemeOpenCDE {c/d/e 线开放}
translate Z OprepTheme1BishopPair {仅一方拥有双象}
translate Z OprepEndgames {残局}
translate Z OprepReportGames {报告对局}
translate Z OprepAllGames    {所有对局}
translate Z OprepEndClass {每局终局时的子力配置}
translate Z OprepTheoryTable {理论表}
translate Z OprepTableComment {基于 %u 局等级分最高的对局生成。}
translate Z OprepExtraMoves {理论表中的额外注释着法}
translate Z OprepMaxTableGames {理论表最大对局数}
translate Z OprepMaxGames {统计所用最大对局数}
translate Z OprepViewHTML {查看 HTML}
translate Z OprepViewLaTeX {查看 LaTeX}

# Player Report:
translate Z PReportTitle {棋手报告}
translate Z PReportColorWhite {执白时}
translate Z PReportColorBlack {执黑时}
translate Z PReportBeginning {始于}
translate Z PReportMoves {在 %s 之后}
translate Z PReportOpenings {开局}
translate Z PReportClipbase {清空剪辑库并将匹配的对局复制进去}

# Piece Tracker window:
translate Z TrackerSelectSingle {鼠标左键选择此棋子。}
translate Z TrackerSelectPair {鼠标左键选择此棋子；右键同时选择其配对棋子。}
translate Z TrackerSelectPawn {鼠标左键选择此兵；右键选择全部8个兵。}
translate Z TrackerStat {统计}
translate Z TrackerGames {包含走到该格着法的对局百分比}
translate Z TrackerTime {在各格停留时间百分比}
translate Z TrackerMoves {着法}
translate Z TrackerMovesStart {输入开始追踪的着法序号。}
translate Z TrackerMovesStop {输入停止追踪的着法序号。}

# Game selection dialogs:
translate Z SelectAllGames {所有对局}
translate Z SelectFilterGames {筛选对局}
translate Z SelectTournamentGames {仅限当前赛事中的对局}
translate Z SelectOlderGames {仅限较早的对局}

# Delete Twins window:
translate Z TwinsNote {要标记为重复对局，对局必须具有相同的棋手及其他如下条件。在检测重复对局前，最好先对数据库进行拼写检查。}
translate Z TwinsCriteria {重复对局判定条件}
translate Z TwinsWhich {检查范围：全部/筛选}
translate Z TwinsColors {棋手执棋颜色相同}
translate Z TwinsEvent {赛事相同}
translate Z TwinsSite {地点相同}
translate Z TwinsRound {轮次相同}
translate Z TwinsYear {年份相同}
translate Z TwinsMonth {月份相同}
translate Z TwinsDay {日期相同}
translate Z TwinsResult {结果相同}
translate Z TwinsECO {ECO 编码相同}
translate Z TwinsMoves {着法相同}
translate Z TwinsPlayers {棋手姓名}
translate Z TwinsPlayersExact {完全匹配}
translate Z TwinsPlayersPrefix {仅前四个字母}
translate Z TwinsWhen {删除重复对局时}
translate Z TwinsSkipShort {忽略少于 5 步的对局}
translate Z TwinsUndelete {先恢复所有已删除对局}
translate Z TwinsSetFilter {将筛选器设为待删除的重复对局}
translate Z TwinsComments {始终保留带注释的对局}
translate Z TwinsVars {始终保留带变着的对局}
translate Z TwinsDeleteWhich {删除哪一个对局？}
translate Z TwinsDeleteShorter {较短的对局}
translate Z TwinsDeleteOlder {序号较小的对局}
translate Z TwinsDeleteNewer {序号较大的对局}
translate Z TwinsDelete {删除对局}

# Name editor window:
translate Z NameEditType {要编辑的名称类型}
translate Z NameEditSelect {要编辑的对局}
translate Z NameEditReplace {替换}
translate Z NameEditWith {替换为}
translate Z NameEditMatches {匹配项：按 Ctrl+1 至 Ctrl+9 选择}
translate Z MatchPattern {匹配模式}

# Classify window:
translate Z Classify {分类}
translate Z ClassifyWhich {ECO 分类}
translate Z ClassifyAll {所有对局（覆盖原有 ECO 编码）}
translate Z ClassifyYear {过去一年内进行的所有对局}
translate Z ClassifyMonth {过去一个月内进行的所有对局}
translate Z ClassifyNew {仅尚未分配 ECO 编码的对局}
translate Z ClassifyCodes {使用的 ECO 编码}
translate Z ClassifyBasic {仅基本编码（如 "B12"）}
translate Z ClassifyExtended {Scid 扩展编码（如 "B12j"）}

# Compaction:
translate Z NameFile {名称文件}
translate Z GameFile {对局文件}
translate Z Names {名称}
translate Z Unused {未使用}
translate Z SizeKb {大小 (KB)}
translate Z CurrentState {当前状态}
translate Z AfterCompaction {压缩后}
translate Z CompactNames {压缩名称文件}
translate Z CompactGames {压缩对局文件}
translate Z NoUnusedNames "没有未使用的名称，名称文件已完全压缩。"
translate Z NoUnusedGames "对局文件已完全压缩。"
translate Z NameFileCompacted {数据库“[file tail [sc_base filename]]”的名称文件已压缩。}
translate Z GameFileCompacted {数据库“[file tail [sc_base filename]]”的对局文件已压缩。}

# Sorting:
translate Z SortCriteria {排序条件}
translate Z AddCriteria {添加条件}
translate Z CommonSorts {常用排序}
translate Z Sort {排序}

# Exporting:
translate Z AddToExistingFile {将对局添加到现有文件？}
translate Z ExportComments {导出注释？}
translate Z ExportVariations {导出变着？}
translate Z IndentComments {注释缩进？}
translate Z IndentVariations {变着缩进？}
translate Z InsertNewlines {每 80 个字符插入换行？}
translate Z ExportColumnStyle {列式格式（每行一个着法）？}
translate Z ExportSymbolStyle {符号注释样式：}
translate Z ExportStripMarks {从注释中移除方格/箭头标记代码？}
translate Z ExportFlags {导出 Scid 标记}

# Goto game/move dialogs:
translate Z LoadGameNumber {加载对局编号}
translate Z GotoMoveNumber {跳转至着法编号}

# Copy games dialog:
translate Z CopyGames {正在复制对局}
translate Z CopyConfirm {
是否将 [::utils::thousands $nGamesToCopy] 个对局
从“$fromName”复制到“$targetName”？
}
translate Z CopyErr {无法复制对局}
translate Z CopyErrSource {源数据库}
translate Z CopyErrTarget {目标数据库}
translate Z CopyErrNoGames {的筛选结果中无对局}
translate Z CopyErrReadOnly {为只读状态}
translate Z CopyErrNotOpen {未打开}

# Colors:
translate Z LightSquares {浅色格}
translate Z DarkSquares {深色格}
translate Z SelectedSquares {选中格}
translate Z SelectedOutline {选中框}
translate Z SuggestedSquares {推荐格}
translate Z Grid {网格}
translate Z Previous {上一步}
translate Z WhitePieces {白方棋子}
translate Z BlackPieces {黑方棋子}
translate Z WhiteBorder {白方边框}
translate Z BlackBorder {黑方边框}
translate Z ArrowMain   {主箭头}
translate Z ArrowVar    {变着箭头}
translate Z ShowArrows  {显示箭头}

# Novelty window:
translate Z FindNovelty {查找新着}
translate Z Novelty {新着}
translate Z NoveltyInterrupt {新着搜索已中断}
translate Z NoveltyNone {本局未发现新着}
translate Z NoveltyHelp {在所选数据库中查找首个独特着法}

# Sounds configuration:
translate Z SoundsFolder {声音文件夹}
translate Z SoundsFolderHelp {该文件夹应包含 King.wav、1.wav 等文件}
translate Z SoundsAnnounceOptions {着法播报}
translate Z SoundsAnnounceNew {播报新着}
translate Z SoundsAnnounceForward {向前移动时播报着法}
translate Z SoundsAnnounceBack {向后移动时播报着法}

# Upgrading databases:
translate Z Upgrading {升级中}
translate Z ConfirmOpenNew {
这是一个旧格式（si3）数据库，无法在 Scid 4.0 中打开，但已创建了新格式（si4）版本。

是否要打开该数据库的新格式版本？
}
translate Z ConfirmUpgrade {
这是一个“si3”格式数据库。必须将其转换为“si4”格式后才能在 Scid vs. PC 4.0 中使用。

此过程不可逆，且只需执行一次。如果耗时过长，您可以取消操作。

是否立即升级此数据库？
}

# Recent files options:
translate Z RecentFilesMenu {文件菜单中显示的文件数量}
translate Z RecentFilesExtra {子菜单中显示的文件数量}

translate Z MyPlayerNamesDescription {
请在下方输入您偏好的棋手名称，每行一个。
每次加载包含列表中棋手的对局时，主棋盘将根据需要自动翻转。
}

#Coach
translate Z showblunderexists {显示 Phalanx 漏着}
translate Z showblundervalue {显示漏着数值}
translate Z showscore {显示评分}
translate Z coachgame {教练对局}
translate Z configurecoachgame {配置战术对局}
translate Z configuregame {配置 UCI 对局}
translate Z Phalanxengine {Phalanx 引擎}
translate Z Coachengine {教练引擎}
translate Z difficulty {难度}
translate Z hard {困难}
translate Z easy {简单}
translate Z Playwith {执}
translate Z white {白方}
translate Z black {黑方}
translate Z both {双方}
translate Z Play {开始}
translate Z Noblunder {无漏着}
translate Z blunder {漏着}
translate Z Noinfo {-- 无信息 --}
translate Z moveblunderthreshold {若损失大于此值，则视为漏着}
translate Z limitanalysis {教练分析时间}
translate Z seconds {秒}
translate Z Abort {中止}
translate Z Quit {退出}
translate Z Resume {继续}
translate Z Restart {重新开始}
translate Z OutOfOpening {已离开开局库}
translate Z NotFollowedLine {你未遵循指定路线}
translate Z DoYouWantContinue {是否继续？}
translate Z CoachIsWatching {教练正在观察}
translate Z Ponder {永久思考}
translate Z LimitELO {限制 ELO 强度}
translate Z DubiousMovePlayedTakeBack {走了疑问着，是否悔棋？}
translate Z WeakMovePlayedTakeBack {走了弱着，是否悔棋？}
translate Z BadMovePlayedTakeBack {走了坏着，是否悔棋？}
translate Z Iresign {我认输}
translate Z yourmoveisnotgood {你的着法不佳}
translate Z EndOfVar {变着结束}
translate Z Openingtrainer {开局训练器}
translate Z DisplayCM {显示候选着法}
translate Z DisplayCMValue {显示候选着法评分}
translate Z DisplayOpeningStats {显示统计数据}
translate Z ShowReport {显示报告}
translate Z NumberOfGoodMovesPlayed {良好着法次数}
translate Z NumberOfDubiousMovesPlayed {疑问着法次数}
translate Z NumberOfTimesPositionEncountered {局面出现次数}
translate Z PlayerBestMove  {仅允许最佳着法}
translate Z OpponentBestMove {对手走最佳着法}
# ====== TODO To be translated ======
translate Z RemoveGames {Don't repeat a game}
translate Z OnlyFlaggedLines {仅标记路线}
translate Z resetStats {重置统计数据}
translate Z Movesloaded {已加载着法}
translate Z PositionsNotPlayed {未练习局面}
translate Z PositionsPlayed {已练习局面}
translate Z Success {成功}
translate Z DubiousMoves {疑问着法}
# ====== TODO To be translated ======
translate Z NumberOfMovesPlayedNotInRepertoire {moves played not in repertoire}
# ====== TODO To be translated ======
translate Z Repertoiretrainingconfiguration {Opening Trainer Configuration}
# ====== TODO To be translated ======
translate Z Loadingrepertoire {Loading repertoire}
# ====== TODO To be translated ======
translate Z Repertoirenotfound {Repertoire not found}
# ====== TODO To be translated ======
translate Z NoRepertoireFound {No Repertoire Database found}
# ====== TODO To be translated ======
translate Z Movenotinrepertoire {Move not in repertoire}
# ====== TODO To be translated ======
translate Z PositionsInRepertoire {Positions in Repertoire}
# ====== TODO To be translated ======
translate Z OutOfRepertoire {Out of Repertoire}
# ====== TODO To be translated ======
translate Z NextGame {Next Game}
translate Z ConfigureTactics {选择习题}
translate Z ResetScores {重置得分}
translate Z LoadingBase {正在加载数据库}
translate Z Tactics {战术}
translate Z ShowSolution {显示解答}
translate Z Next {下一题}
translate Z ResettingScore {正在重置得分}
translate Z LoadingGame {正在加载对局}
translate Z MateFound {已发现杀棋}
translate Z BestSolutionNotFound {未找到最佳解。}
translate Z MateNotFound {未发现杀棋}
translate Z ShorterMateExists {存在更短杀棋}
translate Z ScorePlayed {实际得分}
translate Z Expected {预期}
translate Z ChooseTrainingBase {选择训练数据库}
translate Z Thinking {思考中}
translate Z AnalyzeDone {分析完成}
translate Z WinWonGame {实战练习}
translate Z Lines {路线}
translate Z ConfigureUCIengine {配置引擎}
translate Z SpecificOpening {特定开局}
translate Z ImportECO {导入 ECO}
translate Z StartNewGame {开始新对局}
translate Z FixedLevel {固定等级}
translate Z Opening {开局}
translate Z RandomLevel {随机等级}
translate Z StartFromCurrentPosition {从当前局面开始}
translate Z FixedDepth {固定深度}
translate Z Nodes {节点数}
translate Z Depth {深度}
translate Z Time {时间} 
translate Z Auto {自动}
translate Z Score {评分}
translate Z {Score Combo} {评分组合}
translate Z {Time Combo} {时间组合}
translate Z SecondsPerMove {每步秒数}
translate Z DepthPerMove {每步深度}
translate Z MoveControl {着法控制}
translate Z TimeLabel {每步时间}
translate Z AddVars {添加变着}
translate Z AddScores {添加评分}
translate Z Engine {引擎}
translate Z TimeMode {时间模式}
translate Z TimeBonus {时间+奖励}
translate Z TimeMin {分}
translate Z TimeSec {秒}
translate Z AllExercisesDone {所有习题已完成}
translate Z MoveOutOfBook {走出开局库}
translate Z LastBookMove {最后一着开局库着法}
translate Z AnnotateSeveralGames {批量注释}
translate Z FindOpeningErrors {仅找开局错误}
translate Z MarkTacticalExercises {标记战术习题}
translate Z UseBook {使用开局库}
translate Z MultiPV {多路线分析}
translate Z Hash {哈希内存}
translate Z OwnBook {使用引擎开局库}
translate Z BookFile {开局库文件}
translate Z AnnotateVariations {处理变着}
translate Z ShortAnnotations {简短注释}
translate Z addAnnotatorTag {添加注释者标签}
translate Z AddScoreToShortAnnotations {在注释中加入评分}
translate Z Export {导出}
translate Z BookPartiallyLoaded {开局库部分加载}
translate Z AddLine {添加路线}
translate Z RemLine {移除路线}
translate Z Calvar {变着计算}
translate Z ConfigureCalvar {配置}
# Opening names used in tacgame.tcl
translate Z Reti {列蒂开局}
translate Z English {英国式开局}
translate Z d4Nf6Miscellaneous {1.d4 Nf6 其他}
translate Z Trompowsky {特罗波夫斯基进攻}
translate Z Budapest {布达佩斯弃兵}
translate Z OldIndian {古印度防御}
translate Z BenkoGambit {别诺尼弃兵}
translate Z ModernBenoni {现代别诺尼}
translate Z DutchDefence {荷兰防御}
translate Z Scandinavian {斯堪的纳维亚防御}
translate Z AlekhineDefence {阿廖欣防御}
translate Z Pirc {皮尔茨防御}
translate Z CaroKann {卡罗-卡恩防御}
translate Z CaroKannAdvance {卡罗-卡恩推进变例}
translate Z Sicilian {西西里防御}
translate Z SicilianAlapin {西西里阿拉宾变例}
translate Z SicilianClosed {西西里封闭变例}
translate Z SicilianRauzer {西西里劳泽尔变例}
translate Z SicilianDragon {西西里龙式变例}
translate Z SicilianScheveningen {西西里舍文宁根变例}
translate Z SicilianNajdorf {西西里纳依道夫变例}
translate Z OpenGame {开放性开局}
translate Z Vienna {维也纳开局}
translate Z KingsGambit {王翼弃兵}
translate Z RussianGame {俄罗斯开局}
translate Z ItalianTwoKnights {意大利开局/双马防御}
translate Z Spanish {西班牙开局}
translate Z SpanishExchange {西班牙兑换变例}
translate Z SpanishOpen {西班牙开放变例}
translate Z SpanishClosed {西班牙封闭变例}
translate Z FrenchDefence {法兰西防御}
translate Z FrenchAdvance {法兰西推进变例}
translate Z FrenchTarrasch {法兰西塔拉什变例}
translate Z FrenchWinawer {法兰西维纳维尔变例}
translate Z FrenchExchange {法兰西兑换变例}
translate Z QueensPawn {后翼兵开局}
translate Z Slav {斯拉夫防御}
translate Z QGA {后翼弃兵接受}
translate Z QGD {后翼弃兵拒绝}
translate Z QGDExchange {后翼弃兵拒绝兑换变例}
translate Z SemiSlav {半斯拉夫防御}
translate Z QGDwithBg5 {后翼弃兵拒绝 Bg5 变例}
translate Z QGDOrthodox {后翼弃兵拒绝正统变例}
translate Z Grunfeld {格林菲尔德防御}
translate Z GrunfeldExchange {格林菲尔德兑换变例}
translate Z GrunfeldRussian {格林菲尔德俄罗斯变例}
translate Z Catalan {卡塔兰开局}
translate Z CatalanOpen {卡塔兰开放变例}
translate Z CatalanClosed {卡塔兰封闭变例}
translate Z QueensIndian {后翼印度防御}
translate Z NimzoIndian {尼姆佐-印度防御}
translate Z NimzoIndianClassical {尼姆佐-印度古典变例}
translate Z NimzoIndianRubinstein {尼姆佐-印度鲁宾斯坦变例}
translate Z KingsIndian {王翼印度防御}
translate Z KingsIndianSamisch {王翼印度萨米什变例}
translate Z KingsIndianMainLine {王翼印度主线}

# FICS
translate Z ConfigureFics {配置 FICS}
translate Z FICSLogin {登录}
translate Z FICSGuest {以游客身份登录}
translate Z FICSServerPort {服务器端口}
translate Z FICSServerAddress {IP 地址}
translate Z FICSRefresh {刷新}
translate Z FICSTimeseal {Timeseal}
translate Z FICSTimesealPort {Timeseal 端口}
translate Z FICSSilence {控制台过滤器}
translate Z FICSOffers {对局请求}
translate Z FICSMakeOffer {发出请求}
translate Z FICSGames {对局}
translate Z FICSFindOpponent {寻找对手}
translate Z FICSTakeback {悔一步}
translate Z FICSTakeback2 {悔两步}
translate Z FICSInitTime {初始时间（分钟）}
translate Z FICSIncrement {增量（秒）}
translate Z FICSRatedGame {计分对局}
translate Z FICSAutoColour {自动选择颜色}
translate Z FICSManualConfirm {手动确认}
translate Z FICSFilterFormula {使用公式过滤}
translate Z FICSIssueSeek {发布对局请求}
translate Z FICSAccept {接受}
translate Z FICSDecline {拒绝}
translate Z FICSColour {颜色}
translate Z FICSSend {发送}
translate Z FICSConnect {连接}
translate Z FICSShouts {公共聊天}
translate Z FICSTells {私聊}
translate Z FICSOpponent {对手信息}
translate Z FICSInfo {信息}
translate Z FICSDraw {提议和棋}
translate Z FICSRematch {再战一局}
translate Z FICSQuit {退出 FICS}
translate Z FICSCensor {屏蔽}


# Correspondence Chess Dialogs:
translate Z CCDlgConfigureWindowTitle {配置通信棋}
translate Z CCDlgCGeneraloptions {常规选项}
translate Z CCDlgDefaultDB {默认数据库：}
translate Z CCDlgInbox {收件箱（路径）：}
translate Z CCDlgOutbox {发件箱（路径）：}
translate Z CCDlgXfcc {Xfcc 配置：}
translate Z CCDlgExternalProtocol {外部协议处理器（例如 Xfcc）}
translate Z CCDlgFetchTool {获取工具：}
translate Z CCDlgSendTool {发送工具：}
translate Z CCDlgEmailCommunication {电子邮件通信}
translate Z CCDlgMailPrg {邮件程序：}
translate Z CCDlgBCCAddr {（抄送/密送）地址：}
translate Z CCDlgMailerMode {模式：}
translate Z CCDlgThunderbirdEg {例如 Thunderbird、Mozilla Mail、Icedove……}
translate Z CCDlgMailUrlEg {例如 Evolution}
translate Z CCDlgClawsEg {例如 Sylpheed Claws}
translate Z CCDlgmailxEg {例如 mailx、mutt、nail……}
translate Z CCDlgAttachementPar {附件参数：}
translate Z CCDlgInternalXfcc {使用内置 Xfcc 支持}
translate Z CCDlgConfirmXfcc {确认着法}
translate Z CCDlgSubjectPar {主题参数：}
translate Z CCDlgDeleteBoxes {清空收件箱/发件箱}
translate Z CCDlgDeleteBoxesText {您确定要清空通信棋的收件箱和发件箱文件夹吗？\n此操作后需重新同步才能显示对局的最新状态。}
translate Z CCDlgConfirmMove {确认着法}
translate Z CCDlgConfirmMoveText {如果确认，以下着法和注释将被发送至服务器：}
translate Z CCDlgDBGameToLong {主线不一致}
translate Z CCDlgDBGameToLongError {您数据库中的主线比收件箱中的对局更长。如果收件箱内容是刚同步后的最新对局，则说明数据库主线被错误地添加了多余着法。

此时请将主线缩短至（最多）第
}

translate Z CCDlgStartEmail {开始新电子邮件对局}
translate Z CCDlgYourName {您的姓名：}
translate Z CCDlgYourMail {您的电子邮件地址：}
translate Z CCDlgOpponentName {对手姓名：}
translate Z CCDlgOpponentMail {对手电子邮件地址：}
translate Z CCDlgGameID {对局 ID（唯一）：}

translate Z CCDlgTitNoOutbox {Scid：通信棋发件箱为空}
translate Z CCDlgTitNoInbox {Scid：通信棋收件箱为空}
translate Z CCDlgTitNoGames {Scid：无通信棋对局}
translate Z CCErrInboxDir {通信棋收件箱目录：}
translate Z CCErrOutboxDir {通信棋发件箱目录：}
translate Z CCErrDirNotUsable {不存在或无法访问！\n请检查并修正设置。}
translate Z CCErrNoGames {不包含任何对局！\n请先获取对局。}

translate Z CCDlgTitNoCCDB {Scid：未打开通信棋数据库}
translate Z CCErrNoCCDB {未打开类型为“通信棋”的数据库。请在使用通信棋功能前先打开一个。}

translate Z CCFetchBtn {从服务器获取对局并处理收件箱}
translate Z CCPrevBtn {上一对局}
translate Z CCNextBtn {下一对局}
translate Z CCSendBtn {发送着法}
translate Z CCEmptyBtn {清空收件箱和发件箱}
translate Z CCHelpBtn {图标与状态指示器帮助。\n通用帮助请按 F1！}

translate Z CCDlgServerName {服务器名称}
translate Z CCDlgLoginName {登录名}
translate Z CCDlgPassword {密码}
translate Z CCDlgURL {Xfcc-URL}
translate Z CCDlgRatingType {等级分类型}

translate Z CCDlgDuplicateGame {对局 ID 不唯一}
translate Z CCDlgDuplicateGameError {该对局在您的数据库中存在多个副本。请删除所有重复项，并压缩数据库（文件 → 维护 → 压缩数据库）。}

translate Z CCDlgSortOption {排序方式：}
translate Z CCDlgListOnlyOwnMove {仅显示轮到我走的对局}
translate Z CCOrderClassicTxt {地点、赛事、轮次、结果、白方、黑方}
translate Z CCOrderMyTimeTxt {我的时钟}
translate Z CCOrderTimePerMoveTxt {至下一时限前每步平均用时}
translate Z CCOrderStartDate {开始日期}
translate Z CCOrderOppTimeTxt {对手时钟}

translate Z CCDlgConfigRelay {观战对局}
translate Z CCDlgConfigRelayHelp {前往 http://www.iccf-webchess.com 的对局页面，选择要观战的对局。当您看到棋盘时，请从浏览器复制该对局的 URL 并粘贴到下方列表中。每行仅允许一个 URL！\n示例：http://www.iccf-webchess.com/MakeAMove.aspx?id=266452}


# Connect Hardware dialoges
translate Z ExtHWConfigConnection {配置外部硬件}
translate Z ExtHWPort {端口}
translate Z ExtHWEngineCmd {引擎命令}
translate Z ExtHWEngineParam {引擎参数}
translate Z ExtHWShowButton {在主窗口中显示按钮}
translate Z ExtHWHardware {硬件}
translate Z ExtHWNovag {Novag Citrine}
translate Z ExtHWInputEngine {输入引擎}
translate Z ExtHWNoBoard {无棋盘}

# Input Engine dialogs
translate Z IEConsole {输入引擎控制台}
translate Z IESending {已发送着法，对象为}
translate Z IESynchronise {同步}
translate Z IERotate {旋转}
translate Z IEUnableToStart {无法启动输入引擎：}
# Calculation of Variations
translate Z DoneWithPosition {已处理该局面}

translate Z Board {棋盘}
translate Z showGameInfo {显示对局信息}
translate Z autoResizeBoard {自动调整棋盘大小}
translate Z DockTop {移至顶部}
translate Z DockBottom {移至底部}
translate Z DockLeft {移至左侧}
translate Z DockRight {移至右侧}
translate Z Undock {取消停靠}

# Switcher window
translate Z ChangeIcon {更改图标}
translate Z More {更多}

# Drag & Drop
translate Z CannotOpenUri {无法打开以下 URI：}
translate Z InvalidUri {拖放内容不是有效的 URI 列表。}
translate Z UriRejected {以下文件被拒绝：}
translate Z UriRejectedDetail {仅支持以下列出的文件类型：}
translate Z EmptyUriList {拖放内容为空。}
translate Z SelectionOwnerDidntRespond {拖放操作超时：选择所有者未响应。}

translate Z PasteAnal {粘贴分析}
translate Z SortOpcodes {排序操作码}
translate Z AddPosition {添加局面}
translate Z FindPos {查找最深的对局局面}
translate Z AnalPosition {分析局面}
translate Z StripOpcodes {清除操作码}
translate Z NoMoves {此局面无后续着法}
translate Z CountBestMoves {统计最佳着法数量}

}
# end of chinese.tcl
