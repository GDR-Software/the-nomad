#include "SGame/EntityObject.as"
#include "SGame/InfoSystem/AttackInfo.as"
#include "SGame/InfoSystem/MobInfo.as"
#include "SGame/InfoSystem/ItemInfo.as"
#include "SGame/InfoSystem/InfoDataManager.as"
#include "SGame/ItemObject.as"
#include "SGame/WeaponObject.as"
#include "SGame/MobObject.as"
#include "SGame/PlayrObject.as"

namespace TheNomad::SGame {
    enum CauseOfDeath {
		Cod_Unknown,
		Cod_Bullet,
		Cod_Imploded,
		Cod_Exploded,
		Cod_Suicide,
		Cod_Telefrag,
		Cod_Punch,
		Cod_Falling
	};
	
	enum AttackEffect {
		Effect_Knockback = 0,
		Effect_Stun,
		Effect_Bleed,
		Effect_Blind,
		
		None
	};
	
	enum EntityFlags {
		// DUH.
		Dead      = 0x00000001,
		// can't take damage
		Invul     = 0x00000002,
		// doesn't get drawn
		Invis     = 0x00000004,
		// doesn't get respawned in Nomad or greater difficulties
		PermaDead = 0x00000008,
		// will it bleed?
		Killable  = 0x00000010,

		None      = 0x00000000
	};

	EntitySystem@ EntityManager;
	PlayrObject@ GetPlayerObject() {
		return @EntityManager.GetPlayerObject();
	}

    class EntitySystem : TheNomad::GameSystem::GameObject {
		EntitySystem() {
		}
		
		void OnInit() {
			Engine::CommandSystem::CmdManager.AddCommand( Engine::CommandSystem::CommandFunc( @this.Effect_EntityStun_f, 
				"sgame.effect_entity_stun" ) );
			Engine::CommandSystem::CmdManager.AddCommand( Engine::CommandSystem::CommandFunc( @this.Effect_EntityBleed_f, 
				"sgame.effect_entity_bleed" ) );
			Engine::CommandSystem::CmdManager.AddCommand( Engine::CommandSystem::CommandFunc( @this.Effect_EntityKnockback_f, 
				"sgame.effect_entity_knockback" ) );
			Engine::CommandSystem::CmdManager.AddCommand( Engine::CommandSystem::CommandFunc( @this.Effect_EntityImmolate_f, 
				"sgame.effect_entity_immolate" ) );
			Engine::CommandSystem::CmdManager.AddCommand( Engine::CommandSystem::CommandFunc( @this.PrintPlayerState_f, 
				"sgame.print_player_state" ) );
		}
		void OnShutdown() {
		}

		void DrawEntity( const EntityObject@ ent ) {
		//	const SGame::SpriteSheet@ sheet;
			switch ( ent.GetType() ) {
			case TheNomad::GameSystem::EntityType::Playr: {
		//		cast<PlayrObject>( @ent ).DrawLegs();
				break; }
			case TheNomad::GameSystem::EntityType::Mob: {

				break; }
			case TheNomad::GameSystem::EntityType::Bot: {

				break; }
			case TheNomad::GameSystem::EntityType::Item: {

				break; }
			case TheNomad::GameSystem::EntityType::Weapon: {
		//		@sheet = ent.GetSpriteSheet();
		//		Engine::Renderer::AddSpriteToScene( ent.GetOrigin(), sheet.GetShader(),
		//			ent.GetState().SpriteOffset() );
				break; }
			case TheNomad::GameSystem::EntityType::Wall: {
				break; } // engine should handle this
			default:
				GameError( "DrawEntity: bad type" );
				break;
			};
		}
		
		const string& GetName() const {
			return "EntityManager";
		}
		void OnLoad() {
			EntityObject@ ent;
			TheNomad::GameSystem::SaveSystem::LoadSection section( GetName() );

			if ( !section.Found() ) {
				ConsoleWarning( "EntitySystem::OnLoad: no save section for entity data found\n" );
				return;
			}

			const uint numEntities = section.LoadUInt( "NumEntities" );

			DebugPrint( "Loading entity data from save file...\n" );

			if ( numEntities == 0 ) {
				return;
			}

			m_EntityList.Resize( numEntities );

			for ( uint i = 0; i < numEntities; i++ ) {
				TheNomad::GameSystem::SaveSystem::LoadSection data( "EntityData_" + i );
				if ( !data.Found() ) {
					GameError( "EntitySystem::OnLoad: save section \"EntityData_" + i + "\" not found" );
				}

				// be more efficient with our memory
				if ( @m_EntityList[i] is null ) {
					@ent = EntityObject();
					@m_EntityList[i] = @ent;
				} else {
					@ent = @m_EntityList[i];
				}

				@ent = EntityObject();

				switch ( TheNomad::GameSystem::EntityType( data.LoadUInt( "type" ) ) ) {
				case TheNomad::GameSystem::EntityType::Playr: {
					if ( !cast<PlayrObject@>( @ent ).Load( data ) ) {
						GameError( "EntitySystem::OnLoad: failed to load player data" );
					}
					break; }
				case TheNomad::GameSystem::EntityType::Mob: {
					if ( !cast<MobObject@>( @ent ).Load( data ) ) {
						GameError( "EntitySystem::OnLoad: failed to load mob data" );
					}
					break; }
				case TheNomad::GameSystem::EntityType::Item: {
					if ( !cast<ItemObject@>( @ent ).Load( data ) ) {
						GameError( "EntitySystem::OnLoad: failed to load item data" );
					}
					break; }
				default:
					GameError( "EntityObject::OnLoad: invalid entity type" );
					break;
				};
			}

			DebugPrint( "Loaded " + m_EntityList.Count() + " entities.\n" );
		}
		void OnSave() const {
			TheNomad::GameSystem::SaveSystem::SaveSection section( GetName() );

			DebugPrint( "Saving entity data...\n" );

			for ( uint i = 0; i < m_EntityList.Count(); i++ ) {
				TheNomad::GameSystem::SaveSystem::SaveSection section( "EntityData_" + i );
				m_EntityList[i].OnSave();
			}
		}
		void OnRunTic() {
			EntityObject@ ent;
			
			for ( uint i = 0; i < m_EntityList.Count(); i++ ) {
				@ent = @m_EntityList[i];

				if ( ent.CheckFlags( EntityFlags::Dead ) ) {
					if ( ( sgame_Difficulty.GetInt() > TheNomad::GameSystem::GameDifficulty::Hard
						&& ent.GetType() == TheNomad::GameSystem::EntityType::Mob )
						|| ent.GetType() == TheNomad::GameSystem::EntityType::Playr )
					{
						DeadThink( @ent );
					}
					else {
						// remove it
						if ( ent.GetType() == TheNomad::GameSystem::EntityType::Item ) {
							// unlink the item
							RemoveItem( cast<ItemObject@>( @ent ) );
						}
						m_EntityList.RemoveAt( i );
					}
					continue;
				}

				if ( @ent.GetState() is null ) {
					DebugPrint( "EntitySystem::OnRunTic: null entity state\n" );
					continue;
				} else {
					ent.SetState( @ent.GetState().Run() );
				}

				switch ( ent.GetType() ) {
				case TheNomad::GameSystem::EntityType::Playr: {
					cast<PlayrObject@>( @ent ).Think();
					break; }
				case TheNomad::GameSystem::EntityType::Mob: {
					cast<MobObject@>( @ent ).Think();
					break; }
				case TheNomad::GameSystem::EntityType::Bot:
					break;
				case TheNomad::GameSystem::EntityType::Item:
					cast<ItemObject@>( @ent ).Think();
					break;
				case TheNomad::GameSystem::EntityType::Weapon:
					break; // thinking happens in owner entity
				case TheNomad::GameSystem::EntityType::Wall:
					GameError( "WALLS DON'T THINK, THEY ACT" );
					break;
				default:
					GameError( "EntityManager::OnRunTic: invalid entity type " + formatUInt( uint( ent.GetType() ) ) );
				};

				// update engine data
				ent.GetLink().Update();
				
				// draw entity
				ent.Draw();
				
//				if ( m_EntityList[i].GetState().Done() ) {
//					m_EntityList[i].SetState( m_EntityList[i].GetState().Cycle() );
//					continue;
//				}
			}
		}
		void OnLevelStart() {
			DebugPrint( "Spawning entities...\n" );

			@m_ActiveEntities.prev =
			@m_ActiveEntities.next =
				@m_ActiveEntities;

			const array<MapSpawn>@ spawns = @LevelManager.GetMapData().GetSpawns();
			for ( uint i = 0; i < spawns.Count(); i++ ) {
				Spawn( spawns[i].m_nEntityType, spawns[i].m_nEntityId,
					vec3( spawns[i].m_Origin.x, spawns[i].m_Origin.y, spawns[i].m_Origin.z ) );
			}

			DebugPrint( "Found " + m_EntityList.Count() + " entity spawns.\n" );
		}
		void OnLevelEnd() {
			// clear all level locals
			m_EntityList.Clear();
			@m_PlayrObject = null;
		}
		bool OnConsoleCommand( const string& in cmd ) {
			if ( Util::StrICmp( cmd, "sgame.list_items" ) == 0 ) {
				ListActiveItems();
			}
			else if ( Util::StrICmp( cmd, "sgame.print_player_state" ) == 0 ) {
				PrintPlayerState();
			}

			return false;
		}
		
		private EntityObject@ AllocEntity( TheNomad::GameSystem::EntityType type, uint id, const vec3& in origin ) {
			EntityObject@ ent;

			switch ( type ) {
			case TheNomad::GameSystem::EntityType::Playr:
				@ent = PlayrObject();
				ent.Init( type, id, origin );
				cast<PlayrObject@>( @ent ).Spawn( id, origin );
				break;
			case TheNomad::GameSystem::EntityType::Mob:
				@ent = MobObject();
				cast<MobObject@>( @ent ).Spawn( id, origin );
				break;
			case TheNomad::GameSystem::EntityType::Bot:
				break;
			case TheNomad::GameSystem::EntityType::Item:
				@ent = ItemObject();
				cast<ItemObject@>( @ent ).Spawn( id, origin );
				break;
			case TheNomad::GameSystem::EntityType::Weapon:
				@ent = WeaponObject();
				cast<WeaponObject@>( @ent ).Spawn( id, origin );
				break;
			case TheNomad::GameSystem::EntityType::Wall:
				GameError( "WALLS DON'T THINK, THEY ACT" );
				break;
			default:
				GameError( "EntityManager::Spawn: invalid entity type " + id );
			};

			@m_ActiveEntities.prev.next = @ent;
			@ent.next = @m_ActiveEntities;
			@ent.prev = @m_ActiveEntities.prev;

			m_EntityList.Add( @ent );
			
			return @ent;
		}
		
		EntityObject@ Spawn( TheNomad::GameSystem::EntityType type, int id, const vec3& in origin ) {
			return @AllocEntity( type, id, origin );
		}
		
		void DeadThink( EntityObject@ ent ) {
			if ( ent.GetType() == TheNomad::GameSystem::EntityType::Mob ) {
				if ( sgame_Difficulty.GetInt() < uint( TheNomad::GameSystem::GameDifficulty::VeryHard )
					|| sgame_NoRespawningMobs.GetInt() == 1 || ( cast<MobObject>( @ent ).GetMFlags() & InfoSystem::MobFlags::PermaDead ) != 0 )
				{
					return; // no respawning for this one
				} else {
					// TODO: add respawn code here
				}
			} else if ( ent.GetType() == TheNomad::GameSystem::EntityType::Playr ) {
				PlayrObject@ obj;
				
				@obj = cast<PlayrObject>( @ent );
				
				// is hellbreaker available?
				if ( sgame_HellbreakerOn.GetInt() != 0 && Util::IsModuleActive( "hellbreaker" )
					&& sgame_HellbreakerActive.GetInt() == 0 /* ensure there's no recursion */ )
				{
					HellbreakerInit();
					return; // startup the hellbreak
				}
				
				GlobalState = GameState::DeathMenu;
				ent.SetState( StateNum::ST_PLAYR_DEAD ); // play the death animation
			}
		}
		
		const EntityObject@ GetEntityForNum( uint nIndex ) const {
			return @m_EntityList[ nIndex ];
		}
		EntityObject@ GetEntityForNum( uint nIndex ) {
			return @m_EntityList[ nIndex ];
		}
		const array<EntityObject@>@ GetEntities() const {
			return @m_EntityList;
		}
		array<EntityObject@>@ GetEntities() {
			return @m_EntityList;
		}
		EntityObject@ GetActiveEnts() {
			return @m_ActiveEntities;
		}
		uint NumEntities() const {
			return m_EntityList.Count();
		}

		private bool BoundsIntersectLine( const vec3& in start, const vec3& in end, const TheNomad::GameSystem::BBox& in bounds ) {
			float minX = start.x;
			float maxX = end.x;
			
			if ( start.x > end.x ) {
				minX = end.x;
				maxX = start.x;
			}
			if ( maxX > bounds.m_Maxs.x ) {
				maxX = bounds.m_Maxs.x;
			}
			if ( minX < bounds.m_Mins.x ) {
				minX = bounds.m_Mins.x;
			}
			if ( minX > maxX ) {
				return false;
			}

			float minY = start.y;
			float maxY = end.y;

			const float deltaX = end.x - start.x;

			if ( abs( deltaX ) > 0.0000001f ) {
				float a = ( end.y - start.y ) / deltaX;
				float b = start.y - a * start.x;
				minY = a * minX + b;
				maxY = a * maxX + b;
			}

			if ( minY > maxY ) {
				Util::Swap( maxY, maxX );
			}
			if ( maxY > bounds.m_Maxs.y ) {
				maxY = bounds.m_Maxs.y;
			}
			if ( minY < bounds.m_Mins.y ) {
				minY = bounds.m_Mins.y;
			}
			if ( minY > maxY ) {
				return false;
			}
			return true;
		}

		bool EntityIntersectsLine( const vec3& in origin, const vec3& in end ) {
			for ( uint i = 0; i < m_EntityList.Count(); i++ ) {
				if ( BoundsIntersectLine( origin, end, m_EntityList[i].GetBounds() ) ) {
					return true;
				}
			}
			return false;
		}

		void SpawnProjectile( const vec3& in origin, float angle, const InfoSystem::AttackInfo@ info ) {
			EntityObject@ obj = @Spawn( TheNomad::GameSystem::EntityType::Weapon, info.type, origin );
			obj.SetProjectile( true );
		}

		private void GenObituary( EntityObject@ attacker, EntityObject@ target, const InfoSystem::AttackInfo@ info ) {
			if ( target.GetType() != TheNomad::GameSystem::EntityType::Playr ) {
				// unless it's the player, we don't care about the death
				return;
			}
			
			string message = cast<PlayrObject@>( @target ).GetName();
			
			if ( @attacker is null ) {
				if ( target.GetVelocity() > Util::Vec3Origin || target.GetVelocity() > Util::Vec3Origin ) {
					// killed by impact
					if ( target.GetVelocity().z != 0.0f ) {
						message += " thought they could fly";
					}  else {
						message += " was met by the reality of physics";
					}
				} else {
					// player was killed by unknown reasons
					message += " was killed by strange and mysterious forces beyond our control...";
				}
			}
			else {
				// actual death
				
			}
			
			ConsolePrint( message + "\n" );
		}
		
		private void KillEntity( EntityObject@ attacker, EntityObject@ target ) {
			if ( target.GetType() == TheNomad::GameSystem::EntityType::Mob ) {
				MobObject@ mob = cast<MobObject@>( @target );
				
				// respawn mobs on VeryHard
				if ( uint( sgame_Difficulty.GetInt() ) >= uint( TheNomad::GameSystem::GameDifficulty::VeryHard )
					&& sgame_NoRespawningMobs.GetInt() != 1 && ( uint( mob.GetMFlags() ) & InfoSystem::MobFlags::PermaDead ) == 0 )
				{
					// ST_MOB_DEAD only used with respawning mobs
					target.SetState( cast<InfoSystem::MobInfo@>( @target.GetInfo() ).type + StateNum::ST_MOB_DEAD );
				}
				else {
					@target.prev.next = @target.next;
					@target.next.prev = @target.prev;
				}
			}
			else if ( target.GetType() == TheNomad::GameSystem::EntityType::Playr ) {
				target.SetState( StateNum::ST_PLAYR_DEAD );
			}
		}
		
		void KillEntity( EntityObject@ attacker, EntityObject@ target, const InfoSystem::AttackInfo@ info ) {
			GenObituary( @attacker, @target, @info );
			KillEntity( @attacker, @target );
		}
		void ApplyEntityEffect( EntityObject@ attacker, EntityObject@ target, AttackEffect effect ) {
		}
		
		//
		// DamageEntity: entity v entity
		// NOTE: damage is only ever used when calling from a WeaponObject
		//
		void DamageEntity( EntityObject@ attacker, TheNomad::GameSystem::RayCast@ rayCast, float damage = 1.0f ) {
			if ( @rayCast is null ) {
				return;
			} else if ( rayCast.m_nEntityNumber == ENTITYNUM_INVALID || rayCast.m_nEntityNumber == ENTITYNUM_WALL ) {
				return; // got nothing
			}
			
			switch ( attacker.GetType() ) {
			case TheNomad::GameSystem::EntityType::Mob: {
				m_PlayrObject.Damage( cast<MobObject@>( @attacker ).GetCurrentAttack().damage );
				break; }
			case TheNomad::GameSystem::EntityType::Playr: {
				EntityObject@ target = @m_EntityList[ rayCast.m_nEntityNumber ];
				
				if ( target.GetType() == TheNomad::GameSystem::EntityType::Mob ) {
					cast<MobObject@>( @target ).Damage( damage );
				} else if ( target.GetType() == TheNomad::GameSystem::EntityType::Bot ) {
					// TODO: calculate collateral damage here
				}
				// any other entity is just an inanimate object
				// TODO: add explosive objects
				
				break; }
			default:
				GameError( "EntitySystem::Damage: invalid entity type " + uint( attacker.GetType() ) );
			};
		}

		//
		// DamageEntity: mobs v targets
		//
		void DamageEntity( MobObject@ attacker, TheNomad::GameSystem::RayCast@ rayCast, const InfoSystem::AttackInfo@ info ) {
			EntityObject@ target;

			if ( @rayCast is null ) {
				return; // not a hitscan
			}

			@target = @m_EntityList[ rayCast.m_nEntityNumber ];
			if ( target.GetType() == TheNomad::GameSystem::EntityType::Playr ) {
				// check for a parry
				PlayrObject@ p = cast<PlayrObject@>( @target );
				
				if ( p.CheckParry( @attacker ) ) {
					return; // don't deal damage
				}
			}

			target.Damage( info.damage );
		}

		void SetPlayerObject( PlayrObject@ obj ) {
			@m_PlayrObject = @obj;
		}

		PlayrObject@ GetPlayerObject() {
			return @m_PlayrObject;
		}

		void RemoveItem( ItemObject@ item ) {
			@item.prev.next = @item.next;
			@item.next.prev = @item.prev;
		}
		ItemObject@ FindItemInBounds( const TheNomad::GameSystem::BBox& in bounds ) {
			ItemObject@ item;
			for ( @item = cast<ItemObject@>( @m_ActiveItems.next ); @item !is @m_ActiveItems; @item = cast<ItemObject>( @item.next ) ) {
				if ( Util::BoundsIntersect( bounds, item.GetBounds() ) ) {
					return @item;
				}
			}
			return null;
		}
		ItemObject@ AddItem( uint type, const vec3& in origin ) {
			ItemObject@ item;

			@item = cast<ItemObject@>( @Spawn( TheNomad::GameSystem::EntityType::Item, type, origin ) );
			@m_ActiveItems.prev.next = @item;
			@item.prev = @m_ActiveItems.prev;
			@item.next = @m_ActiveItems;

			return item;
		}

		private void PrintPlayerState() const {
			string msg;
			msg.reserve( MAX_STRING_CHARS );

			msg = "\nPlayer State:\n";
			msg += "Health: " + m_PlayrObject.GetHealth() + "\n";
			msg += "Rage: " + m_PlayrObject.GetRage() + "\n";

			ConsolePrint( msg );
		}
		private void ListActiveItems() {
			string msg;
			msg.reserve( MAX_STRING_CHARS );

//			ConsolePrint( "Active Game Items:\n" );
//			for ( uint i = 0; i < m_ItemList.Count(); i++ ) {
//				msg = "(";
//				msg += i;
//				msg += ") ";
//				msg += cast<const ItemInfo@>( @m_ItemList[i].GetInfo() ).name;
//				msg += " ";
//				msg += "[ ";
//				msg += m_ItemList[i].GetOrigin().x;
//				msg += ", ";
//				msg += m_ItemList[i].GetOrigin().y;
//				msg += ", ";
//				msg += m_ItemList[i].GetOrigin().z;
//				msg += " ]\n";
//				ConsolePrint( msg );
//			}
		}
		
		private array<EntityObject@> m_EntityList;
		private ItemObject m_ActiveItems;
		private EntityObject m_ActiveEntities;
		private PlayrObject@ m_PlayrObject;
		
		//
		// effects
		//
		
		void Effect_Bleed_f() {
			// sgame.effect_entity_bleed <attacker_num>
		}

		void Effect_Knockback_f() {
			// sgame.effect_entity_knockback <attacker_num>

			EntityObject@ target, attacker;
			const uint attackerNum = Convert().ToUInt( Engine::CmdArgv( 1 ) );

			@attacker = GetEntityForNum( attackerNum );
			if ( attacker is null ) {
				GameError( "Effect_Knockback_f triggered on null attacker" );
			}

			@target = cast<MobObject@>( @attacker ).GetTarget();
			if ( target is null ) {
				ConsoleWarning( "Effect_Knockback_f triggered but no target\n" );
				return;
			}

			ApplyEntityEffect( attacker, target, AttackEffect::Effect_Knockback );
		}

		void Effect_Stun_f() {
			// sgame.effect_entity_stun <attacker_num>

			EntityObject@ target, attacker;
			const uint attackerNum = Convert().ToUInt( Engine::CmdArgv( 1 ) );

			@attacker = GetEntityForNum( attackerNum );
			if ( attacker is null ) {
				GameError( "Effect_Stun_f triggered on null attacker" );
			}

			@target = cast<MobObject@>( @attacker ).GetTarget();
			if ( target is null ) {
				ConsoleWarning( "Effect_Stun_f triggered but no target\n" );
				return;
			}

			ApplyEntityEffect( attacker, target, AttackEffect::Effect_Stun );
		}
	};
};