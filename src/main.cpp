#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QSharedMemory>
#include "bigpicturetv.h"
#include "utils.h"

int main(int argc, char *argv[])
{
    QSharedMemory sharedMemory("BigPictureTVUniqueIdentifier");

    if (sharedMemory.attach()) {
        return 0;
    }

    if (!sharedMemory.create(1)) {
        qDebug() << "Unable to create shared memory segment.";
        return 1;
    }

    QApplication a(argc, argv);
    if (isWindows10()) {
        a.setStyle("fusion");
    }

    BigPictureTV w;
    QObject::connect(&a, &QApplication::aboutToQuit, [&sharedMemory]() { sharedMemory.detach(); });

    return a.exec();
}
