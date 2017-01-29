#include "Bot.h"
#include <iostream>

using namespace BWAPI;
using namespace Filter;

void BotAIModule::onStart()
{
	// Hello World!
	Broodwar->sendText("Hello world!");

	// Print the map name.
	// BWAPI returns std::string when retrieving a string, don't forget to add .c_str() when printing!
	Broodwar << "The map is " << Broodwar->mapName() << "!" << std::endl;

	// Set the command optimization level so that common commands can be grouped
	// and reduce the bot's APM (Actions Per Minute).
	Broodwar->setCommandOptimizationLevel(2);
}

void BotAIModule::onEnd(bool isWinner) {
	if (isWinner) {
	}
}

void BotAIModule::onFrame()
{
	// Return if the game is a replay or is paused
	if (Broodwar->isPaused() || !Broodwar->self())
		return;

	// Prevent spamming by only running our onFrame once every number of latency frames.
	// Latency frames are the number of frames before commands are processed.
	if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
		return;

	// Iterate through all the units that we own
	for (auto &u : Broodwar->self()->getUnits())
	{
		// Ignore the unit if it no longer exists
		// Make sure to include this block when handling any Unit pointer!
		if (!u->exists())
			continue;

		// Ignore the unit if it has one of the following status ailments
		if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised())
			continue;

		// Ignore the unit if it is in one of the following states
		if (u->isLoaded() || !u->isPowered() || u->isStuck())
			continue;

		// Ignore the unit if it is incomplete or busy constructing
		if (!u->isCompleted() || u->isConstructing())
			continue;

		// If the unit is a worker unit
		if (u->getType().isWorker())
		{
			// if our worker is idle
			if (u->isIdle())
			{
				if (u->isCarryingGas() || u->isCarryingMinerals())
					u->returnCargo();
				// The worker cannot harvest anything if it
				// is carrying a powerup such as a flag
				else if (!u->getPowerUp()) 
				{                             
					// Harvest from the nearest mineral patch or gas refinery
					if (!u->gather(u->getClosestUnit(IsMineralField || IsRefinery))) {
						// If the call fails, then print the last error message
						Broodwar << Broodwar->getLastError() << std::endl;
					}
				}
			}
		}
		// A resource depot is a Command Center, Nexus, or Hatchery
		else if (u->getType().isResourceDepot()) {
			// Order the depot to construct more workers! But only when it is idle.
			if (u->isIdle() && !u->train(u->getType().getRace().getWorker())) {
				// If that fails, draw the error at the location so that you can visibly see what went wrong!
				// However, drawing the error once will only appear for a single frame
				// so create an event that keeps it on the screen for some frames
				Position pos = u->getPosition();
				Error lastErr = Broodwar->getLastError();
				Broodwar->registerEvent([pos, lastErr](Game*){ Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },   // action
					nullptr,    // condition
					Broodwar->getLatencyFrames());  // frames to run

				// Retrieve the supply provider type in the case that we have run out of supplies
				UnitType supplyProviderType = u->getType().getRace().getSupplyProvider();
				static int lastChecked = 0;

				// If we are supply blocked and haven't tried constructing more recently
				if (lastErr == Errors::Insufficient_Supply &&
					lastChecked + 400 < Broodwar->getFrameCount() &&
					Broodwar->self()->incompleteUnitCount(supplyProviderType) == 0) {
					lastChecked = Broodwar->getFrameCount();

					// Retrieve a unit that is capable of constructing the supply needed
					Unit supplyBuilder = u->getClosestUnit(GetType == supplyProviderType.whatBuilds().first &&
						(IsIdle || IsGatheringMinerals) &&
						IsOwned);
					// If a unit was found
					if (supplyBuilder) {
						TilePosition targetBuildLocation = Broodwar->getBuildLocation(supplyProviderType, supplyBuilder->getTilePosition());
						if (targetBuildLocation) {
							// Register an event that draws the target build location
							Broodwar->registerEvent([targetBuildLocation, supplyProviderType](Game*) {
								Broodwar->drawBoxMap(Position(targetBuildLocation),
									Position(targetBuildLocation + supplyProviderType.tileSize()),
									Colors::Blue);
								},
								nullptr,  // condition
								supplyProviderType.buildTime() + 100);  // frames to run

							// Order the builder to construct the supply structure
							supplyBuilder->build(supplyProviderType, targetBuildLocation);
						}
					}
				} 
			}
		}
	}
}

void BotAIModule::onSendText(std::string text) {

}

void BotAIModule::onReceiveText(BWAPI::Player player, std::string text) {

}

void BotAIModule::onPlayerLeft(BWAPI::Player player) {
}

void BotAIModule::onNukeDetect(BWAPI::Position target)
{
	// Check if the target is a valid position
	if (target) {
		// if so, print the location of the nuclear strike target
		Broodwar << "Nuclear Launch Detected at " << target << std::endl;
	}

	// You can also retrieve all the nuclear missile targets using Broodwar->getNukeDots()!
}

void BotAIModule::onUnitDiscover(BWAPI::Unit unit) {

}

void BotAIModule::onUnitEvade(BWAPI::Unit unit) {

}

void BotAIModule::onUnitShow(BWAPI::Unit unit) {

}

void BotAIModule::onUnitHide(BWAPI::Unit unit) {

}

void BotAIModule::onUnitCreate(BWAPI::Unit unit) {

}

void BotAIModule::onUnitDestroy(BWAPI::Unit unit) {
}

void BotAIModule::onUnitMorph(BWAPI::Unit unit) {

}

void BotAIModule::onUnitRenegade(BWAPI::Unit unit) {

}

void BotAIModule::onSaveGame(std::string gameName) {
	Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void BotAIModule::onUnitComplete(BWAPI::Unit unit) {
	Broodwar << unit->getType().getName() << " is completed." << std::endl;
}