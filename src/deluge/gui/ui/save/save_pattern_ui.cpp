/*
 * Copyright © 2014-2023 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Synthstrom Audible Deluge Firmware is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/ui/save/save_pattern_ui.h"
#include "definitions_cxx.hpp"
#include "gui/context_menu/overwrite_file.h"
#include "gui/l10n/l10n.h"
#include "gui/ui/keyboard/keyboard_screen.h"
#include "gui/views/view.h"
#include "hid/buttons.h"
#include "hid/display/display.h"
#include "hid/display/oled.h"
#include "hid/led/indicator_leds.h"
#include "hid/matrix/matrix_driver.h"
#include "model/clip/instrument_clip.h"
#include "model/clip/instrument_clip_minder.h"
#include "model/instrument/kit.h"
#include "model/song/song.h"
#include "storage/storage_manager.h"
#include "util/functions.h"
#include "util/lookuptables/lookuptables.h"
#include <string.h>

using namespace deluge;

#define PATTERN_RHYTMIC_DEFAULT_FOLDER "PATTERNS/RHYTMIC"
#define PATTERN_MELODIC_DEFAULT_FOLDER "PATTERNS/MELODIC"

SavePatternUI savePatternUI{};

bool SavePatternUI::opened() {

	Instrument* currentInstrument = getCurrentInstrument();
	// Must set this before calling SaveUI::opened(), which uses this to work out folder name
	outputTypeToLoad = currentInstrument->type;

	bool success = SaveUI::opened();
	if (!success) { // In this case, an error will have already displayed.
doReturnFalse:
		renderingNeededRegardlessOfUI(); // Because unlike many UIs we've already gone and drawn the QWERTY interface on
		                                 // the pads.
		return false;
	}

	enteredText.set("Pattern");
	enteredTextEditPos = enteredText.getLength();
	currentFolderIsEmpty = false;

	std::string patternFolder = "";
	if (getCurrentOutputType() == OutputType::KIT) {
		patternFolder = PATTERN_RHYTMIC_DEFAULT_FOLDER;
	}
	else{
		patternFolder = PATTERN_MELODIC_DEFAULT_FOLDER;
	}

	char const* defaultDir = patternFolder.c_str();
tryDefaultDir:
	currentDir.set(defaultDir);

	fileIcon = deluge::hid::display::OLED::midiIcon;
	fileIconPt2 = deluge::hid::display::OLED::midiIconPt2;
	fileIconPt2Width = 0;

	title = "Save Pattern";

	Error error = arrivedInNewFolder(0, enteredText.get(), defaultDir);
	if (error != Error::NONE) {
gotError:
		display->displayError(error);
		goto doReturnFalse;
	}

	focusRegained();
	return true;
}

bool SavePatternUI::performSave(bool mayOverwrite) {
	if (display->have7SEG()) {
		display->displayLoadingAnimation();
	}


	if (display->have7SEG()) {
		display->displayLoadingAnimation();
	}

	//InstrumentClip* clipToSave = (InstrumentClip*)getCurrentInstrumentClip();

	if (getCurrentOutputType() != OutputType::SYNTH) {
		defaultFolder = PATTERN_MELODIC_DEFAULT_FOLDER;
	}
	else {
		defaultFolder = PATTERN_RHYTMIC_DEFAULT_FOLDER;
	}

	String filePath;
	Error error = getCurrentFilePath(&filePath);
	if (error != Error::NONE) {
fail:
		display->displayError(error);
		return false;
	}

	error = StorageManager::createXMLFile(filePath.get(), smSerializer, mayOverwrite, false);

	if (error == Error::FILE_ALREADY_EXISTS) {
		gui::context_menu::overwriteFile.currentSaveUI = this;

		bool available = gui::context_menu::overwriteFile.setupAndCheckAvailability();

		if (available) { // Will always be true.
			display->setNextTransitionDirection(1);
			openUI(&gui::context_menu::overwriteFile);
			return true;
		}
		else {
			error = Error::UNSPECIFIED;
			goto fail;
		}
	}

	else if (error != Error::NONE) {
		goto fail;
	}

	if (display->haveOLED()) {
		deluge::hid::display::OLED::displayWorkingAnimation("Saving");
	}

	Serializer& writer = GetSerializer();

	//clipToSave->writePatternToFile(writer,currentSong);
	instrumentClipView.copyNotesToFile(writer);


	writer.closeFileAfterWriting();

	display->removeWorkingAnimation();
	if (error != Error::NONE) {
		goto fail;
	}


	display->consoleText(deluge::l10n::get(deluge::l10n::String::STRING_FOR_PATTERN_SAVED));
	close();
	return true;
}
