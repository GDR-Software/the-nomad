#include "GameSystem/GameObject.as"
#include "GameSystem/Constants.as"
#include "GameSystem/SaveSystem/LoadSection.as"
#include "GameSystem/SaveSystem/SaveSection.as"
#include "GameSystem/SaveSystem/ObjectDataSync.as"

namespace TheNomad::GameSystem {
    class CampaignManager : GameObject {
		CampaignManager() {
		}

		void OnInit() {
			m_nGameMsec = 0;
			m_nDeltaTics = 0;

			GetGPUGameConfig( m_GPUConfig );

			ConsolePrint( "Width: " + m_GPUConfig.screenWidth + "\n" );
		}
		void OnShutdown() {
		}
		
		void OnLoad() {
			int hSection;
			int numEntities;

			hSection = FindSaveSection( GetName() );
			if ( hSection == FS_INVALID_HANDLE ) {
				return;
			}
		}
		bool OnConsoleCommand( const string& in cmd ) {
			return false;
		}
		void OnSave() const {
			BeginSaveSection( GetName() );
			
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
		
		uint GetDeltaMsec() const {
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

		TheNomad::Engine::Renderer::GPUConfig& GetGPUConfig() {
			return m_GPUConfig;
		}

		void SetMousePos( const ivec2& in mousePos ) {
			m_MousePos = mousePos;
		}
		ivec2& GetMousePos() {
			return m_MousePos;
		}
		const ivec2& GetMousePos() const {
			return m_MousePos;
		}
		
		private ivec2 m_MousePos;
		private uint m_nDeltaTics;
		private uint m_nGameMsec;
		private uint m_nGameTic;
		private TheNomad::Engine::Renderer::GPUConfig m_GPUConfig;
	};

    array<GameObject@> GameSystems;

	GameObject@ AddSystem( GameObject@ SystemHandle ) {
		ConsolePrint( "Added GameObject System \"" + SystemHandle.GetName() + "\"\n" );
		SystemHandle.OnInit();
		GameSystems.Add( @SystemHandle );
		return SystemHandle;
	}

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