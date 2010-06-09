/////////////////////////////////////////////////////////////////////
// File: TreeItem.h                                                //
// Date: 12/3/10                                                   //
// Desc: Represents an item in the window hierarchy tree.          //
//   Two subclasses are defined: ProcessItem for process objects   //
//   and WindowItem for window objects. Each display their own     //
//   info and have their own context menu.                         //
/////////////////////////////////////////////////////////////////////

#ifndef TREE_ITEM_H
#define TREE_ITEM_H

#include "window_detective/include.h"
#include "inspector/inspector.h"
using namespace inspector;


#define HIDDEN_WINDOW_COLOUR  QColor(100, 100, 100)
#define MAX_EXPAND_LEVEL      10      // Max depth to expand to

enum TreeItemType {
    ProcessItemType = QTreeWidgetItem::UserType + 1,
    WindowItemType  = QTreeWidgetItem::UserType + 2
};

/*
  TODO:
   Need an item class for the find result window (probably a table).
   A QTableWidgetItem is different to a tree's since it doesn't have
   columns, each item is a cell. Have to figure out how to do that.
   Probably move to a subfolder - ui/item_widgets
*/

class TreeItem;
class WindowTree;

/*------------------------------------------------------------------+
 | TreeHighlight class                                              |
 | This class is used for highlighting, and subsequently            |
 | unhighlighting, a tree item. It should be created with 'new' and |
 | it will delete itself after the timeout period has elapsed.      |
 +------------------------------------------------------------------*/
class TreeHighlight : public QObject {
    Q_OBJECT
private:
    static QBrush* defaultForeground;
    static QBrush* defaultBackground;

    QTimer timer;
    TreeItem* item;
    UpdateReason reason;
public:
    TreeHighlight(TreeItem* item, UpdateReason reason);
    ~TreeHighlight();

    UpdateReason getReason() { return reason; }
public slots:
    void highlight(bool isImmediate = true);
    void unhighlight();
};

/*------------------------------------------------------------------+
| TreeItem base class                                               |
+------------------------------------------------------------------*/
class TreeItem : public QObject, public QTreeWidgetItem {
    Q_OBJECT
private:
    TreeHighlight* updateHighlighter;
    TreeHighlight* createHighlighter;
    TreeHighlight* destroyHighlighter;
    QTimer* deletionTimer;      // Timer to delete item
public:
    TreeItem() {}
    TreeItem(int type) : QTreeWidgetItem(type) {}
    TreeItem(QTreeWidget* parent, int type) :
        QTreeWidgetItem(parent, type) {}
    TreeItem(QTreeWidgetItem* parent, int type) :
        QTreeWidgetItem(parent, type) {}
    ~TreeItem();

    virtual void initialize();
    virtual void setupData() = 0;
    virtual String tooltipText() = 0;
    TreeItem* ancestor();
    void expandAllChildren();
    void expandAncestors();
    void highlight(UpdateReason reason, bool isImmediate = true);
    void highlightVisible(UpdateReason reason);
public slots:
    virtual void update(UpdateReason reason);
};


/*------------------------------------------------------------------+
| ProcessItem                                                       |
+------------------------------------------------------------------*/
class ProcessItem : public TreeItem {
    Q_OBJECT
private:
    Process* process;
public:
    ProcessItem();
    ProcessItem(Process* process, QTreeWidget* parent = NULL);
    ProcessItem(Process* process, QTreeWidgetItem* parent = NULL);
    ~ProcessItem() {}

    Process* getProcess() { return process; }
    void setupData();
    String tooltipText();
};


/*------------------------------------------------------------------+
| WindowItem                                                        |
+------------------------------------------------------------------*/
class WindowItem : public TreeItem {
    Q_OBJECT
private:
    Window* window;
public:
    WindowItem();
    WindowItem(Window* window, QTreeWidget* parent = NULL);
    WindowItem(Window* window, QTreeWidgetItem* parent = NULL);
    ~WindowItem() {}

    Window* getWindow() { return window; }
    void initialize();
    void setupData();
    String tooltipText();
};

#endif   // TREE_ITEM_H