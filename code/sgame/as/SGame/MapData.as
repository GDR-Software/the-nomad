#include "SGame/MapCheckpoint.as"
#include "SGame/MapSpawn.as"
#include "SGame/MapSecret.as"

namespace TheNomad::SGame {
    class MapData {
		MapData() {
		}

		void Init( const string& in mapName, uint nMapLevels ) {
			m_nWidth = 0;
			m_nHeight = 0;
			m_Name = mapName;
		}
		
		private int GetCheckpointIndex( const MapCheckpoint& in cp ) const {
			return m_Checkpoints.Find( cp );
		}

		void Load( int hMap ) {
			uint nCheckpoints, nSpawns, nTiles;
			uint i;
			uvec3 xyz;
			
			TheNomad::GameSystem::SetActiveMap( hMap, nCheckpoints, nSpawns,
				nTiles );

			TheNomad::GameSystem::GetTileData( @m_TileData );
			TheNomad::Engine::Renderer::LoadWorld( m_Name );
			
			//
			// load the checkpoints
			//
			for ( i = 0; i < nCheckpoints; i++ ) {
				MapCheckpoint cp;
				
				TheNomad::GameSystem::GetCheckpointData( xyz, i );
				cp = MapCheckpoint( xyz );
				
				m_Checkpoints.Add( cp );
			}
			
			//
			// load in spawns
			//
			for ( i = 0; i < nSpawns; i++ ) {
				MapSpawn spawn;
				uint id, type, checkpoint;
				
				TheNomad::GameSystem::GetSpawnData( xyz, type, id, i, checkpoint );
				spawn = MapSpawn( xyz, id, TheNomad::GameSystem::EntityType( type ) );

				DebugPrint( "Spawn " + i + " linked to checkpoint " + checkpoint + "\n" );
				@spawn.m_Checkpoint = @m_Checkpoints[ checkpoint ];
				
				m_Spawns.Add( spawn );
			}

			DebugPrint( "Map \"" + m_Name + "\" loaded with " + m_Checkpoints.Count() + " checkpoints, " +
				m_Spawns.Count() + " spawns, and " + m_Secrets.Count + " secrets.\n" );
		}
		
		const array<array<uint>>@ GetTiles() const {
			return @m_TileData;
		}
		array<array<uint>>@ GetTiles() {
			return @m_TileData;
		}
		
		uint NumLevels() const {
			return m_TileData.Count();
		}

		uint GetLevel( const TheNomad::GameSystem::BBox& in bounds ) const {
			const uint level = uint( floor( bounds.m_nMins.z + bounds.m_nMaxs.z ) );
			return level;
		}
		uint GetTile( const vec3& in origin, const TheNomad::GameSystem::BBox& in bounds ) const {
			return m_TileData[ GetLevel( bounds ) ][ uint( origin.y ) * m_nWidth + uint( origin.x ) ];
		}
		const array<MapSpawn>@ GetSpawns() const {
			return @m_Spawns;
		}
		const array<MapCheckpoint>@ GetCheckpoints() const {
			return @m_Checkpoints;
		}
		array<MapSpawn>@ GetSpawns() {
			return @m_Spawns;
		}
		array<MapCheckpoint>@ GetCheckpoints() {
			return @m_Checkpoints;
		}
		int GetWidth() const {
			return m_nWidth;
		}
		int GetHeight() const {
			return m_nHeight;
		}
		
		private string m_Name;
		private array<MapSecret> m_Secrets;
		private array<MapSpawn> m_Spawns;
		private array<MapCheckpoint> m_Checkpoints;
		private array<array<uint>> m_TileData;
		private int m_nWidth = 0;
		private int m_nHeight = 0;
	};
};