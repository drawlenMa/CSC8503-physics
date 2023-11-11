#pragma once
#include "GameTechRenderer.h"
#ifdef USEVULKAN
#include "GameTechVulkanRenderer.h"
#endif
#include "PhysicsSystem.h"
#include "StateGameObject.h"
#include "../CSC8503CoreClasses/NavigationGrid.h"
#include "../CSC8503CoreClasses/NavigationPath.h"
#include "../CSC8503CoreClasses/StateMachine.h"
#include "../CSC8503CoreClasses/State.h"
#include "../CSC8503CoreClasses/StateTransition.h"
#include "PositionConstraint.h"


namespace NCL {
	namespace CSC8503 {
		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);
			GameObject* playerGoat = nullptr;
			GameObject* playerGoose = nullptr;
			GameObject* pointCoin = nullptr;
			GameObject* pointCoin2 = nullptr;
			GameObject* pointToArea = nullptr;
			GameObject* rotateobs = nullptr;
			GameObject* stateobj = nullptr;
			int currentPoint = 0;
			PositionConstraint* gooseConstraint = nullptr;
			PositionConstraint* gooseConstraint2 = nullptr;
			

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys(float dt);

			void InitWorld();

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on). 
			*/
			void InitGameExamples();

			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);

			void InitDefaultFloor();

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();
			void BridgeConstraintTest();
			void FindGoat(Vector3& gooseposition,Vector3& goatposition, GameObject* goose, NavigationGrid* map);//
			void ChaseGoat();
			void GooseStateMachine(float dt);
			void MazeMap(const std::string& filename);
			void PanelDisplay();
			void TimeCounting(float dt);
			void gooseFindEgg();
			void gooseFindEgg2();
			void GetThePoint();
			void GetThePoint2();
			void RuleOfTheGame();
			void BehaviourTree();
			void ReturnCourt();
			void AddPaneltyToWorld();
			void StateLevel();
			

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);

			GameObject* AddPlayerToWorld(const Vector3& position);
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);
			GameObject* AddGooseToWorld(const Vector3& position);
			GameObject* AddCapsuleToWorld(const Vector3& position);//
			GameObject* AddKeeperToWorld(const Vector3& position);//
			GameObject* AddPointArea(const Vector3& position);
			GameObject* AddBounsToWorld2(const Vector3& position);
			GameObject* AddRotateObstacleToWorld(const Vector3& position);
			
			void TestPathfinding();
			void DisplayPathfinding(float dt);
			
			StateGameObject* AddStateObjectToWorld(const Vector3& position);
			StateGameObject* testStateObject= nullptr;

#ifdef USEVULKAN
			GameTechVulkanRenderer*	renderer;
#else
			GameTechRenderer* renderer;
#endif
			PhysicsSystem*		physics;
			GameWorld*			world;

			bool useGravity;
			bool inSelectionMode;

			float		forceMagnitude;
			float cameraDisGoat = 20.0f;
			float currentTime = 180.0f;
			vector <Vector3 > testNodes;


			GameObject* selectionObject = nullptr;

			MeshGeometry*	capsuleMesh = nullptr;
			MeshGeometry*	cubeMesh	= nullptr;
			MeshGeometry*	sphereMesh	= nullptr;

			TextureBase*	basicTex	= nullptr;
			ShaderBase*		basicShader = nullptr;

			//Coursework Meshes
			MeshGeometry*	charMesh	= nullptr;
			MeshGeometry*	enemyMesh	= nullptr;
			MeshGeometry*	bonusMesh	= nullptr;
			MeshGeometry*   gooseMesh   = nullptr;
			MeshGeometry*   appleMesh   = nullptr;
			MeshGeometry*   keeperMesh  = nullptr;
			MeshGeometry*   cylinderMesh = nullptr;
			MeshGeometry* coinMesh = nullptr;


			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}
			
			GameObject* objClosest = nullptr;
			;

		};
	}
}



