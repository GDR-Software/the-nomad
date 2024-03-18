#include "entity.as"
#include "level.as"
#include "item.as"
#include "game.as"

namespace TheNomad::SGame {
	class QuickShot {
		QuickShot() {
		}
		QuickShot( const vec3& in origin ) {
			m_Targets.resize( sgame_QuickShotMaxTargets.GetInt() );
		}
		
		void Think() {
			if ( m_nLastTargetTime < uint( sgame_QuickShotTime.GetInt() ) ) {
				m_nLastTargetTime++;
				return;
			}
			
			DebugPrint( "QuickShot thinking...\n" );
			m_nLastTargetTime = 0;
			
			const array<EntityObject@>@ EntList = @EntityManager.GetEntities();

			// NOTE: this might be a little bit slow depending on how many mobs are in the area
			for ( uint i = 0; i < EntList.size(); i++ ) {
				if ( m_Targets.find( @EntList[i] ) == -1 ) {
					if ( TheNomad::Util::Distance( EntList[i].GetOrigin(), m_Origin ) > sgame_QuickShotMaxRange.GetFloat() ) {
						continue; // too far away
					}
					// make sure we aren't adding a duplicate
					@m_Targets[m_nTargetsFound] = @EntList[i];
					DebugPrint( "QuickShot added entity " + formatUInt( i ) + "\n" );
				}
			}
		}
		
		void Clear() {
			DebugPrint( "QuickShot cleared.\n" );
			m_nLastTargetTime = 0;
			m_nTargetsFound = 0;
		}

		private vec3 m_Origin;
		private array<EntityObject@> m_Targets;
		private uint m_nTargetsFound;
		private uint m_nLastTargetTime;
	};
	
	class KeyBind {
		KeyBind() {
			down[0] = down[1] = 0;
			downtime = 0;
			msec = 0;
			active = false;
		}
		KeyBind() {
		}
		
		KeyBind& opAssign( const KeyBind& in other ) {
			down[0] = other.down[0];
			down[1] = other.down[1];
			downtime = other.downtime;
			msec = other.msec;
			active = other.active;
			return this;
		}

		void Down() {
			int8[] c( MAX_TOKEN_CHARS );
			int k;
			
			TheNomad::Engine::CmdArgvFixed( c, MAX_TOKEN_CHARS, 1 );
			if ( c[0] != 0 ) {
				k = TheNomad::Util::StringToInt( c );
			} else {
				return;
			}
			
			if ( down[0] == 0 ) {
				down[0] = k;
			} else if ( down[1] == 0 ) {
				down[1] = k;
			} else {
				ConsolePrint( "Three keys down for a button!\n" );
				return;
			}
			
			if ( active ) {
				return; // still down
			}
			
			// save the timestamp for partial frame summing
			TheNomad::Engine::CmdArgvFixed( c, MAX_TOKEN_CHARS, 2 );
			downtime = TheNomad::Util::StringToInt( c );
			
			active = true;
		}
		
		void Up() {
			int8[] c( MAX_TOKEN_CHARS );
			uint uptime;
			uint k;
			
			TheNomad::Engine::CmdArgvFixed( c, MAX_TOKEN_CHARS, 1 );
			if ( c[0] != 0 ) {
				k = TheNomad::Util::StringToInt( c );
			} else {
				return;
			}
			
			if ( down[0] == k ) {
				down[0] = 0;
			} else if ( down[1] == k ) {
				down[1] = 0;
			} else {
				return; // key up without corresponding down (menu pass through)
			}
			
			active = false;
			
			// save timestamp for partial frame summing
			TheNomad::Engine::CmdArgvFixed( c, MAX_TOKEN_CHARS, 2 );
			uptime = TheNomad::Util::StringToInt( c );
			if ( uptime > 0 ) {
				msec += uptime - downtime;
			} else {
				msec += TheNomad::GameSystem::GameManager.GetGameMsec() / 2;
			}
			
			active = false;
		}
		
		uint[] down( 2 );
		uint downtime;
		uint msec;
		bool active;
	};
	
	class PlayrObject : EntityObject {
		PlayrObject() {
			m_WeaponSlots.resize( sgame_MaxPlayerWeapons.GetInt() );

			key_MoveNorth = KeyBind();
			key_MoveSouth = KeyBind();
			key_MoveEast = KeyBind();
			key_MoveWest = KeyBind();
			key_Melee = KeyBind();
			key_Jump = KeyBind();
		}
		
		//
		// controls
		//
		void MoveNorth_Up_f() { key_MoveNorth.Up(); }
		void MoveNorth_Down_f() { key_MoveNorth.Down(); }
		void MoveSouth_Up_f() { key_MoveSouth.Up(); }
		void MoveSouth_Down_f() { key_MoveSouth.Down(); }
		void Jump_Down_f() { key_Jump.Down(); }
		void Jump_Up_f() { key_Jump.Up(); }
		
		void Quickshot_Down_f() {
			return;
//			m_PFlags |= PF_QUICKSHOT;
//			m_QuickShot.Clear();
//			m_QuickShot = QuickShot( m_Link.m_Origin, @ModObject );
		}
		
		void Quickshot_Up_f() {
			return;
			// TODO: perhaps add a special animation for putting the guns away?
//			if ( m_QuickShot.Empty() ) {
//				return;
//			}
			
//			m_QuickShot.Activate();
//			m_PFlags &= ~PF_QUICKSHOT;
		}
		
		void Melee_Down_f() {
			m_nParryBoxWidth = 0.0f;
			SetState( StateNum::ST_PLAYR_MELEE );
		}
		
		void NextWeapon_f() {
			m_CurrentWeapon++;
			if ( m_CurrentWeapon >= m_WeaponSlots.size() ) {
				m_CurrentWeapon = 0;
			}
		}
		void PrevWeapon_f() {
			m_CurrentWeapon--;
			if ( m_CurrentWeapon <= 0 ) {
				m_CurrentWeapon = m_WeaponSlots.size() - 1;
			}
		}
		void SetWeapon_f() {
			
		}
		void Emote_f() {
			m_bEmoting = true;
		}
		
		
		void Damage( float nAmount ) {
			if ( m_bEmoting ) {
				return; // god has blessed thy soul...
			}
			
			m_nHealth -= nAmount;

			if ( m_nHealth < 1 ) {
				if ( m_nFrameDamage > 0 ) {
					return; // as long as you're hitting SOMETHING, you cannot die
				}
				EntityManager.KillEntity( @this );
			}
		}
		
		uint GetFlags() const {
			return m_PFlags;
		}
		
		void Think() override {
			if ( ( m_PFlags & PF_PARRY ) != 0 ) {
				ParryThink();
			} else if ( ( m_PFlags & PF_QUICKSHOT ) != 0 ) {
				m_QuickShot.Think();
			}
			
			switch ( m_State.GetID() ) {
			case StateNum::ST_PLAYR_IDLE:
				IdleThink();
				break; // NOTE: maybe let the player move in combat? (that would require more sprites for the dawgs)
			case StateNum::ST_PLAYR_COMBAT:
				CombatThink();
				break;
			};

			m_nHealth += sgame_PlayerHealBase.GetFloat() * m_nHealMult;
			m_nHealMult -= m_nHealMultDecay * LevelManager.GetDifficultyScale();
		}
		
		bool CheckParry( EntityObject@ ent ) {
			if ( TheNomad::Util::BoundsIntersect( ent.GetBounds(), m_ParryBox ) ) {
				if ( ent.IsProjectile() ) {
					// simply invert the direction and double the speed
					const vec3 v = ent.GetVelocity();
					ent.SetVelocity( vec3( v.x * 2, v.y * 2, v.z * 2 ) );
//					ent.SetDirection( InverseDirs[ ent.GetDirection() ] );
				} else {
					return false;
				}
			} else if ( ent.GetType() == TheNomad::GameSystem::EntityType::Mob ) {
				// just a normal counter
				MobObject@ mob = cast<MobObject>( ent.GetData() );
				
				if ( !mob.CurrentAttack().canParry ) {
					// unblockable, deal damage
					EntityManager.DamageEntity( @ent, @m_Base );
				} else {
					// slap it back
				}
			}

			return true;
		}
		
		private void MakeSound() {
			if ( m_State.GetID() == StateNum::ST_PLAYR_CROUCH ) {
				return;
			}

			SoundData data( m_Velocity.x + m_Velocity.y, vec2( m_Velocity.x, m_Velocity.y ), ivec2( 2, 2 ),
				ivec3( int( m_Link.m_Origin.x ), int( m_Link.m_Origin.y ), int( m_Link.m_Origin.z ) ), 1.0f );
		}
		
		private void IdleThink() {
			
		}
		private void CombatThink() {
			if ( key_Melee.active ) {
				// check for a parry
				
			}
		}
		private void ParryThink() {
			m_ParryBox.m_nWidth = 2.5f + m_nParryBoxWidth;
			m_ParryBox.m_nHeight = 1.0f;
			m_ParryBox.MakeBounds( vec3( m_Link.m_Origin.x + ( m_Link.m_Bounds.m_nWidth / 2.0f ), m_Link.m_Origin.y, m_Link.m_Origin.z ) );

			if ( m_nParryBoxWidth >= 1.5f ) {
				return; // maximum
			}
			
			m_nParryBoxWidth += 0.5f;
		}
		
		uint PF_QUICKSHOT  = 0x00000001;
		uint PF_DOUBLEJUMP = 0x00000002;
		uint PF_PARRY      = 0x00000004;
		
		private KeyBind key_MoveNorth, key_MoveSouth, key_MoveEast, key_MoveWest;
		private KeyBind key_Jump, key_Melee;
		
		private TheNomad::GameSystem::BBox m_ParryBox;
		private float m_nParryBoxWidth;
		
		private array<WeaponObject@> m_WeaponSlots;
		private QuickShot m_QuickShot;
		private uint m_CurrentWeapon;
		private uint m_PFlags;

		// the amount of damage dealt in the frame
		private uint m_nFrameDamage;

		private float m_nDamageMult;
		private float m_nHealMult;

		private bool m_bEmoting;

		private float m_nHealMultDecay = 1.0f;
	};
};