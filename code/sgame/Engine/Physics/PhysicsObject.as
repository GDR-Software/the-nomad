namespace TheNomad::Engine::Physics {
	enum WaterType {
		None,
		Water,
		Lava,
		Acid
	};

    class PhysicsObject {
        PhysicsObject() {
        }

        void Init( TheNomad::SGame::EntityObject@ ent ) {
            @m_EntityData = @ent;
        }
        void Shutdown() {
        }

        float GetAngle() const {
            return m_nAngle;
        }
        void SetAngle( float nAngle ) {
            m_nAngle = nAngle;
        }
        void SetAcceleration( const vec3& in accel ) {
            m_Acceleration = accel;
        }
        void SetVelocity( const vec3& in vel ) {
            m_Velocity = vel;
        }
        void SetWaterLevel( int nWaterLevel ) {
        	m_nWaterLevel = nWaterLevel;
        }
        vec3& GetAcceleration() {
            return m_Acceleration;
        }
        const vec3& GetAcceleration() const {
            return m_Acceleration;
        }
        vec3& GetVelocity() {
            return m_Velocity;
        }
        const vec3& GetVelocity() const {
            return m_Velocity;
        }
        int GetWaterLevel() const {
            return m_nWaterLevel;
        }

        private void ClipBounds() {
			vec3 origin = m_EntityData.GetOrigin();

            if ( origin.x < 0.0f ) {
                m_EntityData.SetOrigin( vec3( 0.0f, origin.y, origin.z ) );
            }
            if ( origin.y < 0.0f ) {
                m_EntityData.SetOrigin( vec3( origin.x, 0.0f, origin.z ) );
		    }
			if ( origin.x >= TheNomad::SGame::LevelManager.GetMapData().GetWidth() ) {
				m_EntityData.SetOrigin( vec3( TheNomad::SGame::LevelManager.GetMapData().GetWidth() - 1, origin.y, origin.z ) );
			}
			if ( origin.y >= TheNomad::SGame::LevelManager.GetMapData().GetHeight() ) {
				m_EntityData.SetOrigin( vec3( origin.x, TheNomad::SGame::LevelManager.GetMapData().GetHeight() - 1, origin.z ) );
			}
		}
		
		void ApplyFriction() {
			vec3 vec;
			float speed, newspeed, control;
			float drop;
			const uint frameTime = TheNomad::GameSystem::GameManager.GetGameTic() * 0.0001f;
			
			vec = m_Velocity;
			speed = Util::VectorLength( vec );
			if ( speed < 1.0f ) {
				m_Velocity[0] = 0.0f;
				m_Velocity[1] = 0.0f;
				m_Velocity[2] = 0.0f; // allow sinking underwater
				return;
			}
			
			drop = 0.0f;
			
			// apply water friction even if just wading
			if ( m_nWaterLevel > 0 ) {
				drop += speed * TheNomad::SGame::sgame_WaterFriction.GetFloat() * m_nWaterLevel;
			} else {
				drop += speed * TheNomad::SGame::sgame_Friction.GetFloat();
			}
			
			// scale the velocity
			newspeed = speed - TheNomad::SGame::sgame_Friction.GetFloat();
			if ( newspeed < 0.0f ) {
				newspeed = 0.0f;
			}
			newspeed /= speed;
			
			m_Velocity[0] = m_Velocity[0] * newspeed;
			m_Velocity[1] = m_Velocity[1] * newspeed;
			m_Velocity[2] = m_Velocity[2] * newspeed;
		}
		
		void SetWaterLevel() {
			vec3 point;
			uint tile;
			TheNomad::GameSystem::BBox bounds;
			
			//
			// get waterlevel, accounting for ducking
			//
			m_nWaterLevel = 0;
			m_nWaterType = WaterType::None;
			
			point = m_EntityData.GetOrigin();
			bounds = m_EntityData.GetBounds();
			tile = TheNomad::SGame::LevelManager.GetMapData().GetTile( point, bounds );
			
			if ( ( tile & SURFACEPARM_WATER ) != 0 || ( tile & SURFACEPARM_LAVA ) != 0 ) {
				m_nWaterType = WaterType( tile );
				m_nWaterLevel = 1; // just walking in a puddle
				
				// check the level below us
				bounds.m_Mins.z--;
				bounds.m_Maxs.z--;
				tile = TheNomad::SGame::LevelManager.GetMapData().GetTile( point, bounds );
				if ( ( tile & SURFACEPARM_WATER ) != 0 || ( tile & SURFACEPARM_LAVA ) != 0 ) {
					m_nWaterLevel = 2; // swimming now
					
					// check the level above us
					bounds.m_Mins.z += 2;
					bounds.m_Maxs.z += 2;
					tile = TheNomad::SGame::LevelManager.GetMapData().GetTile( point, bounds );
					if ( ( tile & SURFACEPARM_WATER ) != 0 || ( tile & SURFACEPARM_LAVA ) != 0 ) {
						m_nWaterLevel = 3; // fully submerged
					}
				}
			}
        }
		
		void OnRunTic() {
			const uint gameTic = TheNomad::GameSystem::GameManager.GetDeltaTics();
			vec3 accel;
			float friction = TheNomad::SGame::sgame_Friction.GetFloat();
			vec3 origin;
			
			// clip it
			ClipBounds();
			SetWaterLevel();
			
			origin = m_EntityData.GetOrigin();
			
			const uint tileFlags = TheNomad::SGame::LevelManager.GetMapData().GetTile( origin, m_EntityData.GetBounds() );
			if ( ( tileFlags & SURFACEPARM_WATER ) != 0 || ( tileFlags & SURFACEPARM_LAVA ) != 0 ) {
				friction = TheNomad::SGame::sgame_WaterFriction.GetFloat() * m_nWaterLevel;
			} else if ( origin.z > 0.0f && m_nWaterLevel == 0 ) {
				friction = TheNomad::SGame::sgame_AirFriction.GetFloat();
			}
			
			// calculate velocity
			m_Velocity.x = m_Acceleration.x;
			m_Velocity.y = m_Acceleration.y;
			m_Velocity.z = m_Acceleration.z;
			
			ApplyFriction();
			
			// apply gravity
			if ( m_Velocity.z > 0.0f ) {
				m_Velocity.z -= TheNomad::SGame::sgame_Gravity.GetFloat() * gameTic;
			}

			TheNomad::GameSystem::BBox bounds;
			bounds.m_nWidth = m_EntityData.GetBounds().m_nWidth;
			bounds.m_nHeight = m_EntityData.GetBounds().m_nHeight;
			bounds.MakeBounds( origin + m_Velocity );
			const array<TheNomad::SGame::EntityObject@>@ entList = @TheNomad::SGame::EntityManager.GetEntities();
			for ( uint i = 0; i < entList.Count(); i++ ) {
				if ( Util::BoundsIntersect( bounds, entList[i].GetBounds() ) ) {
					m_Acceleration = 0.0f;
					return; // clip
				}
			}
			
			origin.x += m_Velocity.x;
			origin.y += m_Velocity.y;
			origin.z += m_Velocity.z;
			
			m_EntityData.SetOrigin( origin );
			
			// clip it
			ClipBounds();
			
			m_Acceleration = 0.0f;
        }
        
        private TheNomad::SGame::EntityObject@ m_EntityData = null;
        private vec3 m_Velocity = vec3( 0.0f );
        private vec3 m_Acceleration = vec3( 0.0f );
        private float m_nAngle = 0.0f;
        private int m_nWaterLevel = 0;
		private WaterType m_nWaterType = WaterType::None;
    };
};