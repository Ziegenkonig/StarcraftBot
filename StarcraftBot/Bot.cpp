#include <iostream>

#include "Bot.h"

// Variable to keep track of the number of barracks
int barracks_count = 0;

//Handles building a barracks as long as no barracks exist and there are enough minerals
void buildBarracks(BWAPI::Unit u) {
	if (u->isGatheringMinerals() && barracks_count < 1 && BWAPI::Broodwar->self()->minerals() >= BWAPI::UnitTypes::Terran_Barracks.mineralPrice() + 50){
		//find a location for barracks and construct it
		BWAPI::TilePosition buildPosition = BWAPI::Broodwar->getBuildLocation(BWAPI::UnitTypes::Terran_Barracks, u->getTilePosition());
		u->build(BWAPI::UnitTypes::Terran_Barracks, buildPosition);
		barracks_count++;
	}
}

void BotAIModule::onStart() {
	// Hello World!
	BWAPI::Broodwar->sendText("Hello world!");

	// Print the map name.
	// BWAPI returns std::string when retrieving a string, don't forget to add .c_str() when printing!
	BWAPI::Broodwar << "The map is " << BWAPI::Broodwar->mapName() << "!" << std::endl;

	// Set the command optimization level so that common commands can be grouped
	// and reduce the bot's APM (Actions Per Minute).
	BWAPI::Broodwar->setCommandOptimizationLevel(2);
}

void BotAIModule::onEnd(bool isWinner) {
	if (isWinner)  {
	}
}

void BotAIModule::onFrame() {
	// Return if the BWAPI::Game is a replay or is paused
	if (BWAPI::Broodwar->isPaused() || !BWAPI::Broodwar->self())
		return;

	// Prevent spamming by only running our onFrame once every number of latency frames.
	// Latency frames are the number of frames before commands are processed.
	if (BWAPI::Broodwar->getFrameCount() % BWAPI::Broodwar->getLatencyFrames() != 0)
		return;

	// Iterate through all the BWAPI::Units that we own
	for (auto &u : BWAPI::Broodwar->self()->getUnits())
	{
		// Ignore the BWAPI::Unit if it no longer exists
		// Make sure to include this block when handling any BWAPI::Unit pointer!
		if (!u->exists())
			continue;

		// Ignore the BWAPI::Unit if it has one of the following status ailments
		if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised())
			continue;

		// Ignore the BWAPI::Unit if it is in one of the following states
		if (u->isLoaded() || !u->isPowered() || u->isStuck())
			continue;

		// Ignore the BWAPI::Unit if it is incomplete or busy constructing
		if (!u->isCompleted() || u->isConstructing())
			continue;

		// If the BWAPI::Unit is a worker BWAPI::Unit
		if (u->getType().isWorker()) {
			// if our worker is idle
			if (u->isIdle()) {
				if (u->isCarryingGas() || u->isCarryingMinerals())
					u->returnCargo();
				// The worker cannot harvest anything if it
				// is carrying a powerup such as a flag
				else if (!u->getPowerUp())  {                             
					// Harvest from the nearest mineral patch or gas refinery
					if (!u->gather(u->getClosestUnit(BWAPI::Filter::IsMineralField || BWAPI::Filter::IsRefinery))) {
						// If the call fails, then print the last BWAPI::Error message
						BWAPI::Broodwar << BWAPI::Broodwar->getLastError() << std::endl;
					}
				}
			}
			else {
				//Build barracks if possible
				buildBarracks(u);
			}
		}
		
		// A resource depot is a Command Center, Nexus, or Hatchery
		else if (u->getType().isResourceDepot()) {
			// Order the depot to construct more workers! But only when it is idle.
			if (u->isIdle() && !u->train(u->getType().getRace().getWorker())) {
				// If that fails, draw the BWAPI::Error at the location so that you can visibly see what went wrong!
				// However, drawing the BWAPI::Error once will only appear for a single frame
				// so create an event that keeps it on the screen for some frames
				BWAPI::Position pos = u->getPosition();
				BWAPI::Error lastErr = BWAPI::Broodwar->getLastError();
				BWAPI::Broodwar->registerEvent([pos, lastErr](BWAPI::Game*){ BWAPI::Broodwar->drawTextMap(pos, "%c%s", BWAPI::Text::White, lastErr.c_str()); },   // action
					nullptr,    // condition
					BWAPI::Broodwar->getLatencyFrames());  // frames to run

				// Retrieve the supply provider type in the case that we have run out of supplies
				BWAPI::UnitType supplyProviderType = u->getType().getRace().getSupplyProvider();
				static int lastChecked = 0; 

				// If we are supply blocked and haven't tried constructing more recently
				if (lastErr == BWAPI::Errors::Insufficient_Supply &&
					lastChecked + 400 < BWAPI::Broodwar->getFrameCount() &&
					BWAPI::Broodwar->self()->incompleteUnitCount(supplyProviderType) == 0) {
					lastChecked = BWAPI::Broodwar->getFrameCount();

					// Retrieve a BWAPI::Unit that is capable of constructing the supply needed
					BWAPI::Unit supplyBuilder = u->getClosestUnit(BWAPI::Filter::GetType == supplyProviderType.whatBuilds().first &&
						(BWAPI::Filter::IsIdle || BWAPI::Filter::IsGatheringMinerals) && BWAPI::Filter::IsOwned);
					// If a BWAPI::Unit was found
					if (supplyBuilder) {
						BWAPI::TilePosition targetBuildLocation = BWAPI::Broodwar->getBuildLocation(supplyProviderType, supplyBuilder->getTilePosition());
						if (targetBuildLocation) {
							// Register an event that draws the target build location
							BWAPI::Broodwar->registerEvent([targetBuildLocation, supplyProviderType](BWAPI::Game*) {
								BWAPI::Broodwar->drawBoxMap(BWAPI::Position(targetBuildLocation),
									BWAPI::Position(targetBuildLocation + supplyProviderType.tileSize()),
									BWAPI::Colors::Blue);
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

void BotAIModule::onNukeDetect(BWAPI::Position target) {
	// Check if the target is a valid BWAPI::Position
	if (target) {
		// if so, print the location of the nuclear strike target
		BWAPI::Broodwar << "Nuclear Launch Detected at " << target << std::endl;
	}

	// You can also retrieve all the nuclear missile targets using BWAPI::Broodwar->getNukeDots()!
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
	BWAPI::Broodwar << "The BWAPI::Game was saved to \"" << gameName << "\"" << std::endl;
}

void BotAIModule::onUnitComplete(BWAPI::Unit unit) {
	BWAPI::Broodwar << unit->getType().getName() << " is completed." << std::endl;
}

