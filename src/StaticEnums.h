#pragma once
namespace StaticEnums
{
  enum class Status
  {
    None = 1 << 0,
    LoggedIn = 1 << 1,
    ChampionSelect = 1 << 2,
    ConnectingToGame = 1 << 3,
    InGame = 1 << 4,
    EndOfGame = 1 << 5,
    Reconnecting = 1 << 6,
  };

  enum class Queue
  {
    CustomGame = 0,
    SR_OneForAll = 70,
    HA_SnowdownShowdown_1x1 = 72,
    HA_SnowdownShowdown_2x2 = 73,
    SR_Hexakill = 75,
    SR_URF = 76,
    HA_OneForAll_MirrorMode = 78,
    SR_COOPvsAI_URF = 83,
    TT_Hexakill = 98,
    BB_ARAM = 100,
    SR_Nemesis = 310,
    SR_BlackMarketBrawlers = 313,
    CS_DefinitelyNotDominion = 317,
    SR_AllRandom = 325,
    SR_DraftPick = 400,
    SR_RankedSolo = 420,
    SR_BlindPick = 430,
    SR_RankedFlex = 440,
    HA_ARAM = 450,
    TT_BlindPick = 460,
    TT_RankedFlex = 470,
    SR_BloodHuntAssassin = 600,
    CR_DarkStarSingularity = 610,
    TT_COOPvsAI_Intermediate = 800,
    TT_COOPvsAI_Intro = 810,
    TT_COOPvsAI_Beginner = 820,
    SR_COOPvsAI_Intro = 830,
    SR_COOPvsAI_Beginner = 840,
    SR_COOPvsAI_Intermediate = 850,
    SR_ARURF = 900,
    CS_Ascension = 910,
    HA_PoroKing = 920,
    SR_NexusSiege = 940,
    SR_DoomBots_Voting = 950,
    SR_DoomBots_Standard = 960,
    VCP_StarGuardianInvasion_Normal = 980,
    VCP_StarGuardianInvasion_Onslaught = 990,
    OP_Hunters = 1000,
    SR_Snow_ARURF = 1010,
  };

} // namespace StaticEnums
