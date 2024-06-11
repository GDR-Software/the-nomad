namespace TheNomad::SGame {
    class ItemObject : EntityObject {
		ItemObject() {
		}

		void Pickup( EntityObject@ ent ) {
			switch ( ent.GetType() ) {
			case GameSystem::EntityType::Item:
			case GameSystem::EntityType::Weapon:
				GameError( "ItemObject::Pickup: invalid pickup entity (item/weapon)" );
			default:
				break;
			};

			@m_Owner = @ent;
			DebugPrint( "Item " + m_Link.m_nEntityNumber + " now owned by " + ent.GetEntityNum() + ".\n" );

			// TODO: in the future, mobs will be able to pick up items just like the player
			m_Info.pickupSfx.Play();
		}
		void Use() {
			m_Info.useSfx.Play();
			Engine::CmdExecuteCommand( m_Info.effect + " " + m_Owner.GetEntityNum() );
		}

		void Save( const TheNomad::GameSystem::SaveSystem::SaveSection& in section ) const {
			SaveBase( section );
			section.SaveBool( "hasOwner", m_Owner !is null );
			if ( @m_Owner !is null ) {
				section.SaveUInt( "owner", m_Owner.GetEntityNum() );
			}
		}
		bool Load( const TheNomad::GameSystem::SaveSystem::LoadSection& in section ) {
			LoadBase( section );
			if ( section.LoadBool( "hasOwner" ) ) {
				@m_Owner = @EntityManager.GetEntityForNum( section.LoadUInt( "owner" ) );
			}

			Spawn( m_Link.m_nEntityId, m_Link.m_Origin );

            return true;
		}

		void Think() const {
			if ( @m_Owner !is null ) {
				return;
			}

			TheNomad::Engine::Renderer::AddSpriteToScene( m_Link.m_Origin, m_hShader, 0, true );
		}
		void Spawn( uint id, const vec3& in origin ) override {
			@m_Info = @InfoSystem::InfoManager.GetItemInfo( id );
			if ( @m_Info is null ) {
				GameError( "ItemObject::Spawn: invalid item id " + id );
			}

			m_hShader = TheNomad::Engine::ResourceCache.GetShader( m_Info.iconShader );
			m_Link.m_nEntityId = id;
			m_Link.m_Origin = origin;
			m_Link.m_Bounds.m_nWidth = m_Info.width;
			m_Link.m_Bounds.m_nHeight = m_Info.height;
			m_Link.m_Bounds.MakeBounds( origin );
		}

		private InfoSystem::ItemInfo@ m_Info = null;
		private EntityObject@ m_Owner = null; // for applying effects
		private int m_hShader = FS_INVALID_HANDLE;
	};
};