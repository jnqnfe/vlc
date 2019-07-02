/*****************************************************************************
 * prefs.m: MacOS X module for vlc
 *****************************************************************************
 * Copyright (C) 2002-2018 VLC authors and VideoLAN
 *
 * Authors: Jon Lech Johansen <jon-vl@nanocrew.net>
 *          Derk-Jan Hartman <hartman at videolan dot org>
 *          Felix Paul Kühne <fkuehne at videolan dot org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/* VLCPrefs manages the main preferences dialog
   the class is related to wxwindows intf, PrefsPanel */
/* VLCTreeItem should contain:
   - the children of the treeitem
   - the associated prefs widgets
   - the documentview with all the prefs widgets in it
   - a saveChanges action
   - a revertChanges action
   - a redraw view action
   - the children action should generate a list of the treeitems children (to be used by VLCPrefs datasource)

   The class is sort of a mix of wxwindows intfs, PrefsTreeCtrl and ConfigTreeData
*/
/* VLCConfigControl are subclassed NSView's containing and managing individual config items
   the classes are VERY closely related to wxwindows ConfigControls */

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# import "config.h"
#endif

#import <stdlib.h>                                      /* malloc(), free() */
#import <sys/param.h>                                    /* for MAXPATHLEN */
#import <string.h>

#import <vlc_common.h>
#import <vlc_actions.h>
#import <vlc_config_cat.h>
#import <vlc_modules.h>
#import <vlc_plugin.h>

#import "extensions/NSString+Helpers.h"
#import "main/VLCMain.h"
#import "preferences/prefs.h"
#import "preferences/VLCSimplePrefsController.h"
#import "preferences/prefs_widgets.h"

#define LEFTMARGIN  18
#define RIGHTMARGIN 18

/* /!\ Warning: Unreadable code :/ */

@interface VLCFlippedView : NSView
@end

@implementation VLCFlippedView

- (BOOL)isFlipped
{
    return YES;
}

@end

@interface VLCTreeItem : NSObject
{
    NSString *_name;
    NSMutableArray *_children;
    NSMutableArray *_options;
    NSMutableArray *_subviews;
}
@property (readwrite, weak) VLCPrefs *prefsViewController;

- (id)initWithName:(NSString*)name;

- (NSInteger)numberOfChildren;
- (VLCTreeItem *)childAtIndex:(NSInteger)i_index;

- (NSString *)name;
- (NSMutableArray *)children;
- (NSMutableArray *)options;
- (void)showView;
- (void)applyChanges;
- (void)resetView;

@end

enum VLCTreeBranchType {
    CategoryBranch = 0,
    SubcategoryBranch = 1,
    PluginBranch = 2,
};

@interface VLCTreeBranchItem : VLCTreeItem
{
    enum VLCTreeBranchType _branchType;
    enum vlc_config_cat _category;
    enum vlc_config_subcat _subcategory;
    module_t *_plugin;
}
+ (VLCTreeBranchItem *)newCategoryTreeBranch:(enum vlc_config_cat)category;
+ (VLCTreeBranchItem *)newSubcategoryTreeBranch:(enum vlc_config_subcat)subcategory;
+ (VLCTreeBranchItem *)newPluginTreeBranch:(module_t *)plugin;

- (id)initWithCategory:(enum vlc_config_cat)category;
- (id)initWithSubcategory:(enum vlc_config_subcat)subcategory;
- (id)initWithPlugin:(module_t *)plugin;

- (VLCTreeBranchItem *)childRepresentingSubcategory:(enum vlc_config_cat)category;
- (VLCTreeBranchItem *)childRepresentingPlugin:(module_t *)plugin;

- (enum VLCTreeBranchType)branchType;
- (enum vlc_config_cat)category;
- (enum vlc_config_subcat)subcategory;
- (module_t *)plugin;
@end

/* individual options. */
@interface VLCTreeLeafItem : VLCTreeItem
{
    module_config_item_t * _configItem;
}
+ (VLCTreeLeafItem *)newTreeLeaf:(module_config_item_t *)configItem;
- (id)initWithConfigItem:(module_config_item_t *)configItem;
- (module_config_item_t *)configItem;
@end

@interface VLCTreeMainItem : VLCTreeItem
{
    NSMutableArray *_configSetOwners;
}
- (VLCTreeBranchItem *)childRepresentingCategory:(enum vlc_config_cat)category;
- (NSMutableArray *)configSetOwners;
@end

@interface VLCTreeConfigSetOwner : NSObject
{
    module_config_item_t *_set;
}
+ (VLCTreeConfigSetOwner *)newConfigSetOwner:(module_config_item_t*)set;
- (id)initWithSet:(module_config_item_t*)set;
@end

#pragma mark -

/*****************************************************************************
 * VLCPrefs implementation
 *****************************************************************************/

@interface VLCPrefs()
{
    VLCTreeMainItem *_rootTreeItem;
    NSView *o_emptyView;
    NSMutableDictionary *o_save_prefs;
}
@end

@implementation VLCPrefs

- (id)init
{
    self = [super initWithWindowNibName:@"Preferences"];
    if (self) {
        o_emptyView = [[NSView alloc] init];
        _rootTreeItem = [[VLCTreeMainItem alloc] init];
    }

    return self;
}

- (void)windowDidLoad

{
    [self.window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenAuxiliary];
    [self.window setHidesOnDeactivate:YES];

    [self.window setTitle: _NS("Preferences")];
    [_saveButton setTitle: _NS("Save")];
    [_cancelButton setTitle: _NS("Cancel")];
    [_resetButton setTitle: _NS("Reset All")];
    [_showBasicButton setTitle: _NS("Show Basic")];

    [_prefsView setBorderType: NSGrooveBorder];
    [_prefsView setHasVerticalScroller: YES];
    [_prefsView setDrawsBackground: NO];
    [_prefsView setDocumentView: o_emptyView];
    [self.window layoutIfNeeded];
    [_tree selectRowIndexes: [NSIndexSet indexSetWithIndex: 0] byExtendingSelection: NO];
}

- (void)setTitle: (NSString *) o_title_name
{
    [self.titleLabel setStringValue: o_title_name];
}

- (void)showPrefsWithLevel:(NSInteger)iWindow_level
{
    [self.window setLevel: iWindow_level];
    [self.window center];
    [self.window makeKeyAndOrderFront:self];
    [_rootTreeItem resetView];
}

- (IBAction)savePrefs: (id)sender
{
    vlc_config_GetWriteLock();
    /* TODO: call savePrefs on Root item */
    [_rootTreeItem applyChanges];
    fixIntfSettings();
    vlc_config_ReleaseLock();
    config_SaveConfigFile(getIntf());
    [[NSNotificationCenter defaultCenter] postNotificationName:VLCConfigurationChangedNotification object:nil];
    [self.window orderOut:self];
}

- (IBAction)closePrefs: (id)sender
{
    [self.window orderOut:self];
}

- (IBAction)showSimplePrefs: (id)sender
{
    [self.window orderOut: self];
    [[[VLCMain sharedInstance] simplePreferences] showSimplePrefs];
}

- (IBAction)resetPrefs:(id)sender
{
    [[[VLCMain sharedInstance] simplePreferences] resetPreferences:sender];
}

- (void)loadConfigTree
{
}

- (void)outlineViewSelectionIsChanging:(NSNotification *)o_notification
{
}

/* update the document view to the view of the selected tree item */
- (void)outlineViewSelectionDidChange:(NSNotification *)o_notification
{
    VLCTreeItem *treeItem = [_tree itemAtRow:[_tree selectedRow]];
    treeItem.prefsViewController = self;
    [treeItem showView];
    [_tree expandItem:[_tree itemAtRow:[_tree selectedRow]]];
}

@end

@implementation VLCPrefs (NSTableDataSource)

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
    return (item == nil) ? [_rootTreeItem numberOfChildren] : [item numberOfChildren];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item
{
    return (item == nil) ? [_rootTreeItem numberOfChildren] : [item numberOfChildren];
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item
{
    return (item == nil) ? (id)[_rootTreeItem childAtIndex:index]: (id)[item childAtIndex:index];
}

- (id)outlineView:(NSOutlineView *)outlineView
    objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
    return (item == nil) ? @"" : [item name];
}

#pragma mark -
#pragma mark split view delegate
- (CGFloat)splitView:(NSSplitView *)splitView constrainMaxCoordinate:(CGFloat)proposedMax ofSubviewAt:(NSInteger)dividerIndex
{
    return 300.;
}

- (CGFloat)splitView:(NSSplitView *)splitView constrainMinCoordinate:(CGFloat)proposedMin ofSubviewAt:(NSInteger)dividerIndex
{
    return 100.;
}

- (BOOL)splitView:(NSSplitView *)splitView canCollapseSubview:(NSView *)subview
{
    return NO;
}

- (BOOL)splitView:(NSSplitView *)splitView shouldAdjustSizeOfSubview:(NSView *)subview
{
    return [splitView.subviews objectAtIndex:0] != subview;
}

@end

#pragma mark -
#pragma mark (Root class for all TreeItems)
@implementation VLCTreeItem

- (id)initWithName:(NSString*)name
{
    self = [super init];
    if (self != nil)
        _name = name;

    return self;
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"%@: name: %@, number of children %li", NSStringFromClass([self class]), [self name], [self numberOfChildren]];
}

- (VLCTreeItem *)childAtIndex:(NSInteger)i_index
{
    return [[self children] objectAtIndex:i_index];
}

- (NSInteger)numberOfChildren
{
    return [[self children] count];
}

- (NSString *)name
{
    return _name;
}

- (void)showView
{
    NSScrollView *prefsView = self.prefsViewController.prefsView;
    NSRect s_vrc;
    NSView *view;

    [self.prefsViewController setTitle: [self name]];
    s_vrc = [[prefsView contentView] bounds]; s_vrc.size.height -= 4;
    view = [[VLCFlippedView alloc] initWithFrame: s_vrc];
    [view setAutoresizingMask: NSViewWidthSizable | NSViewMinYMargin | NSViewMaxYMargin];

    if (!_subviews) {
        _subviews = [[NSMutableArray alloc] initWithCapacity:10];

        NSUInteger count = [[self options] count];
        for (NSUInteger i = 0; i < count; i++) {
            VLCTreeLeafItem * item = [[self options] objectAtIndex:i];

            VLCConfigControl *control;
            control = [VLCConfigControl newControl:[item configItem] withView:view];
            if (control) {
                [control setAutoresizingMask: NSViewMaxYMargin | NSViewWidthSizable];
                [_subviews addObject: control];
            }
        }
    }

    assert(view);

    int i_lastItem = 0;
    int i_yPos = -2;
    int i_max_label = 0;

    NSEnumerator *enumerator = [_subviews objectEnumerator];
    VLCConfigControl *widget;
    NSRect frame;

    while((widget = [enumerator nextObject])) {
        if (i_max_label < [widget labelSize])
            i_max_label = [widget labelSize];
    }

    enumerator = [_subviews objectEnumerator];
    while((widget = [enumerator nextObject])) {
        int i_widget;

        i_widget = [widget viewType];
        i_yPos += [VLCConfigControl calcVerticalMargin:i_widget lastItem:i_lastItem];
        [widget setYPos:i_yPos];
        frame = [widget frame];
        frame.size.width = [view frame].size.width - LEFTMARGIN - RIGHTMARGIN;
        [widget setFrame:frame];
        [widget alignWithXPosition: i_max_label];
        i_yPos += [widget frame].size.height;
        i_lastItem = i_widget;
        [view addSubview:widget];
    }

    frame = [view frame];
    frame.size.height = i_yPos;
    [view setFrame:frame];
    [prefsView setDocumentView:view];
}

- (void)applyChanges
{
    NSUInteger i;
    NSUInteger count = [_subviews count];
    for (i = 0 ; i < count ; i++)
        [[_subviews objectAtIndex:i] applyChanges];

    count = [_children count];
    for (i = 0 ; i < count ; i++)
        [[_children objectAtIndex:i] applyChanges];
}

- (void)resetView
{
    NSUInteger count = [_subviews count];
    for (NSUInteger i = 0 ; i < count ; i++)
        [[_subviews objectAtIndex:i] resetValues];

    count = [_options count];
    for (NSUInteger i = 0 ; i < count ; i++)
        [[_options objectAtIndex:i] resetView];

    count = [_children count];
    for (NSUInteger i = 0 ; i < count ; i++)
        [[_children objectAtIndex:i] resetView];

}

- (NSMutableArray *)children
{
    if (!_children)
        _children = [[NSMutableArray alloc] init];
    return _children;
}

- (NSMutableArray *)options
{
    if (!_options)
        _options = [[NSMutableArray alloc] init];
    return _options;
}
@end

#pragma mark -
@implementation VLCTreeMainItem

- (VLCTreeBranchItem *)childRepresentingCategory:(enum vlc_config_cat)category
{
    NSUInteger childrenCount = [[self children] count];
    for (int i = 0; i < childrenCount; i++) {
        VLCTreeBranchItem * item = [[self children] objectAtIndex:i];
        if ([item branchType] == VLCTreeBranchType::CategoryBranch && [item category] == category)
            return item;
    }
    return nil;
}

/* Creates and returns the array of children
 * Loads children incrementally */
- (NSMutableArray *)children
{
    if (_children) return _children;
    _children = [[NSMutableArray alloc] init];

    /* List the modules */
    size_t count, i;
    module_t ** modules = vlc_module_list_have_config(&count);
    if (!modules) return nil;

    /* Build a tree of the plugins */
    /* Add the capabilities */
    for (i = 0; i < count; i++) {
        VLCTreeBranchItem * categoryItem = nil;
        VLCTreeBranchItem * subcategoryItem = nil;
        VLCTreeBranchItem * pluginItem = nil;
        enum vlc_config_cat cat = CAT_INVALID;
        enum vlc_config_subcat subcat = SUBCAT_INVALID;
        bool create_subcat_node_pending = false;
        bool subcat_is_general = false;

        module_t * p_module = modules[i];
        bool mod_is_main = module_is_main(p_module);

        unsigned int confsize;
        module_config_item_t *p_configs = vlc_module_config_get(p_module, &confsize);
        VLCTreeConfigSetOwner* cfgSetOwner = [VLCTreeConfigSetOwner newConfigSetOwner:p_configs];
        if (!cfgSetOwner) {
            module_config_free(p_configs);
            continue;
        }
        [[self configSetOwners] addObject:cfgSetOwner];

        for (unsigned int j = 0; j < confsize; j++) {
            int configType = p_configs[j].i_type;

            if (configType == CONFIG_SUBCATEGORY) {
                enum vlc_config_subcat new_subcat = (enum vlc_config_subcat) p_configs[j].value.i;
                if (new_subcat != subcat) {
                    subcat = new_subcat;
                    create_subcat_node_pending = true;
                    subcat_is_general = vlc_config_SubcategoryIsGeneral(subcat);
                    pluginItem = nil;
                }
                continue;
            }

            if (vlc_config_SubcategoryIsGUIHidden(subcat) || subcat == SUBCAT_INVALID)
                continue;

            if (!CONFIG_ITEM(configType) && configType != CONFIG_SECTION)
                continue;

            if (create_subcat_node_pending) {
                cat = vlc_config_CategoryFromSubcategory(subcat);

                categoryItem = [self childRepresentingCategory:cat];
                if (!categoryItem) {
                    categoryItem = [VLCTreeBranchItem newCategoryTreeBranch:cat];
                    if (categoryItem) {
                        [[self children] addObject:categoryItem];
                    } else {
                        continue;
                    }
                }

                if (!subcat_is_general) {
                    subcategoryItem = [categoryItem childRepresentingSubcategory:subcat];
                    if (!subcategoryItem) {
                        subcategoryItem = [VLCTreeBranchItem newSubcategoryTreeBranch:subcat];
                        if (subcategoryItem) {
                            [[categoryItem children] addObject:subcategoryItem];
                        } else {
                            continue;
                        }
                    }
                }

                create_subcat_node_pending = false;
            }

            if (!mod_is_main && !pluginItem) {
                if (subcat_is_general) {
                    pluginItem = [categoryItem childRepresentingPlugin:p_module];
                } else {
                    pluginItem = [subcategoryItem childRepresentingPlugin:p_module];
                }

                if (!pluginItem) {
                    pluginItem = [VLCTreePluginItem newPluginTreeBranch:p_module];
                    if (!pluginItem) continue;

                    if (subcat_is_general) {
                        [[categoryItem children] addObject:pluginItem];
                    } else {
                        [[subcategoryItem children] addObject:pluginItem];
                    }
                }
            }

            VLCTreeLeafItem *new_leaf = [VLCTreeLeafItem newTreeLeaf:&p_configs[j]];
            if (!new_leaf) continue;

            if (mod_is_main && subcat_is_general) {
                [[categoryItem options] addObject:new_leaf];
            } else if (mod_is_main) {
                [[subcategoryItem options] addObject:new_leaf];
            } else {
                [[pluginItem options] addObject:new_leaf];
            }
        }
    }
    module_list_free(modules);
    return _children;
}

- (NSMutableArray *)configSetOwners
{
    if (!_configSetOwners)
        _configSetOwners = [[NSMutableArray alloc] init];
    return _configSetOwners;
}
@end

#pragma mark -
@implementation VLCTreeBranchItem
+ (VLCTreeBranchItem *)newCategoryTreeBranch:(enum vlc_config_cat)category
{
    return [[[self class] alloc] initWithCategory:category];
}

+ (VLCTreeBranchItem *)newSubcategoryTreeBranch:(enum vlc_config_subcat)subcategory
{
    return [[[self class] alloc] initWithSubcategory:subcategory];
}

+ (VLCTreeBranchItem *)newPluginTreeBranch:(module_t *)plugin
{
    return [[[self class] alloc] initWithPlugin:plugin];
}

- (id)initWithCategory:(enum vlc_config_cat)category
{
    NSString * name = _NS(vlc_config_CategoryNameGet(category));
    if (self = [super initWithName:name]) {
        _branchType = VLCTreeBranchType::CategoryBranch;
        _category = category;
        _subcategory = SUBCAT_INVALID;
        _plugin = nil;
        //_help = [_NS(vlc_config_CategoryHelpGet(category)) retain];
    }
    return self;
}

- (id)initWithSubcategory:(enum vlc_config_subcat)subcategory
{
    NSString * name = _NS(config_SubcategoryNameGet(subcategory));
    if (self = [super initWithName:name]) {
        _branchType = VLCTreeBranchType::SubcategoryBranch;
        _category = CAT_INVALID;
        _subcategory = subcategory;
        _plugin = nil;
        //_help = [_NS(vlc_config_SubcategoryHelpGet(subcategory)) retain];
    }
    return self;
}

- (id)initWithPlugin:(module_t *)plugin
{
    NSString * name = _NS(vlc_module_GetShortName(plugin));
    if (self = [super initWithName:name]) {
        _branchType = VLCTreeBranchType::PluginBranch;
        _category = CAT_INVALID;
        _subcategory = SUBCAT_INVALID;
        _plugin = plugin;
        //_help = [_NS(vlc_config_SubcategoryHelpGet(subcategory)) retain];
    }
    return self;
}

- (VLCTreeBranchItem *)childRepresentingSubcategory:(enum vlc_config_subcat)subcategory
{
    assert([self isKindOfClass:[VLCTreeBranchItem class]]);
    NSUInteger childrenCount = [[self children] count];
    for (NSUInteger i = 0; i < childrenCount; i++) {
        VLCTreeBranchItem * item = [[self children] objectAtIndex:i];
        if ([item branchType] == VLCTreeBranchType::SubcategoryBranch && [item subcategory] == subcategory)
            return item;
    }
    return nil;
}

- (VLCTreeBranchItem *)childRepresentingPlugin:(module_t *)plugin
{
    assert([self isKindOfClass:[VLCTreeBranchItem class]]);
    NSUInteger childrenCount = [[self children] count];
    for (NSUInteger i = 0; i < childrenCount; i++) {
        VLCTreeBranchItem * item = [[self children] objectAtIndex:i];
        if ([item branchType] == VLCTreeBranchType::PluginBranch && [item plugin] == plugin)
            return item;
    }
    return nil;
}

- (enum VLCTreeBranchType)branchType
{
    return _branchType;
}

- (enum vlc_config_cat)category
{
    return _category;
}

- (enum vlc_config_subcat)subcategory
{
    return _subcategory;
}

- (module_t *)plugin
{
    return _plugin;
}

@end

#pragma mark -
@implementation VLCTreeLeafItem
+ (VLCTreeLeafItem *)newTreeLeaf:(module_config_item_t *)configItem
{
    return [[[self class] alloc] initWithConfigItem:configItem];
}

- (id)initWithConfigItem: (module_config_item_t *) configItem
{
    NSString *name = toNSStr(configItem->psz_name);
    self = [super initWithName:name];
    if (self != nil)
        _configItem = configItem;

    return self;
}

- (module_config_item_t *)configItem
{
    return _configItem;
}
@end

#pragma mark -
@implementation VLCTreeConfigSetOwner
+ (VLCTreeConfigSetOwner *)newConfigSetOwner:(module_config_item_t*)set
{
    return [[[self class] alloc] initWithSet:set];
}

- (id)initWithSet:(module_config_item_t*)set
{
    self = [super init];
    if (self != nil)
        _set = set;
    return self;
}

- (void)dealloc
{
    module_config_free(_set);
}

@end
