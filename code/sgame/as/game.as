#include "util/util.as"
#include "entity.as"
#include "level.as"
#include "convar.as"
#include "draw.as"

namespace TheNomad::GameSystem {
	interface GameObject {
		void OnLoad();
		void OnSave() const;
		void OnRunTic();
		void OnLevelStart();
		void OnLevelEnd();
		void OnConsoleCommand();
		const string& GetName() const;
	};

	array<GameObject@> GameSystems;

	GameObject@ AddSystem( GameObject@ SystemHandle ) {
		ConsolePrint( "Added GameObject System \"" + SystemHandle.GetName() + "\"\n" );
		GameSystems.push_back( SystemHandle );
		return SystemHandle;
	}

	class SaveSection {
		SaveSection( const string& in name ) {
			TheNomad::GameSystem::BeginSaveSection( name );
		}
		~SaveSection() {
			TheNomad::GameSystem::EndSaveSection();
		}
	
		void SaveIntArray( const string& in name, const array<int>& in value ) const {
			SaveArray( name, value );
		}
		void SaveStringArray( const string& in name, const array<string>& in value ) const {
			SaveArray( name, value );
		}
		void SaveString( const string& in name, const string& in value ) const {
			TheNomad::GameSystem::SaveString( name, value );
		}
		void SaveChar( const string& in name, int8 value ) const {
			TheNomad::GameSystem::SaveInt8( name, value );
		}
		void SaveShort( const string& in name, int16 value ) const {
			TheNomad::GameSystem::SaveInt16( name, value );
		}
		void SaveInt( const string& in name, int32 value ) const {
			TheNomad::GameSystem::SaveInt32( name, value );
		}
		void SaveLong( const string& in name, int64 value ) const {
			TheNomad::GameSystem::SaveInt64( name, value );
		}
		void SaveByte( const string& in name, uint8 value ) const {
			TheNomad::GameSystem::SaveInt8( name, value );
		}
		void SaveUShort( const string& in name, uint16 value ) const {
			TheNomad::GameSystem::SaveUInt16( name, value );
		}
		void SaveUInt( const string& in name, uint32 value ) const {
			TheNomad::GameSystem::SaveUInt32( name, value );
		}
		void SaveULong( const string& in name, uint64 value ) const {
			TheNomad::GameSystem::SaveUInt64( name, value );
		}
		void SaveInt8( const string& in name, int8 value ) const {
			TheNomad::GameSystem::SaveInt8( name, value );
		}
		void SaveInt16( const string& in name, int16 value ) const {
			TheNomad::GameSystem::SaveInt16( name, value );
		}
		void SaveInt32( const string& in name, int32 value ) const {
			TheNomad::GameSystem::SaveInt32( name, value );
		}
		void SaveInt64( const string& in name, int64 value ) const {
			TheNomad::GameSystem::SaveInt64( name, value );
		}
		void SaveUInt8( const string& in name, uint8 value ) const {
			TheNomad::GameSystem::SaveUInt8( name, value );
		}
		void SaveUInt16( const string& in name, uint16 value ) const {
			TheNomad::GameSystem::SaveUInt16( name, value );
		}
		void SaveUInt32( const string& in name, uint32 value ) const {
			TheNomad::GameSystem::SaveUInt32( name, value );
		}
		void SaveUInt64( const string& in name, uint64 value ) const {
			TheNomad::GameSystem::SaveUInt64( name, value );
		}
	};
	
	class CampaignManager : GameObject {
		CampaignManager() {
			m_nGameMsec = 0;
			m_nDeltaTics = 0;
		}
		
		void OnLoad() {
			int hSection;
			int numEntities;
			hSection = FindSaveSection( GetName() );
			if ( hSection == FS_INVALID_HANDLE ) {
				return;
			}
		}
		void OnConsoleCommand() {
		}
		void OnSave() const {
			BeginSaveSection( GetName() );
			
//			SaveArray( "soundBits", m_SoundBits );
//			SaveInt( "difficulty", m_Difficulty );
			
			EndSaveSection();
		}
		void OnRunTic() {
		}
		void OnLevelStart() {
		}
		void OnLevelEnd() {
		}
		const string& GetName() const {
			return "CampaignManager";
		}
		
		uint GetDeltaTics() const {
			return m_nDeltaTics;
		}
		uint GetGameTic() const {
			return m_nGameTic;
		}
		uint GetGameMsec() const {
			return m_nGameMsec;
		}
		void SetMsec( uint msec ) {
			m_nDeltaTics = msec - m_nGameMsec;
			m_nGameMsec = msec;
		}
		
		private uint m_nDeltaTics;
		private uint m_nGameMsec;
		private uint m_nGameTic;
	};

	class RayCast {
		RayCast() {
		}

		vec3 m_Start = vec3( 0.0f );
		vec3 m_End = vec3( 0.0f );
		vec3 m_Origin = vec3( 0.0f );
	    uint32 m_nEntityNumber = 0;
		float m_nLength = 0.0f;
		float m_nAngle = 0.0f;
	    uint32 m_Flags = 0; // unused for now
	};

	CampaignManager@ GameManager;
};