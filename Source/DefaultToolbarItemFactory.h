//
//  DefaultToolbarItemFactory.h
//  Synthlab
//
//  Created by Matthias Pueski on 27.04.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#ifndef DefaultToolbarItemFactory_h
#define DefaultToolbarItemFactory_h


class DefaultToolbarItemFactory   : public ToolbarItemFactory
{
public:
    DefaultToolbarItemFactory() {
        
    }
    
    ~DefaultToolbarItemFactory() {
    }
    
    //==============================================================================
    // Each type of item a toolbar can contain must be given a unique ID. These
    // are the ones we'll use in this demo.
    enum oolbarItemIds
    {
        doc_new         = 1,
        doc_open        = 2,
        doc_save        = 3,
        delete_element  = 4,
        app_settings    = 5,
        app_undo        = 6,
        app_redo        = 7,
        app_layer       = 8,
        mod_sources     = 9
        
        
    };
    
    void getAllToolbarItemIds (Array<int>& ids) override
    {
        // This returns the complete list of all item IDs that are allowed to
        // go in our toolbar. Any items you might want to add must be listed here. The
        // order in which they are listed will be used by the toolbar customisation panel.
        
        ids.add (doc_new);
        ids.add (doc_open);
        ids.add (doc_save);
        ids.add (delete_element);
        ids.add (app_settings);
        ids.add (app_undo);
        ids.add (app_redo);
        ids.add (app_layer);
        ids.add (mod_sources);
    }
    
    void getDefaultItemSet (Array<int>& ids) override
    {
        // This returns an ordered list of the set of items that make up a
        // toolbar's default set. Not all items need to be on this list, and
        // items can appear multiple times (e.g. the separators used here).
        ids.add (doc_new);
        ids.add (doc_open);
        ids.add (doc_save);
        ids.add (delete_element);
        ids.add (app_settings);
        ids.add (app_undo);
        ids.add (app_redo);
        ids.add (app_layer);
        ids.add (mod_sources);
    }
    
    ToolbarItemComponent* createItem (int itemId) override
    {
        
        ToolbarItemComponent* b;
        switch (itemId)
        {
            case doc_new:
                b = new ToolbarButton(itemId,"New",getImage(itemId), getImage(itemId));
                return b;
            case doc_open:
                b = new ToolbarButton(itemId,"Open",getImage(itemId), getImage(itemId));
                return b;
            case doc_save:
                b = new ToolbarButton(itemId,"Save",getImage(itemId), getImage(itemId));
                return b;
            case delete_element:
                b = new ToolbarButton(itemId,"Delete",getImage(itemId), getImage(itemId));
                return b;
            case app_settings:
                b = new ToolbarButton(itemId,"Settings",getImage(itemId), getImage(itemId));
                return b;
            case app_undo:
                b = new ToolbarButton(itemId,"Undo",getImage(itemId), getImage(itemId));
                return b;
            case app_redo:
                b = new ToolbarButton(itemId,"Redo",getImage(itemId), getImage(itemId));
                return b;
            case app_layer:
                b = new ToolbarComboBox(itemId);
                return b;
            case mod_sources:
                b = new ToolbarButton(itemId,"Sound sources",getImage(itemId), getImage(itemId));
                return b;
                
            default:
                break;
        }
        
        return nullptr;
    }
    
    Drawable* getImage(int itemId) {
        
        DrawableImage* di = new DrawableImage();
        
        if (itemId == 1)
            di->setImage(ImageCache::getFromMemory(BinaryData::new_png, BinaryData::new_pngSize));
        else if(itemId == 2)
            di->setImage(ImageCache::getFromMemory(BinaryData::open_png, BinaryData::open_pngSize));
        else if(itemId == 3)
            di->setImage(ImageCache::getFromMemory(BinaryData::save_png, BinaryData::save_pngSize));
        else if(itemId == 4)
            di->setImage(ImageCache::getFromMemory(BinaryData::delete_png, BinaryData::delete_pngSize));
        else if(itemId == 5)
            di->setImage(ImageCache::getFromMemory(BinaryData::settings_png, BinaryData::settings_pngSize));
        else if(itemId == 6)
            di->setImage(ImageCache::getFromMemory(BinaryData::undo_png, BinaryData::undo_pngSize));
        else if(itemId == 7)
            di->setImage(ImageCache::getFromMemory(BinaryData::redo_png, BinaryData::redo_pngSize));
        else if(itemId == 9)
            di->setImage(ImageCache::getFromMemory(BinaryData::saw_png, BinaryData::saw_pngSize));
        return di;
    }
    
    int numItems() {
        return 9;
    }
    
};

#endif /* DefaultToolbarItemFactory_h */
