/****************************************************************************
**
** Copyright (C) 2022 Jan Sundermeyer
**
** License: GLP v3
**
****************************************************************************/

#include "diagramitem.h"
#include "diagramscene.h"
#include "diagramtextitem.h"
#include "diagramdrawitem.h"
#include "diagramelement.h"
#include "diagrampathitem.h"
#include "mainwindow.h"

#include <QtWidgets>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>

const int InsertTextButton = 10;
const int InsertDrawItemButton = 64;

//! [0]
MainWindow::MainWindow()
{
    createActions();
    createToolBox();
    createMenus();

    scene = new DiagramScene(itemMenu, this);
    scene->setSceneRect(QRectF(0, 0, 5000, 5000));
    connect(scene, &DiagramScene::itemSelected,
            this, &MainWindow::itemSelected);
    // activate/deactivate shortcuts when text is edited in scene
    connect(scene, SIGNAL(editorHasReceivedFocus()),
            this, SLOT(deactivateShortcuts()));
    connect(scene, SIGNAL(editorHasLostFocus()),
            this, SLOT(activateShortcuts()));
    connect(scene, &DiagramScene::zoomRect,
            this, &MainWindow::doZoomRect);
    connect(scene, &DiagramScene::zoom,
            this, &MainWindow::zoom);
    createToolbars();

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(toolBox);
    view = new QGraphicsView(scene);
    view->setDragMode(QGraphicsView::RubberBandDrag);
    view->setCacheMode(QGraphicsView::CacheBackground);
    view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    layout->addWidget(view);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    setCentralWidget(widget);
    setWindowTitle(tr("QDia"));
    setUnifiedTitleAndToolBarOnMac(true);
}

void MainWindow::buttonGroupClicked(QAbstractButton *button)
{
    QButtonGroup *buttonGroup=qobject_cast<QButtonGroup *>(sender());
    for (QAbstractButton *button : pointerTypeGroup->buttons()) {
        button->setChecked(false);
    }
    const QList<QAbstractButton *> buttons = buttonGroup->buttons();
    for (QAbstractButton *myButton : buttons) {
        if (myButton != button)
            myButton->setChecked(false);
    }
    currentToolButton=button;
    const int id = buttonGroup->id(button);
    if (id == InsertTextButton) {
        scene->setMode(DiagramScene::InsertText);
    } else {
        if ((id&192) == InsertDrawItemButton){
            scene->setItemType(DiagramDrawItem::DiagramType(id&63));
            scene->setMode(DiagramScene::InsertDrawItem);
        }
        else {
            if(id==128){
                QString fn=button->property("fn").toString();
                scene->setItemType(fn);
                scene->setMode(DiagramScene::InsertElement);
            }else{
                scene->setItemType(DiagramItem::DiagramType(id));
                scene->setMode(DiagramScene::InsertItem);
            }
        }
    }
}

void MainWindow::deleteItem()
{
    QList<QGraphicsItem *> selectedItems = scene->selectedItems();

    for (QGraphicsItem *item : qAsConst(selectedItems)) {
         scene->removeItem(item);
         delete item;
     }
}

void MainWindow::pointerGroupClicked(QAbstractButton *button)
{
    // uncheck toolbox
    if(currentToolButton){
        currentToolButton->setChecked(false);
        currentToolButton=nullptr;
    }
    QList<QAbstractButton *> buttons = pointerTypeGroup->buttons();
    foreach (QAbstractButton *mButton, buttons) {
        if(mButton!=button){
            mButton->setChecked(false);
        }
    }
    if(pointerTypeGroup->checkedId()!=DiagramScene::MoveItem) view->setDragMode(QGraphicsView::NoDrag);
    else view->setDragMode(QGraphicsView::RubberBandDrag);
    scene->setMode(DiagramScene::Mode(pointerTypeGroup->checkedId()));
}

void MainWindow::bringToFront()
{
    if (scene->selectedItems().isEmpty())
        return;
    scene->setCursorVisible(false);

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    const QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    for (const QGraphicsItem *item : overlapItems) {
        if (item->zValue() >= zValue)
            zValue = item->zValue() + 0.1;
    }
    selectedItem->setZValue(zValue);
    scene->setCursorVisible(true);
}

void MainWindow::sendToBack()
{
    if (scene->selectedItems().isEmpty())
        return;
    scene->setCursorVisible(false);
    QGraphicsItem *selectedItem = scene->selectedItems().first();
    const QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    for (const QGraphicsItem *item : overlapItems) {
        if (item->zValue() <= zValue)
            zValue = item->zValue() - 0.1;
    }
    selectedItem->setZValue(zValue);
    scene->setCursorVisible(true);
}

void MainWindow::rotateRight()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    QTransform trans=selectedItem->transform();
    selectedItem->setTransform(trans*QTransform().rotate(90),false);
}

void MainWindow::rotateLeft()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    QTransform trans=selectedItem->transform();
    selectedItem->setTransform(trans*QTransform().rotate(-90),false);
}

void MainWindow::flipX()
{
    if (scene->selectedItems().isEmpty())
        return;
    QGraphicsItem *selectedItem = scene->selectedItems().first();
    if(selectedItem->childItems().isEmpty()){
        QTransform trans=selectedItem->transform();
        selectedItem->setTransform(trans*QTransform(-1,0,0,1,0,0),false);
    }else{
        QRectF rect=selectedItem->boundingRect();
        qreal xc=rect.center().x();
        foreach(QGraphicsItem *item,selectedItem->childItems()){
            QTransform trans=item->transform();
            qreal dx=item->x()-xc;
            item->setTransform(trans*QTransform(1,0,0,1,dx,0)*QTransform(-1,0,0,1,0,0)*QTransform(1,0,0,1,-dx,0),false);
        }
    }
}

void MainWindow::flipY()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    if(selectedItem->childItems().isEmpty()){
        QTransform trans=selectedItem->transform();
        selectedItem->setTransform(trans*QTransform(1,0,0,-1,0,0),false);
    }else{
        QRectF rect=selectedItem->boundingRect();
        qreal yc=rect.center().y();
        foreach(QGraphicsItem *item,selectedItem->childItems()){
            QTransform trans=item->transform();
            qreal dy=item->y()-yc;
            item->setTransform(trans*QTransform(1,0,0,1,0,dy)*QTransform(1,0,0,-1,0,0)*QTransform(1,0,0,1,0,-dy),false);
        }
    }
}

//! [9]
void MainWindow::currentFontChanged(const QFont &)
{
    handleFontChange();
}
//! [9]

//! [10]
void MainWindow::fontSizeChanged(const QString &)
{
    handleFontChange();
}
//! [10]

//! [11]
void MainWindow::sceneScaleChanged(const QString &scale)
{
    double newScale = scale.left(scale.indexOf(tr("%"))).toDouble() / 100.0;
    QTransform oldMatrix = view->transform();
    view->resetTransform();
    view->translate(oldMatrix.dx(), oldMatrix.dy());
    view->scale(newScale, newScale);
}
//! [11]

//! [12]
void MainWindow::textColorChanged()
{
    textAction = qobject_cast<QAction *>(sender());
    fontColorToolButton->setIcon(createColorToolButtonIcon(
                                     ":/images/textpointer.png",
                                     qvariant_cast<QColor>(textAction->data())));
    textButtonTriggered();
}
//! [12]

//! [13]
void MainWindow::itemColorChanged()
{
    fillAction = qobject_cast<QAction *>(sender());
    fillColorToolButton->setIcon(createColorToolButtonIcon(
                                     ":/images/floodfill.png",
                                     qvariant_cast<QColor>(fillAction->data())));
    fillButtonTriggered();
}

void MainWindow::lineColorChanged()
{
    lineAction = qobject_cast<QAction *>(sender());
    lineColorToolButton->setIcon(createColorToolButtonIcon(
                                     ":/images/linecolor.png",
                                     qvariant_cast<QColor>(lineAction->data())));
    lineButtonTriggered();
}

void MainWindow::textButtonTriggered()
{
    scene->setTextColor(qvariant_cast<QColor>(textAction->data()));
}

void MainWindow::fillButtonTriggered()
{
    scene->setItemColor(qvariant_cast<QColor>(fillAction->data()));
}

void MainWindow::lineButtonTriggered()
{
    scene->setLineColor(qvariant_cast<QColor>(lineAction->data()));
}

void MainWindow::handleFontChange()
{
    QFont font = fontCombo->currentFont();
    font.setPointSize(fontSizeCombo->currentText().toInt());
    font.setWeight(boldAction->isChecked() ? QFont::Bold : QFont::Normal);
    font.setItalic(italicAction->isChecked());
    font.setUnderline(underlineAction->isChecked());

    scene->setFont(font);
}

void MainWindow::itemSelected(QGraphicsItem *item)
{
    DiagramTextItem *textItem =
    qgraphicsitem_cast<DiagramTextItem *>(item);

    QFont font = textItem->font();
    fontCombo->setCurrentFont(font);
    fontSizeCombo->setEditText(QString().setNum(font.pointSize()));
    boldAction->setChecked(font.weight() == QFont::Bold);
    italicAction->setChecked(font.italic());
    underlineAction->setChecked(font.underline());
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About QDia"),
                       tr("Written by Jan Sundermeyer (C) 20222"
                          "Simple schematic/diagram entry editor."));
}

void MainWindow::createToolBox()
{
    struct Element {QString name; int type; };
    toolBox = new QToolBox;
    toolBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Ignored));

    QButtonGroup *bG = new QButtonGroup(this);
    bG->setExclusive(false);
    connect(bG, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::buttonGroupClicked);
    QGridLayout *layout = new QGridLayout;
    // added DrawItem
    QList<Element> elements{
        {tr("Rectangle"),DiagramDrawItem::Rectangle},
        {tr("Ellipse"),DiagramDrawItem::Ellipse},
        {tr("Circle"),DiagramDrawItem::Circle},
        {tr("RoundedRect"),DiagramDrawItem::RoundedRect},
        {tr("Rhombus"),DiagramDrawItem::Rhombus},
        {tr("Triangle"),DiagramDrawItem::Triangle},
        {tr("DA"),DiagramDrawItem::DA}
    };
    int row=0;
    int col=0;
    for (const Element &element: elements) {
        QWidget *bt=createCellWidget(element.name,element.type+InsertDrawItemButton,bG);
        layout->addWidget(bt, row, col);
        ++col;
        if(col>1){
            col=0;
            ++row;
        }
    }
    if(col==1) ++row;
    layout->setRowStretch(row, 10);
    layout->setColumnStretch(2, 10);

    QWidget *itemWidget = new QWidget;
    itemWidget->setLayout(layout);

    toolBox->addItem(itemWidget, tr("Basic Shapes"));

    bG = new QButtonGroup(this);
    bG->setExclusive(false);
    connect(bG, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::buttonGroupClicked);
    layout = new QGridLayout;
    // added DrawItem
    QString path=":/libs/analog/";
    QStringList files{
        "circle.json",
        "res.json",
        "cap.json",
        "ind.json",
        "nmos.json",
        "pmos.json",
        "vsource.json",
        "isource.json",
        "acsource.json",
        "gnd.json",
        "vdd.json"
    };
    row=0;
    col=0;
    for (const QString &fn: files) {
        QWidget *bt=createCellWidget(path+fn,128,bG);
        layout->addWidget(bt, row, col);
        ++col;
        if(col>1){
            col=0;
            ++row;
        }
    }

    layout->setRowStretch(row, 10);
    layout->setColumnStretch(2, 10);

    itemWidget = new QWidget;
    itemWidget->setLayout(layout);

    toolBox->addItem(itemWidget, tr("Basic Electronic Elements"));

    bG = new QButtonGroup(this);
    bG->setExclusive(false);
    connect(bG, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::buttonGroupClicked);
    layout = new QGridLayout;
    // added DrawItem
    QString path2=":/libs/gates/";
    QStringList files2{
        "and.json",
        "nand.json"
    };
    row=0;
    col=0;
    for (const QString &fn: files2) {
        QWidget *bt=createCellWidget(path2+fn,128,bG);
        layout->addWidget(bt, row, col);
        ++col;
        if(col>1){
            col=0;
            ++row;
        }
    }

    layout->setRowStretch(row, 10);
    layout->setColumnStretch(2, 10);

    itemWidget = new QWidget;
    itemWidget->setLayout(layout);

    toolBox->addItem(itemWidget, tr("Basic Digital Gates"));

    QButtonGroup *buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(false);
    connect(buttonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::buttonGroupClicked);
    layout = new QGridLayout;
    elements={
        {tr("Conditional"), DiagramItem::Conditional},
        {tr("Process"), DiagramItem::Step},
        {tr("Input/Output"), DiagramItem::Io},
    };
    row=0;
    col=0;
    for (const Element &element: elements) {
        QWidget *bt=createCellWidget(element.name,element.type,buttonGroup);
        layout->addWidget(bt, row, col);
        ++col;
        if(col>1){
            col=0;
            ++row;
        }
    }

    QToolButton *textButton = new QToolButton;
    textButton->setCheckable(true);
    buttonGroup->addButton(textButton, InsertTextButton);
    textButton->setIcon(QIcon(QPixmap(":/images/textpointer.png")));
    textButton->setIconSize(QSize(50, 50));
    QGridLayout *textLayout = new QGridLayout;
    textLayout->addWidget(textButton, 0, 0, Qt::AlignHCenter);
    textLayout->addWidget(new QLabel(tr("Text")), 1, 0, Qt::AlignCenter);
    QWidget *textWidget = new QWidget;
    textWidget->setLayout(textLayout);
    layout->addWidget(textWidget, 1, 1);

    layout->setRowStretch(row+1, 10);
    layout->setColumnStretch(2, 10);

    itemWidget = new QWidget;
    itemWidget->setLayout(layout);

    toolBox->setMinimumWidth(itemWidget->sizeHint().width());
    toolBox->addItem(itemWidget, tr("Basic Flowchart Shapes"));
}

void MainWindow::createActions()
{
    toFrontAction = new QAction(QIcon(":/images/bringtofront.svg"),
                                tr("Bring to &Front"), this);
    toFrontAction->setShortcut(tr("Ctrl+F"));
    toFrontAction->setStatusTip(tr("Bring item to front"));
    connect(toFrontAction, &QAction::triggered, this, &MainWindow::bringToFront);

    sendBackAction = new QAction(QIcon(":/images/sendtoback.svg"), tr("Send to &Back"), this);
    sendBackAction->setShortcut(tr("Ctrl+T"));
    sendBackAction->setStatusTip(tr("Send item to back"));
    connect(sendBackAction, &QAction::triggered, this, &MainWindow::sendToBack);

    rotateRightAction = new QAction(QIcon(":/images/object_rotate_right.svg"),
                                    tr("rotate &Right"), this);
    rotateRightAction->setShortcut(tr("R"));
    rotateRightAction->setStatusTip(tr("rotate item 90 degrees right"));
    connect(rotateRightAction, SIGNAL(triggered()),
            this, SLOT(rotateRight()));
    listOfActions.append(rotateRightAction);

    rotateLeftAction = new QAction(QIcon(":/images/object_rotate_left.svg"),
                                   tr("rotate &Left"), this);
    rotateLeftAction->setShortcut(tr("Shift+R"));
    rotateLeftAction->setStatusTip(tr("rotate item 90 degrees left"));
    connect(rotateLeftAction, SIGNAL(triggered()),
            this, SLOT(rotateLeft()));
    listOfActions.append(rotateLeftAction);

    groupAction = new QAction(QIcon(":/images/object_group.svg"),
                              tr("&group Items"), this);
    groupAction->setShortcut(tr("Ctrl+G"));
    groupAction->setStatusTip(tr("group Items"));
    connect(groupAction, SIGNAL(triggered()),
            this, SLOT(groupItems()));

    ungroupAction = new QAction(QIcon(":/images/object_ungroup.svg"),
                                tr("&ungroup Item"), this);
    ungroupAction->setShortcut(tr("Shift+Ctrl+G"));
    ungroupAction->setStatusTip(tr("ungroup Items"));
    connect(ungroupAction, SIGNAL(triggered()),
            this, SLOT(ungroupItems()));

    deleteAction = new QAction(tr("&Delete Item"), this);
    deleteAction->setShortcut(tr("Delete"));
    deleteAction->setStatusTip(tr("Delete item from diagram"));
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteItem);

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcuts(QKeySequence::Quit);
    exitAction->setStatusTip(tr("Quit Scenediagram example"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    boldAction = new QAction(tr("Bold"), this);
    boldAction->setCheckable(true);
    QPixmap pixmap(":/images/bold.svg");
    boldAction->setIcon(QIcon(pixmap));
    boldAction->setShortcut(tr("Ctrl+B"));
    connect(boldAction, &QAction::triggered, this, &MainWindow::handleFontChange);

    italicAction = new QAction(QIcon(":/images/italic.svg"), tr("Italic"), this);
    italicAction->setCheckable(true);
    italicAction->setShortcut(tr("Ctrl+I"));
    connect(italicAction, &QAction::triggered, this, &MainWindow::handleFontChange);

    underlineAction = new QAction(QIcon(":/images/underline.svg"), tr("Underline"), this);
    underlineAction->setCheckable(true);
    underlineAction->setShortcut(tr("Ctrl+U"));
    connect(underlineAction, &QAction::triggered, this, &MainWindow::handleFontChange);

    aboutAction = new QAction(tr("A&bout"), this);
    aboutAction->setShortcut(tr("F1"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);

    printAction = new QAction(QIcon(":/images/document-print.svg"),tr("&Print"), this);
    printAction->setStatusTip(tr("Print Diagram"));
    connect(printAction, SIGNAL(triggered()), this, SLOT(print()));

    exportAction = new QAction(tr("&Export Diagram"), this);
    exportAction->setStatusTip(tr("Export Diagram to image"));
    connect(exportAction, SIGNAL(triggered()), this, SLOT(exportImage()));

    copyAction = new QAction(tr("&Copy"), this);
    copyAction->setShortcut(tr("c"));
    connect(copyAction, SIGNAL(triggered()),
            this, SLOT(copyItems()));
    listOfActions.append(copyAction);

    moveAction = new QAction(tr("&Move"), this);
    moveAction->setShortcut(tr("m"));
    connect(moveAction, SIGNAL(triggered()),
            this, SLOT(moveItems()));
    listOfActions.append(moveAction);

    flipXAction = new QAction(QIcon(":/images/lc_flipvertical.png"),
                              tr("Flip &X"), this);
    flipXAction->setShortcut(tr("f"));
    connect(flipXAction, SIGNAL(triggered()),
            this, SLOT(flipX()));
    listOfActions.append(flipXAction);

    flipYAction = new QAction(QIcon(":/images/lc_fliphorizontal.png"),tr("Flip &Y"), this);
    flipYAction->setShortcut(tr("Shift+F"));
    connect(flipYAction, SIGNAL(triggered()),
            this, SLOT(flipY()));
    listOfActions.append(flipYAction);

    escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape),
                                this);
    connect(escShortcut,&QShortcut::activated,this,&MainWindow::abort);

    // Zoom in/out
    zoomInAction = new QAction(QIcon(":/images/zoomin.svg"),tr("Zoom &in"), this);
    zoomInAction->setShortcut(tr("Shift+z"));
    connect(zoomInAction, SIGNAL(triggered()),
            this, SLOT(zoomIn()));
    listOfActions.append(zoomInAction);

    zoomOutAction = new QAction(QIcon(":/images/zoomout.svg"),tr("Zoom &out"), this);
    zoomOutAction->setShortcut(tr("Ctrl+z"));
    connect(zoomOutAction, SIGNAL(triggered()),
            this, SLOT(zoomOut()));

    zoomAction = new QAction(QIcon(":/images/zoom.svg"),tr("&Zoom area"), this);
    zoomAction->setShortcut(tr("z"));
    connect(zoomAction, SIGNAL(triggered()),
            this, SLOT(zoomRect()));
    listOfActions.append(zoomAction);

    zoomFitAction = new QAction(QIcon(":/images/zoompage.svg"),tr("Zoom &Fit"), this);
    zoomFitAction->setShortcut(tr("v"));
    connect(zoomFitAction, SIGNAL(triggered()),
            this, SLOT(zoomFit()));
    listOfActions.append(zoomFitAction);

    showGridAction = new QAction(QIcon(":/images/view-grid.svg"),tr("Show &Grid"), this);
    showGridAction->setCheckable(true);
    showGridAction->setChecked(false);
    connect(showGridAction, SIGNAL(toggled(bool)),
            this, SLOT(toggleGrid(bool)));

    loadAction = new QAction(QIcon(":/images/document-open.svg"),tr("L&oad ..."), this);
    loadAction->setShortcut(tr("Ctrl+o"));
    connect(loadAction, SIGNAL(triggered()),
            this, SLOT(load()));

    saveAction = new QAction(QIcon(":/images/document-save.svg"),tr("&Save ..."), this);
    saveAction->setShortcut(tr("Ctrl+s"));
    connect(saveAction, SIGNAL(triggered()),
            this, SLOT(save()));

    saveAsAction = new QAction(QIcon(":/images/document-save-as.svg"),tr("Save &As ..."), this);
    saveAsAction->setShortcut(tr("Ctrl+s"));
    connect(saveAsAction, SIGNAL(triggered()),
            this, SLOT(saveAs()));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(loadAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addAction(printAction);
    fileMenu->addAction(exportAction);
    fileMenu->addAction(exitAction);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(zoomInAction);
    viewMenu->addAction(zoomOutAction);
    viewMenu->addAction(zoomAction);
    viewMenu->addAction(zoomFitAction);
    viewMenu->addSeparator();
    viewMenu->addAction(showGridAction);

    itemMenu = menuBar()->addMenu(tr("&Item"));
    itemMenu->addAction(deleteAction);
    itemMenu->addAction(copyAction);
    itemMenu->addAction(moveAction);
    itemMenu->addSeparator();
    itemMenu->addAction(toFrontAction);
    itemMenu->addAction(sendBackAction);
    itemMenu->addSeparator();
    itemMenu->addAction(rotateRightAction);
    itemMenu->addAction(rotateLeftAction);
    itemMenu->addAction(flipXAction);
    itemMenu->addAction(flipYAction);
    itemMenu->addAction(groupAction);
    itemMenu->addAction(ungroupAction);

    aboutMenu = menuBar()->addMenu(tr("&Help"));
    aboutMenu->addAction(aboutAction);
}

void MainWindow::createToolbars()
{
    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(toFrontAction);
    editToolBar->addAction(sendBackAction);

    fontCombo = new QFontComboBox();
    connect(fontCombo, &QFontComboBox::currentFontChanged,
            this, &MainWindow::currentFontChanged);

    fontSizeCombo = new QComboBox;
    fontSizeCombo->setEditable(true);
    for (int i = 8; i < 30; i = i + 2)
        fontSizeCombo->addItem(QString().setNum(i));
    QIntValidator *validator = new QIntValidator(2, 64, this);
    fontSizeCombo->setValidator(validator);
    connect(fontSizeCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::fontSizeChanged);

    fontColorToolButton = new QToolButton;
    fontColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
    fontColorToolButton->setMenu(createColorMenu(SLOT(textColorChanged()), Qt::black));
    textAction = fontColorToolButton->menu()->defaultAction();
    fontColorToolButton->setIcon(createColorToolButtonIcon(":/images/textpointer.png", Qt::black));
    fontColorToolButton->setAutoFillBackground(true);
    connect(fontColorToolButton, &QAbstractButton::clicked,
            this, &MainWindow::textButtonTriggered);

    fillColorToolButton = new QToolButton;
    fillColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
    fillColorToolButton->setMenu(createColorMenu(SLOT(itemColorChanged()), Qt::white));
    fillAction = fillColorToolButton->menu()->defaultAction();
    fillColorToolButton->setIcon(createColorToolButtonIcon(
                                     ":/images/floodfill.png", Qt::white));
    connect(fillColorToolButton, &QAbstractButton::clicked,
            this, &MainWindow::fillButtonTriggered);

    lineColorToolButton = new QToolButton;
    lineColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
    lineColorToolButton->setMenu(createColorMenu(SLOT(lineColorChanged()), Qt::black));
    lineAction = lineColorToolButton->menu()->defaultAction();
    lineColorToolButton->setIcon(createColorToolButtonIcon(
                                     ":/images/linecolor.png", Qt::black));
    connect(lineColorToolButton, &QAbstractButton::clicked,
            this, &MainWindow::lineButtonTriggered);

    textToolBar = addToolBar(tr("Font"));
    textToolBar->addWidget(fontCombo);
    textToolBar->addWidget(fontSizeCombo);
    textToolBar->addAction(boldAction);
    textToolBar->addAction(italicAction);
    textToolBar->addAction(underlineAction);

    colorToolBar = addToolBar(tr("Color"));
    colorToolBar->addWidget(fontColorToolButton);
    colorToolBar->addWidget(fillColorToolButton);
    colorToolBar->addWidget(lineColorToolButton);

    pointerButton = new QToolButton;
    pointerButton->setCheckable(true);
    pointerButton->setChecked(true);
    pointerButton->setIcon(QIcon(":/images/tool-pointer.svg"));
    linePointerButton = new QToolButton;
    linePointerButton->setCheckable(true);
    linePointerButton->setIcon(QIcon(":/images/linepointer.png"));
    linePointerButton->setIcon(createArrowIcon(0));
    linePointerButton->setPopupMode(QToolButton::MenuButtonPopup);
    linePointerButton->setMenu(createArrowMenu(SLOT(lineArrowChanged()),
                                               0));
    arrowAction = linePointerButton->menu()->defaultAction();
    connect(linePointerButton, SIGNAL(clicked()),
            this, SLOT(lineArrowButtonTriggered()));

    pointerTypeGroup = new QButtonGroup(this);
    pointerTypeGroup->setExclusive(false);
    pointerTypeGroup->addButton(pointerButton, int(DiagramScene::MoveItem));
    pointerTypeGroup->addButton(linePointerButton, int(DiagramScene::InsertLine));
    connect(pointerTypeGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::pointerGroupClicked);

    pointerToolbar = addToolBar(tr("Pointer type"));
    pointerToolbar->addWidget(pointerButton);
    pointerToolbar->addWidget(linePointerButton);

    zoomToolbar = addToolBar(tr("Zoom"));
    zoomToolbar->addAction(zoomInAction);
    zoomToolbar->addAction(zoomOutAction);
    zoomToolbar->addAction(zoomAction);
    zoomToolbar->addAction(zoomFitAction);
}

QWidget *MainWindow::createCellWidget(const QString &text,
                      int type,QButtonGroup *buttonGroup)
{
    QToolButton *button = new QToolButton;
    QString name=text;
    if(type==128){
        DiagramElement item(text, itemMenu);
        QIcon icon(item.image());
        button->setIcon(icon);
        button->setProperty("fn",text);
        name=item.getName();
    }else{
        if(type>63){
            DiagramDrawItem item(static_cast<DiagramDrawItem::DiagramType>(type-64), itemMenu);
            item.setPos2(230,230);
            QIcon icon(item.image());
            button->setIcon(icon);
        }else{
            DiagramItem item(static_cast<DiagramItem::DiagramType>(type), itemMenu);
            QIcon icon(item.image());
            button->setIcon(icon);
        }
    }
    button->setIconSize(QSize(50, 50));
    button->setCheckable(true);
    buttonGroup->addButton(button, type);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(button, 0, 0, Qt::AlignHCenter);
    layout->addWidget(new QLabel(name), 1, 0, Qt::AlignCenter);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    return widget;
}

QMenu *MainWindow::createColorMenu(const char *slot, QColor defaultColor)
{
    QList<QColor> colors;
    colors << Qt::black << Qt::white << Qt::red << Qt::blue << Qt::yellow;
    QStringList names;
    names << tr("black") << tr("white") << tr("red") << tr("blue")
          << tr("yellow");

    QMenu *colorMenu = new QMenu(this);
    for (int i = 0; i < colors.count(); ++i) {
        QAction *action = new QAction(names.at(i), this);
        action->setData(colors.at(i));
        action->setIcon(createColorIcon(colors.at(i)));
        connect(action, SIGNAL(triggered()), this, slot);
        colorMenu->addAction(action);
        if (colors.at(i) == defaultColor)
            colorMenu->setDefaultAction(action);
    }
    return colorMenu;
}

QIcon MainWindow::createColorToolButtonIcon(const QString &imageFile, QColor color)
{
    QPixmap pixmap(50, 80);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    QPixmap image(imageFile);
    // Draw icon centred horizontally on button.
    QRect target(4, 0, 42, 43);
    QRect source(0, 0, 42, 43);
    painter.fillRect(QRect(0, 60, 50, 80), color);
    painter.drawPixmap(target, image, source);

    return QIcon(pixmap);
}

QIcon MainWindow::createColorIcon(QColor color)
{
    QPixmap pixmap(20, 20);
    QPainter painter(&pixmap);
    painter.setPen(Qt::NoPen);
    painter.fillRect(QRect(0, 0, 20, 20), color);

    return QIcon(pixmap);
}
//! [32]

void MainWindow::copyItems()
{
    scene->setMode(DiagramScene::CopyItem);
    view->setDragMode(QGraphicsView::RubberBandDrag);
}

void MainWindow::groupItems()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItemGroup *test = scene->createItemGroup(scene->selectedItems());
    test->setFlag(QGraphicsItem::ItemIsMovable, true);
    test->setFlag(QGraphicsItem::ItemIsSelectable, true);
}

void MainWindow::ungroupItems()
{
    if (scene->selectedItems().isEmpty())
        return;

    foreach (QGraphicsItem *item, scene->selectedItems()) {
        if (item->type()==10) {
            QGraphicsItemGroup *group = (QGraphicsItemGroup*) item;
            scene->destroyItemGroup(group);
        }
    }
    //QGraphicsItemGroup *group = (QGraphicsItemGroup*)scene->selectedItems().first();
    //scene->destroyItemGroup(group);
    //scene->destroyItemGroup(scene->selectedItems());
}

void MainWindow::activateShortcuts()
{
    foreach(QAction* item,listOfActions){
        item->setEnabled(true);
    }
    foreach(QShortcut* item,listOfShortcuts){
        item->setEnabled(true);
    }
}

void MainWindow::deactivateShortcuts()
{
    foreach(QAction* item,listOfActions){
        item->setEnabled(false);
    }
    foreach(QShortcut* item,listOfShortcuts){
        item->setEnabled(false);
    }
}

void MainWindow::print()
{
    QPrinter printer;
    if (QPrintDialog(&printer).exec() == QDialog::Accepted) {
        QPainter painter(&printer);
        painter.setRenderHint(QPainter::Antialiasing);
        scene->render(&painter);
    }
}

void MainWindow::moveItems()
{
    scene->setMode(DiagramScene::MoveItems);
    view->setDragMode(QGraphicsView::RubberBandDrag);
}

void MainWindow::abort()
{
    scene->abort();
    pointerButton->setChecked(true);
    pointerGroupClicked(pointerButton);
}

void MainWindow::exportImage()
{
    scene->setCursorVisible(false);
    QFileDialog::Options options;
    QString selectedFilter;
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Export Diagram to ..."),
            ".jpg",
            tr("Jpg (*.jpg);;Png (*.png);;Pdf (*.pdf);;Postscript (*.ps)"),
            &selectedFilter,
            options);
    if (!fileName.isEmpty()){
        if((selectedFilter=="Pdf (*.pdf)")or(selectedFilter=="Postscript (*.ps)")) {
            QRectF rect=scene->itemsBoundingRect(); // Bonding der Elemente in scene
            QPrinter printer;
            printer.setOutputFileName(fileName);
            QRectF size=printer.pageRect(QPrinter::Millimeter); // definiere Paper mit gleichen Aspectratio wie rect
            size.setHeight(size.width()*rect.height()/rect.width());
            //printer.setPageSize(size,QPrinter::Millimeter);
            //printer.setPageMargins(0,0,0,0,QPrinter::Millimeter);
            QPainter painter(&printer);// generate PDF/PS
            painter.setRenderHint(QPainter::Antialiasing);
            scene->render(&painter,QRectF(),rect);
        }
        else {
            QPixmap pixmap(1000,1000);
            pixmap.fill();
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            QRectF rect=scene->itemsBoundingRect();
            scene->render(&painter,QRectF(),rect);
            painter.end();

            pixmap.save(fileName);
        }

    }
    scene->setCursorVisible(true);
}

void MainWindow::zoomIn()
{
    zoom(2.0);
}

void MainWindow::zoomOut()
{
    zoom(0.5);
}

void MainWindow::zoom(const qreal factor)
{
    QPointF topLeft     = view->mapToScene( 0, 0 );
    QPointF bottomRight = view->mapToScene( view->viewport()->width() - 1, view->viewport()->height() - 1 );
    qreal width=bottomRight.x()-topLeft.x();
    qreal height=bottomRight.y()-topLeft.y();
    if((width/factor<=5000)&&(height/factor<=5000)){
        QTransform oldMatrix = view->transform();
        qreal newScale=oldMatrix.m11()*factor;
        view->resetTransform();
        view->translate(oldMatrix.dx(), oldMatrix.dy());
        view->scale(newScale, newScale);

        setGrid();
    }
}

void MainWindow::zoomRect()
{
    scene->setMode(DiagramScene::Zoom);
    view->setDragMode(QGraphicsView::RubberBandDrag);
    setGrid();
}

void MainWindow::doZoomRect(QPointF p1,QPointF p2)
{
    QRectF r(p1,p2);
    view->fitInView(r.normalized(),Qt::KeepAspectRatio);
    setGrid();
}

void MainWindow::zoomFit()
{
    scene->setCursorVisible(false);
    view->fitInView(scene->itemsBoundingRect(),Qt::KeepAspectRatio);
    scene->setCursorVisible(true);
    setGrid();
}

void MainWindow::toggleGrid(bool grid)
{
    scene->setGridVisible(grid);
    QPointF topLeft     = view->mapToScene( 0, 0 );
    QPointF bottomRight = view->mapToScene( view->viewport()->width() - 1, view->viewport()->height() - 1 );
    scene->invalidate(topLeft.x(),topLeft.y(),bottomRight.x()-topLeft.x(),bottomRight.y()-topLeft.y());
}

void MainWindow::setGrid()
{
    if(scene->isGridVisible()){
        QPointF topLeft     = view->mapToScene( 0, 0 );
        QPointF bottomRight = view->mapToScene( view->viewport()->width() - 1, view->viewport()->height() - 1 );
        qreal width=bottomRight.x()-topLeft.x();
        qreal height=bottomRight.y()-topLeft.y();
        qreal zw=width;
        if(zw<height) zw=height;
        int n=int(zw)/int(scene->grid());
        int k=1;
        while(n/k>50)
        {
            k=k*2;
        }
        scene->setGridScale(k);
        scene->update();
    }
}

void MainWindow::saveAs()
{
    QFileDialog::Options options;
    QString selectedFilter;
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save Diagram as ..."),
            ".json",
            tr("QDiagram (*.json)"),
            &selectedFilter,
            options);
    if (!fileName.isEmpty()){
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
            QMessageBox::warning(this,tr("File operation error"),file.errorString());
        }
        else
        {
            if(scene->save_json(&file)){
                myFileName=fileName;
            }
            file.close();
            if(file.error()){
                qDebug() << "Error: cannot write file "
                << file.fileName()
                << file.errorString();
            }
        }
    }
}

void MainWindow::save()
{
    if (!myFileName.isEmpty()){
        QFile file(myFileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
            QMessageBox::warning(this,tr("File operation error"),file.errorString());
        }
        else
        {
            scene->save_json(&file);
        }
    }
}

void MainWindow::load()
{
    QFileDialog::Options options;
    QString selectedFilter;
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Load Diagram"),
            ".json",
            tr("QDiagram (*.json)"),
            &selectedFilter,
            options);
    if (!fileName.isEmpty()){
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
            QMessageBox::warning(this,tr("File operation error"),file.errorString());
        }
        else
        {
            scene->clear();
            scene->load_json(&file);
            myFileName=fileName;
        }

    }
}

QMenu *MainWindow::createArrowMenu(const char *slot, const int def)
{
    QStringList names;
        names << tr("Path") << tr("Start") << tr("End") << tr("StartEnd");
    QMenu *arrowMenu = new QMenu;
    for (int i = 0; i < names.count(); ++i) {
        QAction *action = new QAction(names.at(i), this);
        action->setData(i);
        action->setIcon(createArrowIcon(i));
        connect(action, SIGNAL(triggered()),
                this, slot);
        arrowMenu->addAction(action);
        if (i == def) {
            arrowMenu->setDefaultAction(action);
        }
    }
    return arrowMenu;
}

QIcon MainWindow::createArrowIcon(const int i)
{
    QPixmap pixmap(50, 80);
    DiagramPathItem* item=new DiagramPathItem(DiagramPathItem::DiagramType(i),0,0);
    pixmap=item->icon();
    delete item;

    return QIcon(pixmap);
}

void MainWindow::lineArrowButtonTriggered()
{
    scene->setArrow(arrowAction->data().toInt());
    pointerTypeGroup->button(int(DiagramScene::MoveItem))->setChecked(false);
}

void MainWindow::lineArrowChanged()
{
    arrowAction = qobject_cast<QAction *>(sender());
    linePointerButton->setIcon(createArrowIcon(arrowAction->data().toInt()));
    lineArrowButtonTriggered();
}