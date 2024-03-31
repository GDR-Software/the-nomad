namespace TheNomad::SGame {
    class LevelRankData {
		LevelRankData() {
		}
		
		LevelRank rank = LevelRank::RankS;
		uint minStyle = 0;
		uint minKills = 0;
		uint minTime = 0;
		uint maxDeaths = 0;
		uint maxCollateral = 0;
		bool requiresClean = true; // no warcrimes, no innocent deaths, etc. required for perfect score
	};
};