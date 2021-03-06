/*****************************************************************************
 * preferences.cpp : Preferences
 *****************************************************************************
 * Copyright (C) 2006-2007 the VideoLAN team
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "dialogs/preferences.hpp"
#include "util/qvlcframe.hpp"
#include "dialogs/errors.hpp"

#include "components/complete_preferences.hpp"
#include "components/expert_preferences.hpp"
#include "components/simple_preferences.hpp"
#include "util/searchlineedit.hpp"
#include "util/qvlcframe.hpp"
#include "main_interface.hpp"

#include <QHBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QPushButton>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QStackedWidget>
#include <QSplitter>
#include <QShortcut>
#include <QScrollArea>

#include <vlc_modules.h>

PrefsDialog::PrefsDialog( QWidget *parent, intf_thread_t *_p_intf )
            : QVLCDialog( parent, _p_intf ), p_intf( _p_intf )
{
    setWindowTitle( qtr( "Preferences" ) );
    setWindowRole( "vlc-preferences" );
    setWindowModality( Qt::WindowModal );

    /* Whether we want it or not, we need to destroy on close to get
       consistency when reset */
    setAttribute( Qt::WA_DeleteOnClose );

    p_list = vlc_module_list_have_config( &count );

    QGridLayout *main_layout = new QGridLayout( this );
    setLayout( main_layout );

    /* View (panel) selection */
    types = new QGroupBox( qtr("Show settings") );
    types->setAlignment( Qt::AlignHCenter );
    QHBoxLayout *types_l = new QHBoxLayout;
    types_l->setSpacing( 3 );
    types_l->setMargin( 3 );
    simple = new QRadioButton( qtr( "Simple" ), types );
    all = new QRadioButton( qtr("All"), types );
    expert = new QRadioButton( qtr("Expert"), types );
    simple->setToolTip( qtr( "Switch to simple preferences view" ) );
    all->setToolTip( qtr( "Switch to full preferences view" ) );
    expert->setToolTip( qtr( "Switch to expert preferences view" ) );
    types_l->addWidget( simple );
    types_l->addWidget( all );
    types_l->addWidget( expert );
    types->setLayout( types_l );
    simple->setChecked( true );

    /* Buttons */
    QDialogButtonBox *buttonsBox = new QDialogButtonBox();
    QPushButton *save = new QPushButton( qtr( "&Save" ) );
    QPushButton *close = new QPushButton( qtr( "&Close" ) );
    QPushButton *reset = new QPushButton( qtr( "&Reset Preferences" ) );

    buttonsBox->addButton( close, QDialogButtonBox::DestructiveRole );
    buttonsBox->addButton( save, QDialogButtonBox::ApplyRole );
    buttonsBox->addButton( reset, QDialogButtonBox::ResetRole );

    /* View (panel) stack */
    stack = new QStackedWidget();

    /* Simple view (panel) */
    simple_split_widget = new QWidget();
    simple_split_widget->setLayout( new QVBoxLayout );

    simple_tree_panel = new QWidget;
    simple_tree_panel->setLayout( new QVBoxLayout );
    simple_tree = NULL;
    simple_panels_stack = new QStackedWidget;
    for( int i = 0; i < SPrefsMax ; i++ )
        simple_panels[i] = NULL;

    simple_split_widget->layout()->addWidget( simple_tree_panel );
    simple_split_widget->layout()->addWidget( simple_panels_stack );

    simple_tree_panel->layout()->setMargin( 1 );
    simple_panels_stack->layout()->setContentsMargins( 6, 0, 0, 3 );
    simple_split_widget->layout()->setMargin( 0 );

    stack->insertWidget( SIMPLE, simple_split_widget );

    /* Advanced view (panel) */
    advanced_split_widget = new QSplitter();

    advanced_tree_panel = new QWidget;
    advanced_tree_panel->setLayout( new QVBoxLayout );
    tree_filter = NULL;
    current_filter = NULL;
    advanced_tree = NULL;
    advanced_panels_stack = new QStackedWidget;

    advanced_split_widget->addWidget( advanced_tree_panel );
    advanced_split_widget->addWidget( advanced_panels_stack );

    advanced_split_widget->setSizes(QList<int>() << 320 << 580);
    advanced_tree_panel->sizePolicy().setHorizontalStretch(1);
    advanced_panels_stack->sizePolicy().setHorizontalStretch(2);

    stack->insertWidget( ADVANCED, advanced_split_widget );

    /* Expert view (panel) */
    expert_widget = new QWidget;
    expert_widget_layout = new QVBoxLayout;
    expert_widget->setLayout( expert_widget_layout );

    expert_tree_filter = NULL;
    expert_tree = NULL;

    expert_text = new QLabel;
    expert_longtext = new QLabel;

    expert_text->setWordWrap(true);
    expert_longtext->setWordWrap(true);

    QFont textFont = QApplication::font();
    textFont.setPointSize( textFont.pointSize() + 2 );
    textFont.setUnderline( true );
    expert_text->setFont( textFont );

    expert_widget_layout->addWidget( expert_text );
    expert_widget_layout->addWidget( expert_longtext );

    stack->insertWidget( EXPERT, expert_widget );

    /* Layout  */
    main_layout->addWidget( stack, 0, 0, 3, 3 );
    main_layout->addWidget( types, 3, 0, 2, 1 );
    main_layout->addWidget( buttonsBox, 4, 2, 1 ,1 );
    main_layout->setRowStretch( 2, 4 );
    main_layout->setMargin( 9 );

    if( var_InheritBool( p_intf, "qt-advanced-pref" ) )
        setAdvanced();
    else
        setSimple();

    BUTTONACT( save, save() );
    BUTTONACT( close, close() );
    BUTTONACT( reset, reset() );

    BUTTONACT( simple, setSimple() );
    BUTTONACT( all, setAdvanced() );
    BUTTONACT( expert, setExpert() );

    QVLCTools::restoreWidgetPosition( p_intf, "Preferences", this, QSize( 900, 700 ) );
}

PrefsDialog::~PrefsDialog()
{
    module_list_free( p_list );
}

void PrefsDialog::setExpert()
{
    /* Lazy creation */
    if( !expert_tree )
    {
        expert_tree_filter = new SearchLineEdit( expert_widget );
        expert_tree_filter->setMinimumHeight( 26 );

        QShortcut *search = new QShortcut( QKeySequence( QKeySequence::Find ), expert_tree_filter );

        expert_tree = new PrefsTreeExpert( p_intf, expert_widget, p_list, count );

        expert_widget_layout->insertWidget( 0, expert_tree_filter );
        expert_widget_layout->insertWidget( 1, expert_tree );
//        expert_widget->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );

        CONNECT( expert_tree,
                 currentItemChanged( QTreeWidgetItem *, QTreeWidgetItem * ),
                 this, changeExpertDesc( QTreeWidgetItem * ) );
        CONNECT( expert_tree_filter, textChanged( const QString &  ),
                 this, expertTreeFilterChanged( const QString & ) );
        CONNECT( search, activated(), expert_tree_filter, setFocus() );

        /* Set initial selection */
        expert_tree->setCurrentIndex(
                expert_tree->model()->index( 0, 0, QModelIndex() ) );
    }

    expert->setChecked( true );
    stack->setCurrentIndex( EXPERT );
    setWindowTitle( qtr( "Expert Preferences" ) );
}

void PrefsDialog::setAdvanced()
{
    /* Lazy creation */
    if( !advanced_tree )
    {
        tree_filter = new SearchLineEdit( advanced_tree_panel );
        tree_filter->setMinimumHeight( 26 );

        current_filter = new QCheckBox( qtr("Only show current") );
        current_filter->setToolTip(
                    qtr("Only show modules related to current playback") );

        QShortcut *search = new QShortcut( QKeySequence( QKeySequence::Find ), tree_filter );

        advanced_tree = new PrefsTree( p_intf, advanced_tree_panel, p_list, count );

        advanced_tree_panel->layout()->addWidget( tree_filter );
        advanced_tree_panel->layout()->addWidget( current_filter );
        advanced_tree_panel->layout()->addWidget( advanced_tree );
        advanced_tree_panel->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

        CONNECT( advanced_tree,
                 currentItemChanged( QTreeWidgetItem *, QTreeWidgetItem * ),
                 this, changeAdvPanel( QTreeWidgetItem * ) );
        CONNECT( tree_filter, textChanged( const QString &  ),
                 this, advancedTreeFilterChanged( const QString & ) );
        CONNECT( current_filter, stateChanged(int),
                 this, onlyLoadedToggled() );
        CONNECT( search, activated(), tree_filter, setFocus() );

        /* Set initial selection */
        advanced_tree->setCurrentIndex(
                advanced_tree->model()->index( 0, 0, QModelIndex() ) );
    }

    all->setChecked( true );
    stack->setCurrentIndex( ADVANCED );
    setWindowTitle( qtr( "Advanced Preferences" ) );
}

void PrefsDialog::setSimple()
{
    /* If no simple_tree, create one, connect it */
    if( !simple_tree )
    {
         simple_tree = new SPrefsCatList( p_intf, simple_tree_panel );
         CONNECT( simple_tree,
                  currentItemChanged( int ),
                  this, changeSimplePanel( int ) );
        simple_tree_panel->layout()->addWidget( simple_tree );
        simple_tree_panel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
    }

    if( ! simple_panels[SPrefsDefaultCat] )
        changeSimplePanel( SPrefsDefaultCat );

    simple->setChecked( true );
    stack->setCurrentIndex( SIMPLE );
    setWindowTitle( qtr( "Simple Preferences" ) );
}

/* Switching from on simple panel to another */
void PrefsDialog::changeSimplePanel( int number )
{
    if( ! simple_panels[number] )
    {
        SPrefsPanel *insert = new SPrefsPanel( p_intf, simple_panels_stack, number ) ;
        simple_panels_stack->insertWidget( number, insert );
        simple_panels[number] = insert;
    }
    simple_panels_stack->setCurrentWidget( simple_panels[number] );
}

/* Changing from one Advanced Panel to another */
void PrefsDialog::changeAdvPanel( QTreeWidgetItem *item )
{
    if( item == NULL ) return;
    PrefsItemData *data = item->data( 0, Qt::UserRole ).value<PrefsItemData*>();

    if( !data->panel )
    {
        data->panel = new AdvPrefsPanel( p_intf, advanced_panels_stack, data );
        advanced_panels_stack->addWidget( data->panel );
    }
    advanced_panels_stack->setCurrentWidget( data->panel );
}

/* Changing from one Expert item description to another */
void PrefsDialog::changeExpertDesc( QTreeWidgetItem *item )
{
    if( item == NULL ) return;
    ExpertPrefsItemData *data = item->data( 0, Qt::UserRole ).value<ExpertPrefsItemData*>();

    expert_text->setText( data->title );
    if (data->item->psz_longtext)
        expert_longtext->setText( qtr( data->item->psz_longtext ) );
    else
        expert_longtext->setText( qtr( data->item->psz_text ) );
}

/* Actual apply and save for the preferences */
void PrefsDialog::save()
{
    vlc_config_GetWriteLock();

    if( simple->isChecked() && simple_tree->isVisible() )
    {
        msg_Dbg( p_intf, "Saving the simple preferences" );
        for( int i = 0 ; i< SPrefsMax; i++ ){
            if( simple_panels_stack->widget(i) )
                qobject_cast<SPrefsPanel *>(simple_panels_stack->widget(i))->apply();
        }
    }
    else if( all->isChecked() && advanced_tree->isVisible() )
    {
        msg_Dbg( p_intf, "Saving the advanced preferences" );
        advanced_tree->applyAll();
    }
    else if( expert->isChecked() && expert_tree->isVisible() )
    {
        msg_Dbg( p_intf, "Saving the expert preferences" );
        expert_tree->applyAll();
    }

    vlc_config_ReleaseLock();

    /* Save to file */
    if( config_SaveConfigFile( p_intf ) != 0 )
    {
        ErrorsDialog::getInstance (p_intf)->addError( qtr( "Cannot save Configuration" ),
            qtr("Preferences file could not be saved") );
    }

    if( p_intf->p_sys->p_mi )
        p_intf->p_sys->p_mi->reloadPrefs();
}

/* Clean the preferences, dunno if it does something really */
void PrefsDialog::close()
{
    QVLCTools::saveWidgetPosition( p_intf, "Preferences", this );

    reject();
}

/* Reset all the preferences, when you click the button */
void PrefsDialog::reset()
{
    int ret = QMessageBox::question(
                 this,
                 qtr( "Reset Preferences" ),
                 qtr( "Are you sure you want to reset your VLC media player preferences?" ),
                 QMessageBox::Ok | QMessageBox::Cancel,
                 QMessageBox::Ok);

    if( ret == QMessageBox::Ok )
    {
        config_ResetAll();
        config_SaveConfigFile( p_intf );
        getSettings()->clear();

#ifdef _WIN32
        simple_panels[0]->cleanLang();
#endif

        accept();
    }
}

void PrefsDialog::expertTreeFilterChanged( const QString & text )
{
    expert_tree->filter( text );
}

void PrefsDialog::advancedTreeFilterChanged( const QString & text )
{
    advanced_tree->filter( text );
}

void PrefsDialog::onlyLoadedToggled()
{
    advanced_tree->setLoadedOnly( current_filter->isChecked() );
}
