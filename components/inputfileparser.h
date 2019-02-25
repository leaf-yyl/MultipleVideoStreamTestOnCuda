#ifndef INPUTFILEPARSER_H
#define INPUTFILEPARSER_H

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

    void emitSignalSetDecoder(AVCodecID, int, void *);
    void emitSignalPacketReady(void *sp, void *pkt_pool);
    void emitSignalParserDone();
};

#endif // INPUTFILEPARSER_H
