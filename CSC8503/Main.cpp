#include "Window.h"

#include "Debug.h"

#include "StateMachine.h"
#include "StateTransition.h"
#include "State.h"

#include "GameServer.h"
#include "GameClient.h"

#include "NavigationGrid.h"
#include "NavigationMesh.h"

#include "TutorialGame.h"
#include "NetworkedGame.h"

#include "PushdownMachine.h"

#include "PushdownState.h"

#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"

using namespace NCL;
using namespace CSC8503;

#include <chrono>
#include <thread>
#include <sstream>
#include "TutorialGame.h"

vector <Vector3 > testNodes;
void TestPathfinding() {
	NavigationGrid grid("Grid3.txt");
	
	NavigationPath outPath;
	
    Vector3 startPos(0, 6, 0);
	Vector3 endPos(140, 6, 140);
	
	
	bool found = grid.FindPath(startPos, endPos, outPath);
	
	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		testNodes.push_back(pos);
	}
}

void DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];
		
			Debug::DrawLine(Vector3(a.x + 6, a.y, a.z + 6), Vector3(b.x + 6, b.y, b.z + 6), Vector4(0, 0, 1, 1));
		
	}
}


void TestStateMachine() {
	StateMachine * testMachine = new StateMachine();
	int data = 0;
	State* A = new State([&](float dt)->void
		{
		std::cout << "I��m in state A!\n";
		data++;
		}
	);
	
		State * B = new State([&](float dt)->void
			{
		std::cout << "I��m in state B!\n";
		data--;
		}
	);
		StateTransition* stateAB = new StateTransition(A, B, [&](void)->bool
			{
			return data > 10;
			}
	);
		StateTransition * stateBA = new StateTransition(B, A, [&](void)->bool
			{
			return data < 0;
			}
		);
		testMachine->AddState(A);
		testMachine->AddState(B);
		testMachine->AddTransition(stateAB);
		testMachine->AddTransition(stateBA);
		
			for (int i = 0; i < 100; ++i) {
			testMachine->Update(1.0f);
			
		}
		
}

/*

The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead. 

This time, we've added some extra functionality to the window class - we can
hide or show the 

*/
int main() {
	Window*w = Window::CreateGameWindow("CSC8503 Game technology!", 1280, 720);

	if (!w->HasInitialised()) {
		return -1;
	}	

	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);
	TestPathfinding();

	TutorialGame* g = new TutorialGame();
	w->GetTimer()->GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!
	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
		float dt = w->GetTimer()->GetTimeDeltaSeconds();
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NEXT)) {
			w->ShowConsole(false);
		}

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::T)) {
			w->SetWindowPosition(0, 0);
		}
		DisplayPathfinding();

		w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));

		g->UpdateGame(dt);
	}
	Window::DestroyGameWindow();
}