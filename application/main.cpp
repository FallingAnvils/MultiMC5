#include "MultiServerMC.h"
#include "MainWindow.h"
#include "LaunchController.h"
#include <InstanceList.h>
#include <QDebug>

// #define BREAK_INFINITE_LOOP
// #define BREAK_EXCEPTION
// #define BREAK_RETURN

#ifdef BREAK_INFINITE_LOOP
#include <thread>
#include <chrono>
#endif

int main(int argc, char *argv[])
{
#ifdef BREAK_INFINITE_LOOP
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
#endif
#ifdef BREAK_EXCEPTION
    throw 42;
#endif
#ifdef BREAK_RETURN
    return 42;
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // initialize Qt
    MultiServerMC app(argc, argv);

    switch (app.status())
    {
    case MultiServerMC::StartingUp:
    case MultiServerMC::Initialized:
    {
        Q_INIT_RESOURCE(multiservermc);

        return app.exec();
    }
    case MultiServerMC::Failed:
        return 1;
    case MultiServerMC::Succeeded:
        return 0;
    }
}
