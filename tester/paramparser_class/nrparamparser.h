#ifndef NRPARAMPARSER_H
#define NRPARAMPARSER_H

#include <QObject>
#include <QString>
#include <QPair>
#include <QList>
#include <QVariant>

class Param
{
    QString m_name;
    QVariant  m_value;
    QString m_longname,m_description;
    bool m_needsValue, m_optional, m_parsed;

public:
    /* CTORS */
    explicit Param(const QString &aName,const QString &aLongName, bool aNeedsValue, bool isOptional):
        m_name(aName),
        m_value(""),
        m_longname(aLongName),
        m_description(""),
        m_needsValue(aNeedsValue),
        m_optional(isOptional),
        m_parsed(false) {}
    explicit Param(const Param& aParam) {
        this->m_name        = aParam.m_name;
        this->m_longname    = aParam.m_longname;
        this->m_description = aParam.m_description;
        this->m_parsed      = aParam.m_parsed;
        this->m_value       = aParam.m_value;
    }
    explicit Param(const Param* aParam) {
        this->m_name        = aParam->m_name;
        this->m_longname    = aParam->m_longname;
        this->m_description = aParam->m_description;
        this->m_parsed      = aParam->m_parsed;
        this->m_value       = aParam->m_value;
    }

    bool parsed() const         { return m_parsed;     }
    void parsed(bool aValue)    { m_parsed=aValue;     }
    void value(QVariant aValue) { m_value=aValue;      }
    QVariant value() const      { return m_value;      }
    bool needsValue() const     { return m_needsValue; }
    bool nameMatch(const QString &aName) const {
        if (m_name==aName || m_longname==aName)
            return true;

        return false;
    }
    void reset() {
        m_parsed=false;
        m_value.clear();
    }
    const QString names() const { return "("+m_name+"|"+m_longname+")"; }

};

class NRParamParser
{
    QString m_paramSeparator, m_longparamSeparator;
    QString m_error;
    QList<Param*> m_parsedParamList, m_acceptedParamList, m_mandatoryParamList;

    Param* findParam(const QString& aParamName);
    const Param* findParsedParam(const QString& aParamName);

    explicit NRParamParser();

public:
    static const NRParamParser& instance();

    bool parse(int argc, char**argv);
    QString error()                             { return m_error; }
    void setSeparator(const QString &aSep)      { m_paramSeparator=aSep;        }
    void setLongSeparator(const QString &aSep)  { m_longparamSeparator=aSep;    }

    QVariant paramValue(const QString& aParamName);
    void acceptParam(const QString& aShortName, const QString& aLongName, bool needsValue, bool optional=true);

    bool isSet(const QString& aParamName);
    enum ParamValueEnum {
        E_INTEGER = 0,
        E_DOUBLE = 1,
        E_STRING = 2
    };
};

#endif // NRPARAMPARSER_H
