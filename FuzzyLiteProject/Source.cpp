#include <iostream> 
#include "FlsImporter.h"
#include <fl/Headers.h>
#include <chrono>
#include <thread>

using namespace fl;

float clamp(float value, float minValue, float maxValue)
{
	return std::max(minValue, std::min(value, maxValue));
}

int main()
{
	Engine* engine = new Engine();

	// Input Variable: Obstacle
	InputVariable* obstacle = new InputVariable;
	obstacle->setName("obstacle");
	obstacle->setRange(0.0, 1.0);
	obstacle->addTerm(new Triangle("Left", 0, 0.05, 0.6));
	obstacle->addTerm(new Triangle("Right", 0.4, 0.95, 1.0));
	engine->addInputVariable(obstacle);

	// Input Variable: Collectible
	InputVariable* collectible = new InputVariable;
	collectible->setName("collectible");
	collectible->setRange(0.0, 1.0);
	collectible->addTerm(new Triangle("Left", 0.0, 0.1, 0.3));
	collectible->addTerm(new Triangle("Right", 0.7, 0.9, 1.0));
	engine->addInputVariable(collectible);

	// Input Variable: Speed
	InputVariable* speed = new InputVariable;
	speed->setName("speed");
	speed->setRange(0.0, 1.0);
	speed->addTerm(new Triangle("movieLeft", 0, 0.3, 0.6));
	speed->addTerm(new Triangle("movieRight", 0.4, 0.7, 1.0));
	speed->addTerm(new Triangle("none", 0.2, 0.5, 0.8));
	engine->addInputVariable(speed);

	// Output Variable: Steering Decision
	OutputVariable* mSteer = new OutputVariable;
	mSteer->setName("mSteer");
	mSteer->setRange(0.0, 1.0);
	mSteer->addTerm(new Triangle("Left", 0, 0.3, 0.6));
	mSteer->addTerm(new Triangle("Right", 0.4, 0.7, 1.0));
	mSteer->addTerm(new Triangle("slightLeft", 0, 0.15, 0.3));
	mSteer->addTerm(new Triangle("slightRight", 0.7, 0.85, 1.0));
	mSteer->setDefuzzifier(new WeightedAverage());
	mSteer->setAggregation(new Maximum);
	mSteer->setDefaultValue(fl::nan);
	engine->addOutputVariable(mSteer);

	// Rule Block
	RuleBlock* mamdani = new RuleBlock;
	mamdani->setConjunction(new Minimum);
	mamdani->setImplication(new AlgebraicProduct);
	mamdani->addRule(Rule::parse("if obstacle is Left then mSteer is Right", engine));
	mamdani->addRule(Rule::parse("if obstacle is Right then mSteer is Left", engine));
	mamdani->addRule(Rule::parse("if obstacle is not Left and collectible is Left then mSteer is slightLeft", engine));
	mamdani->addRule(Rule::parse("if obstacle is not Right and collectible is Right then mSteer is slightRight", engine));
	engine->addRuleBlock(mamdani);

	// Check if engine is ready
	std::string status;
	if (!engine->isReady(&status))
	{
		std::cerr << "Engine not ready: " << status << std::endl;
		return 1;
	}

	// Game Settings
	const int GAME_WIDTH = 20, GAME_HEIGHT = 10;
	int carPos = GAME_WIDTH / 2, score = 0;
	int obstaclePosX = rand() % GAME_WIDTH, obstaclePosY = 0;
	int collectiblePosX = rand() % GAME_WIDTH, collectiblePosY = 0;

	while (true)
	{
		// Clear screen
		std::cout << "\033[2J\033[1;1H";

		// Update obstacle and collectible positions
		obstaclePosY = (obstaclePosY + 1) % GAME_HEIGHT;
		collectiblePosY = (collectiblePosY + 1) % GAME_HEIGHT;

		if (obstaclePosY == 0) obstaclePosX = rand() % GAME_WIDTH;
		if (collectiblePosY == 0) collectiblePosX = rand() % GAME_WIDTH;

		// Calculate inputs for the fuzzy engine
		float obstacleValue = 1.0f - std::abs(carPos - obstaclePosX) / float(GAME_WIDTH);
		float collectibleValue = 1.0f - std::abs(carPos - collectiblePosX) / float(GAME_WIDTH);

		obstacle->setValue(obstacleValue);
		collectible->setValue(clamp(collectibleValue * 1.5f, 0.0f, 1.0f));
		// Get fuzzy logic inputs
		std::cout << "Collectible X: " << collectiblePosX << ", Obstacle X: " << obstaclePosX << ", Car X: " << carPos << std::endl;
		std::cout << "Obstacle Value: " << obstacleValue << ", Collectible Value: " << collectibleValue << std::endl;

		engine->process();

		// Get steering output
		float steerOutput = clamp(mSteer->getValue(), 0.0f, 1.0f);
		std::cout << "Steer Output: " << steerOutput << std::endl;

		// Adjust car position based on fuzzy output
		if (steerOutput <= 0.3f && carPos > 0) carPos--;
		else if (steerOutput >= 0.7f && carPos < GAME_WIDTH - 1) carPos++;

		// Display game state
		for (int y = 0; y < GAME_HEIGHT; y++)
		{
			for (int x = 0; x < GAME_WIDTH; x++)
			{
				if (y == GAME_HEIGHT - 1 && x == carPos) std::cout << "C"; // Car
				else if (y == obstaclePosY && x == obstaclePosX) std::cout << "O"; // Obstacle
				else if (y == collectiblePosY && x == collectiblePosX) std::cout << "$"; // Collectible, $ used cause £ caused an Error
				else std::cout << ".";
			}
			std::cout << std::endl;
		}

		if (obstacleValue < 0.0 || obstacleValue > 1.0 || collectibleValue < 0.1 || collectibleValue > 1.0)
		{
			std::cout << "Error: Input values must be between 0.0 and 1.0." << std::endl;
			continue;
		}

		std::cout << "Unclamped Steer Output: " << mSteer->getValue() << std::endl;

		// Collision or collectible check
		if (carPos == obstaclePosX && obstaclePosY == GAME_HEIGHT - 1)
		{
			std::cout << "You Crashed! Game Over." << std::endl;
			break;
		}
		if (carPos == collectiblePosX && collectiblePosY == GAME_HEIGHT - 1)
		{
			std::cout << "Collectible Aquired!" << std::endl;
			score += 10;
		}

		std::cout << "Score: " << score << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	delete engine;
	return 0;
}
