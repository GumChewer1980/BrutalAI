/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "TestState.h"
#include "TestPaletteState.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Mod/Mod.h"
#include <fstream>
#include "../Engine/Exception.h"
#include "../Engine/FileMap.h"
#include "../Engine/Logger.h"
#include "../Mod/RuleTerrain.h"
#include "../Mod/MapBlock.h"
#include "../Mod/MapDataSet.h"
#include "../Mod/MapData.h"
#include "../Mod/RuleUfo.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the test screen.
 */
TestState::TestState()
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(300, 17, 10, 7);
	_txtPalette = new Text(66, 9, 10, 30);
	_cbxPalette = new ComboBox(this, 114, 16, 78, 26);
	_cbxPaletteAction = new ComboBox(this, 114, 16, 198, 26);
	_txtTestCase = new Text(66, 9, 10, 50);
	_cbxTestCase = new ComboBox(this, 234, 16, 78, 46);
	_txtDescription = new Text(300, 25, 10, 66);
	_lstOutput = new TextList(284, 80, 10, 94);
	_btnRun = new TextButton(146, 16, 10, 176);
	_btnCancel = new TextButton(146, 16, 164, 176);

	// Set palette
	setInterface("tests");

	add(_window, "window", "tests");
	add(_txtTitle, "heading", "tests");
	add(_txtPalette, "text", "tests");
	add(_txtTestCase, "text", "tests");
	add(_txtDescription, "heading", "tests");
	add(_lstOutput, "text", "tests");
	add(_btnRun, "button2", "tests");
	add(_btnCancel, "button2", "tests");
	add(_cbxTestCase, "button1", "tests"); // add as last (display over all other components)
	add(_cbxPalette, "button1", "tests"); // add as last (display over all other components)
	add(_cbxPaletteAction, "button1", "tests"); // add as last (display over all other components)

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK05.SCR"));

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TEST_SCREEN"));

	_txtPalette->setText(tr("STR_PALETTE"));

	for (auto pal : _game->getMod()->getPalettes())
	{
		if (pal.first.find("BACKUP_") != 0)
		{
			_paletteList.push_back(pal.first);
		}
	}

	_cbxPalette->setOptions(_paletteList);

	std::vector<std::string> _actionList;
	_actionList.push_back("STR_PREVIEW");
	_actionList.push_back("STR_TINY_BORDERLESS");
	_actionList.push_back("STR_TINY_BORDER");
	_actionList.push_back("STR_SMALL_LOW_CONTRAST");
	_actionList.push_back("STR_SMALL_HIGH_CONTRAST");
	_actionList.push_back("STR_BIG_LOW_CONTRAST");
	_actionList.push_back("STR_BIG_HIGH_CONTRAST");

	_cbxPaletteAction->setOptions(_actionList);
	_cbxPaletteAction->onChange((ActionHandler)&TestState::cbxPaletteAction);

	_txtTestCase->setText(tr("STR_TEST_CASE"));

	_testCases.push_back("STR_BAD_NODES");
	_testCases.push_back("STR_ZERO_COST_MOVEMENT");

	_cbxTestCase->setOptions(_testCases);
	_cbxTestCase->onChange((ActionHandler)&TestState::cbxTestCaseChange);

	_txtDescription->setWordWrap(true);
	cbxTestCaseChange(0); // update description

	_lstOutput->setColumns(1, 284);
	_lstOutput->setBackground(_window);
	_lstOutput->setWordWrap(true);

	_btnRun->setText(tr("STR_RUN"));
	_btnRun->onMouseClick((ActionHandler)&TestState::btnRunClick);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&TestState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&TestState::btnCancelClick, Options::keyCancel);
}

TestState::~TestState()
{
}

/**
* Updates the test case description.
* @param action Pointer to an action.
*/
void TestState::cbxTestCaseChange(Action *)
{
	_lstOutput->clearList();

	size_t index = _cbxTestCase->getSelected();
	_txtDescription->setText(tr(_testCases[index]+"_DESC"));
}

/**
* Returns to the previous screen.
* @param action Pointer to an action.
*/
void TestState::btnRunClick(Action *action)
{
	_lstOutput->clearList();

	size_t index = _cbxTestCase->getSelected();
	switch (index)
	{
		case 0: testCase0(); break;
		case 1: testCase1(); break;
		default: break;
	}
}

/**
* Returns to the previous screen.
* @param action Pointer to an action.
*/
void TestState::btnCancelClick(Action *action)
{
	_game->popState();
}

/**
 * Shows palette preview with low contrast.
 * @param action Pointer to an action.
 */
void TestState::cbxPaletteAction(Action *action)
{
	size_t index = _cbxPalette->getSelected();
	const std::string palette = _paletteList[index];

	PaletteActionType type = (PaletteActionType)_cbxPaletteAction->getSelected();

	_game->pushState(new TestPaletteState(palette, type));
}

void TestState::testCase1()
{
	_lstOutput->addRow(1, tr("STR_TESTS_STARTING").c_str());
	_lstOutput->addRow(1, tr("STR_CHECKING_TERRAIN").c_str());
	int total = 0;
	std::map<std::string, std::set<int>> uniqueResults;
	for (auto terrainName : _game->getMod()->getTerrainList())
	{
		RuleTerrain *terrainRule = _game->getMod()->getTerrain(terrainName);
		total += checkMCD(terrainRule, uniqueResults);
	}
	_lstOutput->addRow(1, tr("STR_CHECKING_UFOS").c_str());
	for (auto ufoName : _game->getMod()->getUfosList())
	{
		RuleUfo *ufoRule = _game->getMod()->getUfo(ufoName);
		RuleTerrain *terrainRule = ufoRule->getBattlescapeTerrainData();
		if (!terrainRule)
		{
			continue;
		}
		total += checkMCD(terrainRule, uniqueResults);
	}
	_lstOutput->addRow(1, tr("STR_CHECKING_CRAFT").c_str());
	for (auto craftName : _game->getMod()->getCraftsList())
	{
		RuleCraft *craftRule = _game->getMod()->getCraft(craftName);
		RuleTerrain *terrainRule = craftRule->getBattlescapeTerrainData();
		if (!terrainRule)
		{
			continue;
		}
		total += checkMCD(terrainRule, uniqueResults);
	}
	if (total > 0)
	{
		_lstOutput->addRow(1, tr("STR_TESTS_ERRORS_FOUND").arg(total).c_str());
		_lstOutput->addRow(1, tr("STR_DETAILED_INFO_IN_LOG_FILE").c_str());
	}
	else
	{
		_lstOutput->addRow(1, tr("STR_TESTS_NO_ERRORS_FOUND").c_str());
	}

	// summary (unique)
	if (total > 0)
	{
		Log(LOG_INFO) << "----------";
		Log(LOG_INFO) << "SUMMARY";
		Log(LOG_INFO) << "----------";
		for (auto mapItem : uniqueResults)
		{
			std::ostringstream ss;
			ss << mapItem.first << ": ";
			bool first = true;
			for (auto setItem : mapItem.second)
			{
				if (!first)
				{
					ss << ", ";
				}
				else
				{
					first = false;
				}
				ss << setItem;
			}
			std::string line = ss.str();
			Log(LOG_INFO) << line;
		}
	}
	_lstOutput->addRow(1, tr("STR_TESTS_FINISHED").c_str());
}

int TestState::checkMCD(RuleTerrain *terrainRule, std::map<std::string, std::set<int>> &uniqueResults)
{
	int errors = 0;
	for (auto myMapDataSet : *terrainRule->getMapDataSets())
	{
		int index = 0;
		myMapDataSet->loadData();
		for (auto myMapData : *myMapDataSet->getObjects())
		{
			if (myMapData->getObjectType() == O_FLOOR)
			{
				if (myMapData->getTUCost(MT_WALK) < 1)
				{
					std::ostringstream ss;
					ss << " walk:" << myMapData->getTUCost(MT_WALK) << " terrain:" << terrainRule->getName() << " dataset:" << myMapDataSet->getName() << " index:" << index;
					std::string str = ss.str();
					Log(LOG_INFO) << "Zero movement cost on floor object: " << str << ". Found using OXCE+ test cases.";
					errors++;
					uniqueResults[myMapDataSet->getName()].insert(index);
				}
				if (myMapData->getTUCost(MT_FLY) < 1)
				{
					std::ostringstream ss;
					ss << "  fly:" << myMapData->getTUCost(MT_FLY) << " terrain:" << terrainRule->getName() << " dataset:" << myMapDataSet->getName() << " index:" << index;
					std::string str = ss.str();
					Log(LOG_INFO) << "Zero movement cost on floor object: " << str << ". Found using OXCE+ test cases.";
					errors++;
					uniqueResults[myMapDataSet->getName()].insert(index);
				}
				if (myMapData->getTUCost(MT_SLIDE) < 1)
				{
					std::ostringstream ss;
					ss << "slide:" << myMapData->getTUCost(MT_SLIDE) << " terrain:" << terrainRule->getName() << " dataset:" << myMapDataSet->getName() << " index:" << index;
					std::string str = ss.str();
					Log(LOG_INFO) << "Zero movement cost on floor object: " << str << ". Found using OXCE+ test cases.";
					errors++;
					uniqueResults[myMapDataSet->getName()].insert(index);
				}
			}
			index++;
		}
		//myMapDataSet->unloadData();
	}
	return errors;
}

void TestState::testCase0()
{
	_lstOutput->addRow(1, tr("STR_TESTS_STARTING").c_str());
	_lstOutput->addRow(1, tr("STR_CHECKING_TERRAIN").c_str());
	int total = 0;
	for (std::vector<std::string>::const_iterator i = _game->getMod()->getTerrainList().begin(); i != _game->getMod()->getTerrainList().end(); ++i)
	{
		RuleTerrain *terrRule = _game->getMod()->getTerrain((*i));
		for (std::vector<MapBlock*>::iterator j = terrRule->getMapBlocks()->begin(); j != terrRule->getMapBlocks()->end(); ++j)
		{
			total += checkRMP((*j));
		}
	}
	_lstOutput->addRow(1, tr("STR_CHECKING_UFOS").c_str());
	for (std::vector<std::string>::const_iterator i = _game->getMod()->getUfosList().begin(); i != _game->getMod()->getUfosList().end(); ++i)
	{
		RuleUfo *ufoRule = _game->getMod()->getUfo((*i));
		if (ufoRule->getBattlescapeTerrainData())
		{
			for (std::vector<MapBlock*>::iterator j = ufoRule->getBattlescapeTerrainData()->getMapBlocks()->begin(); j != ufoRule->getBattlescapeTerrainData()->getMapBlocks()->end(); ++j)
			{
				total += checkRMP((*j));
			}
		}
	}
	_lstOutput->addRow(1, tr("STR_CHECKING_CRAFT").c_str());
	for (std::vector<std::string>::const_iterator i = _game->getMod()->getCraftsList().begin(); i != _game->getMod()->getCraftsList().end(); ++i)
	{
		RuleCraft *craftRule = _game->getMod()->getCraft((*i));
		if (craftRule->getBattlescapeTerrainData())
		{
			for (std::vector<MapBlock*>::iterator j = craftRule->getBattlescapeTerrainData()->getMapBlocks()->begin(); j != craftRule->getBattlescapeTerrainData()->getMapBlocks()->end(); ++j)
			{
				total += checkRMP((*j));
			}
		}
	}
	if (total > 0)
	{
		_lstOutput->addRow(1, tr("STR_TESTS_ERRORS_FOUND").arg(total).c_str());
		_lstOutput->addRow(1, tr("STR_DETAILED_INFO_IN_LOG_FILE").c_str());
	}
	else
	{
		_lstOutput->addRow(1, tr("STR_TESTS_NO_ERRORS_FOUND").c_str());
	}
	_lstOutput->addRow(1, tr("STR_TESTS_FINISHED").c_str());
}

int TestState::checkRMP(MapBlock *mapblock)
{
	// Z-dimension is not in the ruleset, it's hacked on-the-fly
	int mapblockSizeZ = loadMAP(mapblock);

	unsigned char value[24];
	std::ostringstream filename;
	filename << "ROUTES/" << mapblock->getName() << ".RMP";

	// Load file
	std::ifstream mapFile(FileMap::getFilePath(filename.str()).c_str(), std::ios::in | std::ios::binary);
	if (!mapFile)
	{
		throw Exception(filename.str() + " not found");
	}

	int nodesAdded = 0;
	int errors = 0;
	while (mapFile.read((char*)&value, sizeof(value)))
	{
		int pos_x = value[1];
		int pos_y = value[0];
		int pos_z = value[2];
		if (pos_x >= 0 && pos_x < mapblock->getSizeX() &&
			pos_y >= 0 && pos_y < mapblock->getSizeY() &&
			pos_z >= 0 && pos_z < mapblockSizeZ) // using "mapblockSizeZ" instead of "mapblock->getSizeZ()"!!
		{
			// all ok
		}
		else
		{
			Log(LOG_INFO) << "Bad node in RMP file: " << filename.str() << " Node #" << nodesAdded << " is outside map boundaries at X:" << pos_x << " Y:" << pos_y << " Z:" << pos_z << ". Found using OXCE+ test cases.";
			errors++;
		}
		nodesAdded++;
	}

	if (!mapFile.eof())
	{
		throw Exception("Invalid RMP file: " + filename.str());
	}

	mapFile.close();

	return errors;
}

int TestState::loadMAP(MapBlock *mapblock)
{
	int sizez;
	char size[3];
	std::ostringstream filename;
	filename << "MAPS/" << mapblock->getName() << ".MAP";

	// Load file
	std::ifstream mapFile(FileMap::getFilePath(filename.str()).c_str(), std::ios::in | std::ios::binary);
	if (!mapFile)
	{
		throw Exception(filename.str() + " not found");
	}

	mapFile.read((char*)&size, sizeof(size));
	//sizey = (int)size[0];
	//sizex = (int)size[1];
	sizez = (int)size[2];

	//mapblock->setSizeZ(sizez); // commented out, for testing purposes we don't need to HACK it like in real code

	mapFile.close();

	return sizez;
}

}
