#include "codepaintdevice.h"
#include <QtDebug>

const static QString ind1 = QLatin1String("    ");
const static QString ind2 = ind1 + ind1;
const static QString ind3 = ind2 + ind1;

class MyPaintEngine : public QObject, public QPaintEngine
{
    Q_OBJECT

public:
    explicit MyPaintEngine(QObject *parent = 0, PaintEngineFeatures features = 0);

    bool begin(QPaintDevice *pdev);
    bool end();
    void updateState(const QPaintEngineState &state);
    void drawPath(const QPainterPath &path);
    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);
    Type type() const;

signals:
    void stateUpdated(const QPaintEngineState &state);
    void pathDrawn(const QPainterPath &path);
};

MyPaintEngine::MyPaintEngine(QObject *parent, PaintEngineFeatures features)
    : QObject(parent)
    , QPaintEngine(features)
{
}

bool MyPaintEngine::begin(QPaintDevice *pdev)
{
    return true;
}

bool MyPaintEngine::end()
{
    return true;
}

void MyPaintEngine::updateState(const QPaintEngineState &state)
{
    emit stateUpdated(state);
}

void MyPaintEngine::drawPath(const QPainterPath &path)
{
    emit pathDrawn(path);
}

void MyPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
}

QPaintEngine::Type MyPaintEngine::type() const
{
    return QPaintEngine::User;
}

CodePaintDevice::CodePaintDevice(const QString &prefix, QObject *parent, QPaintEngine::PaintEngineFeatures features)
    : QObject(parent)
    , QPaintDevice()
    , m_prefix(prefix)
    , m_pen(Qt::NoPen)
    , m_brush(Qt::NoBrush)
    , m_activePen(Qt::NoPen)
    , m_activeBrush(Qt::NoBrush)
    , m_paintEngine(new MyPaintEngine(this, features))
{
    connect(m_paintEngine, SIGNAL(stateUpdated(QPaintEngineState)), SLOT(updateState(QPaintEngineState)));
    connect(m_paintEngine, SIGNAL(pathDrawn(QPainterPath)), SLOT(drawPath(QPainterPath)));
}

CodePaintDevice::~CodePaintDevice()
{
}

void CodePaintDevice::addElement(const Element &element)
{
    m_pen.setStyle(Qt::NoPen);
    m_brush.setStyle(Qt::NoBrush);
    m_activePen.setStyle(Qt::NoPen);
    m_activeBrush.setStyle(Qt::NoBrush);
    m_elements.append(element);
}

QPaintEngine *CodePaintDevice::paintEngine() const
{
    return m_paintEngine;
}

int CodePaintDevice::metric(PaintDeviceMetric m) const
{
    switch (m) {
    default: return 100;
    }
}

void CodePaintDevice::updateState(const QPaintEngineState &state)
{
    m_pen = state.pen();
    m_brush = state.brush();
    m_activeTransform = state.transform();
}

void CodePaintDevice::drawPath(const QPainterPath &path)
{
    onDrawPath(path);
}

QString CodePaintDeviceQt::code() const
{
    return QString();
}

void CodePaintDeviceQt::onNewElement(const Element &element)
{
}

void CodePaintDeviceQt::onDrawPath(const QPainterPath &path)
{
}

CodePaintDeviceHTML5Canvas::CodePaintDeviceHTML5Canvas(const QString &prefix, QObject *parent)
    : CodePaintDevice(prefix, parent, QPaintEngine::PainterPaths | QPaintEngine::PrimitiveTransform)
{
}

QString CodePaintDeviceHTML5Canvas::code() const
{
    QString result = "// This file has been generated by svg2code\n";
    const QString dictionaryName = m_prefix;
    result.append("\nvar " + dictionaryName + " = {\n" + ind1 + "elements: {\n");
    int count = 0;
    foreach (const Element &element, m_elements) {
        if (count++ > 0)
            result.append(",\n");
        result.append(ind2 + "'" + element.id + "': { id: '" + element.id + "', bounds: {"
                          + " x: " + QString::number(element.rect.x(), 'f', 1)
                          + ", y: " + QString::number(element.rect.y(), 'f', 1)
                          + ", width: " + QString::number(element.rect.width(), 'f', 1)
                          + ", height: " + QString::number(element.rect.height(), 'f', 1)
                          + " }, drawfunction: function(c) {\n" + element.code + ind2 + "}}");
    }
    result.append("\n" + ind1 + "},\n\n");
    result.append(ind1 + "draw: function(context, id, x, y, width, height)\n"
                  + ind1 + "{\n"
                  + ind1 + "    var element = " + dictionaryName + ".elements[id];\n"
                  + ind1 + "    if (element !== undefined) {\n"
                  + ind1 + "        context.save();\n"
                  + ind1 + "        context.translate(x, y);\n"
                  + ind1 + "        if (width !== undefined && height !== undefined)\n"
                  + ind1 + "            context.scale(width / element.bounds.width, height / element.bounds.height);\n"
                  + ind1 + "        context.translate(-element.bounds.x, -element.bounds.y);\n"
                  + ind1 + "        element.drawfunction(context);\n"
                  + ind1 + "        context.restore();\n"
                  + ind1 + "    }\n"
                  + ind1 + "}\n};\n");
    return result;
}

void CodePaintDeviceHTML5Canvas::onNewElement(const Element &element)
{
    Q_UNUSED(element)
}

static QString toCssStyle(const QColor &color)
{
    QString rgb = QString::number(color.red()) + ", "
            + QString::number(color.green()) + ", "
            + QString::number(color.blue());
    return color.alpha() == 255 ? "rgb(" + rgb + ")"
                                : "rgba(" + rgb + ", " + QString::number(color.alphaF(), 'f', 1) + ")";
}

void CodePaintDeviceHTML5Canvas::onDrawPath(const QPainterPath &path)
{
    if (m_pen.style() == Qt::NoPen && m_brush.style() == Qt::NoBrush)
        return;
    QPainterPath transformedPath = m_activeTransform.map(path);
    QString &code = m_elements.last().code;
    code.append(ind3 + "c.beginPath();\n");
    for (int i = 0; i < transformedPath.elementCount(); i++) {
        const QPainterPath::Element &element = transformedPath.elementAt(i);
        switch (element.type) {
            case QPainterPath::LineToElement:
                code.append(ind3 + "c.lineTo(" + QString::number(element.x, 'f', 1) + ", " + QString::number(element.y, 'f', 1) + ");\n");
            break;
            case QPainterPath::CurveToElement: {
                code.append(ind3 + "c.bezierCurveTo(" + QString::number(element.x, 'f', 1) + ", " + QString::number(element.y, 'f', 1)  + ", ");
                const QPainterPath::Element &dataElement1 = transformedPath.elementAt(++i);
                code.append(QString::number(dataElement1.x, 'f', 1) + ", " + QString::number(dataElement1.y, 'f', 1)  + ", ");
                const QPainterPath::Element &dataElement2 = transformedPath.elementAt(++i);
                code.append(QString::number(dataElement2.x, 'f', 1) + ", " + QString::number(dataElement2.y, 'f', 1)  + ");\n");
            }
            break;
            default:
            case QPainterPath::MoveToElement:
                code.append(ind3 + "c.moveTo(" + QString::number(element.x, 'f', 1) + ", " + QString::number(element.y, 'f', 1) + ");\n");
            break;
        }
    }
    code.append(ind3 + "c.closePath();\n");
    if (m_pen.style() != Qt::NoPen) {
        if (m_pen != m_activePen) {
            m_activePen = m_pen;
            m_elements.last().code.append(
                        ind3 + "c.strokeStyle = '" + toCssStyle(m_activePen.color()) + "';\n");
        }
        code.append(ind3 + "c.stroke();\n");
    }
    if (m_brush.style() != Qt::NoBrush) {
        if (m_brush != m_activeBrush) {
            m_activeBrush = m_brush;
            m_elements.last().code.append(
                        ind3 + "c.fillStyle = '" + toCssStyle(m_activeBrush.color()) + "';\n");
        }
        code.append(ind3 + "c.fill();\n");
    }
}

#include "codepaintdevice.moc"
