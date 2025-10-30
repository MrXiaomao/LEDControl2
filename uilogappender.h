#ifndef UILOGAPPENDER_H
#define UILOGAPPENDER_H

#include <log4qt/appenderskeleton.h>
#include <log4qt/loggingevent.h>
#include <log4qt/layout.h>
#include <QObject>
#include <QString>

class UiLogAppender : public Log4Qt::AppenderSkeleton
{
    Q_OBJECT
public:
    explicit UiLogAppender(QObject *parent = nullptr)
        : Log4Qt::AppenderSkeleton(false, parent) {}

    bool requiresLayout() const override { return true; }

signals:
    void logToUi(const QString &logMessage);

protected:
    void append(const Log4Qt::LoggingEvent &event) override
    {
        // 忽略 DEBUG 级别的日志
        if (event.level() == Log4Qt::Level::DEBUG_INT)
            return;

        if (layout())
            emit logToUi(layout()->format(event));
    }
};

#endif // UILOGAPPENDER_H
