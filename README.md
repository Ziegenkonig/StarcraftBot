# Installation
Create an environment variable titled BW_API_PATH that points to the root folder for BWAPI which contains the headers and lib files to compile against.

After building the dll's successfully copy paste them to the bw-api/AI folder located within the starcraft root installation location. 

Open bwapi.ini and modify the following line "ai = bwapi-data/AI/ExampleAIModule.dll" to "ai = bwapi-data/AI/StarcraftBot.dll"