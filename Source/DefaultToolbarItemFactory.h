//
//  DefaultToolbarItemFactory.h
//  Synthlab
//
//  Created by Matthias Pueski on 27.04.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#ifndef DefaultToolbarItemFactory_h
#define DefaultToolbarItemFactory_h


class DefaultToolbarItemFactory : public juce::ToolbarItemFactory
{
public:
	DefaultToolbarItemFactory() {

	}

	~DefaultToolbarItemFactory() {
	}

	
	enum oolbarItemIds
	{
		doc_new = 1,
		doc_open = 2,
		doc_save = 3,
		delete_element = 4,
		app_settings = 5,
		app_undo = 6,
		app_redo = 7,
		app_layer = 8,
		mod_sources = 9,
		app_browser = 10,
		app_play = 11,
		app_stop = 12,
		app_pause = 13,
		app_fast_backwards = 14,
		app_backwards = 15,
		app_forward = 16,
		app_fast_forward = 17

	};

	void getAllToolbarItemIds(juce::Array<int>& ids) override
	{
		// This returns the complete list of all item IDs that are allowed to
		// go in our toolbar. Any items you might want to add must be listed here. The
		// order in which they are listed will be used by the toolbar customisation panel.

		ids.add(doc_new);
		ids.add(doc_open);
		ids.add(doc_save);
		ids.add(delete_element);
		ids.add(app_settings);
		ids.add(app_undo);
		ids.add(app_redo);
		ids.add(app_layer);
		ids.add(mod_sources);
		ids.add(app_browser);
		ids.add(app_play);
		ids.add(app_stop);
		ids.add(app_pause);
		ids.add(app_fast_backwards);
		ids.add(app_backwards);
		ids.add(app_forward);
		ids.add(app_fast_forward);

	}

	void getDefaultItemSet(juce::Array<int>& ids) override
	{
		// This returns an ordered list of the set of items that make up a
		// toolbar's default set. Not all items need to be on this list, and
		// items can appear multiple times (e.g. the separators used here).
		ids.add(doc_new);
		ids.add(doc_open);
		ids.add(doc_save);
		ids.add(delete_element);
		ids.add(app_settings);
		ids.add(app_undo);
		ids.add(app_redo);
		ids.add(app_layer);
		ids.add(mod_sources);
		ids.add(app_browser);
		ids.add(app_play);
		ids.add(app_stop);
		ids.add(app_pause);
		ids.add(app_fast_backwards);
		ids.add(app_backwards);
		ids.add(app_forward);
		ids.add(app_fast_forward);

	}

	juce::ToolbarItemComponent* createItem(int itemId) override
	{

		juce::ToolbarItemComponent* b;
		switch (itemId)
		{
		case doc_new:
			b = new juce::ToolbarButton(itemId, "New", getImage(itemId), getImage(itemId));
			return b;
		case doc_open:
			b = new juce::ToolbarButton(itemId, "Open", getImage(itemId), getImage(itemId));
			return b;
		case doc_save:
			b = new juce::ToolbarButton(itemId, "Save", getImage(itemId), getImage(itemId));
			return b;
		case delete_element:
			b = new juce::ToolbarButton(itemId, "Delete", getImage(itemId), getImage(itemId));
			return b;
		case app_settings:
			b = new juce::ToolbarButton(itemId, "Settings", getImage(itemId), getImage(itemId));
			return b;
		case app_undo:
			b = new juce::ToolbarButton(itemId, "Undo", getImage(itemId), getImage(itemId));
			return b;
		case app_redo:
			b = new juce::ToolbarButton(itemId, "Redo", getImage(itemId), getImage(itemId));
			return b;
		case app_browser:
			b = new juce::ToolbarButton(itemId, "Modules", getImage(itemId), getImage(itemId));
			return b;
		case app_layer:
			b = new ToolbarComboBox(itemId);
			return b;
		case mod_sources:
			b = new juce::ToolbarButton(itemId, "Sound sources", getImage(itemId), getImage(itemId));
			return b;
		case app_stop:
			b = new juce::ToolbarButton(itemId, "Stop", getImage(itemId), getImage(itemId));
			return b;
		case app_play:
			b = new juce::ToolbarButton(itemId, "Play", getImage(itemId), getImage(itemId));
			return b;
		case app_pause:
			b = new juce::ToolbarButton(itemId, "Pause", getImage(itemId), getImage(itemId));
			return b;
		case app_fast_backwards:
			b = new juce::ToolbarButton(itemId, "Fast back", getImage(itemId), getImage(itemId));
			return b;
		case app_fast_forward:
			b = new juce::ToolbarButton(itemId, "Fast forward", getImage(itemId), getImage(itemId));
			return b;
		case app_backwards:
			b = new juce::ToolbarButton(itemId, "Back", getImage(itemId), getImage(itemId));
			return b;
		case app_forward:
			b = new juce::ToolbarButton(itemId, "Forward", getImage(itemId), getImage(itemId));
			return b;

		default:
			break;
		}

		return nullptr;
	}

	std::unique_ptr<juce::Drawable> getImage(int itemId) {

		// const juce::DrawableImage* img = new juce::DrawableImage();

		std::unique_ptr<juce::DrawableImage> di = std::make_unique<juce::DrawableImage>();

		if (itemId == doc_new)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::new_png, BinaryData::new_pngSize));
		else if (itemId == doc_open)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::open_png, BinaryData::open_pngSize));
		else if (itemId == doc_save)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::save_png, BinaryData::save_pngSize));
		else if (itemId == delete_element)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::delete_png, BinaryData::delete_pngSize));
		else if (itemId == app_settings)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::settings_png, BinaryData::settings_pngSize));
		else if (itemId == app_undo)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::undo_png, BinaryData::undo_pngSize));
		else if (itemId == app_redo)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::redo_png, BinaryData::redo_pngSize));
		else if (itemId == mod_sources)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::saw_png, BinaryData::saw_pngSize));
		else if (itemId == app_browser)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize));
		else if (itemId == app_play)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::playsolid_png, BinaryData::playsolid_pngSize));
		else if (itemId == app_stop)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::stop_png, BinaryData::stop_pngSize));
		else if (itemId == app_pause)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::pausesolid_png, BinaryData::pausesolid_pngSize));
		else if (itemId == app_fast_backwards)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::fastbackwardsolid_png, BinaryData::fastbackwardsolid_pngSize));
		else if (itemId == app_fast_forward)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::fastforwardsolid_png, BinaryData::fastforwardsolid_pngSize));
		else if (itemId == app_backwards)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::backwardsolid_png, BinaryData::backwardsolid_pngSize));
		else if (itemId == app_forward)
			di->setImage(juce::ImageCache::getFromMemory(BinaryData::forwardsolid_png, BinaryData::forwardsolid_pngSize));


		return di;
	}

	int numItems() {
		return 17;
	}

};

#endif /* DefaultToolbarItemFactory_h */
