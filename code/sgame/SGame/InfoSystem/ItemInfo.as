#include "SGame/InfoSystem/InfoDataManager.as"

namespace TheNomad::SGame::InfoSystem {
    class ItemInfo : InfoLoader {
		ItemInfo() {
		}
		
		bool Load( json@ json ) {
			string str;

			if ( !json.get( "Name", name ) ) {
				ConsoleWarning( "invalid item info, missing variable 'Name'\n" );
				return false;
			}
			if ( !json.get( "Id", str ) ) {
				ConsoleWarning( "invalid item info, missing variable 'Id'\n" );
				return false;
			} else {
				if ( !InfoManager.GetItemTypes().TryGetValue( str, type ) ) {
					GameError( "invalid item info, Type \"" + str + "\" wasn't found" );
				}
			}
			if ( !json.get( "Effect", effect ) ) {
				ConsoleWarning( "invalid item info, missing variable 'Effect'\n" );
				return false;
			}
			json.get( "Cost", cost );
			if ( !json.get( "Icon", iconShader ) ) {
				ConsoleWarning( "invalid item info, missing variable 'Icon'\n" );
				return false;
			} else {
				TheNomad::Engine::ResourceCache.GetShader( iconShader );
			}
			if ( !json.get( "SpriteOffsetX", spriteOffsetX ) ) {
				ConsoleWarning( "invalid item info, missing variable 'SpriteOffsetX'\n" );
				return false;
			}
			if ( !json.get( "SpriteOffsetY", spriteOffsetY ) ) {
				ConsoleWarning( "invalid item info, missing variable 'SpriteOffsetY'\n" );
				return false;
			}
			if ( !json.get( "PickupSfx", str ) ) {
				ConsoleWarning( "invalid item info, missing variable 'PickupSfx'\n" );
				return false;
			} else {
				pickupSfx.Set( str );
			}
			if ( !json.get( "UseSfx", str ) ) {
				ConsoleWarning( "invalid item info, missing variable 'UseSfx'\n" );
				return false;
			} else {
				useSfx.Set( str );
			}
			if ( !json.get( "Width", width ) ) {
				ConsoleWarning( "invalid item info, missing variable 'Width'\n" );
				return false;
			}
			if ( !json.get( "Height", height ) ) {
				ConsoleWarning( "invalid item info, missing variable 'Height'\n" );
				return false;
			}

			TheNomad::GameSystem::GetString( name + "_DESC", description );

			ConsolePrint( "Loaded item info for '" + name + "'\n" );

			return true;
		}

		string name;
		string description;
		string effect;
		string iconShader;
		uint type = 0;
		int cost = 0;
		uint spriteOffsetX = 0;
		uint spriteOffsetY = 0;
		uint maxStackSize = 0;
		float width = 0.0f;
		float height = 0.0f;

		TheNomad::Engine::SoundSystem::SoundEffect pickupSfx;
		TheNomad::Engine::SoundSystem::SoundEffect useSfx;
	};
};