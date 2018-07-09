#include <QCoreApplication>
#include <QFile>
#include <QDebug>

#include <nrparamparser.h>
#include <SimpleXmlParser.h>


int main(int argc, char** argv) {

    NRParamParser pp = NRParamParser::instance();
    pp.parse(argc,argv);
    QCoreApplication app(argc,argv);
    SimpleXmlParser xml;

    QFile f("testplan_76.xml");
    f.open(QIODevice::ReadOnly);
    QString tp = f.readAll();
    f.close();

    xml.setStartTag("TestPlan");
    xml.addData(tp);

    QString s = xml.getNextMessage();
    QStringList sl = xml.getTagsValues(s,"TestData");

    foreach (QString s, sl) {
        qDebug() << "parsed: " << s;
    }

return app.exec();
}
