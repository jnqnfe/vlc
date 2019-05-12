/*****************************************************************************
 * expert_preferences.cpp : Detailed preferences overview
 *****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * Authors: Lyndon Brown <jnqnfe@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <QLabel>
#include <QTreeWidget>
#include <QVariant>
#include <QString>
#include <QFont>
#include <QGridLayout>
#include <QGuiApplication>
#include <QClipboard>
#include <QDialogButtonBox>

#include "components/expert_preferences.hpp"
#include "components/preferences_widgets.hpp"

#include <vlc_config_cat.h>
#include <vlc_intf_strings.h>
#include <vlc_modules.h>
#include <vlc_plugin.h>
#include <assert.h>

#define ITEM_HEIGHT 30

#define COL_NAME  0
#define COL_STATE 1
#define COL_TYPE  2
#define COL_VALUE 3

/*********************************************************************
 * The Table
 *********************************************************************/
PrefsTreeExpert::PrefsTreeExpert( intf_thread_t *_p_intf, QWidget *_parent,
                                  module_t **p_list, size_t count ) :
    QTreeWidget( _parent ), p_intf( _p_intf )
{
    setColumnCount( 4 );
    headerItem()->setText( COL_NAME, qtr( "Option" ) );
    headerItem()->setText( COL_STATE, qtr( "Status" ) );
    headerItem()->setText( COL_TYPE, qtr( "Type" ) );
    headerItem()->setText( COL_VALUE, qtr( "Value" ) );
    setAlternatingRowColors( true );

    state_modified_text = qtr( "modified" );
    state_default_text = qtr( "default" );

    for( size_t i = 0; i < count; i++ )
    {
        module_t *p_module = p_list[i];
        bool is_core = module_is_main( p_module);

        unsigned confsize;
        module_config_item_t *const p_config = vlc_module_config_get (p_module, &confsize);

        config_sets.append( p_config );

        enum vlc_config_subcat subcat = SUBCAT_INVALID;

        const char *mod_name_pretty = vlc_module_GetShortName(p_module);
        const char *mod_name = module_get_object(p_module);
        size_t mod_name_len = strlen(mod_name);

        for (size_t j = 0; j < confsize; j++)
        {
            module_config_item_t *p_item = p_config + j;

            if( p_item->i_type == CONFIG_SUBCATEGORY )
            {
                subcat = (enum vlc_config_subcat) p_item->value.i;
                continue;
            }
            if (subcat == SUBCAT_INVALID || subcat == SUBCAT_HIDDEN)
                continue;

            if( CONFIG_ITEM(p_item->i_type) )
                this->createItemNode( p_item, mod_name_pretty, mod_name, mod_name_len, is_core );
        }
    }

    // We got everything, just sort a bit
    sortItems( COL_NAME, Qt::AscendingOrder );

    resizeColumnToContents( COL_NAME );

    for( int i = 0 ; i < topLevelItemCount(); i++ )
    {
        QTreeWidgetItem *item = topLevelItem( i );
        item->setSizeHint( 0, QSize( -1, ITEM_HEIGHT ) );
    }

    /* context menu actions */
    resetAct = new QAction( tr( "&Reset" ), this );
    resetAct->setStatusTip( tr( "Reset option state and value to default" ) );
    connect( resetAct, &QAction::triggered, this, &PrefsTreeExpert::reset );

    toggleAct = new QAction( tr( "&Toggle" ), this );
    toggleAct->setStatusTip( tr( "Toggle boolean state" ) );
    CONNECT( toggleAct, triggered(bool), this, toggle() );

    modifyAct = new QAction( tr( "&Modify" ), this );
    CONNECT( modifyAct, triggered(bool), this, modify() );

    copyNameAct = new QAction( tr( "Copy &name" ), this );
    connect( copyNameAct, &QAction::triggered, this, &PrefsTreeExpert::copyName );

    copyValueAct = new QAction( tr( "Copy &value" ), this );
    connect( copyValueAct, &QAction::triggered, this, &PrefsTreeExpert::copyValue );

    /* edit sub-dialog */
    expert_edit = new ExpertPrefsEditDialog( p_intf, this );

    CONNECT( this, itemDoubleClicked( QTreeWidgetItem *, int ),
             this, doubleClickedItem( QTreeWidgetItem * ) );
}

void PrefsTreeExpert::createItemNode( module_config_item_t* config,
                                      const char *mod_name_pretty,
                                      const char *mod_name,
                                      size_t mod_name_len,
                                      bool is_core )
{
    char *psz_tmp;

    QTreeWidgetItem *item = new QTreeWidgetItem();

    ExpertPrefsItemData *data = new ExpertPrefsItemData( this );
    data->item = config;

    /* form "title" text label (<mod-name-pretty>: item-text) */
    psz_tmp = NULL;
    if (is_core) mod_name_pretty = "Core";
    if (asprintf(&psz_tmp, "%s: %s", mod_name_pretty, vlc_gettext(config->psz_text)) != -1) {
        data->title = qfu( psz_tmp );
    }
    free( psz_tmp );

    /* form dot-name from module name and option name
       many plugins prefix the plugin name to their option names, which we remove here */
    psz_tmp = NULL;
    const char *opt_name = config->psz_name;
    if (!is_core && opt_name[mod_name_len] == '-' &&
        strncmp(mod_name, opt_name, mod_name_len) == 0)
    {
        opt_name += mod_name_len + 1;
    }
    if (asprintf(&psz_tmp, "%s.%s", mod_name, opt_name) != -1) {
        data->name = qfu( psz_tmp );
    }
    free( psz_tmp );

    const char *type_name;
    switch (config->i_type) {
        case CONFIG_ITEM_BOOL:             type_name = "boolean";     break;
        case CONFIG_ITEM_FLOAT:            type_name = "float";       break;
        case CONFIG_ITEM_INTEGER:          type_name = "integer";     break;
        case CONFIG_ITEM_RGB:
        case CONFIG_ITEM_RGBA:             type_name = "color";       break;
        case CONFIG_ITEM_STRING:           type_name = "string";      break;
        case CONFIG_ITEM_PASSWORD:         type_name = "password";    break;
        case CONFIG_ITEM_KEY:              type_name = "hotkey";      break;
        case CONFIG_ITEM_MODULE_CAT:
        case CONFIG_ITEM_MODULE:           type_name = "module";      break;
        case CONFIG_ITEM_MODULE_LIST_CAT:
        case CONFIG_ITEM_MODULE_LIST:      type_name = "module-list"; break;
        case CONFIG_ITEM_LOADFILE:
        case CONFIG_ITEM_SAVEFILE:         type_name = "file";        break;
        case CONFIG_ITEM_DIRECTORY:        type_name = "directory";   break;
        case CONFIG_ITEM_FONT:             type_name = "font";        break;
        case CONFIG_ITEM_FOURCC:           type_name = "fourcc";      break;
        case CONFIG_ITEM_INFO:
        default:                           type_name = "unknown";     break;
    }

    item->setText( COL_NAME, data->name );
    item->setText( COL_TYPE, qtr( type_name ) );

    updateDisplayedValue( item, data );

    data->is_modified = false;
    bool is_modified_actual = vlc_config_ItemIsModified(config);
    if (is_modified_actual) {
        setItemModifiedState( item, is_modified_actual, data );
    } else {
        item->setText( COL_STATE, state_default_text );
    }

    item->setData( 0, Qt::UserRole, QVariant::fromValue( data ) );

    addTopLevelItem( item );
}

PrefsTreeExpert::~PrefsTreeExpert()
{
    foreach ( module_config_item_t *config_set, config_sets )
        module_config_free( config_set );
}

void PrefsTreeExpert::updateDisplayedValue( QTreeWidgetItem *item,
                                            ExpertPrefsItemData *data )
{
    char *psz_tmp = NULL;

    switch (CONFIG_CLASS(data->item->i_type)) {
        case CONFIG_ITEM_CLASS_BOOL:
            data->value = qtr( (data->item->value.b) ? "true" : "false" );
            break;
        case CONFIG_ITEM_CLASS_FLOAT:
            if (asprintf(&psz_tmp, "%g", data->item->value.f) != -1) {
                data->value = qfu( psz_tmp );
            }
            break;
        case CONFIG_ITEM_CLASS_INTEGER:
            if (data->item->i_type == CONFIG_ITEM_RGB || data->item->i_type == CONFIG_ITEM_RGBA) {
                if (asprintf(&psz_tmp, "%#x", (unsigned)data->item->value.i) != -1) {
                    data->value = qfu( psz_tmp );
                }
            } else {
                if (asprintf(&psz_tmp, "%" PRIi64, data->item->value.i) != -1) {
                    data->value = qfu( psz_tmp );
                }
            }
            break;
        case CONFIG_ITEM_CLASS_STRING:
            data->value = qfu((data->item->i_type == CONFIG_ITEM_PASSWORD) ?
                "•••••" : data->item->value.psz);
            break;
        case CONFIG_ITEM_CLASS_INFO:
        case CONFIG_ITEM_CLASS_SPECIAL:
        default:
            break;
    }
    free( psz_tmp );

    item->setText( COL_VALUE, data->value );
}

void PrefsTreeExpert::setItemModifiedState( QTreeWidgetItem *item,
                                            bool is_modified,
                                            ExpertPrefsItemData *data )
{
    if (data->is_modified == is_modified)
        return;

    data->is_modified = is_modified;

    item->setText( COL_STATE, (is_modified) ? state_modified_text : state_default_text );

    QFont font = item->font( 0 );

    if (is_modified) {
        font.setWeight( QFont::Weight::Bold );
    } else {
        font.setWeight( QFont::Weight::Normal );
    }

    item->setFont( COL_NAME, font );
    item->setFont( COL_STATE, font );
    item->setFont( COL_TYPE, font );
    item->setFont( COL_VALUE, font );
}

void PrefsTreeExpert::applyAll()
{
    for( int i_index = 0 ; i_index < topLevelItemCount();
             i_index++ )
    {
        QTreeWidgetItem *item = topLevelItem( i_index );
        ExpertPrefsItemData *data = item->data( 0, Qt::UserRole).
                                                value<ExpertPrefsItemData *>();

        /* save from copy to actual */
        switch (CONFIG_CLASS(data->item->i_type))
        {
            case CONFIG_ITEM_CLASS_BOOL:
            case CONFIG_ITEM_CLASS_INTEGER:
                config_PutInt( data->item->psz_name, data->item->value.i );
                break;
            case CONFIG_ITEM_CLASS_FLOAT:
                config_PutFloat( data->item->psz_name, data->item->value.f );
                break;
            case CONFIG_ITEM_CLASS_STRING:
                config_PutPsz( data->item->psz_name, data->item->value.psz );
                break;
        }
    }
}

void PrefsTreeExpert::cleanAll()
{}

bool PrefsTreeExpert::filterItems( QTreeWidgetItem *item, const QString &text,
                                   Qt::CaseSensitivity cs )
{
    ExpertPrefsItemData *data = item->data( 0, Qt::UserRole ).value<ExpertPrefsItemData *>();

    bool filtered = !data->contains( text, cs );
    item->setHidden( filtered );

    return filtered;
}

/* apply filter on tree */
void PrefsTreeExpert::filter( const QString &text )
{
    bool clear_filter = text.isEmpty();

    for( int i = 0 ; i < topLevelItemCount(); i++ )
    {
        QTreeWidgetItem *item = topLevelItem( i );
        if ( !clear_filter )
            filterItems( item, text, Qt::CaseInsensitive );
        else
            item->setHidden( false );
    }
}

#ifndef QT_NO_CONTEXTMENU
void PrefsTreeExpert::contextMenuEvent(QContextMenuEvent *event)
{
    QTreeWidgetItem *item = this->currentItem();
    if (!item) return;
    ExpertPrefsItemData *data = item->data( 0, Qt::UserRole ).value<ExpertPrefsItemData *>();

    QMenu menu(this);
    if (CONFIG_CLASS(data->item->i_type) == CONFIG_ITEM_CLASS_BOOL)
        menu.addAction(toggleAct);
    else
        menu.addAction(modifyAct);
    //FIXME: remove this next line, sort out how to deal with individual hotkey item editing
    if (data->item->i_type == CONFIG_ITEM_KEY) { modifyAct->setEnabled(false); } else { modifyAct->setEnabled(true); }
    menu.addSeparator();
    menu.addAction(copyNameAct);
    menu.addAction(copyValueAct);
    /* if this is removed/disabled, don't forget to uncomment the special
       handlying in copyValue() to avoid just copying asterisks! */
    copyValueAct->setEnabled(data->item->i_type != CONFIG_ITEM_PASSWORD);
    menu.addSeparator();
    menu.addAction(resetAct);
    resetAct->setEnabled(data->is_modified);
    menu.exec(event->globalPos());
}
#endif // QT_NO_CONTEXTMENU

void PrefsTreeExpert::reset()
{
    QTreeWidgetItem *item = this->currentItem();
    if (!item) return;
    ExpertPrefsItemData *data = item->data( 0, Qt::UserRole ).value<ExpertPrefsItemData *>();

    data->item->value = data->item->orig;
    updateDisplayedValue( item, data );
    setItemModifiedState( item, false, data );
}

void PrefsTreeExpert::toggle()
{
    QTreeWidgetItem *item = this->currentItem();
    if (!item) return;
    toggle( item );
}

void PrefsTreeExpert::toggle( QTreeWidgetItem *item )
{
    ExpertPrefsItemData *data = item->data( 0, Qt::UserRole ).value<ExpertPrefsItemData *>();

    /* this 'toggle' action only applies to boolean options! */
    data->item->value.b = !data->item->value.b;
    updateDisplayedValue( item, data );
    setItemModifiedState( item, !data->is_modified, data );
}

void PrefsTreeExpert::modify()
{
    QTreeWidgetItem *item = this->currentItem();
    if (!item) return;
    modify( item );
}

void PrefsTreeExpert::modify( QTreeWidgetItem *item )
{
    ExpertPrefsItemData *data = item->data( 0, Qt::UserRole ).value<ExpertPrefsItemData *>();

    expert_edit->addControl( item, data );
    expert_edit->exec();
}

void PrefsTreeExpert::copyName()
{
    QTreeWidgetItem *item = this->currentItem();
    if (!item) return;
    ExpertPrefsItemData *data = item->data( 0, Qt::UserRole ).value<ExpertPrefsItemData *>();

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(data->name);
}

void PrefsTreeExpert::copyValue()
{
    QTreeWidgetItem *item = this->currentItem();
    if (!item) return;
    ExpertPrefsItemData *data = item->data( 0, Qt::UserRole ).value<ExpertPrefsItemData *>();

    QClipboard *clipboard = QGuiApplication::clipboard();
//    if (data->item->i_type == CONFIG_ITEM_PASSWORD)
//        clipboard->setText( qfu( data->item->value.psz ) ); // get the value, not the displayed asterisks
//    else
        clipboard->setText(data->value); // just use the displayed string
}

void PrefsTreeExpert::doubleClickedItem( QTreeWidgetItem *item )
{
    ExpertPrefsItemData *data = item->data( 0, Qt::UserRole ).value<ExpertPrefsItemData *>();
    if (CONFIG_CLASS(data->item->i_type) == CONFIG_ITEM_CLASS_BOOL)
        toggle( item );
    //FIXME: remove this condition, sort out how to deal with individual hotkey item editing
    else if (data->item->i_type == CONFIG_ITEM_KEY)
    {/* do nothing */}
    else
        modify( item );
}

ExpertPrefsItemData::ExpertPrefsItemData( QObject *_parent ) : QObject( _parent )
{
    name = "";
    value = "";
    item = NULL;
    is_modified = false;
    owned_string = false;
}

/* search name and value columns */
bool ExpertPrefsItemData::contains( const QString &text, Qt::CaseSensitivity cs )
{
    return (name.contains( text, cs ) || value.contains( text, cs ));
}

/*********************************************************************
 * The Edit Dialog
 *********************************************************************/
ExpertPrefsEditDialog::ExpertPrefsEditDialog( intf_thread_t *_p_intf,
                                              PrefsTreeExpert *_tree ) :
    QDialog( _tree ), p_intf( _p_intf ), tree( _tree )
{
    tree_item = NULL;
    data = NULL;
    control = NULL;
    control_widget = NULL;

    setWindowTitle( qtr( "Set option value" ) );
    setWindowRole( "vlc-preferences" );
    setWindowModality( Qt::WindowModal );

    setMinimumSize( 400, 120 );

    layout = new QVBoxLayout( this );
    layout->setMargin( 9 );

    QDialogButtonBox *buttonBox = new QDialogButtonBox();
    QPushButton *ok = new QPushButton( qtr( "&Ok" ) );
    QPushButton *cancel = new QPushButton( qtr( "&Cancel" ) );
    buttonBox->addButton( ok, QDialogButtonBox::AcceptRole );
    buttonBox->addButton( cancel, QDialogButtonBox::RejectRole );

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ExpertPrefsEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ExpertPrefsEditDialog::reject);

    layout->addWidget( buttonBox );

    setLayout( layout );
}

void ExpertPrefsEditDialog::addControl( QTreeWidgetItem *_tree_item,
                                        ExpertPrefsItemData *_data )
{
    tree_item = _tree_item;
    data = _data;
    control_widget = new QWidget( this );
    QVBoxLayout *control_layout = new QVBoxLayout();
    control = ConfigControl::createControl( p_intf, data->item, this, control_layout, 0 );
    control_widget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    control_widget->setLayout( control_layout );
    layout->insertWidget( 0, control_widget );
}

void ExpertPrefsEditDialog::clearControl()
{
    if (control) {
        delete control;
        control = NULL;
    }
    if (control_widget) {
        layout->removeWidget( control_widget );
        delete control_widget;
        control_widget = NULL;
    }
    tree_item = NULL;
    data = NULL;
}

void ExpertPrefsEditDialog::accept()
{
    data->clearOwnedStringVal();
    control->storeValue( false ); //store and assert ownership (applies to string values)
    data->owned_string = true;
    tree->updateDisplayedValue( tree_item, data );
    tree->setItemModifiedState( tree_item, vlc_config_ItemIsModified(data->item), data );
    clearControl();
    QDialog::accept();
}

void ExpertPrefsEditDialog::reject()
{
    clearControl();
    QDialog::reject();
}
