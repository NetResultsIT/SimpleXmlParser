#include "nrparamparser.h"

#include <QDebug>

NRParamParser::NRParamParser()
    :m_paramSeparator("-"),
     m_longparamSeparator("--")
{
}

const NRParamParser&
NRParamParser::instance()
{
    static NRParamParser _instance;

    return _instance;
}

Param*
NRParamParser::findParam(const QString& aParamName)
{
    Param *retp=0;
    foreach (Param *p, m_acceptedParamList) {
        if (p->nameMatch(aParamName)) {
            retp=p;
            break;
        }
    }
    return retp;
}

const Param*
NRParamParser::findParsedParam(const QString& aParamName)
{
    const Param *retp=0;
    foreach (const Param *p, m_parsedParamList) {
        if (p->nameMatch(aParamName)) {
            retp=p;
            break;
        }
    }
    return retp;
}

bool
NRParamParser::isSet(const QString &aParamName)
{
    const Param *p = findParsedParam(aParamName);

    if (p)
        return true;

    return false;
}

bool
NRParamParser::parse(int argc, char **argv)
{
    bool ok=true;
    QString pname;

    //do a cleanup in case we're calling this method multiple times
    m_parsedParamList.clear();
    foreach (Param *p, m_acceptedParamList) {
        p->reset();
    }

    for (int i=1; i<argc; i++) {
        QString s(argv[i]);
#ifdef PP_DEBUG
        qDebug() << "parsing param: " << s;
#endif
        if (s.startsWith(m_paramSeparator) || s.startsWith(m_longparamSeparator)) {
            if (s.startsWith(m_longparamSeparator)) {
                pname = s.remove(0,m_longparamSeparator.length());
            }
            else if (s.startsWith(m_paramSeparator)) {
                pname = s.remove(0,m_paramSeparator.length());
            }
            Param *p = findParam(pname);
            if (p) { //we found a param
                if (p->parsed()) {
                    ok=false;
                    m_error = QString("Parameter already parsed: ")+pname;
                    break;
                }
                if (p->needsValue()) {
                    QString ss(argv[++i]);
                    p->value(ss);
                }
                m_parsedParamList.append(p);
                p->parsed(true);
            }
            else {
                ok=false;
                m_error = QString("Unknown Parameter: ")+pname;
                break;
            }
        }
        else {
            ok=false;
            m_error = QString("Unknown Token on cmdline: ")+s;
            break;
        }

    }

    if (ok) { //All params were parsed good, now we need to check if all mandatory ones are present
        foreach (Param *mp, m_mandatoryParamList)
            if (!m_parsedParamList.contains(mp)) {
                ok=false;
                m_error = QString("Missing mandatory parameter: ")+mp->names();
                break;
            }
    }

    return ok;
}

void
NRParamParser::acceptParam(const QString &aShortName, const QString &aLongName, bool needsValue, bool optional)
{
    Param *p = new Param(aShortName, aLongName, needsValue, optional);
    m_acceptedParamList.append(p);
    if (!optional)
        m_mandatoryParamList.append(p);
}

QVariant
NRParamParser::paramValue(const QString &aParamName)
{
    QVariant v;

    const Param *p = findParsedParam(aParamName);
    if(p)
        v = p->value();

    return v;
}
