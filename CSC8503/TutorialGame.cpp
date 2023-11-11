#include "TutorialGame.h"
#include "GameWorld.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "TextureLoader.h"

#include "PositionConstraint.h"
#include "OrientationConstraint.h"
#include "StateGameObject.h"
#include "Maths.h"
#include "Assets.h"
#include <iostream>
#include <fstream>
#include "../CSC8503CoreClasses/NavigationPath.h"
#include "../CSC8503CoreClasses/StateMachine.h"
#include "../CSC8503CoreClasses/State.h"
#include "../CSC8503CoreClasses/StateTransition.h"


using namespace NCL;
using namespace CSC8503;
using namespace std;

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
#ifdef USEVULKAN
	renderer	= new GameTechVulkanRenderer(*world);
#else 
	renderer = new GameTechRenderer(*world);
#endif

	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = false;

	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	cubeMesh	= renderer->LoadMesh("cube.msh");
	sphereMesh	= renderer->LoadMesh("sphere.msh");
	charMesh	= renderer->LoadMesh("goat.msh");
	enemyMesh	= renderer->LoadMesh("Keeper.msh");
	bonusMesh	= renderer->LoadMesh("apple.msh");
	capsuleMesh = renderer->LoadMesh("capsule.msh");
	gooseMesh = renderer->LoadMesh("goose.msh");
	appleMesh = renderer->LoadMesh("apple.mesh");
	keeperMesh = renderer->LoadMesh("keeper.msh");
	cylinderMesh = renderer->LoadMesh("cylinder.msh");
	coinMesh = renderer->LoadMesh("cion.msh");

	basicTex	= renderer->LoadTexture("checkerboard.png");
	basicShader = renderer->LoadShader("scene.vert", "scene.frag");



	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete charMesh;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::UpdateGame(float dt) {

	ReturnCourt();

	gooseFindEgg();
	gooseFindEgg2();
	GetThePoint();
	GetThePoint2();
	RuleOfTheGame();
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
		float goatViewX = cameraDisGoat * cos((world->GetMainCamera()->GetYaw() + 270.0f) * 3.14 / 180) + playerGoat->GetTransform().GetPosition().x;
		float goatViewZ = cameraDisGoat * sin((world->GetMainCamera()->GetYaw() + 270.0f) * 3.14 / 180) + playerGoat->GetTransform().GetPosition().z;
		float goatViewY = cameraDisGoat * -sin((world->GetMainCamera()->GetPitch() + 270.0f) * 3.14 / 180) + playerGoat->GetTransform().GetPosition().y;

		world->GetMainCamera()->SetPosition({ goatViewX,goatViewY+20,goatViewZ });
	}
	if (inSelectionMode)
	{
		world->GetMainCamera()->UpdateCamera(dt);
	}
	
	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0,1,0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);

	}

	

	UpdateKeys(dt);
	AddPaneltyToWorld();
	

	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(5, 95), Debug::RED);
	}
	else {
		Debug::Print("(G)ravity off", Vector2(5, 95), Debug::RED);
	}

	RayCollision closestCollision;
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::K) && selectionObject) {
		Vector3 rayPos;
		Vector3 rayDir;

		rayDir = selectionObject->GetTransform().GetOrientation() * Vector3(0, 0, -1);

		rayPos = selectionObject->GetTransform().GetPosition();

		Ray r = Ray(rayPos, rayDir);

		if (world->Raycast(r, closestCollision, true, selectionObject)) {
			if (objClosest) {
				objClosest->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
			}
			objClosest = (GameObject*)closestCollision.node;

			objClosest->GetRenderObject()->SetColour(Vector4(1, 0, 1, 1));

		}
	}

	if (testStateObject) {
		testStateObject->Update(dt);
	}


	Debug::DrawLine(Vector3(), Vector3(0, 100, 0), Vector4(1, 0, 0, 1));

	SelectObject();
	MoveSelectedObject();
	PanelDisplay();
	TimeCounting(dt);
	StateLevel();
	

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);

	renderer->Render();
	
	Debug::UpdateRenderables(dt);
}

void TutorialGame::UpdateKeys(float dt) {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::P)) {
		playerGoat->usePenalty = !(playerGoat->usePenalty);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}
	TestPathfinding();
	DisplayPathfinding(dt);
	TimeCounting(dt);
	
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();


	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		selectionObject->GetPhysicsObject()->AddForce(fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		selectionObject->GetPhysicsObject()->AddForce(-fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NEXT)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(0,-10,0));
	}

}



void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (!inSelectionMode ) {
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::W))
		{
			playerGoat->GetPhysicsObject()->AddForce(playerGoat->GetTransform().GetOrientation()* Vector3(-50, 0,0 ));
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::S))
		{
			playerGoat->GetPhysicsObject()->AddForce(playerGoat->GetTransform().GetOrientation() * Vector3(50, 0, 0));
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::A))
		{
			playerGoat->GetPhysicsObject()->AddForce(playerGoat->GetTransform().GetOrientation() * Vector3(0, 0, 50));
			
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::D))
		{
			playerGoat->GetPhysicsObject()->AddForce(playerGoat->GetTransform().GetOrientation() * Vector3(0, 0, -50));
			//playerGoat->GetPhysicsObject()->AddTorque(Vector3(0, 0.5, 0));
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::R))
		{
			playerGoat->GetPhysicsObject()->AddTorque(Vector3(0, 60, 0));
		}
		
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::L))
		{
			rotateobs->GetPhysicsObject()->AddTorque(Vector3(0, 60, 0));
		}
		
		
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}
}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-10.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	testStateObject = AddStateObjectToWorld(Vector3{110,30,0});

	//InitMixedGridWorld(15, 15, 3.5f, 3.5f);

	InitGameExamples();
	InitDefaultFloor();
	BridgeConstraintTest();
	MazeMap("Grid3.txt");

}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();

	Vector3 floorSize = Vector3(100, 2, 100);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetPosition(position).SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddPlayerToWorld(const Vector3& position) {
	float meshSize		= 1.7f;
	float inverseMass	= 0.5f;

	GameObject* character = new GameObject();
	SphereVolume* volume  = new SphereVolume(1.0f);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), charMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();
	character->GetRenderObject()->SetColour(Vector4(0, 1, 1, 1));
	playerGoat = character;
	

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
	GameObject* apple = new GameObject();

	SphereVolume* volume = new SphereVolume(0.5f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(2, 2, 2))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), capsuleMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();
	pointCoin = apple;

	world->AddGameObject(apple);

	return apple;
}

StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position) {
	StateGameObject* apple = new StateGameObject();

	SphereVolume* volume = new SphereVolume(0.5f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(5, 5, 5))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), cylinderMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(0.1f);
	apple->GetPhysicsObject()->InitSphereInertia();
	apple->GetRenderObject()->SetColour(Vector4(0, 0.5, 1, 1));
	stateobj = apple;

	world->AddGameObject(apple);

	return apple;
}



GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position)//
{
	float size = 1.2f;
	float inverseMass = 1.0f;

	GameObject* capsule = new GameObject("capsule");

	SphereVolume* volume = new SphereVolume(size);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform().SetScale(Vector3(size, size, size)).SetPosition(position);
	

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, nullptr, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitSphereInertia();
	

	world->AddGameObject(capsule);

	return capsule;
}

GameObject* TutorialGame::AddGooseToWorld(const Vector3& position)//
{
	float size = 1.6f;
	float inverseMass = 1.0f;

	GameObject* goose = new GameObject("goose");

	SphereVolume* volume = new SphereVolume(size);
	goose->SetBoundingVolume((CollisionVolume*)volume);

	goose->GetTransform().SetScale(Vector3(size, size, size)).SetPosition(position);


	goose->SetRenderObject(new RenderObject(&goose->GetTransform(), gooseMesh, nullptr, basicShader));
	goose->SetPhysicsObject(new PhysicsObject(&goose->GetTransform(), goose->GetBoundingVolume()));

	goose->GetPhysicsObject()->SetInverseMass(inverseMass);
	goose->GetPhysicsObject()->InitSphereInertia();
	goose->GetRenderObject()->SetColour({ 0,0,1,1 });
	playerGoose = goose;

	world->AddGameObject(goose);
	float distanceToGoat;

	return goose;
}

GameObject* TutorialGame::AddPointArea(const Vector3& position)
{
	float size = 5.0f;
	float inverseMass = 0.0f;

	GameObject* pointArea = new GameObject("pointarea");

	SphereVolume* volume = new SphereVolume(size);
	pointArea->SetBoundingVolume((CollisionVolume*)volume);

	pointArea->GetTransform().SetScale(Vector3(size, size, size)).SetPosition(position);


	pointArea->SetRenderObject(new RenderObject(&pointArea->GetTransform(), cubeMesh, nullptr, basicShader));
	pointArea->SetPhysicsObject(new PhysicsObject(&pointArea->GetTransform(), pointArea->GetBoundingVolume()));

	pointArea->GetPhysicsObject()->SetInverseMass(inverseMass);
	pointArea->GetPhysicsObject()->InitSphereInertia();
	pointArea->GetRenderObject()->SetColour({ 0,0,1,1 });
	pointArea->GetRenderObject()->SetColour(Vector4(1, 0, 1, 1));
	pointToArea = pointArea;

	world->AddGameObject(pointArea);
	
	return pointArea;
}

GameObject* TutorialGame::AddBounsToWorld2(const Vector3& position)
{
	GameObject* bouns2 = new GameObject();

	SphereVolume* volume = new SphereVolume(0.5f);
	bouns2->SetBoundingVolume((CollisionVolume*)volume);
	bouns2->GetTransform()
		.SetScale(Vector3(2, 2, 2))
		.SetPosition(position);

	bouns2->SetRenderObject(new RenderObject(&bouns2->GetTransform(), capsuleMesh, nullptr, basicShader));
	bouns2->SetPhysicsObject(new PhysicsObject(&bouns2->GetTransform(), bouns2->GetBoundingVolume()));

	bouns2->GetPhysicsObject()->SetInverseMass(1.0f);
	bouns2->GetPhysicsObject()->InitSphereInertia();
	pointCoin2 = bouns2;

	world->AddGameObject(bouns2);

	return bouns2;
}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(50, -3, 50));
	
}

void TutorialGame::InitGameExamples() {
	AddPlayerToWorld(Vector3(140, 3, 145));
	//AddEnemyToWorld(Vector3(5, 5, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
	AddSphereToWorld(Vector3(75, 3, 80),2.0f,1.0f);
	//AddCapsuleToWorld(Vector3(0, 5, 5));
	//AddKeeperToWorld(Vector3(0, 5, 10));
	AddGooseToWorld(Vector3(140, 3, 12));
	AddBonusToWorld(Vector3(70, 3, 75));
	AddPointArea(Vector3(15, 3, 15));
	AddBounsToWorld2(Vector3(30,3,55));
	AddRotateObstacleToWorld(Vector3(80, 3, 115));
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}



/*
Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		Debug::Print("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;

				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				Debug::DrawLine(world->GetMainCamera()->GetPosition(), closestCollision.collidedAt, Vector4(1, 0, 0, 1), 120.0f);
				return true;
			}
			else {
				return false;
			}
		}
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
			if (selectionObject) {
				if (lockedObject == selectionObject) {
					lockedObject = nullptr;
				}
				else {
					lockedObject = selectionObject;
				}
			}
		}
	}
	else {
		Debug::Print("Press Q to change to select mode!", Vector2(5, 85));
	}


	//Debug::DrawLine(, Vector3(0, 100, 0), Vector4(1, 0, 0, 1));
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void TutorialGame::MoveSelectedObject() {
	Debug::Print("Click Force:" + std::to_string(forceMagnitude), Vector2(5, 90));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject) {
		return;//we haven't selected anything!
	}
	//Push the selected object!
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
}
void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(2.5f, 2.5f, 2.5f);

	float invCubeMass = 5; //how heavy the middle pieces are
	int numLinks = 10;
	float maxDistance = 30; // constraint distance
	float cubeDistance = 17; // distance between links

	Vector3 startPos = Vector3(0, 50, 35);

	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0);

	GameObject* previous = start;

	for (int i = 1; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, 2);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;

	}
	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}


void TutorialGame::MazeMap(const std::string& filename)
{
	ifstream fileMap(Assets::DATADIR + filename);
	int nodeOfSize = 0;
	int widthOfGrid = 0;
	int heightOfGrid = 0;
	
	
	fileMap >> nodeOfSize;
	fileMap >> widthOfGrid;
	fileMap >> heightOfGrid;

	for (int i = 0; i < widthOfGrid; i++)
	{
		for (int j = 0; j < heightOfGrid; j++)
		{
			char symbol = 0;
			fileMap >> symbol;
			if (symbol == 'x')
			{
				GameObject* obj = AddCubeToWorld(Vector3(j * nodeOfSize+6, 1, i * nodeOfSize+6),Vector3(nodeOfSize/2,6, nodeOfSize / 2),0);
				obj->isWall = true;
			}

		}
	}
}



void TutorialGame::TestPathfinding() {
	NavigationGrid grid("Grid3.txt");

	NavigationPath outPath;

	Vector3 startPos(playerGoose->GetTransform().GetPosition());
	Vector3 endPos(playerGoat->GetTransform().GetPosition());


	bool found = grid.FindPath(startPos, endPos, outPath);
	testNodes.clear();
	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		testNodes.push_back(pos);
	}
}

void TutorialGame::DisplayPathfinding(float dt) {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];

		Debug::DrawLine(Vector3(a.x + 6, a.y, a.z + 6), Vector3(b.x + 6, b.y, b.z + 6), Vector4(0, 1, 0, 1));

		Vector3 localDestination = testNodes[1];
		playerGoose->GetTransform().SetPosition(playerGoose->GetTransform().GetPosition() + (localDestination - playerGoose->GetTransform().GetPosition()).Normalised() * dt * 10.0f);
	}
}

void TutorialGame::FindGoat(Vector3& gooseposition, Vector3& goatposition, GameObject* goose, NavigationGrid* map)
{
	for (int i = 1; i < testNodes.size(); ++i) {
		goose->GetPhysicsObject()->AddForce((Vector3(testNodes[i].x + 6, testNodes[i].y, testNodes[i].z + 6) - Vector3(testNodes[i - 1].x + 6, testNodes[i - 1].y, testNodes[i - 1].z + 6)).Normalised() * 10);
		Vector3 directionOfForce = goose->GetPhysicsObject()->GetForce();
		directionOfForce.Normalise();
		float goAlong = directionOfForce.Length();
		
	}
}
void TutorialGame::ChaseGoat()
{
	float disBetGooToGoat;
	disBetGooToGoat = (playerGoat->GetTransform().GetPosition() - playerGoose->GetTransform().GetPosition()).Length();
	Vector3 goatPos = playerGoat->GetTransform().GetPosition();
	Vector3 goosePos = playerGoose->GetTransform().GetPosition();
	NavigationGrid* map = new NavigationGrid("Grid3.txt");

	FindGoat(goosePos, goatPos, playerGoose, map);
}
void TutorialGame::GooseStateMachine(float dt)
{
	Vector3 localDestination = testNodes[1];
	playerGoose->GetTransform().SetPosition((localDestination - playerGoose->GetTransform().GetPosition()).Normalised() * dt * 20.0f);

}



void TutorialGame::PanelDisplay()
{
	Debug::Print("the point of the goat: " + std::to_string((int)currentPoint), Vector2(1, 5),Vector4(0,1,1,1));
}
void TutorialGame::TimeCounting(float dt) {
	
	
	if (currentTime >= 0)
	{
		currentTime -= dt;
		Debug::Print("Time left: " + std::to_string((int)currentTime), Vector2(70, 5), Vector4(1, 0, 0, 1));
	}
}
void TutorialGame::gooseFindEgg()
{
	Vector3 eggPos = pointCoin->GetTransform().GetPosition();
	if ((eggPos - playerGoose->GetTransform().GetPosition()).Length() < 6.0f)
	{
		Vector3 pullPos = playerGoose->GetTransform().GetPosition();
		GameObject* startPull = playerGoose;
		GameObject* endPull = pointCoin;
		float gooseCoinDistance = 5;
		PositionConstraint* gooseConstraint = new PositionConstraint(playerGoose, pointCoin, gooseCoinDistance);
		
		if (this->gooseConstraint == nullptr) {
			world->AddConstraint(gooseConstraint);
			this->gooseConstraint = gooseConstraint;
		}
	}
}

void TutorialGame::gooseFindEgg2()
{
	Vector3 eggPos2 = pointCoin2->GetTransform().GetPosition();
	if ((eggPos2 - playerGoose->GetTransform().GetPosition()).Length() < 6.0f)
	{
		Vector3 pullPos = playerGoose->GetTransform().GetPosition();
		GameObject* startPull = playerGoose;
		GameObject* endPull = pointCoin2;
		float gooseCoinDistance = 8;
		PositionConstraint* gooseConstraint2 = new PositionConstraint(playerGoose, pointCoin2, gooseCoinDistance);

		if (this->gooseConstraint2 == nullptr) {
			world->AddConstraint(gooseConstraint2);
			this->gooseConstraint2 = gooseConstraint2;
		}
	}
}

void TutorialGame::GetThePoint()
{
	if ((pointToArea->GetTransform().GetPosition() - pointCoin->GetTransform().GetPosition()).Length() < 8.0f)
	{
		currentPoint++;
		world->RemoveConstraint(this->gooseConstraint);
		pointCoin->GetTransform().SetPosition(Vector3(0, 0, -20));
	}
}
void TutorialGame::GetThePoint2()
{
	if ((pointToArea->GetTransform().GetPosition() - pointCoin2->GetTransform().GetPosition()).Length() < 8.0f)
	{
		currentPoint++;
		world->RemoveConstraint(this->gooseConstraint2);
		pointCoin2->GetTransform().SetPosition(Vector3(0, 0, -30));
	}
}
void TutorialGame::RuleOfTheGame()
{
	bool me1nu = true;
	bool me2nu = true;
	if (me1nu = true)
	{
		Debug::Print("Press M to the display the menu", Vector2(2, 25), Vector4(1, 0, 1, 1));
		bool me2nu = true;
	}
	
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::M)&&me2nu)
	{
		Debug::Print("Press G to the first level hard mode", Vector2(2, 28), Vector4(1, 0, 1, 1));
		Debug::Print("Press Z to the higher level of machine", Vector2(2, 31), Vector4(1, 0, 1, 1));
		Debug::Print("Press X to the higher higher level of machine", Vector2(2, 34), Vector4(1, 0, 1, 1));
		me1nu = false;
	}
	
	if (currentPoint == 2&&currentTime >0)
	{
		Debug::Print("Hey! you win the game!!!" , Vector2(15, 35), Vector4(1, 0, 1, 1));
	}
	else if (currentTime < 0 && currentPoint < 2)
	{
		Debug::Print("sorry !!time runs out!!!", Vector2(15, 35), Vector4(1, 0, 1, 1));
	}
}


//state level
void TutorialGame::StateLevel()
{
	bool levela = false;
	bool levelb = false;
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::Z))
	{
		levela = true;
		levelb = false;
	}
	if (levela)
	{
		stateobj->GetTransform().SetScale(Vector3(7, 7, 7));
		stateobj->GetPhysicsObject()->SetLinearVelocity(Vector3(4, 0, 2));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::X))
	{
		levela = false;
		levelb = true;
	}
	if (levelb)
	{
		stateobj->GetTransform().SetScale(Vector3(12, 12, 12));
		stateobj->GetPhysicsObject()->ApplyAngularImpulse(Vector3(0, 0, 4));
		stateobj->GetPhysicsObject()->SetLinearVelocity(Vector3(-10, 0, 100));
		stateobj->GetPhysicsObject()->AddTorque(Vector3(0, 500, 0));
	}


}




//penalty mode (egg)
void TutorialGame::AddPaneltyToWorld()
{
	if (currentTime <= 30)
	{
		playerGoat->GetPhysicsObject()->AddTorque(Vector3(0, 0, 100));
	}
}
//a trouble obstacle
GameObject* TutorialGame:: AddRotateObstacleToWorld(const Vector3& position)//sorry no rotate (--!)
{
	float size = 1.6f;
	float inverseMass = 1.0f;

	GameObject* rotateObs = new GameObject("rotateObs");

	OBBVolume* volume = new OBBVolume(Vector3(5.0f,3.0f,5.0f));
	rotateObs->SetBoundingVolume((CollisionVolume*)volume);
	
	rotateObs->GetTransform().SetScale(Vector3(5.0f,3.0f, 5.0f)).SetPosition(position);


	rotateObs->SetRenderObject(new RenderObject(&rotateObs->GetTransform(), cubeMesh, nullptr, basicShader));
	rotateObs->SetPhysicsObject(new PhysicsObject(&rotateObs->GetTransform(), rotateObs->GetBoundingVolume()));

	rotateObs->GetPhysicsObject()->SetInverseMass(inverseMass);
	rotateObs->GetPhysicsObject()->InitSphereInertia();
	rotateObs->GetRenderObject()->SetColour({ 1,0,0,1 });
	rotateobs = rotateObs;


	world->AddGameObject(rotateObs);

	return rotateObs;
}
//go out initial(return)¡Ì
void TutorialGame::ReturnCourt()
{
	float outOfEdge = playerGoat->GetTransform().GetPosition().y;
	if (outOfEdge < -5)
	{
		playerGoat->GetTransform().SetPosition(Vector3(140, 3, 145));
	}
}
//obb vs sphere¡Ì


