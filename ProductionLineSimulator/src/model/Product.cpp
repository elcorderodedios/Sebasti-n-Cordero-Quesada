#include "Product.h"
#include <QJsonArray>

Product::Product(ProductType type)
    : m_type(type)
    , m_currentState(ProductState::Created)
    , m_createdTime(QDateTime::currentDateTime())
    , m_inRework(false)
{
    m_id = generateId();
}

void Product::advanceState()
{
    switch (m_currentState) {
        case ProductState::Created:
            m_currentState = ProductState::AtIntake;
            break;
        case ProductState::AtIntake:
            m_currentState = ProductState::AtAssembler;
            break;
        case ProductState::AtAssembler:
            m_currentState = ProductState::AtQualityInspection;
            break;
        case ProductState::AtQualityInspection:
            if (m_inRework) {
                m_currentState = ProductState::AtAssembler;
                m_inRework = false;
            } else {
                m_currentState = ProductState::AtPackaging;
            }
            break;
        case ProductState::AtPackaging:
            m_currentState = ProductState::AtShipping;
            break;
        case ProductState::AtShipping:
            m_currentState = ProductState::Finished;
            break;
        case ProductState::InRework:
            m_currentState = ProductState::AtAssembler;
            break;
        default:
            // No advancement for finished or rejected states
            break;
    }
}

void Product::setState(ProductState state)
{
    m_currentState = state;
}

void Product::addTraceEntry(const QString& station)
{
    m_trace.append(QString("%1 at %2").arg(station, QDateTime::currentDateTime().toString()));
}

QString Product::showInfo() const
{
    return QString("Product ID: %1, Type: %2, State: %3, Created: %4")
           .arg(m_id, getTypeString(), getStateString(), m_createdTime.toString());
}

QString Product::getTypeString() const
{
    switch (m_type) {
        case ProductType::Washer: return "Washer";
        case ProductType::Dryer: return "Dryer";
        case ProductType::Refrigerator: return "Refrigerator";
        case ProductType::Dishwasher: return "Dishwasher";
        case ProductType::Oven: return "Oven";
    }
    return "Unknown";
}

QString Product::getStateString() const
{
    switch (m_currentState) {
        case ProductState::Created: return "Created";
        case ProductState::AtIntake: return "At Intake";
        case ProductState::AtAssembler: return "At Assembler";
        case ProductState::AtQualityInspection: return "At Quality Inspection";
        case ProductState::AtPackaging: return "At Packaging";
        case ProductState::AtShipping: return "At Shipping";
        case ProductState::Finished: return "Finished";
        case ProductState::Rejected: return "Rejected";
        case ProductState::InRework: return "In Rework";
    }
    return "Unknown";
}

QJsonObject Product::toJson() const
{
    QJsonObject obj;
    obj["id"] = m_id;
    obj["type"] = static_cast<int>(m_type);
    obj["currentState"] = static_cast<int>(m_currentState);
    obj["createdTime"] = m_createdTime.toString(Qt::ISODate);
    obj["inRework"] = m_inRework;
    
    QJsonArray traceArray;
    for (const QString& entry : m_trace) {
        traceArray.append(entry);
    }
    obj["trace"] = traceArray;
    
    return obj;
}

void Product::fromJson(const QJsonObject& json)
{
    m_id = json["id"].toString();
    m_type = static_cast<ProductType>(json["type"].toInt());
    m_currentState = static_cast<ProductState>(json["currentState"].toInt());
    m_createdTime = QDateTime::fromString(json["createdTime"].toString(), Qt::ISODate);
    m_inRework = json["inRework"].toBool();
    
    m_trace.clear();
    QJsonArray traceArray = json["trace"].toArray();
    for (const auto& value : traceArray) {
        m_trace.append(value.toString());
    }
}

QString Product::generateId() const
{
    return QString("P-%1").arg(QUuid::createUuid().toString().mid(1, 8).toUpper());
}
