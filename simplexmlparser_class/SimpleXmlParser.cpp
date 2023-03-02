/********************************************************************************
 *   Copyright (C) 2012-2016 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica		<f.lamonica@netresults.it>              *
 ********************************************************************************/

#include "SimpleXmlParser.h"

#include <QDebug>
#include <QStringList>
#include <QRegularExpression>

/*!
   \class SimpleXmlParser
   \brief this class implements a very simple xml parser that has an hybrid function between SAX and DOM
   \note it does not require the XML module from Qt
   \note it does not yet support attribute parsing (we said it's simple :) )
  */
SimpleXmlParser::SimpleXmlParser(QObject *parent)
    : QObject(parent),
      m_maxBufferSizeInBytes(0)
{
    m_notifyMode = E_NotifyOnly;
}


void
SimpleXmlParser::setMaxBufferSize(int sizeInBytes)
{
    if (sizeInBytes >= 0) {
        m_maxBufferSizeInBytes = sizeInBytes;
    }
}


QString
SimpleXmlParser::getCurrentBuffer() const
{
    return m_buffer;
}



void
SimpleXmlParser::emptyBuffer()
{
    m_buffer.clear();
}



QString
SimpleXmlParser::unquoteString(const QString &s)
{
    if ( s.at(0) == s.at(s.length()-1) && ( s.at(0) == '\'' || s.at(0) == '"' ) )
        return s.mid(1, s.length() - 2);

    return s;
}



QString
SimpleXmlParser::decodeEntities(const QString &s)
{
    QString ret(s);
    ret.replace("&amp;", "&").replace("&gt;", ">").replace("&lt;", "<").replace("&quot;", "\"").replace("&apos;", "'");

    // remove invalid chars (replacement char has U+FFFD as unicode code)
    ret.replace(QChar(0xFFFD), " ");

    QRegularExpression re("&#([0-9]+);|&#x([0-9A-F]+);",
                          QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);

    QRegularExpressionMatchIterator i = re.globalMatch(ret);
    int offset = 0;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString matched = match.captured(0);
        int position = match.capturedStart();
        int length = matched.length();
        QString decoded = QChar(matched.startsWith("&#x") ? match.captured(2).toInt(0, 16) : match.captured(1).toInt(0, 10));

        ret.replace(position + offset, length, decoded);
        offset += decoded.length() - length;
    }

    return ret;
}



QString
SimpleXmlParser::encodeEntities(const QString &s, bool encodeNonAscii)
{
    QString ret(s);
    if (encodeNonAscii)
    {
        uint len = ret.length();
        uint i = 0;
        while(i < len)
        {
            if(ret[i].unicode() > 128)
            {
                QString rp = "&#x" + QString::number(ret[i].unicode(), 16) + ";";
                ret.replace(i, 1, rp);
                len += rp.length() -1;
                i += rp.length();
            }
            else
            {
                i++;
            }
        }
    }
    ret.replace("&", "&amp;").replace(">", "&gt;").replace("<", "&lt;").replace("\"", "&quot;").replace("'", "&apos;");
    return ret;
}



bool
SimpleXmlParser::findStartTagDelimiters(const QString &i_msg, const QString &i_tagname, int i_beginidx, int &o_startIdx, int &o_endIdx)
{
    QRegularExpression rx("<"+i_tagname+"[>|\\s]");     //find the beginning of the start tag (we use the \\s to avoid mismatches
    QRegularExpression endrx("(>|/>)");            //to check where the start tag ends (handling properties)


    QRegularExpressionMatch startMatch = rx.match(i_msg, i_beginidx);
        o_startIdx = startMatch.capturedStart();
        if (!startMatch.hasMatch()) {
            return false;
        }

        QRegularExpressionMatch endMatch = endrx.match(i_msg, o_startIdx);
        o_endIdx = endMatch.capturedStart();
        if (!endMatch.hasMatch()) {
            return false;
        }

    #ifdef SXML_DBG
        qDebug() << "Matched/captured text:" << endMatch.capturedTexts();
        qDebug() << "idx, endix: " << o_startIdx << o_endIdx;
    #endif

        return endMatch.captured(1) == "/>";
}



/*!
  \brief this is a commodity function that parses a string looking for an xml tag and returns what is inside
  \param i_msg the message to parse
  \param i_tag the tag we want to find and parse
  \param i_offset the initial offset we should start looking the tag from, its default is 0
  \return the string contained within the found tag
  \note we assume the tag ALWAYS exists
  */
QString
SimpleXmlParser::getTagValue(const QString & i_msg, const QString & i_tag, int i_offset, QString defaultValue)
{
    QString tagname = i_tag;
    tagname.remove(QRegularExpression("[<>]"));
    QString endtag = "</" + tagname + ">";

    int idx, endidx;
    bool emptytag = findStartTagDelimiters(i_msg, tagname, i_offset, idx, endidx);

#ifdef SXML_DBG
    qDebug() << "idx, endix: " << idx << endidx;
#endif
    if (emptytag) {
        return "";
    }

    //it was not empty... go on
    int idx2 = i_msg.indexOf(endtag, idx);
    if (idx < 0 || idx2 < 0)
        return defaultValue;

    QString tag = i_msg.mid(endidx + 1, idx2 - (endidx + 1));
    return tag;
}



QString
SimpleXmlParser::getDecodedTagValue(const QString &msg, const QString &tag, int beginidx, QString defaultValue)
{
    return decodeEntities(getTagValue(msg, tag, beginidx, defaultValue));
}



/*!
  \brief this method counts the number of tag occurence in the message and then calls repeatedly the getTagValue passing the specific offset
  \param _msg the entire message to parse
  \param _tag the tag we want to gather the values
  \return a list of string containing all the values of the specified tags
  */
QStringList
SimpleXmlParser::getTagsValues(const QString & _msg, const QString & _tag)
{
        QStringList vlist;
        int idx,last=0;
        QString ntag = _tag;
        ntag.remove(QRegularExpression("[<>]"));
        QRegularExpression rx("<" + ntag + "[\\s*|>]");
        int numtags = _msg.count(rx);
        for (int i=0; i<numtags; i++) {
#ifdef SXML_DBG
            qDebug() << "parsing loop " << i << " idx=" << last;
#endif
            idx = _msg.indexOf(rx,last);
            vlist << getTagValue(_msg,_tag,idx);
            last = idx+1;
        }
        return vlist;
}



QStringList
SimpleXmlParser::getDecodedTagsValues(const QString &msg, const QString &tag)
{
    QStringList rawValues = getTagsValues(msg, tag);
    QStringList retval;
    foreach(QString rawVal, rawValues) {
        retval << decodeEntities(rawVal);
    }
    return retval;
}



QMap<QString, QString>
SimpleXmlParser::getTagProperties(const QString &i_msg, const QString &i_tag, int i_offset)
{
    QMap<QString, QString> map;

    QString tagname = i_tag;
    tagname.remove(QRegularExpression("[<>]"));

    int idx, endidx;
    findStartTagDelimiters(i_msg, tagname, i_offset, idx, endidx);

    QString tmpprop = i_msg.mid(idx + tagname.length() + 1, endidx - (idx + tagname.length() + 1) );

#ifdef SXML_DBG
    qDebug() << "Properties string: " << tmpprop;
#endif

    tmpprop.replace(QRegularExpression("\\s*=\\s*"),"=");

#ifdef SXML_DBG
    qDebug() << "sanitized Properties string: " << tmpprop.trimmed();
#endif

    QStringList sl;

    QRegularExpression rx("(\\w+(?:(?:-\\w+)*)?=\".*\")", QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpressionMatchIterator rxMatchIterator = rx.globalMatch(tmpprop.trimmed());

    while (rxMatchIterator.hasNext()) {
        QRegularExpressionMatch match = rxMatchIterator.next();
        if (match.hasMatch() && !match.captured(0).isEmpty()) {
            sl << match.captured(0);
        }
    }

    QRegularExpression rx2("(\\w+(?:(?:-\\w+)*)?='[^']*')", QRegularExpression::UseUnicodePropertiesOption);

    QRegularExpressionMatchIterator rx2MatchIterator = rx2.globalMatch(tmpprop.trimmed());
    while (rx2MatchIterator.hasNext()) {
        QRegularExpressionMatch match = rx2MatchIterator.next();
        sl << match.captured();
    }

#ifdef SXML_DBG
    qDebug() << "prop string list: " << sl;
#endif

    foreach (QString s, sl) {

#ifdef SXML_DBG
        qDebug() << "Found Property: " << s;
#endif

        QStringList sl2 = s.trimmed().split("=",Qt::SkipEmptyParts);

#ifdef SXML_DBG
        qDebug() << sl2;
#endif

        map[sl2.at(0).trimmed()] = unquoteString(sl2.at(1));
    }

    return map;
}



QList<QMap<QString, QString> >
SimpleXmlParser::getTagsProperties(const QString &i_msg, const QString &i_tag)
{
    QList<QMap<QString, QString> >maplist;

    int idx,last=0;
    QString ntag = i_tag;
    ntag.remove(QRegularExpression("[<>]"));
    QRegularExpression rx("<" + ntag + "[\\s*|>]");
    int numtags = i_msg.count(rx);
    for (int i=0; i<numtags; i++) {
#ifdef SXML_DBG
        qDebug() << "parsing loop " << i << " idx=" << last;
#endif
        idx = i_msg.indexOf(rx,last);
        maplist << getTagProperties(i_msg, i_tag, idx);
        last = idx+1;
    }

    return maplist;
}

/********TEST FNXS *********/

void
SimpleXmlParser::test_getTag()
{
    QString ts1 = "<pippo>ciao</pippo>";
    QString ts1b = "<pippo2>ciao</pippo2>";
    QString ts2 = "<pippo/>";
    QString ts3 = "<pippo   />";
    QString ts4 = "<pippo  >ciao</pippo>";
    QString ts5 = "<pippo p1='bello' >ciao</pippo>";

    QString ts6 = "<pippolist>\
            <pippo>ciao</pippo>\n\
            <pippo>ciao2</pippo>\
            </pippolist>";

    QString ts7 = "<pippo p1='ciao' >alice &lt; bob&#x2019;s mom &amp; '3 &gt; 1' &#233;&#224;&#8364;</pippo>";

    QString rs;
    QStringList rsl;

    rs = SimpleXmlParser::getTagValue(ts1, "pippo");
    qDebug() << "Result: " << rs;
    Q_ASSERT(rs=="ciao");
    qDebug() << "Test 1 passed\n----------\n";

    rs = SimpleXmlParser::getTagValue(ts1b, "pippo");
    qDebug() << "Result: " << rs;
    Q_ASSERT(rs!="ciao");
    qDebug() << "Test 1b passed\n----------\n";

    rs = SimpleXmlParser::getTagValue(ts1, "<pippo>");
    qDebug() << "Result: " << rs;
    Q_ASSERT(rs=="ciao");
    qDebug() << "Test 1c passed\n----------\n";

    rs = SimpleXmlParser::getTagValue(ts2, "pippo");
    qDebug() << "Result: " << rs;
    Q_ASSERT(rs=="");
    qDebug() << "Test 2 passed\n----------\n";

    rs = SimpleXmlParser::getTagValue(ts3, "pippo");
    qDebug() << "Result: " << rs;
    Q_ASSERT(rs=="");
    qDebug() << "Test 3 passed\n----------\n";

    rs = SimpleXmlParser::getTagValue(ts4, "pippo");
    qDebug() << "Result: " << rs;
    Q_ASSERT(rs=="ciao");
    qDebug() << "Test 4 passed\n----------\n";

    rs = SimpleXmlParser::getTagValue(ts5, "pippo");
    qDebug() << "Result: " << rs;
    Q_ASSERT(rs=="ciao");
    qDebug() << "Test 5 passed\n----------\n";

    rsl = SimpleXmlParser::getTagsValues(ts6, "pippo");
    qDebug() << "Result: " << rsl;
    Q_ASSERT(rsl.size()==2);
    Q_ASSERT(rsl.at(0)=="ciao");
    Q_ASSERT(rsl.at(1)=="ciao2");
    qDebug() << "Test 6 passed\n----------\n";

    rsl = SimpleXmlParser::getTagsValues(ts6, "<pippo>");
    qDebug() << "Result: " << rsl;
    Q_ASSERT(rsl.size()==2);
    Q_ASSERT(rsl.at(0)=="ciao");
    Q_ASSERT(rsl.at(1)=="ciao2");
    qDebug() << "Test 6b passed\n----------\n";

    rs = SimpleXmlParser::getTagValue(ts7, "pippo");
    qDebug() << "Result: " << decodeEntities(rs);
    Q_ASSERT(decodeEntities(rs)=="alice < bob’s mom & '3 > 1' éà€");
    qDebug() << "Test 7 passed\n----------\n";
}

void
SimpleXmlParser::test_getProperty()
{
    QString ts1 = "<pippo p1='bello' >ciao</pippo>";
    QString ts2 = "<pippo p1  =  'bello  sguardo' >ciao</pippo>";
    QString ts3 = "<pippo p1=\"bello 'sguardo' \" p2='ciccio'>ciao</pippo>";
    QString ts4 = "<pippolist> <pippo p1=\"bello 'sguardo' \" p2='ciccio'>ciao</pippo><pippo p1=\"bello 'sguardo' \" p3='ciccio2'>ciao</pippo></pippolist>";

    QString ts5 = "<TrapList>\
            <trap\
            eventType='userInput'\
            networkType='eth'\
            isAlice='true'\
            userName='mario.rossi@gmail.com'\
            fwVersion='13.16.00'\
            raVersion='01.11.00'\
            loVersion='027.072.000'\
            timestamp='2013/06/07_14:45:13'>\
            <body\
            eventName='PLAY'\
            videoUrl='http%3A%2F%2Fctv.alice.cdn.interbusiness.it%2FDAM%2FV1%2FFilm%2F2012%2F05%2FRA3_50266398.wmv%7CCOMPONENT%3DWMDRM'\
            videoTitle='Il Gladiatore'\
            />\
            </trap>\
            </TrapList>";

    QString ts6 = "<pippo p1=\"alice &lt; bob&#x2019;s mom &amp; '3 &gt; 1'\" p2='&#233;&#224;&#8364;'>ciao</pippo>";

    QMultiMap<QString, QString>* rm;
    QMultiMap<QString, QString>* rm2;
    QList<QMap<QString, QString> >rmlist;

    rm = new QMultiMap<QString, QString>(SimpleXmlParser::getTagProperties(ts1, "pippo"));
    qDebug() << "Result: " << *rm;
    Q_ASSERT(rm->contains("p1"));
    Q_ASSERT(rm->value("p1")=="bello");
    qDebug() << "Test 1 passed\n----------\n";
    delete rm;

    rm = new QMultiMap<QString, QString>(SimpleXmlParser::getTagProperties(ts2, "pippo"));
    qDebug() << "Result: " << *rm;
    Q_ASSERT(rm->contains("p1"));
    Q_ASSERT(rm->value("p1")=="bello  sguardo");
    qDebug() << "Test 2 passed\n----------\n";
    delete rm;

    rm = new QMultiMap<QString, QString>(SimpleXmlParser::getTagProperties(ts3, "pippo"));
    qDebug() << "Result: " << *rm;
    Q_ASSERT(rm->contains("p1"));
    Q_ASSERT(rm->value("p1")=="bello 'sguardo' ");
    Q_ASSERT(rm->contains("p2"));
    Q_ASSERT(rm->value("p2")=="ciccio");
    qDebug() << "Test 3 passed\n----------\n";
    delete rm;

    rmlist = SimpleXmlParser::getTagsProperties(ts4, "pippo");
    qDebug() << "Result0: " << rmlist.at(0);
    Q_ASSERT(rmlist.at(0).contains("p1"));
    Q_ASSERT(rmlist.at(0)["p1"]=="bello 'sguardo' ");
    Q_ASSERT(rmlist.at(0).contains("p2"));
    Q_ASSERT(rmlist.at(0)["p2"]=="ciccio");
    qDebug() << "Result1: " << rmlist.at(1);
    Q_ASSERT(rmlist.at(1).contains("p1"));
    Q_ASSERT(rmlist.at(1)["p1"]=="bello 'sguardo' ");
    Q_ASSERT(rmlist.at(1).contains("p3"));
    Q_ASSERT(rmlist.at(1)["p3"]=="ciccio2");
    qDebug() << "Test 4 passed\n----------\n";

    rm = new QMultiMap<QString, QString>(SimpleXmlParser::getTagProperties(ts5, "trap"));
    rm2 = new QMultiMap<QString, QString>(SimpleXmlParser::getTagProperties(ts5, "body"));
    rm->unite(*rm2);
    qDebug() << "Result: " << *rm;
    Q_ASSERT(rm->contains("eventType"));
    Q_ASSERT(rm->value("eventType")=="userInput");
    Q_ASSERT(rm->contains("networkType"));
    Q_ASSERT(rm->value("networkType")=="eth");
    Q_ASSERT(rm->contains("eventName"));
    Q_ASSERT(rm->value("eventName")=="PLAY");
    qDebug() << "Test 5 passed\n----------\n";
    delete rm2;
    delete rm;

    rm = new QMultiMap<QString, QString>(SimpleXmlParser::getTagProperties(ts6, "pippo"));
    qDebug() << "Result: " << *rm;
    Q_ASSERT(rm->contains("p1"));
    Q_ASSERT(decodeEntities(rm->value("p1"))=="alice < bob’s mom & '3 > 1'");
    Q_ASSERT(rm->contains("p2"));
    Q_ASSERT(decodeEntities(rm->value("p2"))=="éà€");
    qDebug() << "Test 6 passed\n----------\n";
    delete rm;
}

void
SimpleXmlParser::test_addData()
{
    /* Note: I'm using decodeEntities() to add unicode text to QString
     * irrespective of IDE/System encoding (GS)
     */
    QString tsPart1 = SimpleXmlParser::decodeEntities("<pippo>alice &lt; bob&#x2019;s mom &amp; '3 ");
    QString tsPart2 = SimpleXmlParser::decodeEntities("&gt; 1' &#233;&#224;&#8364;&#xc29f;</pippo>");

    SimpleXmlParser xmlParser;
    xmlParser.setStartTag("pippo");
    xmlParser.addData(tsPart1);
    xmlParser.addData(tsPart2);

    QString rs;
    if (xmlParser.hasPendingMessages())
    {
        rs = xmlParser.getNextMessage();
    }
    qDebug() << "Result: " << decodeEntities(rs);
    Q_ASSERT(rs == (tsPart1 + tsPart2));
    qDebug() << "Test 1 passed\n----------\n";
}

/************* END OF TEST FNXS ************/

/*!

  */
QString
SimpleXmlParser::getNextMessage()
{
    QString s;

    muxMsgList.lock();
        s = m_parsedMessages.isEmpty() ? "" : m_parsedMessages.takeFirst();
    muxMsgList.unlock();

    return s;
}

void
SimpleXmlParser::addData(const QString &aMsgpart) {
    QString msg;

    if (m_maxBufferSizeInBytes > 0 && m_buffer.size() > m_maxBufferSizeInBytes) {
        emit parseErrorFound(E_MessageTooBig);
#ifdef SXML_DBG
        qWarning() << "Buffer size is: "<< m_buffer.size() << " we passed the limit, appending not done!";
#endif
        return;
    }

    m_buffer.append(aMsgpart);

    int idx = m_buffer.indexOf("<" + m_StartTag + ">");
    int idx2 = m_buffer.indexOf("</" + m_StartTag + ">");

    if (idx2 < 0) { //entire message data did not fit in the read chunk!
#ifdef SXML_DBG
        qDebug() << "############ SXML - Storing arrived data for next Chunk #############";
        qDebug() << "SXML - Current buffer is:\n" << m_buffer;
#endif
        return;
    }
    else {
        if (idx >= 0 && idx < idx2) {//normal message
            msg = m_buffer.mid(idx, idx2 - idx + 3 + m_StartTag.length()); //the three bytes are "</>" that sourrounds the star tag
            m_buffer = m_buffer.mid(idx2 + 3 + m_StartTag.length());
#ifdef SXML_DBG
            qDebug() << "SXML - We got a message: " << msg;
            qDebug() << "SXML - Whats left in the buffer:\n" << m_buffer;
#endif

            //here we have a completed message;
            if (m_notifyMode == E_DispatchMessageAndDelete) {
                emit parsedMessage(msg);
            }
            else {
                muxMsgList.lock();
                    m_parsedMessages.append(msg);
                muxMsgList.unlock();

                switch(m_notifyMode) {
                    case E_NotifyOnly:
                        emit messageCompleted();
                        break;
                    case E_DispatchMessage:
                        emit parsedMessage(msg);
                        break;
                    case E_NotifyAndDispatch:
                        emit messageCompleted();
                        emit parsedMessage(msg);
                        break;
                    case E_DispatchMessageAndDelete:
                        //We cannot be here, added just to avoid compilation warning
                        break;
                }
            }

            //now if we still have something in the buffer we go for another check
            if (!m_buffer.isEmpty()) {
#ifdef SXML_DBG
                qDebug() << "Buffer size is: "<< m_buffer.size() << " rechecking buffer!";
#endif
                addData("");
            }
        }
        else {
            if (idx2 < idx) {
                m_buffer = m_buffer.mid(idx);
#ifdef SXML_DBG
                qCritical() << "SXML - END tag is *before* START tag... we probably lost a chunk, dropping remainder!";
                qDebug() << "SXML - New buffer contents:\n" << m_buffer;
#endif
                emit parseErrorFound(E_EndTagNotMatched);
                addData("");
            }
        }
    }
}

bool
SimpleXmlParser::hasPendingMessages()
{
    bool retval = false;
    muxMsgList.lock();
        if (m_parsedMessages.count() > 0)
            retval = true;
    muxMsgList.unlock();
    return retval;
}
