namespace TheNomad::SGame {
	class EntityStateSystem : TheNomad::GameSystem::GameObject {
		EntityStateSystem() {
		}
		
		const string& GetName() const {
			return "EntityStateSystem";
		}
		void OnInit() {
		}
		void OnShutdown() {
		}
		void OnLevelStart() {
		}
		void OnLevelEnd() {
		}
		void OnSave() const {
		}
		void OnLoad() {
		}
		void OnRunTic() {
		}
		bool OnConsoleCommand() {
			return false;
		}
			
		const EntityState@ GetStateForNum( uint nIndex ) const {
			return m_StateList[ nIndex ];
		}
		EntityState@ GetStateForNum( uint nIndex ) {
			return m_StateList[ nIndex ];
		}
		
		private List<EntityState@> m_StateList;
	};

	EntityStateSystem@ StateManager;
};