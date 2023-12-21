#include <QApplication>
#include <QQmlApplicationEngine>

#include "markdownhighlighter.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    qmlRegisterType<MarkdownHighlighter>("MarkdownHighlighter", 1, 0,
                                         "MarkdownHighlighter");

    QQmlApplicationEngine engine;

    const QUrl url(QStringLiteral("qrc:/example.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
