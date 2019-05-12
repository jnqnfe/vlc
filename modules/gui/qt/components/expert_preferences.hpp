/*****************************************************************************
 * expert_preferences.hpp : Detailed preferences overview
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

#ifndef VLC_QT_EXPERT_PREFERENCES_HPP_
#define VLC_QT_EXPERT_PREFERENCES_HPP_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_interface.h>
#include <vlc_config_cat.h>
#include <vlc_configuration.h>

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QSet>
#include <QFont>
#include <QDialog>
#include <QVBoxLayout>

class ConfigControl;
class ExpertPrefsEditDialog;

class ExpertPrefsItemData : public QObject
{
    Q_OBJECT
public:
    ExpertPrefsItemData( QObject * );
    ~ExpertPrefsItemData() { clearOwnedStringVal(); }
    bool contains( const QString &text, Qt::CaseSensitivity cs );
    void clearOwnedStringVal()
    {
        if (owned_string && item->value.psz) {
            free(item->value.psz);
            item->value.psz = NULL;
            owned_string = false;
        }
    }
    QString name;
    QString value;
    QString title;
    module_config_item_t *item;
    bool is_modified;
    bool owned_string;
};

Q_DECLARE_METATYPE( ExpertPrefsItemData* );

class PrefsTreeExpert : public QTreeWidget
{
    Q_OBJECT

public:
    PrefsTreeExpert( intf_thread_t *, QWidget *, module_t **, size_t );
    ~PrefsTreeExpert();

    void applyAll();
    void cleanAll();
    void filter( const QString &text );
    void updateDisplayedValue( QTreeWidgetItem*, ExpertPrefsItemData* );
    void setItemModifiedState( QTreeWidgetItem*, bool, ExpertPrefsItemData* );

protected:
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
#endif // QT_NO_CONTEXTMENU

private:
    void createItemNode( module_config_item_t*, const char*, const char*, size_t, bool );
    bool filterItems( QTreeWidgetItem *item, const QString &text, Qt::CaseSensitivity cs );
    void toggle( QTreeWidgetItem * );
    void modify( QTreeWidgetItem * );
    QAction *resetAct;
    QAction *toggleAct;
    QAction *modifyAct;
    QAction *copyNameAct;
    QAction *copyValueAct;
    intf_thread_t *p_intf;
    ExpertPrefsEditDialog *expert_edit;
    QList<module_config_item_t *> config_sets;
    QString state_modified_text;
    QString state_default_text;

private slots:
    void reset();
    void toggle();
    void modify();
    void copyName();
    void copyValue();
    void doubleClickedItem( QTreeWidgetItem * );
};

class ExpertPrefsEditDialog : public QDialog
{
    Q_OBJECT
public:
    ExpertPrefsEditDialog( intf_thread_t *, PrefsTreeExpert * );
    void addControl( QTreeWidgetItem *, ExpertPrefsItemData * );
private:
    void clearControl();
    intf_thread_t *p_intf;
    PrefsTreeExpert *tree;
    QVBoxLayout *layout;
    QWidget *control_widget;
    ConfigControl *control;
    QTreeWidgetItem *tree_item;
    ExpertPrefsItemData *data;
private slots:
    void accept();
    void reject();
};

#endif
