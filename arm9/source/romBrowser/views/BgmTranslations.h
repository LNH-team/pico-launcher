#pragma once
#include <string.h>

/// BGM 文件名到中文显示名的映射表
struct BgmTranslation
{
    const char* key;        // 去掉前缀和后缀后的英文名（下划线版本）
    const char16_t* zhName; // 中文名
};

static const BgmTranslation kBgmTranslations[] =
{
    // 3DS
    { "3DS_Menu", u"3DS 菜单" },
    { "3DS_Menu_No_Startup_Sound", u"3DS 菜单(无启动音)" },
    { "Activity_Log", u"活动记录" },
    { "Downloads_Trailer", u"下载预告" },
    { "Elegance_Hanafuda_theme", u"花札主题" },
    { "Famicom_Memories_Custom_Menu_theme", u"FC 回忆主题" },
    { "Find_Mii_Battle_Results", u"擦身而过 战斗结果" },
    { "Find_Mii_Battle", u"擦身而过 战斗" },
    { "Find_Mii_King_Captured", u"擦身而过 国王被捕" },
    { "Find_Mii_Prepare_for_Battle", u"擦身而过 备战" },
    { "Find_Mii_II_-_Save_the_World,_Heroes!", u"擦身而过II 拯救世界" },
    { "Find_Mii", u"擦身而过" },
    { "Flower_Garden_Custom_Menu_Theme", u"花园主题" },
    { "Friend_List", u"好友列表" },
    { "Gourmet_Race_Brass_Band_Version_Custom_Menu_Theme", u"美食大赛(铜管版)" },
    { "Green_Greens_and_Peanut_Plains_Acoustic_Version_Custom_Menu_Theme", u"绿茵草地(原声版)" },
    { "Green_Greens_Pop_Version_Custom_Menu_Theme", u"绿茵草地(流行版)" },
    { "Ground_Theme_Custom_Menu_Theme", u"地上主题" },
    { "Ground_Theme_Holiday_Version_Custom_Menu_Theme", u"地上主题(假日版)" },
    { "Ground_Theme_Japanese_Version_Custom_Menu_Theme", u"地上主题(日版)" },
    { "Health_and_Safety_Information", u"健康与安全" },
    { "Hyrule_Main_Theme_Custom_Menu_Theme", u"海拉鲁主题" },
    { "Internet_Settings", u"网络设置" },
    { "Nintendo_Network_ID_Settings", u"NNID 设置" },
    { "Nintendo_Video", u"任天堂视频" },
    { "Nintendo_Zone_Ambience", u"任天堂Zone" },
    { "Nintendo_eShop_Alternate_Theme", u"eShop 主题" },
    { "Notifications", u"通知" },
    { "Photo_Print_Channel", u"照片打印" },
    { "Puzzle_Swap_Progress", u"拼图交换 进度" },
    { "Puzzle_Swap", u"拼图交换" },
    { "Secret_Level_Custom_Menu_Theme", u"秘密关卡主题" },
    { "SpotPass_TV", u"擦身通信TV" },
    { "StreetPass_Main_Theme_Medley", u"擦身通信主题混音" },
    { "StreetPass_Mii_Plaza_Many_Miis", u"Mii广场(很多Mii)" },
    { "StreetPass_Mii_Plaza_Entrance", u"Mii广场 入口" },
    { "StreetPass_Mii_Plaza_Theme_1", u"Mii广场 主题1" },
    { "Successive_Hardware_Japan_Europe-only_Custom_Menu_theme", u"历代主机主题" },
    { "Super_Bell_Hill_Custom_Menu_Theme", u"超级铃铛山主题" },
    { "System_Settings", u"系统设置" },
    { "Underground_Custom_Menu_Theme", u"地下主题" },
    { "eShop_Options", u"eShop 选项" },
    { "eShop_Rating", u"eShop 评分" },
    { "eShop_Starting_Up", u"eShop 启动" },
    { "eShop_Wishlist", u"eShop 愿望单" },

    // DSi
    { "Camera_Album", u"相机 相册" },
    { "Camera_Edit_Photos", u"相机 编辑" },
    { "Camera_Main_Menu", u"相机 主菜单" },
    { "Camera_Options", u"相机 选项" },
    { "Camera_Tutorial", u"相机 教程" },
    { "Menu", u"菜单" },
    { "Slideshow_Memories", u"幻灯片 回忆" },
    { "Slideshow_Showtime", u"幻灯片 放映" },
    { "Slideshow_Sparkle", u"幻灯片 闪耀" },
    { "Slideshow_Whistle", u"幻灯片 口哨" },
    // { "System_Settings", u"系统设置" }, // 和3DS共用

    // NS2
    { "Initial_Setup", u"初始设置" },

    // PSV
    { "Default_Theme", u"默认主题" },
    // { "Initial_Setup", u"初始设置" }, // 和NS2共用
    { "Introduction_and_Welcome_Video", u"介绍与欢迎" },
    { "New_PSN_Account", u"新建PSN账号" },
    { "PlayStation_Store", u"PlayStation商店" },
    { "PlayStation®_20th_Anniversary_Theme", u"PS 20周年主题" },
    { "Skate_Axis_and_Hello_Face", u"Skate Axis" },
    { "Stitching_Theme", u"缝合主题" },
    { "Welcome_Park__Digit_Chase_and_Snap_+_Slide", u"欢迎公园 数字追逐" },
    { "Welcome_Park__Main_Menu", u"欢迎公园 主菜单" },
    { "Welcome_Park__Sound_Loop_1", u"欢迎公园 循环1" },
    { "Welcome_Park__Sound_Loop_2", u"欢迎公园 循环2" },
    { "Welcome_Park__Sound_Loop_3", u"欢迎公园 循环3" },
    { "near", u"near" },

    // Switch Sports
    { "Title_Screen", u"标题画面" },
    { "Customize_Your_Sportsmate", u"自定义角色" },
    { "Settings", u"设置" },
    { "First_Setup", u"初始设置" },
    { "Ambience__Bowling_Alley", u"氛围 保龄球馆" },
    { "Ambience__Cafe_Buttons", u"氛围 咖啡馆" },
    { "Select_a_Sport", u"选择运动" },
    { "Tutorial", u"教程" },
    { "Finding_a_Match", u"寻找对手" },
    { "Game_Start", u"比赛开始" },
    { "Chambara__Preparing", u"击剑 准备" },
    { "Chambara", u"击剑" },
    { "Chambara__Replay", u"击剑 回放" },
    { "Results__Round_Over", u"回合结束" },
    { "Chambara__Preparing_Final_Round", u"击剑 决赛准备" },
    { "Chambara__Final_Round", u"击剑 决赛" },
    { "Results__Final_Round", u"决赛结果" },
    { "Results__Victory", u"胜利" },
    { "Tennis", u"网球" },
    { "Tennis__Replay", u"网球 回放" },
    { "Game_Start__Survival_Bowling_Player_Rankings", u"生存保龄 排名" },
    { "Game_Start__Survival_Bowling", u"生存保龄 开始" },
    { "Bowling__Start_Round_1_and_2", u"保龄球 第1-2轮" },
    { "Bowling__Round_1", u"保龄球 第1轮" },
    { "Bowling__Round_1_Spectating", u"保龄球 第1轮观战" },
    { "Bowling__Round_2", u"保龄球 第2轮" },
    { "Bowling__Round_2_Spectating", u"保龄球 第2轮观战" },
    { "Bowling__Start_Round_3", u"保龄球 第3轮" },
    { "Bowling__Round_3", u"保龄球 第3轮" },
    { "Bowling__Round_3_Spectating", u"保龄球 第3轮观战" },
    { "Bowling__Start_Round_4", u"保龄球 第4轮" },
    { "Bowling__Round_4", u"保龄球 第4轮" },
    { "Bowling__Round_4_Spectating", u"保龄球 第4轮观战" },

    // Wii
    { "Nintendo_Wii_Music__Everybody_Votes_Channel_Results", u"大家投票 结果" },
    { "Nintendo_Wii_Music__Everybody_Votes_Channel_Suggest_a_question", u"大家投票 提问" },
    { "Nintendo_Wii_Music__Everybody_Votes_Channel", u"大家投票频道" },
    { "Nintendo_Wii_Music__Menu", u"Wii 菜单" },
    { "Nintendo_Wii_Music__Mii_Plaza", u"Mii 广场" },
    { "Nintendo_Wii_Music__News_Channel_Night_Time", u"新闻频道(夜间)" },
    { "Nintendo_Wii_Music__News_Channel", u"新闻频道" },
    { "Nintendo_Wii_Music__Photo_Channel_Doodle", u"照片频道 涂鸦" },
    { "Nintendo_Wii_Music__Photo_Channel_Puzzle_Theme", u"照片频道 拼图" },
    { "Nintendo_Wii_Music__Photo_Channel__Slideshow_Beautiful", u"幻灯片 优美" },
    { "Nintendo_Wii_Music__Photo_Channel__Slideshow_Bright", u"幻灯片 明亮" },
    { "Nintendo_Wii_Music__Photo_Channel__Slideshow_Calm", u"幻灯片 平静" },
    { "Nintendo_Wii_Music__Photo_Channel__Slideshow_Fun", u"幻灯片 欢乐" },
    { "Nintendo_Wii_Music__Photo_Channel__Slideshow_Nostalgic", u"幻灯片 怀旧" },
    { "Nintendo_Wii_Music__Photo_Channel__Slideshow_Scenic", u"幻灯片 风景" },
    { "Nintendo_Wii_Music__Photo_Channel", u"照片频道" },
    { "Nintendo_Wii_Music__Shop_Channel", u"Wii商店频道" },
    { "Nintendo_Wii_Music__System_Transfer", u"系统转移" },
    { "Nintendo_Wii_Music__Weather_Channel_Globe_Night_time", u"天气频道(夜间)" },
    { "Nintendo_Wii_Music__Weather_Channel_Globe", u"天气频道 地球" },
    { "Nintendo_Wii_Music__Weather_Channel", u"天气频道" },

    // Wii U
    { "Wii_U_Menu", u"Wii U 菜单" },
    { "Mii_Maker__Boot_Up_TV", u"Mii工坊 启动(TV)" },
    { "Mii_Maker__Boot_Up_Gamepad", u"Mii工坊 启动(手柄)" },
    { "Mii_Maker__Menu_TV", u"Mii工坊 菜单(TV)" },
    { "Mii_Maker__Menu_Gamepad", u"Mii工坊 菜单(手柄)" },
    { "Mii_Maker_TV", u"Mii工坊(TV)" },
    { "Mii_Maker_Gamepad", u"Mii工坊(手柄)" },
    { "Mii_Maker__Editing_a_Mii_TV", u"Mii工坊 编辑(TV)" },
    { "Mii_Maker__Editing_a_Mii_Gamepad", u"Mii工坊 编辑(手柄)" },
    { "Account_Settings", u"账号设置" },
    { "Daily_Log", u"每日记录" },
    { "Friend_List", u"好友列表" },
    { "Miiverse__Loading", u"Miiverse 加载" },
    { "Miiverse__Welcome_to_Miiverse", u"Miiverse 欢迎" },
    { "Miiverse__Welcome_to_Miiverse_Jingle", u"Miiverse 欢迎铃声" },
    { "Miiverse__Menu", u"Miiverse 菜单" },
    { "Miiverse__Settings", u"Miiverse 设置" },
    { "Wii_U_Chat__Boot_Up", u"Wii U聊天 启动" },
    { "Wii_U_Chat__Lobby_TV", u"Wii U聊天 大厅(TV)" },
    { "Wii_U_Chat__Lobby_Gamepad", u"Wii U聊天 大厅(手柄)" },
    { "Wii_U_Chat__Calling_a_Friend", u"Wii U聊天 呼叫" },
    { "Wii_U_Chat__Incoming_Call", u"Wii U聊天 来电" },
    { "Wii_U_Chat__Connecting", u"Wii U聊天 连接中" },
    { "Wii_U_Chat__Goodbye", u"Wii U聊天 再见" },
    { "Nintendo_TVii__Loading", u"Nintendo TVii 加载" },
    { "Nintendo_TVii__Boot_Up", u"Nintendo TVii 启动" },
    { "eShop__Loading", u"eShop 加载" },
    { "eShop__Menu", u"eShop 菜单" },
    { "eShop__Settings", u"eShop 设置" },
    { "Notifications_Menu", u"通知菜单" },
};

static constexpr int kBgmTranslationCount = sizeof(kBgmTranslations) / sizeof(kBgmTranslations[0]);

/// 查找 BGM 中文翻译。key 是去掉平台前缀后的文件名（不含 .bcstm）。
/// 返回中文名，找不到返回 nullptr。
inline const char16_t* FindBgmChineseTranslation(const char* strippedName)
{
    if (!strippedName) return nullptr;
    for (int i = 0; i < kBgmTranslationCount; i++)
    {
        if (!strcmp(kBgmTranslations[i].key, strippedName))
            return kBgmTranslations[i].zhName;
    }
    return nullptr;
}
