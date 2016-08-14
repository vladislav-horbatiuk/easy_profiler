/************************************************************************
* file name         : main_window.cpp
* ----------------- :
* creation time     : 2016/06/26
* copyright         : (c) 2016 Victor Zarubkin
* author            : Victor Zarubkin
* email             : v.s.zarubkin@gmail.com
* ----------------- :
* description       : The file contains implementation of MainWindow for easy_profiler GUI.
* ----------------- :
* change log        : * 2016/06/26 Victor Zarubkin: Initial commit.
*                   :
*                   : * 2016/06/27 Victor Zarubkin: Passing blocks number to ProfTreeWidget::setTree().
*                   :
*                   : * 2016/06/29 Victor Zarubkin: Added menu with tests.
*                   :
*                   : * 2016/06/30 Sergey Yagovtsev: Open file by command line argument
*                   :
*                   : *
* ----------------- :
* license           : TODO: add license text
************************************************************************/

#include <QStatusBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QSettings>
#include "main_window.h"
#include "blocks_tree_widget.h"
#include "blocks_graphics_view.h"
#include "globals.h"
#include <QTextCodec>
//////////////////////////////////////////////////////////////////////////

ProfMainWindow::ProfMainWindow() : QMainWindow(), m_treeWidget(nullptr), m_graphicsView(nullptr)
{
    setObjectName("ProfilerGUI_MainWindow");
    setWindowTitle("easy_profiler reader");
    setDockNestingEnabled(true);
    resize(800, 600);
    
    setStatusBar(new QStatusBar());

    auto graphicsView = new ProfGraphicsViewWidget();
    m_graphicsView = new QDockWidget("Blocks diagram");
    m_graphicsView->setMinimumHeight(50);
    m_graphicsView->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_graphicsView->setWidget(graphicsView);

    auto treeWidget = new ProfTreeWidget();
    m_treeWidget = new QDockWidget("Blocks hierarchy");
    m_treeWidget->setMinimumHeight(50);
    m_treeWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_treeWidget->setWidget(treeWidget);

    addDockWidget(Qt::TopDockWidgetArea, m_graphicsView);
    addDockWidget(Qt::BottomDockWidgetArea, m_treeWidget);

    auto actionOpen = new QAction("Open", nullptr);
    connect(actionOpen, &QAction::triggered, this, &This::onOpenFileClicked);

    auto actionReload = new QAction("Reload", nullptr);
    connect(actionReload, &QAction::triggered, this, &This::onReloadFileClicked);

    auto actionExit = new QAction("Exit", nullptr);
    connect(actionExit, &QAction::triggered, this, &This::onExitClicked);

    auto actionTestView = new QAction("Test viewport", nullptr);
    connect(actionTestView, &QAction::triggered, this, &This::onTestViewportClicked);

    auto menu = new QMenu("File");
    menu->addAction(actionOpen);
    menu->addAction(actionReload);
    menu->addSeparator();
    menu->addAction(actionExit);
    menuBar()->addMenu(menu);

    menu = new QMenu("Tests");
    menu->addAction(actionTestView);
    menuBar()->addMenu(menu);


    QSettings settings(::profiler_gui::ORGANAZATION_NAME, ::profiler_gui::APPLICATION_NAME);
    settings.beginGroup("main");

    QString encoding = settings.value("encoding","UTF-8").toString();

    auto default_codec_mib = QTextCodec::codecForName(encoding.toStdString().c_str())->mibEnum() ;
    auto default_codec = QTextCodec::codecForMib(default_codec_mib);
    QTextCodec::setCodecForLocale(default_codec);
    settings.endGroup();

    menu = new QMenu("Settings");
    auto encodingMenu = menu->addMenu(tr("&Encoding"));

    QActionGroup* codecs_actions = new QActionGroup(this);
    codecs_actions->setExclusive(true);
    foreach (int mib, QTextCodec::availableMibs())
    {
        auto codec = QTextCodec::codecForMib(mib)->name();

        QAction* action = new QAction(codec,codecs_actions);

        action->setCheckable(true);
        if(mib == default_codec_mib)
        {
            action->setChecked(true);
        }
        encodingMenu->addAction(action);
        connect(action, &QAction::triggered, this, &This::onEncodingChanged);

    }

    menuBar()->addMenu(menu);

    connect(graphicsView->view(), &ProfGraphicsView::intervalChanged, treeWidget, &ProfTreeWidget::setTreeBlocks);

    loadSettings();

    if(QCoreApplication::arguments().size() > 1)
    {
        auto opened_filename = QCoreApplication::arguments().at(1).toStdString();
        loadFile(opened_filename);
    }
}

ProfMainWindow::~ProfMainWindow()
{
}

//////////////////////////////////////////////////////////////////////////

void ProfMainWindow::onOpenFileClicked(bool)
{
    auto filename = QFileDialog::getOpenFileName(this, "Open profiler log", m_lastFile.c_str(), "Profiler Log File (*.prof);;All Files (*.*)");
    loadFile(filename.toStdString());
}

//////////////////////////////////////////////////////////////////////////

void ProfMainWindow::loadFile(const std::string& stdfilename)
{
    ::profiler::SerializedData data;
    ::profiler::thread_blocks_tree_t prof_blocks;
    auto nblocks = fillTreesFromFile(stdfilename.c_str(), data, prof_blocks, true);

    if (nblocks != 0)
    {
        static_cast<ProfTreeWidget*>(m_treeWidget->widget())->clearSilent(true);

        m_lastFile = stdfilename;
        m_serializedData = ::std::move(data);
        ::profiler_gui::EASY_GLOBALS.selected_thread = 0;
        ::profiler_gui::set_max(::profiler_gui::EASY_GLOBALS.selected_block);
        ::profiler_gui::EASY_GLOBALS.profiler_blocks.swap(prof_blocks);
        ::profiler_gui::EASY_GLOBALS.gui_blocks.resize(nblocks);
        memset(::profiler_gui::EASY_GLOBALS.gui_blocks.data(), 0, sizeof(::profiler_gui::ProfBlock) * nblocks);
        for (auto& guiblock : ::profiler_gui::EASY_GLOBALS.gui_blocks) ::profiler_gui::set_max(guiblock.tree_item);

        static_cast<ProfGraphicsViewWidget*>(m_graphicsView->widget())->view()->setTree(::profiler_gui::EASY_GLOBALS.profiler_blocks);
    }
}

//////////////////////////////////////////////////////////////////////////

void ProfMainWindow::onReloadFileClicked(bool)
{
    if (m_lastFile.empty())
    {
        return;
    }

    ::profiler::SerializedData data;
    ::profiler::thread_blocks_tree_t prof_blocks;
    auto nblocks = fillTreesFromFile(m_lastFile.c_str(), data, prof_blocks, true);

    if (nblocks != 0)
    {
        static_cast<ProfTreeWidget*>(m_treeWidget->widget())->clearSilent(true);

        m_serializedData = ::std::move(data);
        ::profiler_gui::EASY_GLOBALS.selected_thread = 0;
        ::profiler_gui::set_max(::profiler_gui::EASY_GLOBALS.selected_block);
        ::profiler_gui::EASY_GLOBALS.profiler_blocks.swap(prof_blocks);
        ::profiler_gui::EASY_GLOBALS.gui_blocks.resize(nblocks);
        memset(::profiler_gui::EASY_GLOBALS.gui_blocks.data(), 0, sizeof(::profiler_gui::ProfBlock) * nblocks);
        for (auto& guiblock : ::profiler_gui::EASY_GLOBALS.gui_blocks) ::profiler_gui::set_max(guiblock.tree_item);

        static_cast<ProfGraphicsViewWidget*>(m_graphicsView->widget())->view()->setTree(::profiler_gui::EASY_GLOBALS.profiler_blocks);
    }
}

//////////////////////////////////////////////////////////////////////////

void ProfMainWindow::onExitClicked(bool)
{
    close();
}

//////////////////////////////////////////////////////////////////////////

void ProfMainWindow::onTestViewportClicked(bool)
{
    static_cast<ProfTreeWidget*>(m_treeWidget->widget())->clearSilent(true);

    auto view = static_cast<ProfGraphicsViewWidget*>(m_graphicsView->widget())->view();
    view->clearSilent();

    m_serializedData.clear();
    ::profiler_gui::EASY_GLOBALS.gui_blocks.clear();
    ::profiler_gui::EASY_GLOBALS.profiler_blocks.clear();
    ::profiler_gui::EASY_GLOBALS.selected_thread = 0;
    ::profiler_gui::set_max(::profiler_gui::EASY_GLOBALS.selected_block);

    //view->test(18000, 40000000, 2);
    view->test(100, 9000, 1);
}

void ProfMainWindow::onEncodingChanged(bool)
{
   auto _sender = qobject_cast<QAction*>(sender());
   auto name = _sender->text();
   QTextCodec *codec = QTextCodec::codecForName(name.toStdString().c_str());
   QTextCodec::setCodecForLocale(codec);
}

//////////////////////////////////////////////////////////////////////////

void ProfMainWindow::closeEvent(QCloseEvent* close_event)
{
    saveSettings();
    QMainWindow::closeEvent(close_event);
}

//////////////////////////////////////////////////////////////////////////

void ProfMainWindow::loadSettings()
{
    QSettings settings(::profiler_gui::ORGANAZATION_NAME, ::profiler_gui::APPLICATION_NAME);
    settings.beginGroup("main");

    auto geometry = settings.value("geometry").toByteArray();
    if (!geometry.isEmpty())
    {
        restoreGeometry(geometry);
    }

    auto last_file = settings.value("last_file");
    if (!last_file.isNull())
    {
        m_lastFile = last_file.toString().toStdString();
    }

    settings.endGroup();
}

void ProfMainWindow::saveSettings()
{
	QSettings settings(::profiler_gui::ORGANAZATION_NAME, ::profiler_gui::APPLICATION_NAME);
	settings.beginGroup("main");

	settings.setValue("geometry", this->saveGeometry());
    settings.setValue("last_file", m_lastFile.c_str());
    settings.setValue("encoding", QTextCodec::codecForLocale()->name());

	settings.endGroup();
}

//////////////////////////////////////////////////////////////////////////
