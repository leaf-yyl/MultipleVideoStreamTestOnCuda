#ifndef INPUTFILEPARSER_H
#define INPUTFILEPARSER_H

#include <QThread>

#include "inputparser.h"

class InputFileParser : public InputParser
{
    Q_OBJECT
public:
    explicit InputFileParser(QObject *parent = 0);

    void setInput(QString path) override;

protected:
    void run();

private:
    QString m_filepath;
};

#endif // INPUTFILEPARSER_H
